/* 
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "sprite.h"

#include "file.h"
#include "opentyr.h"
#include "video.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#include "font_chs.h"

Sprite_array sprite_table[SPRITE_TABLES_MAX];

Sprite2_array shopSpriteSheet;

Sprite2_array explosionSpriteSheet;

Sprite2_array enemySpriteSheets[4];
Uint8 enemySpriteSheetIds[4];

Sprite2_array destructSpriteSheet;

Sprite2_array spriteSheet8;
Sprite2_array spriteSheet9;
Sprite2_array spriteSheet10;
Sprite2_array spriteSheet11;
Sprite2_array spriteSheet12;
Sprite2_array spriteSheetT2000;

void load_sprites_file(unsigned int table, const char *filename)
{
	free_sprites(table);
	
	FILE *f = dir_fopen_die(data_dir(), filename, "rb");
	
	load_sprites(table, f);
	
	fclose(f);
}

void load_sprites(unsigned int table, FILE *f)
{
	free_sprites(table);
	
	Uint16 temp;
	fread_u16_die(&temp, 1, f);
	
	sprite_table[table].count = temp;
	
	assert(sprite_table[table].count <= SPRITES_PER_TABLE_MAX);
	
	for (unsigned int i = 0; i < sprite_table[table].count; ++i)
	{
		Sprite * const cur_sprite = sprite(table, i);

		bool populated;
		fread_bool_die(&populated, f);
		if (!populated) // sprite is empty
			continue;
		
		fread_u16_die(&cur_sprite->width,  1, f);
		fread_u16_die(&cur_sprite->height, 1, f);
		fread_u16_die(&cur_sprite->size,   1, f);
		
		cur_sprite->data = malloc(cur_sprite->size);
		
		fread_u8_die(cur_sprite->data, cur_sprite->size, f);
	}
}

void free_sprites(unsigned int table)
{
	for (unsigned int i = 0; i < sprite_table[table].count; ++i)
	{
		Sprite * const cur_sprite = sprite(table, i);
		
		cur_sprite->width  = 0;
		cur_sprite->height = 0;
		cur_sprite->size   = 0;
		
		free(cur_sprite->data);
		cur_sprite->data = NULL;
	}
	
	sprite_table[table].count = 0;
}

// does not clip on left or right edges of surface
void blit_sprite(SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index)
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
				*pixels = *data;
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite_blend(SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index)
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
				*pixels = (*data & 0xf0) | (((*pixels & 0x0f) + (*data & 0x0f)) / 2);
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
// unsafe because it doesn't check that value won't overflow into hue
// we can replace it when we know that we don't rely on that 'feature'
void blit_sprite_hv_unsafe(SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index, Uint8 hue, Sint8 value)
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	hue <<= 4;
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
				*pixels = hue | ((*data & 0x0f) + value);
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite_hv(SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index, Uint8 hue, Sint8 value)
{
#if ENABLE_CHS
	if (table == TINY_FONT)
	{
		blit_sprite_hv_chs(surface, x, y, get_char_index(index), hue, value);
		return;
	}
#endif
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	hue <<= 4;
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
			{
				Uint8 temp_value = (*data & 0x0f) + value;
				if (temp_value > 0xf)
					temp_value = (temp_value >= 0x1f) ? 0x0 : 0xf;
				
				*pixels = hue | temp_value;
			}
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite_hv_blend(SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index, Uint8 hue, Sint8 value)
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	hue <<= 4;
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
			{
				Uint8 temp_value = (*data & 0x0f) + value;
				if (temp_value > 0xf)
					temp_value = (temp_value >= 0x1f) ? 0x0 : 0xf;
				
				*pixels = hue | (((*pixels & 0x0f) + temp_value) / 2);
			}
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite_dark(SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index, bool black)
{
#if ENABLE_CHS
	if (table == TINY_FONT)
	{
		blit_sprite_dark_chs(surface, x, y, get_char_index(index), (int)black);
		return;
	}
#endif
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
				*pixels = black ? 0x00 : ((*pixels & 0xf0) | ((*pixels & 0x0f) / 2));
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

void JE_loadCompShapes(Sprite2_array *sprite2s, char s)
{
	free_sprite2s(sprite2s);

	char buffer[20];
	snprintf(buffer, sizeof(buffer), "newsh%c.shp", tolower((unsigned char)s));
	
	FILE *f = dir_fopen_die(data_dir(), buffer, "rb");
	
	sprite2s->size = ftell_eof(f);
	
	JE_loadCompShapesB(sprite2s, f);
	
	fclose(f);
}

void JE_loadCompShapesB(Sprite2_array *sprite2s, FILE *f)
{
	assert(sprite2s->data == NULL);

	sprite2s->data = malloc(sprite2s->size);
	fread_u8_die(sprite2s->data, sprite2s->size, f);
}

void free_sprite2s(Sprite2_array *sprite2s)
{
	free(sprite2s->data);
	sprite2s->data = NULL;

	sprite2s->size = 0;
}

// does not clip on left or right edges of surface
void blit_sprite2(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index)
{
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);
	
	for (; *data != 0x0f; ++data)
	{
		pixels += *data & 0x0f;                   // second nibble: transparent pixel count
		unsigned int count = (*data & 0xf0) >> 4; // first nibble: opaque pixel count
		
		if (count == 0) // move to next pixel row
		{
			pixels += VGAScreen->pitch - 12;
		}
		else
		{
			while (count--)
			{
				++data;
				
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll)
					*pixels = *data;
				
				++pixels;
			}
		}
	}
}

void blit_sprite2_clip(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index)
{
	assert(surface->format->BitsPerPixel == 8);

	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);

	for (; *data != 0x0f; ++data)
	{
		if (y >= surface->h)
			return;

		Uint8 skip_count = *data & 0x0f;
		Uint8 fill_count = (*data >> 4) & 0x0f;

		x += skip_count;

		if (fill_count == 0) // move to next pixel row
		{
			y += 1;
			x -= 12;
		}
		else if (y >= 0)
		{
			Uint8 *const pixel_row = (Uint8 *)surface->pixels + (y * surface->pitch);
			do
			{
				++data;

				if (x >= 0 && x < surface->pitch)
					pixel_row[x] = *data;
				x += 1;
			} while (--fill_count);
		}
		else
		{
			data += fill_count;
			x += fill_count;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite2_blend(SDL_Surface *surface,  int x, int y, Sprite2_array sprite2s, unsigned int index)
{
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);
	
	for (; *data != 0x0f; ++data)
	{
		pixels += *data & 0x0f;                   // second nibble: transparent pixel count
		unsigned int count = (*data & 0xf0) >> 4; // first nibble: opaque pixel count
		
		if (count == 0) // move to next pixel row
		{
			pixels += VGAScreen->pitch - 12;
		}
		else
		{
			while (count--)
			{
				++data;
				
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll)
					*pixels = (((*data & 0x0f) + (*pixels & 0x0f)) / 2) | (*data & 0xf0);
				
				++pixels;
			}
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite2_darken(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index)
{
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);
	
	for (; *data != 0x0f; ++data)
	{
		pixels += *data & 0x0f;                   // second nibble: transparent pixel count
		unsigned int count = (*data & 0xf0) >> 4; // first nibble: opaque pixel count
		
		if (count == 0) // move to next pixel row
		{
			pixels += VGAScreen->pitch - 12;
		}
		else
		{
			while (count--)
			{
				++data;
				
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll)
					*pixels = ((*pixels & 0x0f) / 2) + (*pixels & 0xf0);
				
				++pixels;
			}
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite2_filter(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index, Uint8 filter)
{
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);
	
	for (; *data != 0x0f; ++data)
	{
		pixels += *data & 0x0f;                   // second nibble: transparent pixel count
		unsigned int count = (*data & 0xf0) >> 4; // first nibble: opaque pixel count
		
		if (count == 0) // move to next pixel row
		{
			pixels += VGAScreen->pitch - 12;
		}
		else
		{
			while (count--)
			{
				++data;
				
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll)
					*pixels = filter | (*data & 0x0f);
				
				++pixels;
			}
		}
	}
}

void blit_sprite2_filter_clip(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index, Uint8 filter)
{
	assert(surface->format->BitsPerPixel == 8);

	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);

	for (; *data != 0x0f; ++data)
	{
		if (y >= surface->h)
			return;

		Uint8 skip_count = *data & 0x0f;
		Uint8 fill_count = (*data >> 4) & 0x0f;

		x += skip_count;

		if (fill_count == 0) // move to next pixel row
		{
			y += 1;
			x -= 12;
		}
		else if (y >= 0)
		{
			Uint8 *const pixel_row = (Uint8 *)surface->pixels + (y * surface->pitch);
			do
			{
				++data;

				if (x >= 0 && x < surface->pitch)
					pixel_row[x] = filter | (*data & 0x0f);;
				x += 1;
			} while (--fill_count);
		}
		else
		{
			data += fill_count;
			x += fill_count;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite2x2(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index)
{
	blit_sprite2(surface, x,      y,      sprite2s, index);
	blit_sprite2(surface, x + 12, y,      sprite2s, index + 1);
	blit_sprite2(surface, x,      y + 14, sprite2s, index + 19);
	blit_sprite2(surface, x + 12, y + 14, sprite2s, index + 20);
}

void blit_sprite2x2_clip(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index)
{
	blit_sprite2_clip(surface, x,      y,      sprite2s, index);
	blit_sprite2_clip(surface, x + 12, y,      sprite2s, index + 1);
	blit_sprite2_clip(surface, x,      y + 14, sprite2s, index + 19);
	blit_sprite2_clip(surface, x + 12, y + 14, sprite2s, index + 20);
}

// does not clip on left or right edges of surface
void blit_sprite2x2_blend(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index)
{
	blit_sprite2_blend(surface, x,      y,      sprite2s, index);
	blit_sprite2_blend(surface, x + 12, y,      sprite2s, index + 1);
	blit_sprite2_blend(surface, x,      y + 14, sprite2s, index + 19);
	blit_sprite2_blend(surface, x + 12, y + 14, sprite2s, index + 20);
}

// does not clip on left or right edges of surface
void blit_sprite2x2_darken(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index)
{
	blit_sprite2_darken(surface, x,      y,      sprite2s, index);
	blit_sprite2_darken(surface, x + 12, y,      sprite2s, index + 1);
	blit_sprite2_darken(surface, x,      y + 14, sprite2s, index + 19);
	blit_sprite2_darken(surface, x + 12, y + 14, sprite2s, index + 20);
}

// does not clip on left or right edges of surface
void blit_sprite2x2_filter(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index, Uint8 filter)
{
	blit_sprite2_filter(surface, x,      y,      sprite2s, index, filter);
	blit_sprite2_filter(surface, x + 12, y,      sprite2s, index + 1, filter);
	blit_sprite2_filter(surface, x,      y + 14, sprite2s, index + 19, filter);
	blit_sprite2_filter(surface, x + 12, y + 14, sprite2s, index + 20, filter);
}

void blit_sprite2x2_filter_clip(SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index, Uint8 filter)
{
	blit_sprite2_filter_clip(surface, x,      y,      sprite2s, index, filter);
	blit_sprite2_filter_clip(surface, x + 12, y,      sprite2s, index + 1, filter);
	blit_sprite2_filter_clip(surface, x,      y + 14, sprite2s, index + 19, filter);
	blit_sprite2_filter_clip(surface, x + 12, y + 14, sprite2s, index + 20, filter);
}

void JE_loadMainShapeTables(const char *shpfile)
{
	enum { SHP_NUM = 13 };
	
	FILE *f = dir_fopen_die(data_dir(), shpfile, "rb");
	
	JE_word shpNumb;
	JE_longint shpPos[SHP_NUM + 1]; // +1 for storing file length
	
	fread_u16_die(&shpNumb, 1, f);
	assert(shpNumb + 1u == COUNTOF(shpPos));
	
	fread_s32_die(shpPos, shpNumb, f);
	
	fseek(f, 0, SEEK_END);
	for (unsigned int i = shpNumb; i < COUNTOF(shpPos); ++i)
		shpPos[i] = ftell(f);
	
	int i;
	// fonts, interface, option sprites
	for (i = 0; i < 7; i++)
	{
		fseek(f, shpPos[i], SEEK_SET);
		load_sprites(i, f);
	}
	
	// player shot sprites
	spriteSheet8.size = shpPos[i + 1] - shpPos[i];
	JE_loadCompShapesB(&spriteSheet8, f);
	i++;
	
	// player ship sprites
	spriteSheet9.size = shpPos[i + 1] - shpPos[i];
	JE_loadCompShapesB(&spriteSheet9 , f);
	i++;
	
	// power-up sprites
	spriteSheet10.size = shpPos[i + 1] - shpPos[i];
	JE_loadCompShapesB(&spriteSheet10, f);
	i++;
	
	// coins, datacubes, etc sprites
	spriteSheet11.size = shpPos[i + 1] - shpPos[i];
	JE_loadCompShapesB(&spriteSheet11, f);
	i++;
	
	// more player shot sprites
	spriteSheet12.size = shpPos[i + 1] - shpPos[i];
	JE_loadCompShapesB(&spriteSheet12, f);
	i++;

	// tyrian 2000 ship sprites
	spriteSheetT2000.size = shpPos[i + 1] - shpPos[i];
	JE_loadCompShapesB(&spriteSheetT2000, f);
	
	fclose(f);
}

void free_main_shape_tables(void)
{
	for (uint i = 0; i < COUNTOF(sprite_table); ++i)
		free_sprites(i);
	
	free_sprite2s(&spriteSheet8);
	free_sprite2s(&spriteSheet9);
	free_sprite2s(&spriteSheet10);
	free_sprite2s(&spriteSheet11);
	free_sprite2s(&spriteSheet12);
}
