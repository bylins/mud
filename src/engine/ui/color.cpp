/**
\file color.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Цвета текста.
\details Константы и функции для работы с цветами telnet.
*/

#include "engine/ui/color.h"

#include "utils/utils.h"

const char *kColorNrm{"\x1B[0;37m"};
const char *kColorRed{"\x1B[0;31m"};
const char *kColorGrn{"\x1B[0;32m"};
const char *kColorYel{"\x1B[0;33m"};
const char *kColorBlu{"\x1B[0;34m"};
const char *kColorMag{"\x1B[0;35m"};
const char *kColorCyn{"\x1B[0;36m"};
const char *kColorWht{"\x1B[1;37m"};
const char *kColorGry{"\x1B[0;37m"};

const char *kColorBoldBlk{"\x1B[1;30m"};
const char *kColorBoldRed{"\x1B[1;31m"};
const char *kColorBoldGrn{"\x1B[1;32m"};
const char *kColorBoldYel{"\x1B[1;33m"};
const char *kColorBoldBlu{"\x1B[1;34m"};
const char *kColorBoldMag{"\x1B[1;35m"};
const char *kColorBoldCyn{"\x1B[1;36m"};
const char *kColorBoldWht{"\x1B[1;37m"};

const char *kColorNormal{"\x1B[0;0m"};
const char *kColorBlk{"\x1B[0;30m"};
const char *kColorBlackBlk{"\x1B[40m"};
const char *kColorBlackRed{"\x1B[41m"};
const char *kColorBlackGrn{"\x1B[42m"};
const char *kColorBlackYel{"\x1B[43m"};
const char *kColorBlackBlu{"\x1B[44m"};
const char *kColorBlackMag{"\x1B[45m"};
const char *kColorBlackCyn{"\x1B[46m"};
const char *kColorBlackWht{"\x1B[47m"};
const char *kAmp{"&"};
const char *kSlh{"\\"};
const char *kUdl{"\x1B[4m"};        // Underline ANSI code
const char *kFsh{"\x1B[5m"};        /* Flashing ANSI code.  Change to #define CFSH "" if
* you want to disable flashing colour codes
*/
const char *kRvs{"\x1B[7m"};        // Reverse video ANSI code

const char *kColorList[] = {kColorNormal, kColorRed, kColorGrn, kColorYel, kColorBlu, kColorMag, kColorCyn, kColorGry,
							kColorBoldRed, kColorBoldGrn, kColorBoldYel, kColorBoldBlu, kColorBoldMag, kColorBoldCyn,
							kColorBoldWht, kColorBlackRed, kColorBlackGrn, kColorBlackYel, kColorBlackBlu,
							kColorBlackMag, kColorBlackCyn, kColorBlackWht, kAmp, kSlh, kColorBlackBlk, kColorBlk,
							kFsh, kRvs, kUdl, kColorBoldBlk
};

const int kMaxColors = 30;

std::size_t count_colors(const char *str, std::size_t len = 0);

int IsNumber(char s) {
	return ((s >= '0') && (s <= '9'));
}

int IsColor(char code) {
	switch (code) {
		case 'k': return 25;    // Black
		case 'r': return 1;        // Red
		case 'g': return 2;        // Green
		case 'y': return 3;        // Yellow
		case 'b': return 4;        // Blue
		case 'm': return 5;        // Magenta
		case 'c': return 6;        // Cyan
		case 'w': return 7;        // White
		case 'K': return 29;    // Bold black (Just for completeness)
		case 'R': return 8;        // Bold red
		case 'G': return 9;        // Bold green
		case 'Y': return 10;    // Bold yellow
		case 'B': return 11;    // Bold blue
		case 'M': return 12;    // Bold magenta
		case 'C': return 13;    // Bold cyan
		case 'W': return 14;    // Bold white
		case '0': return 24;    // Black background
		case '1': return 15;    // Red background
		case '2': return 16;    // Green background
		case '3': return 17;    // Yellow background
		case '4': return 18;    // Blue background
		case '5': return 19;    // Magenta background
		case '6': return 20;    // Cyan background
		case '7': return 21;    // White background
//	case '&': return 22;		// The & character
		case '\\': return 23;    // The \ character
		case 'n': return 0;        // Normal
		case 'f': return 26;    // Flash
		case 'v': return 27;    // Reverse video
		case 'u': return 28;    // Underline (Only for mono screens)
		case 'q': return -2;    // set default color to current
		case 'Q': return -3;    // set default color back to normal
		case 'e': return -4;    // print error when color not reseted to normal
		default: return -1;
	}
}

