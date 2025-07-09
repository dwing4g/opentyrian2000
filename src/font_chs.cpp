#include <stdio.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <windows.h>

#include "font_chs.h"
#include "font_chs_data.h"

extern "C" int strlen_utf8(const char* s)
{
#if ENABLE_CHS
	for (int n = 0;; n++)
	{
		int c = *(const unsigned char*)s;
		if (!c)
			return n;
		if (c < 0x80) // 0xxx xxxx
			s++;
		else if (c < 0xe0) // 110x xxxx
			s += 2;
		else if (c < 0xf0) // 1110 xxxx
			s += 3;
		else
			s += 4;
	}
#else
	return (int)strlen(s);
#endif
}

extern "C" int next_utf8_char(const char** pp)
{
#if ENABLE_CHS
	int c = *(const unsigned char*)(*pp);
	if (c < 0x80) // 0xxx xxxx
	{
		if (c > 0)
			(*pp)++;
		return c;
	}
	(*pp)++;
	if (c < 0xe0) // 110x xxxx
	{
		int d = *(const unsigned char*)((*pp)++);
		return ((c & 0x1f) << 6) + (d & 0x3f);
	}
	if (c < 0xf0) // 1110 xxxx
	{
		int d = *(const unsigned char*)((*pp)++);
		int e = *(const unsigned char*)((*pp)++);
		return ((c & 0xf) << 12) + ((d & 0x3f) << 6) + (e & 0x3f);
	}
	int d = *(const unsigned char*)((*pp)++);
	int e = *(const unsigned char*)((*pp)++);
	int f = *(const unsigned char*)((*pp)++);
	return ((c & 0x7) << 18) + ((d & 0x3f) << 12) + ((e & 0x3f) << 6) + (f & 0x3f);
#else
	return *(*pp)++;
#endif
}

extern "C" int get_char_index(int c)
{
	if (c <= 0xffff)
	{
		int idx = font_chs_index[c];
		if (idx != (int)(uint16_t)-1)
			return idx;
	}
	return font_chs_index[0x231b]; // for showing unavailable char
}

extern "C" int char_advance(int c)
{
	if (c == '~')
		return 0;
	return (int)(font_chs_data[get_char_index(c) * 2 + 1] >> 57);
}

extern "C" int JE_textWidth_chs(const char* s, unsigned int font)
{
	(void)font; // only support tiny font now
	// s = translate2(s, 0);
	for (int w = 0;;)
	{
		int c = next_utf8_char(&s);
		if (!c)
			return w;
		if (c != '~')
			w += char_advance(c);
	}
}

extern "C" int blit_sprite_hv_chs(SDL_Surface* surface, int x, int y, int index, Uint8 hue, Sint8 value)
{
	Uint8 v = (Uint8)(8 + value);
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
				d0 = d1 >> (b -= 64);
		}
	}
	return (int)(d1 >> 57);
}

extern "C" int blit_sprite_hv_blend_chs(SDL_Surface* surface, int x, int y, int index, Uint8 hue, Sint8 value)
{
	Uint8 v = (Uint8)(8 + value);
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
				bp[i] = (Uint8)(hue + (((bp[i] & 0xf) + v) >> 1));
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
				d0 = d1 >> (b -= 64);
		}
	}
	return (int)(d1 >> 57);
}

extern "C" int blit_sprite_dark_chs(SDL_Surface* surface, int x, int y, int index, int black)
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
				bp[i] = (Uint8)(black ? 0 : ((bp[i] & 0xf0) + ((bp[i] & 0xf) >> 1)));
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
				d0 = d1 >> (b -= 64);
		}
	}
	return (int)(d1 >> 57);
}

extern "C" void draw_font_hv_chs(SDL_Surface* surface, int x, int y, const char* text, Uint8 hue, Sint8 value)
{
	// text = translate(text);
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
			x += blit_sprite_hv_chs(surface, x, y, get_char_index(c), hue, value);
	}
}

extern "C" void draw_font_hv_blend_chs(SDL_Surface* surface, int x, int y, const char* text, Uint8 hue, Sint8 value)
{
	// text = translate(text);
	for (int c; (c = next_utf8_char(&text)) != 0;)
	{
		if (c <= 0xffff && c != '~')
			x += blit_sprite_hv_blend_chs(surface, x, y, get_char_index(c), hue, value);
	}
}

extern "C" void draw_font_dark_chs(SDL_Surface* surface, int x, int y, const char* text, int black)
{
	// text = translate(text);
	for (int c; (c = next_utf8_char(&text)) != 0;)
	{
		if (c <= 0xffff && c != '~')
			x += blit_sprite_dark_chs(surface, x, y, get_char_index(c), black);
	}
}

extern "C" void JE_outText_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int colorbank, int brightness)
{
	// s = translate(s);
	for (int bright = 0, c; (c = next_utf8_char(&s)) != 0;)
	{
		if (c == '~')
			bright = (bright == 0) ? 4 : 0;
		else if (c <= 0xffff)
		{
			int idx = get_char_index(c);
			if (brightness >= 0)
				x += blit_sprite_hv_chs(screen, x, y, idx, colorbank, brightness + bright); // blit_sprite_hv_unsafe_chs
			else
				x += blit_sprite_dark_chs(screen, x, y, idx, 1);
		}
	}
}

