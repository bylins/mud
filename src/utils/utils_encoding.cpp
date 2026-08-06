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

// Tables of raw bytes, spelled as \xNN escapes on purpose. They used to be written as string
// literals of high-byte characters, which made their contents depend on the encoding of this
// file: with UTF-8 sources every Cyrillic/pseudographic character became two or three bytes and
// the tables silently grew past 128 entries, corrupting every legacy client's output. Escapes
// are plain ASCII and mean the same thing in any source encoding (issue #3681).
char AltToKoi[] = {
		"\xE1\xE2\xF7\xE7\xE4\xE5\xF6\xFA\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0"
		"\xF2\xF3\xF4\xF5\xE6\xE8\xE3\xFE\xFB\xFD\xFF\xF9\xF8\xFC\xE0\xF1"
		"\xC1\xC2\xD7\xC7\xC4\xC5\xD6\xDA\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0"
		"\x90\x91\x92\x81\x87\xB2\x2B\x2B\x2B\xB5\xA1\xA8\xAE\x2B\xAC\x83"
		"\x84\x89\x88\x86\x80\x8A\xAF\xB0\xAB\xA5\xBB\xB8\xB1\xA0\xBE\xB9"
		"\xBA\x2B\x2B\xAA\xA9\xA2\x2B\x2B\xBC\x85\x82\x8D\x8C\x8E\x8F\x8B"
		"\xD2\xD3\xD4\xD5\xC6\xC8\xC3\xDE\xDB\xDD\xDF\xD9\xD8\xDC\xC0\xD1"
		"\xB3\xA3\xBD\xAD\xB4\xA4\xB6\xA6\xB7\xA7\x9E\x96\x3F\x3F\x94\x9A"
	};
char KoiToAlt[] = {
		"\xC4\xB3\xDA\xBF\xC0\xD9\xC3\xB4\xC2\xC1\xC5\xDF\xDC\xDB\xDD\xDE"
		"\xB0\xB1\xB2\x2B\xFE\x2B\xFB\x2B\x2B\x2B\xFF\x2B\x2B\x2B\xFA\x2B"
		"\xCD\xBA\xD5\xF1\xF5\xC9\xF7\xF9\xBB\xD4\xD3\xC8\xBE\xF3\xBC\xC6"
		"\xC7\xCC\xB5\xF0\xF4\xB9\xF6\xF8\xCB\xCF\xD0\xCA\xD8\xF2\xCE\x2B"
		"\xEE\xA0\xA1\xE6\xA4\xA5\xE4\xA3\xE5\xA8\xA9\xAA\xAB\xAC\xAD\xAE"
		"\xAF\xEF\xE0\xE1\xE2\xE3\xA6\xA2\xEC\xEB\xA7\xE8\xED\xE9\xE7\xEA"
		"\x9E\x80\x81\x96\x84\x85\x94\x83\x95\x88\x89\x8A\x8B\x8C\x8D\x8E"
		"\x8F\x9F\x90\x91\x92\x93\x86\x82\x9C\x9B\x87\x98\x9D\x99\x97\x9A"
	};
char WinToKoi[] = {
		"\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B"
		"\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B"
		"\x9A\x2B\x2B\x2B\x2B\xBD\x2B\x2B\xB3\xBF\xB4\x2B\x2B\x2B\x2B\xB7"
		"\x9C\x2B\xB6\xA6\xAD\x2B\x2B\x9E\xA3\x2B\xA4\x2B\x2B\x2B\x2B\xA7"
		"\xE1\xE2\xF7\xE7\xE4\xE5\xF6\xFA\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0"
		"\xF2\xF3\xF4\xF5\xE6\xE8\xE3\xFE\xFB\xFD\xFF\xF9\xF8\xFC\xE0\xF1"
		"\xC1\xC2\xD7\xC7\xC4\xC5\xD6\xDA\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0"
		"\xD2\xD3\xD4\xD5\xC6\xC8\xC3\xDE\xDB\xDD\xDF\xD9\xD8\xDC\xC0\xD1"
	};
char KoiToWin[] = {
		"\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B"
		"\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\xA0\x2B\xB0\x2B\xB7\x2B"
		"\x2B\x2B\x2B\xB8\xBA\x2B\xB3\xBF\x2B\x2B\x2B\x2B\x2B\xB4\x2B\x2B"
		"\x2B\x2B\x2B\xA8\xAA\x2B\xB2\xAF\x2B\x2B\x2B\x2B\x2B\xA5\x2B\xA9"
		"\xFE\xE0\xE1\xF6\xE4\xE5\xF4\xE3\xF5\xE8\xE9\xEA\xEB\xEC\xED\xEE"
		"\xEF\xFF\xF0\xF1\xF2\xF3\xE6\xE2\xFC\xFB\xE7\xF8\xFD\xF9\xF7\xFA"
		"\xDE\xC0\xC1\xD6\xC4\xC5\xD4\xC3\xD5\xC8\xC9\xCA\xCB\xCC\xCD\xCE"
		"\xCF\xDF\xD0\xD1\xD2\xD3\xC6\xC2\xDC\xDB\xC7\xD8\xDD\xD9\xD7\xDA"
	};
char KoiToWin2[] = {
		"\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B"
		"\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\x2B\xA0\x2B\xB0\x2B\xB7\x2B"
		"\x2B\x2B\x2B\xB8\xBA\x2B\xB3\xBF\x2B\x2B\x2B\x2B\x2B\xB4\x2B\x2B"
		"\x2B\x2B\x2B\xA8\xAA\x2B\xB2\xAF\x2B\x2B\x2B\x2B\x2B\xA5\x2B\xA9"
		"\xFE\xE0\xE1\xF6\xE4\xE5\xF4\xE3\xF5\xE8\xE9\xEA\xEB\xEC\xED\xEE"
		"\xEF\x7A\xF0\xF1\xF2\xF3\xE6\xE2\xFC\xFB\xE7\xF8\xFD\xF9\xF7\xFA"
		"\xDE\xC0\xC1\xD6\xC4\xC5\xD4\xC3\xD5\xC8\xC9\xCA\xCB\xCC\xCD\xCE"
		"\xCF\xDF\xD0\xD1\xD2\xD3\xC6\xC2\xDC\xDB\xC7\xD8\xDD\xD9\xD7\xDA"
	};
char AltToLat[] = {
		"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F"
		"\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
		"\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF"
		"\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF"
		"\x30\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F"
		"\x70\x71\x72\x73\x74\x59\x31\x76\x32\x33\x7A\x34\x35\x36\x37\x38"
		"\x30\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F"
		"\x50\x51\x52\x53\x54\x59\x31\x56\x32\x33\x5A\x34\x35\x36\x37\x38"
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

// Placeholder for a code point KOI8-R cannot represent AND for which translit_koi8 has no
// equivalent. '?' rather than the former '+': a plus is ordinary text in this game ("+5 to
// damage"), so it read as content rather than as a lost character (issue #3681).
#define KOI8_UNKNOWN_CHAR  '?'

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