constexpr auto kMaxColorStringLength = kMaxSockBuf * 2 - kGarbageSpace;
int proc_color(char *inbuf) {
	int p = 0;
	int c = 0, tmp = 0, nc = 0; // normal colour CNRM by default
	bool show_all = false;
	char out_buf[kMaxSockBuf * 2];

	if (inbuf == nullptr) {
		return -1;
	}

	size_t len = strlen(inbuf);
	if (len == 0) {
		return -2;
	}

	size_t j = 0;
	while (j < len) {
		if (p > kMaxColorStringLength) {
			snprintf(&out_buf[p], kMaxSockBuf * 2 - p, "\r\n%s%s\r\n", kColorNormal, "***ПЕРЕПОЛНЕНИЕ***");
			strcpy(inbuf, out_buf);
			return 0;
		}

		// WorM: Добавил ключи &S и &s, начало и конец текста без обработки цветов и _
		if (inbuf[j] == '&' && ((!show_all && inbuf[j + 1] == 'S') || (show_all && inbuf[j + 1] == 's'))) {
			show_all = !show_all;
			j += 2;
			continue;
		}
		if (!show_all && (inbuf[j] == '\\') && (inbuf[j + 1] == 'c')
			&& IsNumber(inbuf[j + 2]) && IsNumber(inbuf[j + 3])) {
			c = (inbuf[j + 2] - '0') * 10 + inbuf[j + 3] - '0';
			j += 4;
		} else if (!show_all && (inbuf[j] == '&') && !((tmp = IsColor(inbuf[j + 1])) == -1)) {
			j += 2;
			if (tmp == -4) {
				// ключ &e выводит три воскл.знака если тут не закрыты цвета ключом &n
				if (c != 0 && c != nc) {
					out_buf[p++] = '!';
					out_buf[p++] = '!';
					out_buf[p++] = '!';
				}
			} else {
				// WorM: ключ &q сохраняем текущий цвет как стандартный &n вместо 0 будет возвращать его
				(tmp < 0 ? nc : c) = (tmp == -2 ? c : (tmp == -3 ? 0 : tmp));
			}
			if (tmp < 0)
				continue;
		} else {
			out_buf[p] = (!show_all && inbuf[j] == '_' ? ' ' : inbuf[j]);
			j++;
			p++;
			continue;
		}
		if (c >= kMaxColors) {
			c = 0;
		}
		size_t max = strlen(kColorList[(c == 0 && nc != 0 ? nc : c)]);
		for (size_t k = 0; k < max; k++) {
			out_buf[p] = kColorList[(c == 0 && nc != 0 ? nc : c)][k];
			p++;
		}
	}
	out_buf[p] = '\0';

	strcpy(inbuf, out_buf);
	if (j > len) {
		return static_cast<int>(j);
	}
	return 0;
}

const char *GetWarmValueColor(int current, int max) {
	switch (posi_value(current, max)) {
		case -1:
		case 0: [[fallthrough]];
		case 1: return kColorRed;
		case 2: [[fallthrough]];
		case 3: return kColorBoldRed;
		case 4: [[fallthrough]];
		case 5: return kColorBoldYel;
		case 6:
		case 7: [[fallthrough]];
		case 8: return kColorBoldGrn;
		default: return kColorGrn;
	}
}

const char *GetColdValueColor(int current, int max) {
	switch (posi_value(current, max)) {
		case -1:
		case 0: [[fallthrough]];
		case 1: return kColorGry;
		case 2: [[fallthrough]];
		case 3: return kColorBlu;
		case 4: [[fallthrough]];
		case 5: return kColorBoldBlu;
		case 6:
		case 7: [[fallthrough]];
		case 8: return kColorBoldCyn;
		default: return kColorCyn;
	}
}

/// длина строки без символов цвета из count_colors()
size_t strlen_no_colors(const char *str) {
	const size_t len = strlen(str);
	return len - count_colors(str, len) * 2;
}

/// считает кол-во цветов &R и т.п. в строке
/// size_t len = 0 - по дефолту считает strlen(str)
size_t count_colors(const char *str, size_t len) {
	unsigned int c, cc = 0;
	len = len ? len : strlen(str);

	for (c = 0; c < len - 1; c++) {
		if (*(str + c) == '&' && *(str + c + 1) != '&')
			cc++;
	}
	return cc;
}

//возвращает строку длины len + кол-во цветов*2 для того чтоб в табличке все было ровненько
//left_align выравнивание строки влево
char *colored_name(const char *str, size_t len, const bool left_align) {
	static char cstr[128];
	static char fmt[6];
	size_t cc = len + count_colors(str) * 2;

	if (strlen(str) < cc) {
		snprintf(fmt, sizeof(fmt), "%%%s%ds", (left_align ? "-" : ""), static_cast<int>(cc));
		snprintf(cstr, sizeof(cstr), fmt, str);
	} else {
		snprintf(cstr, sizeof(cstr), "%s", str);
	}

	return cstr;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