extern "C" void JE_outTextModify_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int filter, unsigned int brightness)
{
	// s = translate(s);
	for (int c; (c = next_utf8_char(&s)) != 0;)
	{
		if (c <= 0xffff)
			x += blit_sprite_hv_blend_chs(screen, x, y, get_char_index(c), filter, (Sint8)brightness);
	}
}

extern "C" void JE_outTextAdjust_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int filter, int brightness, int shadow)
{
	// s = translate(s);
	for (int bright = 0, c; (c = next_utf8_char(&s)) != 0;)
	{
		if (c == '~')
			bright = (bright == 0) ? 4 : 0;
		else if (c <= 0xffff)
		{
			int idx = get_char_index(c);
			if (shadow)
				blit_sprite_dark_chs(screen, x + 2, y + 2, idx, 0);
			x += blit_sprite_hv_chs(screen, x, y, idx, filter, brightness + bright);
		}
	}
}

extern "C" void JE_outTextAndDarken_chs(SDL_Surface* screen, int x, int y, const char* s, unsigned int colorbank, unsigned int brightness)
{
	// s = translate(s);
	for (int bright = 0, c; (c = next_utf8_char(&s)) != 0;)
	{
		if (c == '~')
			bright = (bright == 0) ? 4 : 0;
		else if (c <= 0xffff)
		{
			int idx = get_char_index(c);
			blit_sprite_dark_chs(screen, x + 1, y + 1, idx, 0);
			x += blit_sprite_hv_chs(screen, x, y, idx, colorbank, (Sint8)(brightness + bright)); // blit_sprite_hv_unsafe_chs
		}
	}
}

static std::unordered_map<std::string, std::string>* s_trans = 0;
static std::unordered_map<std::string, std::string>* s_untrans = 0;

static void try_init()
{
	if (s_trans)
		return;
	s_trans = new std::unordered_map<std::string, std::string>;
	s_untrans = new std::unordered_map<std::string, std::string>;
	const int MAX_BUF_SIZE = 4000;
	char buf[MAX_BUF_SIZE];
	int n = 0;
	WIN32_FIND_DATAW fd;
	HANDLE hf = FindFirstFileW(L"translation\\*.txt", &fd);
	if (hf != INVALID_HANDLE_VALUE)
	{
		do
			if (fd.cFileName[0] != '.')
			{
				wchar_t pathName[MAX_PATH];
				wsprintf(pathName, L"translation\\%s", fd.cFileName);
				FILE* fp = _wfopen(pathName, L"rb");
				if (fp)
				{
					std::string bufe;
					while (fgets(buf, MAX_BUF_SIZE, fp))
					{
						size_t n = strlen(buf);
						while (n && (unsigned char)buf[n - 1] < ' ')
							n--;
						buf[n] = 0;
						if (*buf)
						{
							if (bufe.empty())
								bufe = buf;
							else
							{
								s_trans->insert(std::make_pair(bufe, buf));
								s_untrans->insert(std::make_pair(buf, bufe));
								bufe.clear();
							}
						}
						else
							bufe.clear();
					}
					fclose(fp);
				}
				n++;
			}
		while (FindNextFileW(hf, &fd));
		FindClose(hf);
	}
	printf("loaded %d,%d pairs in %d files\n", (int)s_trans->size(), (int)s_untrans->size(), n);
}

const char* translate2(const char* msg, int may_export)
{
	(void)may_export;
#if ENABLE_CHS
	if (!msg || !*msg)
		return msg;
	try_init();
	size_t n = strlen(msg);
	while (n > 0 && msg[n - 1] <= ' ')
		n--;
	std::string str(msg, n);
	for (size_t i = 0; i < n; i++)
	{
		if (str[i] < 0) // maybe: 0xfe 0xf8
			str[i] = ' ';
	}
	std::unordered_map<std::string, std::string>::const_iterator it = s_trans->find(str);
	if (it != s_trans->end())
		return it->second.c_str();
	static FILE* s_fp = 0;
	static std::unordered_set<std::string>* untrans = 0;
	if (!untrans)
	{
		untrans = new std::unordered_set<std::string>;
		s_fp = _wfopen(L"untrans.txt", L"ab");
	}
	if (may_export && s_fp && untrans->insert(str).second)
	{
		fputs(msg, s_fp);
		fputs("\n\n", s_fp);
		fflush(s_fp);
	}
#endif
	return msg;
}

extern "C" const char* translate(const char* msg)
{
	return translate2(msg, 1);
}

extern "C" void translate_inline(char* line, int size)
{
	const char* trans = translate2(line, 1);
	if (trans != line)
	{
		strncpy(line, trans, (size_t)size - 1);
		line[size - 1] = '\0';
	}
}

extern "C" const char* untranslate(const char* msg)
{
#if ENABLE_CHS
	if (msg && *msg)
	{
		try_init();
		std::unordered_map<std::string, std::string>::const_iterator it = s_untrans->find(msg);
		if (it != s_untrans->end())
			return it->second.c_str();
	}
#endif
	return msg;
}
