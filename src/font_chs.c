#include "font_chs.h"
#include "font_chs_data.h"

size_t strlen_utf8(const char* s)
{
	for (size_t n = 0;; n++)
	{
		int c = *(const unsigned char*)s;
		if (!c)
			return n;
		if (c < 0x80)
			s++;
		else if (c < 0xe0)
			s += 2;
		else if (c < 0xf0)
			s += 3;
		else
			s += 4;
	}
}

int next_utf8_char(const char** pp)
{
	int c = *(const unsigned char*)(*pp);
	if (c < 0x80)
	{
		if (c > 0)
			(*pp)++;
		return c;
	}
	(*pp)++;
	if (c < 0xe0)
	{
		int d = *(const unsigned char*)((*pp)++);
		return ((c & 0x1f) << 6) + (d & 0x3f);
	}
	if (c < 0xf0)
	{
		int d = *(const unsigned char*)((*pp)++);
		int e = *(const unsigned char*)((*pp)++);
		return ((c & 0xf) << 12) + ((d & 0x3f) << 6) + (e & 0x3f);
	}
	int d = *(const unsigned char*)((*pp)++);
	int e = *(const unsigned char*)((*pp)++);
	int f = *(const unsigned char*)((*pp)++);
	return ((c & 0x7) << 18) + ((d & 0x3f) << 12) + ((e & 0x3f) << 6) + (f & 0x3f);
}

int char_advance(int c)
{
	if (c <= 0xffff)
	{
		int idx = font_chs_index[c];
		if (idx >= 0)
			return (int)(font_chs_data[idx * 2 + 1] >> 57);
	}
	return 0;
}

int JE_textWidth_chs(const char* s, unsigned int font)
{
	(void)font; // only support tiny font now
	for (int w = 0;;)
	{
		int c = next_utf8_char(&s);
		if (!c)
			return w;
		w += char_advance(c);
	}
}

int blit_sprite_hv_chs(SDL_Surface* surface, int x, int y, int index, Uint8 hue, Sint8 value)
{
	Uint8 v = 8 + value;
	if (v > 0xf)
		v = 0xf;
	v += hue << 4;

	uint64_t d0 = font_chs_data[index * 2];
	uint64_t d1 = font_chs_data[index * 2 + 1];
	int b = 0;

	Uint8* sp = (Uint8*)surface->pixels;
	int ss = surface->pitch;
	int xe = x + 11;
	if (xe > surface->w)
		xe = surface->w;
	int ye = y + 11;
	if (ye > surface->h)
		ye = surface->h;
	for (Uint8* bp = sp + ss * y + x; y < ye; y++, bp += ss)
	{
		int i = 0;
		for (; x + i < xe; i++)
		{
			if (d0 & 1)
				bp[i] = v;
			d0 >>= 1;
			if (++b == 64)
			{
				b = 0;
				d0 = d1;
			}
		}
		if (i < 11)
		{
			i = 11 - i;
			d0 >>= i;
			if ((b += i) >= 64)
			{
				b -= 64;
				d0 = d1 >> b;
			}
		}
	}
	return (int)(d1 >> 57);
}

int blit_sprite_hv_blend_chs(SDL_Surface* surface, int x, int y, int index, Uint8 hue, Sint8 value)
{
	Uint8 v = 8 + value;
	if (v > 0xf)
		v = 0xf;
	hue <<= 4;

	uint64_t d0 = font_chs_data[index * 2];
	uint64_t d1 = font_chs_data[index * 2 + 1];
	int b = 0;

	Uint8* sp = (Uint8*)surface->pixels;
	int ss = surface->pitch;
	int xe = x + 11;
	if (xe > surface->w)
		xe = surface->w;
	int ye = y + 11;
	if (ye > surface->h)
		ye = surface->h;
	for (Uint8* bp = sp + ss * y + x; y < ye; y++, bp += ss)
	{
		int i = 0;
		for (; x + i < xe; i++)
		{
			if (d0 & 1)
				bp[i] = hue + (((bp[i] & 0xf) + v) >> 1);
			d0 >>= 1;
			if (++b == 64)
			{
				b = 0;
				d0 = d1;
			}
		}
		if (i < 11)
		{
			i = 11 - i;
			d0 >>= i;
			if ((b += i) >= 64)
			{
				b -= 64;
				d0 = d1 >> b;
			}
		}
	}
	return (int)(d1 >> 57);
}

