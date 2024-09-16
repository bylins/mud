/**
\file color.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Цвета текста.
\details Константы и функции для работы с цветами telnet.
*/

#include "engine/structs/structs.h"
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

const char *kColorBoldDrk{"\x1B[1;30m"};
const char *kColorBoldRed{"\x1B[1;31m"};
const char *kColorBoldGrn{"\x1B[1;32m"};
const char *kColorBoldYel{"\x1B[1;33m"};
const char *kColorBoldBlu{"\x1B[1;34m"};
const char *kColorBoldMag{"\x1B[1;35m"};
const char *kColorBoldCyn{"\x1B[1;36m"};
const char *kColorBoldWht{"\x1B[1;37m"};

#define CNRM  "\x1B[0;0m"
#define CBLK  "\x1B[0;30m"
#define CRED  "\x1B[0;31m"
#define CGRN  "\x1B[0;32m"
#define CYEL  "\x1B[0;33m"
#define CBLU  "\x1B[0;34m"
#define CMAG  "\x1B[0;35m"
#define CCYN  "\x1B[0;36m"
#define CWHT  "\x1B[0;37m"

#define BBLK  "\x1B[1;30m"
#define BRED  "\x1B[1;31m"
#define BGRN  "\x1B[1;32m"
#define BYEL  "\x1B[1;33m"
#define BBLU  "\x1B[1;34m"
#define BMAG  "\x1B[1;35m"
#define BCYN  "\x1B[1;36m"
#define BWHT  "\x1B[1;37m"

#define BKBLK  "\x1B[40m"
#define BKRED  "\x1B[41m"
#define BKGRN  "\x1B[42m"
#define BKYEL  "\x1B[43m"
#define BKBLU  "\x1B[44m"
#define BKMAG  "\x1B[45m"
#define BKCYN  "\x1B[46m"
#define BKWHT  "\x1B[47m"

#define CAMP  "&"
#define CSLH  "\\"

#define CUDL  "\x1B[4m"        // Underline ANSI code
#define CFSH  "\x1B[5m"        /* Flashing ANSI code.  Change to #define CFSH "" if
* you want to disable flashing colour codes
*/
#define CRVS  "\x1B[7m"        // Reverse video ANSI code

const char *COLOURLIST[] = {CNRM, CRED, CGRN, CYEL, CBLU, CMAG, CCYN, CWHT,
							BRED, BGRN, BYEL, BBLU, BMAG, BCYN, BWHT,
							BKRED, BKGRN, BKYEL, BKBLU, BKMAG, BKCYN, BKWHT,
							CAMP, CSLH, BKBLK, CBLK, CFSH, CRVS, CUDL, BBLK
};

const int kMaxColors = 30;

int isnum(char s) {
	return ((s >= '0') && (s <= '9'));
}

int is_colour(char code) {
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
			snprintf(&out_buf[p], kMaxSockBuf * 2 - p, "\r\n%s%s\r\n", CNRM, "***ПЕРЕПОЛНЕНИЕ***");
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
			&& isnum(inbuf[j + 2]) && isnum(inbuf[j + 3])) {
			c = (inbuf[j + 2] - '0') * 10 + inbuf[j + 3] - '0';
			j += 4;
		} else if (!show_all && (inbuf[j] == '&') && !((tmp = is_colour(inbuf[j + 1])) == -1)) {
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
		size_t max = strlen(COLOURLIST[(c == 0 && nc != 0 ? nc : c)]);
		for (size_t k = 0; k < max; k++) {
			out_buf[p] = COLOURLIST[(c == 0 && nc != 0 ? nc : c)][k];
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
	static char color[8];
	switch (posi_value(current, max)) {
		case -1:
		case 0:
		case 1: sprintf(color, "&r");
			break;
		case 2:
		case 3: sprintf(color, "&R");
			break;
		case 4:
		case 5: sprintf(color, "&Y");
			break;
		case 6:
		case 7:
		case 8: sprintf(color, "&G");
			break;
		default: sprintf(color, "&g");
			break;
	}
	return (color);
}

const char *GetColdValueColor(int current, int max) {
	static char color[8];
	switch (posi_value(current, max)) {
		case -1:
		case 0:
		case 1: sprintf(color, "&w");
			break;
		case 2:
		case 3: sprintf(color, "&b");
			break;
		case 4:
		case 5: sprintf(color, "&B");
			break;
		case 6:
		case 7:
		case 8: sprintf(color, "&C");
			break;
		default: sprintf(color, "&c");
			break;
	}
	return (color);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
