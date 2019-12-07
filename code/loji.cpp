#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

int selected_menu_item_index = -1;
int mode = -1;

int x_offset = 0;
int y_offset = 0;

void test_render_text(offscreen_buffer* buffer, font_buffer* font_bitmaps)
{
	int width = font_bitmaps->memory[0].width;
	int height = font_bitmaps->memory[0].height;

	unsigned char* dest_row = (unsigned char*)buffer->memory + ((buffer->width / 2) * buffer->bytes_per_pixel) + ((buffer->height / 2) * buffer->pitch);
	unsigned char* source = font_bitmaps->memory[15].memory;
	for (int y = 0; y < height; y++)
	{
		unsigned int* dest_pixel = (unsigned int*)dest_row;
		
		for (int x = 0; x < width; x++)
		{
			unsigned char alpha = *source;
			*dest_pixel = ((alpha << 24)
						  | (alpha << 16)
						  | (alpha << 8)
						  | (alpha << 0));
						  
			dest_pixel++;
			source++;
		}
		
		dest_row += buffer->pitch;
	}
}

void draw_rectangle(offscreen_buffer* buffer, int min_x, int min_y, int max_x, int max_y, unsigned int color, int border_width, unsigned int border_color)
{
	if (min_x < 0) min_x = 0;
	if (min_y < 0) min_y = 0;
	if (max_x >= buffer->width) max_x = buffer->width - 1;
	if (max_y >= buffer->height) max_y = buffer->height - 1;
	
	unsigned char* row = (unsigned char*)buffer->memory
						 + (min_x * buffer->bytes_per_pixel)
						 + (min_y * buffer->pitch);
					
	for (int y = min_y; y <= max_y; y++)
	{	
		unsigned int* pixel = (unsigned int*)row;
		
		for (int x = min_x; x <= max_x; x++)
		{
			if (y < min_y + border_width || y > max_y - border_width || x < min_x + border_width || x > max_x - border_width)
			{
				*pixel = border_color;
			}
			else
			{
				*pixel = color;	
			}
			
			pixel++;
		}
		
		row += buffer->pitch;
	}
}		
/*
void render_weird_gradient(offscreen_buffer* buffer, int mouse_x_pos, int mouse_y_pos)
{
	unsigned char* row = (unsigned char*)buffer->memory;
	
	for (int y = 0; y < buffer->height; y++)
	{
		unsigned int* pixel = (unsigned int*)row;
		
		for (int x = 0; x < buffer->width; x++)
		{
			if (get_absolute_value(x - mouse_x_pos) <= 10 && get_absolute_value(y - mouse_y_pos) <= 10)
			{
				*pixel = 0xFFFFFF;
			}
			else
			{
				unsigned char green = x + x_offset;
				unsigned char blue = y + y_offset;
				
				*pixel = (green << 8) | blue;
			}
			
			pixel++;
		}
		
		row += buffer->pitch;
	}
}
*/

