// TEMPORARY
#include <stdio.h>

#include <windows.h>
#include <windowsx.h>
#include <dsound.h>

#include "loji.h"
#include "loji.cpp"

struct win32_offscreen_buffer
{
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int bytes_per_pixel;
	int pitch;
};

struct window_dimension
{
	int width;
	int height;
};

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

static LPDIRECTSOUNDBUFFER secondary_sound_buffer;
static win32_offscreen_buffer global_buffer;
static font_buffer font_bitmaps;

static keyboard_input k_input;
static mouse_input m_input;

static bool should_quit = false;

void init_dsound(HWND window, int samples_per_second, int buffer_size)
{
    // NOTE(casey): Load the library
    HMODULE dsound_library = LoadLibraryA("dsound.dll");
    if(dsound_library)
    {
        // NOTE(casey): Get a DirectSound object! - cooperative
		// HRESULT WINAPI DirectSoundCreate(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter );
		
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)
            GetProcAddress(dsound_library, "DirectSoundCreate");

        // TODO(casey): Double-check that this works on XP - DirectSound8 or 7??
        LPDIRECTSOUND direct_sound;
        if(SUCCEEDED(DirectSoundCreate(0, &direct_sound, 0)))
        {
            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.nSamplesPerSec = samples_per_second;
            wave_format.wBitsPerSample = 16;
            wave_format.nBlockAlign = (wave_format.nChannels*wave_format.wBitsPerSample) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec*wave_format.nBlockAlign;
            wave_format.cbSize = 0;

            if(SUCCEEDED(direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC buffer_description = {};
                buffer_description.dwSize = sizeof(buffer_description);
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // NOTE(casey): "Create" a primary buffer
                // TODO(casey): DSBCAPS_GLOBALFOCUS?
                LPDIRECTSOUNDBUFFER primary_buffer;
                if(SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &primary_buffer, 0)))
                {
                    HRESULT error_code = primary_buffer->SetFormat(&wave_format);
                    if(SUCCEEDED(error_code))
                    {
                        // NOTE(casey): We have finally set the format!
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO(casey): Diagnostic
                    }
                }
                else
                {
                    // TODO(casey): Diagnostic
                }
            }
            else
            {
                // TODO(casey): Diagnostic
            }

            // TODO(casey): DSBCAPS_GETCURRENTPOSITION2
            DSBUFFERDESC buffer_description = {};
            buffer_description.dwSize = sizeof(buffer_description);
            buffer_description.dwFlags = 0;
            buffer_description.dwBufferBytes = buffer_size;
            buffer_description.lpwfxFormat = &wave_format;
            HRESULT error_code = direct_sound->CreateSoundBuffer(&buffer_description, &secondary_sound_buffer, 0);
            if(SUCCEEDED(error_code))
            {
                OutputDebugStringA("Secondary buffer created successfully.\n");
            }
        }
        else
        {
            // TODO(casey): Diagnostic
        }
    }
    else
    {
        // TODO(casey): Diagnostic
    }
}

void test_load_arial(float size)
{
	if (font_bitmaps.memory)
	{
		// VirtualFree(font_bitmaps.memory, 0, MEM_RELEASE);
		free(font_bitmaps.memory);
	}
	
	unsigned char* ttf_buffer = (unsigned char*)malloc(1 << 25);
	stbtt_fontinfo font;
	
	fread(ttf_buffer, 1, 1 << 25, fopen("c:/windows/fonts/arialbd.ttf", "rb"));
	stbtt_InitFont(&font, (unsigned char*)ttf_buffer, stbtt_GetFontOffsetForIndex((unsigned char*)ttf_buffer, 0));
	
	// int char_bitmap_width = (int)size;
	// int char_bitmap_height = (int)size;
	
	// unsigned int character_memory_size = ((int)size * (int)size) * 4 /*+ 8*/;
	// int font_memory_size = character_memory_size * 52;
	
	unsigned int font_memory_size = sizeof(character_bitmap) * 52;
	
	// font_bitmaps.memory = (character_bitmap*)VirtualAlloc(0, font_memory_size, MEM_COMMIT, PAGE_READWRITE);
	font_bitmaps.memory = (character_bitmap*)malloc(font_memory_size);
	
	char alphabet[53] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	character_bitmap* current_character = font_bitmaps.memory;
	for (int i = 0; i < 52; i++)
	{
		// current_character->width = char_bitmap_width;
		// current_character->height = char_bitmap_height;
		
		current_character->memory = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, size), alphabet[i],
												 &current_character->width, &current_character->height, 0, 0);
		current_character++;
	}
}

