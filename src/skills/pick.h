#pragma once

#include <string>

class CHAR_DATA;

struct PickProbabilityInformation {
	// показываемый текст игроку при осмотре замка
	std::string text;

	// шанс сломать замок в %
	// 0 - замок не может быть взломан
	unsigned short probability;

	// можно ли повысить умение при взломе
	bool skill_train_allowed;
};

PickProbabilityInformation get_pick_probability(CHAR_DATA *ch, int lock_complexity);

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
