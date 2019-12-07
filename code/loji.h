struct character_bitmap
{
	unsigned char* memory;
	int width;
	int height;
};

struct font_buffer
{
	character_bitmap* memory;
};

struct offscreen_buffer
{
	void* memory;
	int width;
	int height;
	int bytes_per_pixel;
	int pitch;
};

struct keyboard_input
{
	bool left;
	bool right;
	bool up;
	bool down;
	
	bool enter;
	bool escape;
};

struct mouse_input
{
	bool clicked;
	int x_pos;
	int y_pos;
};