#pragma once

#include <string>

class CharData;

struct PickProbabilityInformation {
	// показываемый текст игроку при осмотре замка
	std::string text;

	// шанс открыть замок в %
	// 0 - замок не может быть взломан
	// имеет более высокий приоритет над шансом сломать замок
	unsigned short unlock_probability;

	// шанс сломать замок
	// 0 - замок гарантировано не сломается
	unsigned short brake_lock_probability;

	// можно ли повысить умение при взломе
	bool skill_train_allowed;
};

PickProbabilityInformation get_pick_probability(CharData *ch, int lock_complexity);

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
