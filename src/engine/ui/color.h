/**
\file color.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Цвета текста.
\details Константы и функции для работы с цветами telnet.
*/

#ifndef COLOR_H_
#define COLOR_H_

extern const char *kColorNrm;
extern const char *kColorRed;
extern const char *kColorGrn;
extern const char *kColorYel;
extern const char *kColorBlu;
extern const char *kColorMag;
extern const char *kColorCyn;
extern const char *kColorWht;
extern const char *kColorGry;

extern const char *kColorBoldDrk;
extern const char *kColorBoldRed;
extern const char *kColorBoldGrn;
extern const char *kColorBoldYel;
extern const char *kColorBoldBlu;
extern const char *kColorBoldMag;
extern const char *kColorBoldCyn;
extern const char *kColorBoldWht;

int proc_color(char *inbuf);
const char *GetWarmValueColor(int current, int max);
const char *GetColdValueColor(int current, int max);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