void update_and_render(offscreen_buffer* buffer, keyboard_input* k_input, mouse_input* m_input, font_buffer* font_bitmaps)
{
	if (k_input->left)
	{
		if (selected_menu_item_index <= 0) selected_menu_item_index = 0;
		else selected_menu_item_index--;
	}
	if (k_input->right)
	{
		if (selected_menu_item_index < 0) selected_menu_item_index = 0;
		else if (selected_menu_item_index >= 5) selected_menu_item_index = 5;
		else selected_menu_item_index++;
	}
	if (k_input->up)
	{
		if (selected_menu_item_index < 0) selected_menu_item_index = 0;
		else if (selected_menu_item_index > 1) selected_menu_item_index -= 2;
	}
	if (k_input->down)
	{
		if (selected_menu_item_index < 0) selected_menu_item_index = 0;
		else if (selected_menu_item_index < 4) selected_menu_item_index += 2;
	}
	if (k_input->enter)
	{
		mode = selected_menu_item_index;
	}
	if (k_input->escape)
	{
		mode = -1;
	}
		
	int width = buffer->width;
	int height = buffer->height;
	
	// BACKGROUND
	// draw_rectangle(buffer, 0, 0, width - 1, height - 1, 0x302782);
	
	// BUTTONS
	
	// draw_rectangle(buffer, width / 8, height / 26, (width / 8) + 75, (height / 26) + 60, 0xBBBBBB);
	// draw_rectangle(buffer, (width / 8) + 100, height / 26, (width / 8) + 175, (height / 26 + 60), 0xBBBBBB);

/*	
	unsigned int button_color = 0xBBBBBB;
	ui_rectangle button1;

	button1.min_x = width / 8;
	button1.min_y = height / 26;
	button1.max_x = (width / 8) + 75;
	button1.max_y = (height / 26) + 60;
	button1.color = button_color;
	
	ui_rectangle button2;
	
	button2.min_x = (width / 8) + 100;
	button2.min_y = height / 26;
	button2.max_x = (width / 8) + 175;
	button2.max_y = (height / 26) + 60;
	button2.color = button_color;
	
	draw_rectangle(buffer, button1.min_x, button1.min_y, button1.max_x, button1.max_y, button1.color);
	draw_rectangle(buffer, button2.min_x, button2.min_y, button2.max_x, button2.max_y, button2.color);
*/

	if (mode < 0)
	{
		draw_rectangle(buffer, 0, 0, width - 1, height - 1, 0x302782, 0, 0);
		
		int menu_item_count = 6;
		int menu_item_width = 135;
		int menu_item_height = 100;
		
		int center_x = width / 2;
		int center_y = height / 2;

		// HACK: This is terrible for really small window sizes. (Is this still true?)
		int center_y_offset = 160;
		
		int current_y = center_y - (menu_item_height * (menu_item_count / 2)) + center_y_offset;
		int num_rows = menu_item_count / 2;
		
		int current_menu_item_index = 0;
		for (int i = 0; i < num_rows; i++)
		{
			unsigned int border_color = 0x000000;
			if (current_menu_item_index == selected_menu_item_index) border_color = 0xFFFFFF;
			draw_rectangle(buffer, center_x - menu_item_width, current_y, center_x, current_y + menu_item_height, 0xBBBBBB, 5, border_color);
			
			current_menu_item_index++;
			
			if (current_menu_item_index == selected_menu_item_index) border_color = 0xFFFFFF;
			else border_color = 0x000000;
			
			draw_rectangle(buffer, center_x, current_y, center_x + menu_item_width, current_y + menu_item_height, 0xBBBBBB, 5, border_color);
			
			current_y += menu_item_height;
			current_menu_item_index++;
		}
	}
	else if (mode == 0)
	{
		draw_rectangle(buffer, 0, 0, width - 1, height - 1, 0x00FF00, 0, 0);
	}
	else if (mode == 1)
	{
		draw_rectangle(buffer, 0, 0, width - 1, height - 1, 0xFF0000, 0, 0);
	}
	else if (mode == 2)
	{
		draw_rectangle(buffer, 0, 0, width - 1, height - 1, 0x0000FF, 0, 0);
	}
	
	test_render_text(buffer, font_bitmaps);

/*	
	unsigned int tile_map[9][17] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 1, 1, 0,  0, 1, 1, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 1, 1, 0,  0, 1, 1, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 1, 1, 0,  0, 1, 1, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 1, 1, 0,  0, 1, 1, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
    };

    // int upper_left_x = -30;
    // int upper_left_y = 0;
    int tile_width = 60;
    int tile_height = 60;
    
    for(int row = 0; row < 9; row++)
    {
        for(int column = 0; column < 17; column++)
        {
            unsigned int tile_id = tile_map[row][column];
            unsigned int gray = 0xBBBBBB;
			
            if(tile_id == 1)
            {
                int min_x = column * tile_width;
				int min_y = row * tile_height;
				int max_x = min_x + tile_width;
				int max_y = min_y + tile_height;
				draw_rectangle(buffer, min_x, min_y, max_x, max_y, gray, 0, 0);
            }
        }
    }
*/
	
	// SEARCH BAR
	// draw_rectangle(buffer, width / 7, (int)(height / 4.6f), width - (width / 8), (int)(height / 2.9f), 0xFFFFFF);
	
	// draw_rectangle(buffer, 0 + (width / 7), 0 + (height / 4), width - (width / 2), height - (height / 8), 0xFFFFFF); 
	
	// render_weird_gradient(buffer, m_input->x_pos, m_input->y_pos);
	
	if (m_input->clicked)
	{
		int x = m_input->x_pos;
		int y = m_input->y_pos;
		
		int x_offset = 50;
		int y_offset = 35;
		
		draw_rectangle(buffer, x - x_offset, y - y_offset, x + x_offset, y + y_offset, 0xFFFFFF, 0, 0);
	}
}