window_dimension get_window_dimension(HWND window)
{
	RECT client_rect;
	GetClientRect(window, &client_rect);
	
	window_dimension result;
	
	result.width = client_rect.right - client_rect.left;
	result.height = client_rect.bottom - client_rect.top;
	
	return result;
}

static void resize_dib_section(win32_offscreen_buffer* buffer, int width, int height)
{
	if (buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}
	
	int bytes_per_pixel = 4;
	
	buffer->width = width;
	buffer->height = height;
	
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;
	
	int bitmap_memory_size = (buffer->width * buffer->height) * bytes_per_pixel;
	buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
	
	buffer->pitch = buffer->width * bytes_per_pixel;
	buffer->bytes_per_pixel = bytes_per_pixel;
}

static void display_buffer_in_window(HDC device_context, window_dimension dimension)
{
	StretchDIBits(device_context,
				  0, 0, dimension.width, dimension.height,
				  0, 0, global_buffer.width, global_buffer.height,
				  global_buffer.memory,
				  &global_buffer.info,
				  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	LRESULT result = 0;

	switch (message)
	{
		case WM_SIZE:
		{
			// window_dimension dim = get_window_dimension(window);
			// resize_dib_section(&global_buffer, dim.width, dim.height);
		}
		break;

		case WM_CLOSE:
		{
			OutputDebugStringA("WM_CLOSE\n");
			should_quit = true;
		}
		break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP");
		}
		break;

		case WM_DESTROY:
		{
			OutputDebugStringA("WM_DESTROY");
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC device_context = BeginPaint(window, &paint);
			
			window_dimension dimension = get_window_dimension(window);
			display_buffer_in_window(device_context, dimension);

			EndPaint(window, &paint);
		}
		break;
		
		case WM_LBUTTONDOWN:
		{
			m_input.clicked = true;
			
			m_input.x_pos = GET_X_LPARAM(l_param);
			m_input.y_pos = GET_Y_LPARAM(l_param);
		}
		break;
		
		case WM_KEYDOWN:
		{
			OutputDebugStringA("KEY DOWN\n");
			
			if (w_param == VK_LEFT)
			{
				k_input.left = true;
				// x_offset += 5;
			}
			if (w_param == VK_RIGHT)
			{
				k_input.right = true;
				// x_offset -= 5;
			}
			if (w_param == VK_UP)
			{
				k_input.up = true;
				// y_offset += 5;
			}
			if (w_param == VK_DOWN)
			{
				k_input.down = true;
				// y_offset -= 5;
			}
			if (w_param == VK_RETURN)
			{
				k_input.enter = true;
			}
			if (w_param == VK_ESCAPE)
			{
				k_input.escape = true;
			}
		}
		break;
		
		case WM_KEYUP:
		{
		}
		break;

		default:
		{
			result = DefWindowProc(window, message, w_param, l_param);
		}
		break;
	}

	return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
	WNDCLASS window_class = {};

	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = window_proc;
	window_class.hInstance = instance;
	window_class.lpszClassName = "LojiWindowClass";
	
	// resize_dib_section(&global_buffer, 960, 540);

	if (RegisterClassA(&window_class))
	{
		HWND window = CreateWindowExA(0, window_class.lpszClassName, "Loji",
			                          WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
									  CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

		if (window)
		{
			HDC device_context = GetDC(window);
			
			window_dimension dim = get_window_dimension(window);
			resize_dib_section(&global_buffer, dim.width, dim.height);
			
			test_load_arial(128.0f);
			
			/*
			character_bitmap font_bitmap;
			font_bitmap.width = 128;
			font_bitmap.height = 128;
			
			unsigned char* ttf_buffer = (unsigned char*)malloc(1 << 25);
			stbtt_fontinfo font;
			
			fread(ttf_buffer, 1, 1 << 25, fopen("c:/windows/fonts/arialbd.ttf", "rb"));
			stbtt_InitFont(&font, (unsigned char*)ttf_buffer, stbtt_GetFontOffsetForIndex((unsigned char*)ttf_buffer, 0));
			
			font_bitmap.memory = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, 128.0f), 'A',
														  &font_bitmap.width, &font_bitmap.height, 0, 0);
			*/
			
			// Sound test
            int samples_per_second = 48000;
            int tone_hz = 256;
            short tone_volume = 3000;
            unsigned int running_sample_index = 0;
            int square_wave_period = samples_per_second / tone_hz;
            int half_square_wave_period = square_wave_period / 2;
            int bytes_per_sample = sizeof(short) * 2;
            int secondary_buffer_size = samples_per_second * bytes_per_sample;
			bool sound_is_playing = false;
			
			init_dsound(window, samples_per_second, secondary_buffer_size);
			
			while (!should_quit)
			{
				k_input = {};
				
				MSG message;

				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
					{
						should_quit = true;
					}
					
					TranslateMessage(&message);
					DispatchMessageA(&message);
				}
				
				// NOTE(casey): DirectSound output test
                DWORD play_cursor;
                DWORD write_cursor;
                if(SUCCEEDED(secondary_sound_buffer->GetCurrentPosition(&play_cursor, &write_cursor)))
                {
                    DWORD byte_to_lock = running_sample_index * bytes_per_sample % secondary_buffer_size;
                    DWORD bytes_to_write;
                    if(byte_to_lock == play_cursor)
                    {
                        bytes_to_write = secondary_buffer_size;
                    }
                    else if(byte_to_lock > play_cursor)
                    {
                        bytes_to_write = secondary_buffer_size - byte_to_lock;
                        bytes_to_write += play_cursor;
                    }
                    else
                    {
                        bytes_to_write = play_cursor - byte_to_lock;
                    }

                    // TODO(casey): More strenuous test!
                    // TODO(casey): Switch to a sine wave
                    VOID *region1;
                    DWORD region1_size;
                    VOID *region2;
                    DWORD region2_size;
                    if(SUCCEEDED(secondary_sound_buffer->Lock(byte_to_lock, bytes_to_write,
                                                             &region1, &region1_size,
                                                             &region2, &region2_size,
                                                             0)))
                    {
                        // TODO(casey): assert that Region1Size/Region2Size is valid

                        // TODO(casey): Collapse these two loops
                        DWORD region1_sample_count = region1_size / bytes_per_sample;
                        short* sample_out = (short*)region1;
                        for (DWORD sample_index = 0; sample_index < region1_sample_count; sample_index++)
                        {
                            short sample_value = ((running_sample_index++ / half_square_wave_period) % 2) ? tone_volume : -tone_volume;
                            *sample_out++ = sample_value;
                            *sample_out++ = sample_value;
                        }

                        DWORD region2_sample_count = region2_size / bytes_per_sample;
                        sample_out = (short*)region2;
                        for (DWORD sample_index = 0; sample_index < region2_sample_count; sample_index++)
                        {
                            short sample_value = ((running_sample_index++ / half_square_wave_period) % 2) ? tone_volume : -tone_volume;
                            *sample_out++ = sample_value;
                            *sample_out++ = sample_value;
                        }

                        secondary_sound_buffer->Unlock(region1, region1_size, region2, region2_size);
                    }
                }

                if(!sound_is_playing)
                {
                    secondary_sound_buffer->Play(0, 0, DSBPLAY_LOOPING);
                    sound_is_playing = true;
                }
				
				offscreen_buffer buffer;
				buffer.memory = global_buffer.memory;
				buffer.width = global_buffer.width;
				buffer.height = global_buffer.height;
				buffer.bytes_per_pixel = global_buffer.bytes_per_pixel;
				buffer.pitch = global_buffer.pitch;
				
				update_and_render(&buffer, &k_input, &m_input, &font_bitmaps);
				
				window_dimension dimension = get_window_dimension(window);
				display_buffer_in_window(device_context, dimension);
				
				// x_offset++;
				// y_offset++;
			}
		}
		else
		{
			OutputDebugStringA("ERROR: Unable to create window.");
		}
	}
	else
	{
		OutputDebugStringA("ERROR: Unable to register the window class.");
	}
}