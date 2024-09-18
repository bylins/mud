/**
\file color.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Цвета текста.
\details Константы и функции для работы с цветами telnet.
*/

#ifndef COLOR_H_
#define COLOR_H_

#include "engine/structs/structs.h"

extern const char *kColorNrm;
extern const char *kColorRed;
extern const char *kColorGrn;
extern const char *kColorYel;
extern const char *kColorBlu;
extern const char *kColorMag;
extern const char *kColorCyn;
extern const char *kColorWht;
extern const char *kColorGry;

extern const char *kColorBoldBlk;
extern const char *kColorBoldRed;
extern const char *kColorBoldGrn;
extern const char *kColorBoldYel;
extern const char *kColorBoldBlu;
extern const char *kColorBoldMag;
extern const char *kColorBoldCyn;
extern const char *kColorBoldWht;

std::size_t strlen_no_colors(const char *str);
int proc_color(char *inbuf);
char *colored_name(const char *str, std::size_t len, bool left_align = false);
const char *GetWarmValueColor(int current, int max);
const char *GetColdValueColor(int current, int max);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