int blit_sprite_dark_chs(SDL_Surface* surface, int x, int y, int index, int black)
{
	uint64_t d0 = font_chs_data[index * 2];
	uint64_t d1 = font_chs_data[index * 2 + 1];
	int b = 0;

	Uint8* sp = (Uint8*)surface->pixels;
	int ss = surface->pitch;
	int xe = x + 11;
	if (xe > surface->w)
		xe = surface->w;
	int ye = y + 11;
	if (ye > surface->h)
		ye = surface->h;
	for (Uint8* bp = sp + ss * y + x; y < ye; y++, bp += ss)
	{
		int i = 0;
		for (; x + i < xe; i++)
		{
			if (d0 & 1)
				bp[i] = black ? 0 : ((bp[i] & 0xf0) + ((bp[i] & 0xf) >> 1));
			d0 >>= 1;
			if (++b == 64)
			{
				b = 0;
				d0 = d1;
			}
		}
		if (i < 11)
		{
			i = 11 - i;
			d0 >>= i;
			if ((b += i) >= 64)
			{
				b -= 64;
				d0 = d1 >> b;
			}
		}
	}
	return (int)(d1 >> 57);
}

void draw_font_hv_chs(SDL_Surface* surface, int x, int y, const char* text, Uint8 hue, Sint8 value)
{
	for (int highlight = 0, c; (c = next_utf8_char(&text)) != 0;)
	{
		if (c == '~')
		{
			highlight ^= 1;
			if (highlight)
				value += 4;
			else
				value -= 4;
		}
		else if (c <= 0xffff)
		{
			int idx = font_chs_index[c];
			if (idx >= 0)
				x += blit_sprite_hv_chs(surface, x, y, idx, hue, value);
		}
	}
}

void draw_font_hv_blend_chs(SDL_Surface* surface, int x, int y, const char* text, Uint8 hue, Sint8 value)
{
	for (int c; (c = next_utf8_char(&text)) != 0;)
	{
		if (c <= 0xffff && c != '~')
		{
			int idx = font_chs_index[c];
			if (idx >= 0)
				x += blit_sprite_hv_blend_chs(surface, x, y, idx, hue, value);
		}
	}
}

void draw_font_dark_chs(SDL_Surface* surface, int x, int y, const char* text, int black)
{
	for (int c; (c = next_utf8_char(&text)) != 0;)
	{
		if (c <= 0xffff && c != '~')
		{
			int idx = font_chs_index[c];
			if (idx >= 0)
				x += blit_sprite_dark_chs(surface, x, y, idx, black);
		}
	}
}

void JE_outText_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int colorbank, int brightness)
{
	for (int bright = 0, c; (c = next_utf8_char(&s)) != 0;)
	{
		if (c == '~')
		{
			bright = (bright == 0) ? 4 : 0;
		}
		else if (c <= 0xffff)
		{
			int idx = font_chs_index[c];
			if (idx >= 0)
			{
				if (brightness >= 0)
					x += blit_sprite_hv_chs(screen, x, y, idx, colorbank, brightness + bright); // blit_sprite_hv_unsafe_chs
				else
					x += blit_sprite_dark_chs(screen, x, y, idx, 1);
			}
		}
	}
}

void JE_outTextModify_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int filter, unsigned int brightness)
{
	for (int c; (c = next_utf8_char(&s)) != 0;)
	{
		if (c <= 0xffff)
		{
			int idx = font_chs_index[c];
			if (idx >= 0)
				x += blit_sprite_hv_blend_chs(screen, x, y, idx, filter, brightness);
		}
	}
}

void JE_outTextAdjust_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int filter, int brightness, int shadow)
{
	for (int bright = 0, c; (c = next_utf8_char(&s)) != 0;)
	{
		if (c == '~')
		{
			bright = (bright == 0) ? 4 : 0;
		}
		else if (c <= 0xffff)
		{
			int idx = font_chs_index[c];
			if (idx >= 0)
			{
				if (shadow)
					blit_sprite_dark_chs(screen, x + 2, y + 2, idx, 0);
				x += blit_sprite_hv_chs(screen, x, y, idx, filter, brightness + bright);
			}
		}
	}
}

void JE_outTextAndDarken_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int colorbank, unsigned int brightness)
{
	for (int bright = 0, c; (c = next_utf8_char(&s)) != 0;)
	{
		if (c == '~')
		{
			bright = (bright == 0) ? 4 : 0;
		}
		else if (c <= 0xffff)
		{
			int idx = font_chs_index[c];
			if (idx >= 0)
			{
				blit_sprite_dark_chs(screen, x + 1, y + 1, idx, 0);
				x += blit_sprite_hv_chs(screen, x, y, idx, colorbank, brightness + bright); // blit_sprite_hv_unsafe_chs
			}
		}
	}
}
