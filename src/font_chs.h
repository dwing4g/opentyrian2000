#ifndef FONT_CHS_H
#define FONT_CHS_H

#include "SDL.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_CHS 1

int strlen_utf8(const char* s);
int next_utf8_char(const char** pp);
int get_char_index(int c);
int char_advance(int c);
int JE_textWidth_chs(const char* s, unsigned int font);

int blit_sprite_hv_chs(SDL_Surface* surface, int x, int y, int index, Uint8 hue, Sint8 value);
int blit_sprite_hv_blend_chs(SDL_Surface* surface, int x, int y, int index, Uint8 hue, Sint8 value);
int blit_sprite_dark_chs(SDL_Surface* surface, int x, int y, int index, int black);

void draw_font_hv_chs(SDL_Surface* surface, int x, int y, const char* text, Uint8 hue, Sint8 value);
void draw_font_hv_blend_chs(SDL_Surface* surface, int x, int y, const char* text, Uint8 hue, Sint8 value);
void draw_font_dark_chs(SDL_Surface* surface, int x, int y, const char* text, int black);

void JE_outText_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int colorbank, int brightness);
void JE_outTextModify_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int filter, unsigned int brightness);
void JE_outTextAdjust_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int filter, int brightness, int shadow);
void JE_outTextAndDarken_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int colorbank, unsigned int brightness);

const char* translate(const char* line);
void translate_inline(char* line, int size);
const char* untranslate(const char* line);

#ifdef __cplusplus
}
#endif

#endif
