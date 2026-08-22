/**
\file russian_keys.h - a part of the Bylins engine.
\brief Russian letters as switch-able constants, valid in both native encodings (issue #3681).

Menus and OLC editors dispatch on a single letter the player typed:

	switch (*arg) {
		case 'y': case 'Y': case 'd': case 'D': ...

A Cyrillic *character* literal cannot survive the encoding flip: with UTF-8 sources 'd' (Cyrillic)
is a multi-character constant, so the compiler folds it to an implementation-defined value and the
menu silently stops responding. Nor can such a literal be written portably for both encodings.

The way out is to keep the switch but dispatch on a number instead of a literal. Under KOI8-R a
letter is one byte, under UTF-8 it is a code point, so each letter is spelled out numerically for
both and the constants below are what the switch compares against:

	switch (native_text::first_char_code(arg)) {
		case 'y': case 'Y': case rus::kDa: case rus::kDaUpper: ...

Note that ASCII cases stay ordinary character literals -- those are identical in both encodings.

Once the flip is permanent (track D) the KOI8-R half goes away and these can collapse into plain
U'...' literals.
*/

#ifndef BYLINS_SRC_UTILS_RUSSIAN_KEYS_H_
#define BYLINS_SRC_UTILS_RUSSIAN_KEYS_H_

namespace rus {


// Unicode code points: U+0410..U+042F (upper), U+0430..U+044F (lower), U+0401/U+0451 (Yo).
#define BYLINS_RUS_LETTER(name, upper_cp, lower_cp) \
	constexpr char32_t name##Upper = upper_cp;      \
	constexpr char32_t name = lower_cp


BYLINS_RUS_LETTER(kA,   0x0410, 0x0430);  // А а
BYLINS_RUS_LETTER(kBe,  0x0411, 0x0431);  // Б б
BYLINS_RUS_LETTER(kVe,  0x0412, 0x0432);  // В в
BYLINS_RUS_LETTER(kGe,  0x0413, 0x0433);  // Г г
BYLINS_RUS_LETTER(kDe,  0x0414, 0x0434);  // Д д
BYLINS_RUS_LETTER(kIe,  0x0415, 0x0435);  // Е е
BYLINS_RUS_LETTER(kYo,  0x0401, 0x0451);  // Ё ё
BYLINS_RUS_LETTER(kZhe, 0x0416, 0x0436);  // Ж ж
BYLINS_RUS_LETTER(kZe,  0x0417, 0x0437);  // З з
BYLINS_RUS_LETTER(kI,   0x0418, 0x0438);  // И и
BYLINS_RUS_LETTER(kIi,  0x0419, 0x0439);  // Й й
BYLINS_RUS_LETTER(kKa,  0x041A, 0x043A);  // К к
BYLINS_RUS_LETTER(kEl,  0x041B, 0x043B);  // Л л
BYLINS_RUS_LETTER(kEm,  0x041C, 0x043C);  // М м
BYLINS_RUS_LETTER(kEn,  0x041D, 0x043D);  // Н н
BYLINS_RUS_LETTER(kO,   0x041E, 0x043E);  // О о
BYLINS_RUS_LETTER(kPe,  0x041F, 0x043F);  // П п
BYLINS_RUS_LETTER(kEr,  0x0420, 0x0440);  // Р р
BYLINS_RUS_LETTER(kEs,  0x0421, 0x0441);  // С с
BYLINS_RUS_LETTER(kTe,  0x0422, 0x0442);  // Т т
BYLINS_RUS_LETTER(kU,   0x0423, 0x0443);  // У у
BYLINS_RUS_LETTER(kEf,  0x0424, 0x0444);  // Ф ф
BYLINS_RUS_LETTER(kHa,  0x0425, 0x0445);  // Х х
BYLINS_RUS_LETTER(kTse, 0x0426, 0x0446);  // Ц ц
BYLINS_RUS_LETTER(kChe, 0x0427, 0x0447);  // Ч ч
BYLINS_RUS_LETTER(kSha, 0x0428, 0x0448);  // Ш ш
BYLINS_RUS_LETTER(kScha,0x0429, 0x0449);  // Щ щ
BYLINS_RUS_LETTER(kHard,0x042A, 0x044A);  // Ъ ъ
BYLINS_RUS_LETTER(kYery,0x042B, 0x044B);  // Ы ы
BYLINS_RUS_LETTER(kSoft,0x042C, 0x044C);  // Ь ь
BYLINS_RUS_LETTER(kE,   0x042D, 0x044D);  // Э э
BYLINS_RUS_LETTER(kYu,  0x042E, 0x044E);  // Ю ю
BYLINS_RUS_LETTER(kYa,  0x042F, 0x044F);  // Я я

#undef BYLINS_RUS_LETTER

}  // namespace rus

#endif  // BYLINS_SRC_UTILS_RUSSIAN_KEYS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
