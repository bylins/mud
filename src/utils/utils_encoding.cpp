/**
\file utils_encoding.cpp - a part of the Bylins engine.
\brief Codepage conversion (KOI8-R <-> Alt/CP866, Windows-1251, Latin, UTF-8).
\detail issue.utils-cleaning: moved verbatim from utils.cpp. Conversion tables are byte-sensitive
        string literals -- they were transferred byte-for-byte.
*/

#include "utils_encoding.h"

#include <cstdio>
#include <cstring>
#include <string>
#ifdef HAVE_ICONV
#include <iconv.h>
#endif

namespace codepages {

char AltToKoi[] = {
		"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмноп░▒▓│┤╡+++╣║╗╝+╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨++╙╘╒++╪┘┌█▄▌▐▀рстуфхцчшщъыьэюяЁё╫╜╢╓╤╕╥╖·√??■ "
	};
char KoiToAlt[] = {
		"дЁз©юыц╢баеъэшщч╟╠╡+Ч+Ш+++Ъ+++З+м╨уЯУиВЫ╩тсх╬С╪фгл╣ПТ╧ЖЬкопйьРн+Н═║Ф╓╔ДёЕ╗╘╙╚╛╜╝╞ОЮАБЦ╕╒ЛК╖ХМИГЙ·─│√└┘■┐∙┬┴┼▀▄█▌▐÷░▒▓⌠├┌°⌡┤≤²≥≈ "
	};
char WinToKoi[] = {
		"++++++++++++++++++++++++++++++++ ++++╫++Ё©╢++++╥°+╤╕╜++·ё+╓++++╖АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмнопрстуфхцчшщъыьэюя"
	};
char KoiToWin[] = {
		"++++++++++++++++++++++++++═+╟+╥++++╦╨+Ё©+++++╢+++++╗╙+╡╞+++++╔+╘ЧЮАЖДЕТЦУХИЙКЛМНОЪПЯРСФБЭШГЬЩЫВЗчюаждетцухийклмноъпярсфбэшгьщывз"
	};
char KoiToWin2[] = {
		"++++++++++++++++++++++++++═+╟+╥++++╦╨+Ё©+++++╢+++++╗╙+╡╞+++++╔+╘ЧЮАЖДЕТЦУХИЙКЛМНОzПЯРСФБЭШГЬЩЫВЗчюаждетцухийклмноъпярсфбэшгьщывз"
	};
char AltToLat[] = {
		"─│┌┐└┘├┤┬┴┼▀▄█▌▐░▒▓⌠■∙√≈≤≥ ⌡°²·÷═║╒ё╓╔╕╖╗╘╙╚╛╜╝╞╟╠╡Ё╢╣╤╥╦╧╨╩╪╫╬©0abcdefghijklmnopqrstY1v23z456780ABCDEFGHIJKLMNOPQRSTY1V23Z45678"
	};

void koi_to_win(char *str, int size) {
	for (; size > 0; *str = KtoW(*str), size--, str++);
}

void koi_to_alt(char *str, int size) {
	for (; size > 0; *str = KtoA(*str), size--, str++);
}

std::string koi_to_alt(const std::string &input) {
	std::string result = input;
	for (std::size_t i = 0; i < result.size(); ++i) {
		result[i] = KtoA(result[i]);
	}
	return result;
}

#ifdef HAVE_ICONV
void koi_to_utf8(char *str_i, char *str_o)
{
	iconv_t cd;
	size_t len_i, len_o = kMaxSockBuf * 6;
	size_t i;

	if ((cd = iconv_open("UTF-8","KOI8-R")) == (iconv_t) - 1)
	{
		printf("koi_to_utf8: iconv_open error\n");
		return;
	}
	len_i = strlen(str_i);
	// const_cast at the next line is required for Linux, because there iconv has non-const input argument.
	if ((i = iconv(cd, &str_i, &len_i, &str_o, &len_o)) == (size_t) - 1)
	{
		printf("koi_to_utf8: iconv error\n");
		return;
	}
	*str_o = 0;
	if (iconv_close(cd) == -1)
	{
		printf("koi_to_utf8: iconv_close error\n");
		return;
	}
}

void utf8_to_koi(char *str_i, char *str_o)
{
	iconv_t cd;
	size_t len_i, len_o = kMaxSockBuf * 6;
	size_t i;

	if ((cd = iconv_open("KOI8-R", "UTF-8")) == (iconv_t) - 1)
	{
		perror("utf8_to_koi: iconv_open error");
		return;
	}
	len_i = strlen(str_i);
	if ((i=iconv(cd, &str_i, &len_i, &str_o, &len_o)) == (size_t) - 1)
	{
		perror("utf8_to_koi: iconv error");
	}
	if (iconv_close(cd) == -1)
	{
		perror("utf8_to_koi: iconv_close error");
		return;
	}
}

#else  // HAVE_ICONV

#define KOI8_UNKNOWN_CHAR  '+'  // char to use when cannot represent unicode codepoint in KOI8-R

// Simple implementation of UTF-8/KOI8-R converter, supports all codes available in KOI8-R
void utf8_to_koi(char *str_i, char *str_o) {
	unsigned char c;

	while ((c = static_cast<unsigned char>(*str_i++))) {
		if (c < 0x80) {
			*str_o++ = c;
		} else if (c < 0xC0) {
			// unexpected as a first byte in a sequence, skip
		} else if (c < 0xE0) {
			// one more byte to follow, must be b10xxxxxx
			unsigned char c1 = static_cast<unsigned char>(*str_i);
			if ((c1 & 0xC0) == 0x80) {
				// valid utf-8, but we are only interested in characters from
				// the first half of Unicode Cyrillic Block (0x400 to 0x47F)

				// init with unknown
				*str_o = KOI8_UNKNOWN_CHAR;

				if ((c & 0xFE) == 0xD0) // 0x0400 - 0x047F Cyrillic Unicode block first half
				{
					// get offset
					c1 -= (c & 1) ? 0x40 : 0x80;
					if (c1 >= 0x10 && c1 < 0x50) {
						const unsigned char Utf8ToKoiAlpha[] = {
							0xE1, 0xE2, 0xF7, 0xE7, 0xE4, 0xE5, 0xF6, 0xFA, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
							0xF0, // koi8-r АБВГДЕЖЗИЙКЛМНОП
							0xF2, 0xF3, 0xF4, 0xF5, 0xE6, 0xE8, 0xE3, 0xFE, 0xFB, 0xFD, 0xFF, 0xF9, 0xF8, 0xFC, 0xE0,
							0xF1,    // koi8-r РСТУФХЦЧШЩЪЫЬЭЮЯ
							0xC1, 0xC2, 0xD7, 0xC7, 0xC4, 0xC5, 0xD6, 0xDA, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
							0xD0, // koi8-r абвгдежзийклмноп
							0xD2, 0xD3, 0xD4, 0xD5, 0xC6, 0xC8, 0xC3, 0xDE, 0xDB, 0xDD, 0xDF, 0xD9, 0xD8, 0xDC, 0xC0,
							0xD1  // koi8-r рстуфхцчшщъыьэюя
						};
						*str_o = static_cast<char>(Utf8ToKoiAlpha[c1 - 0x10]);
					} else if (c1 == 0x01) {
						*str_o = '\xB3'; // koi8-r Ё
					} else if (c1 == 0x51) {
						*str_o = '\xA3'; // koi8-r ё
					}
				} else if (c == 0xC2) // 0x0080 - 0x00BF
				{
					// 0x00B0, 0x00B2, 0x00B7, 0x00F7
					if (c1 == 0xA0) {
						*str_o = '\x9A'; // NO-BREAK SPACE
					} else
					if (c1 == 0xA9) {
						*str_o = '\xBF';
					} else if (c1 == 0xB0) {
						*str_o = '\x9C';
					} else if (c1 == 0xB2) {
						*str_o = '\x9D';
					} else if (c1 == 0xB7) {
						*str_o = '\x9E';
					}
				} else if ((c == 0xC3) && (c1 == 0xB7)) // 0x00F7
				{
					*str_o = '\x9F';
				}
				str_o++;
				str_i++;
			}
		} else if (c < 0xF0) {
			// two more bytes to follow, must be b10xxxxxx

			if ((str_i[0] & 0xC0) == 0x80
				&& (str_i[1] & 0xC0) == 0x80) {
				// valid utf-8
				// calculate unicode codepoint
				unsigned short u = static_cast<unsigned short>(c) & 0x0F;
				u = (u << 6) | (str_i[0] & 0x3F);
				u = (u << 6) | (str_i[1] & 0x3F);
				*str_o = KOI8_UNKNOWN_CHAR;

				if (u >= 0x2550 && u <= 0x256C)// 0x2550-0x256C
				{
					const unsigned char Utf8ToKoiPg[] = {
						0xA0, 0xA1, 0xA2, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
						0xAF, // koi8-r ═║╒╓╔╕╖╗╘╙╚╛╜╝╞
						0xB0, 0xB1, 0xB2, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD,
						0xBE        // koi8-r ╟╠╡╢╣╤╥╦╧╨╩╪╫╬
					};
					*str_o = static_cast<char>(Utf8ToKoiPg[u - 0x2550]);
				} else // random non-sequencitial bits and pieces (other pseudographics and some math symbols)
				{
					switch (u) {
						case 0x2500: *str_o = '\x80';
							break;
						case 0x2502: *str_o = '\x81';
							break;
						case 0x250C: *str_o = '\x82';
							break;
						case 0x2510: *str_o = '\x83';
							break;
						case 0x2514: *str_o = '\x84';
							break;
						case 0x2518: *str_o = '\x85';
							break;
						case 0x251C: *str_o = '\x86';
							break;
						case 0x2524: *str_o = '\x87';
							break;
						case 0x252C: *str_o = '\x88';
							break;
						case 0x2534: *str_o = '\x89';
							break;
						case 0x253C: *str_o = '\x8A';
							break;
						case 0x2580: *str_o = '\x8B';
							break;
						case 0x2584: *str_o = '\x8C';
							break;
						case 0x2588: *str_o = '\x8D';
							break;
						case 0x258C: *str_o = '\x8E';
							break;
						case 0x2590: *str_o = '\x8F';
							break;
						case 0x2591: *str_o = '\x90';
							break;
						case 0x2592: *str_o = '\x91';
							break;
						case 0x2593: *str_o = '\x92';
							break;
						case 0x2320: *str_o = '\x93';
							break;
						case 0x25A0: *str_o = '\x94';
							break;
						case 0x2219: *str_o = '\x95';
							break;
						case 0x221A: *str_o = '\x96';
							break;
						case 0x2248: *str_o = '\x97';
							break;
						case 0x2264: *str_o = '\x98';
							break;
						case 0x2265: *str_o = '\x99';
							break;
							//   0x00A0        to 0x9A decoded elsewhere
						case 0x2321: *str_o = '\x9B';
							break;
					}
				}

				str_o++;
				str_i += 2;
			}
		} else if (c < 0xF8) {
			// three more bytes to follow, must be b10xxxxxx
			if ((str_i[0] & 0xC0) == 0x80
				&& (str_i[1] & 0xC0) == 0x80
				&& (str_i[2] & 0xC0) == 0x80) {
				// valid utf-8, but we don't handle those - not present in koi8-r
				*str_o++ = KOI8_UNKNOWN_CHAR;
				str_i += 3;
			}
		} else // c >= 0xF8
		{
			// not a valid utf-8 byte, ignore
		}
	}

	*str_o = 0;
}

void koi_to_utf8(char *str_i, char *str_o) {
	// KOI8-R to Cyrillic Unicode block 0x0400 - 0x047F
	const unsigned short KoiToUtf8[] =
		{
			0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524, 0x252C, 0x2534, 0x253C, 0x2580, 0x2584,
			0x2588, 0x258C, 0x2590,
			0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248, 0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0,
			0x00B2, 0x00B7, 0x00F7,
			0x2550, 0x2501, 0x2502, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556, 0x2557, 0x2558, 0x2559, 0x255A, 0x255B,
			0x255C, 0x255D, 0x255E,
			0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565, 0x2566, 0x2567, 0x2568, 0x2569, 0x256A,
			0x256B, 0x256C, 0x00A9,
			0x44E, 0x430, 0x431, 0x446, 0x434, 0x435, 0x444, 0x433, 0x445, 0x438, 0x439, 0x43A, 0x43B, 0x43C, 0x43D,
			0x43E,
			0x43F, 0x44F, 0x440, 0x441, 0x442, 0x443, 0x436, 0x432, 0x44C, 0x44B, 0x437, 0x448, 0x44D, 0x449, 0x447,
			0x44A,
			0x42E, 0x410, 0x411, 0x426, 0x414, 0x415, 0x424, 0x413, 0x425, 0x418, 0x419, 0x41A, 0x41B, 0x41C, 0x41D,
			0x41E,
			0x41F, 0x42F, 0x420, 0x421, 0x422, 0x423, 0x416, 0x412, 0x42C, 0x42B, 0x417, 0x428, 0x42D, 0x429, 0x427,
			0x42A
		};
	unsigned int c;

	while ((c = static_cast<unsigned char>(*str_i++))) {
		if (c < 0x80) {
			*str_o++ = c;
		} else {
			c = KoiToUtf8[c - 0x80];
			if (c < 0x800) {
				*str_o++ = 0xC0 | (c >> 6);
				*str_o++ = 0x80 | (c & 0x3F);
			} else // (c < 0x10000) - two bytes. more than 16 bits not supported
			{
				*str_o++ = 0xE0 | (c >> 12);
				*str_o++ = 0x80 | ((c >> 6) & 0x3F);
				*str_o++ = 0x80 | (c & 0x3F);
			}
		}
	}

	*str_o = 0;
}

#endif // HAVE_ICONV

}  // namespace codepages

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
