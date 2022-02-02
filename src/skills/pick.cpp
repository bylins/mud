#include "pick.h"

#include "entities/char.h"
#include "structs/global_objects.h"

PickProbabilityInformation get_pick_probability(CharacterData *ch, int lock_complexity) {
	// для прокачки умения скилл должен быть в промежутке от -10 сложности замка, до сложности замка
	const int allowed_difference = 10;
	const bool skill_train_allowed = (lock_complexity > ch->get_skill(ESkill::kPickLock)) &&
		(ch->get_skill(ESkill::kPickLock) + allowed_difference >= lock_complexity);

	// используем сложность скила для дополнительного ограничения шанса прокачки
	const int skill = CalcCurrentSkill(ch, ESkill::kPickLock, nullptr);
	const int prob_diff = number(1, MUD::Skills()[ESkill::kPickLock].difficulty);

	// высчитываем дополнительный шанс ограничения прокачки (от сложности скила)
	const int min_chance = 50;
	const int restriction_percent = std::clamp(skill - prob_diff, min_chance, 100);
	const bool complexity_restriction = number(1, 100) > restriction_percent ? true : false;

	// разница между скилом и сложностью замка
	const int skill_difference = static_cast<int>(ch->get_skill(ESkill::kPickLock)) - static_cast<int>(lock_complexity);

	// считаем шанс взлома
	int pick_probability_percent = 0;
	if (skill_difference < -10) {
		// сложные замки не взламываются
		pick_probability_percent = 0;
	} else if (skill_difference < 0) {
		// раница в промежутся от -10 до -1 -> 10% шанс взлома
		pick_probability_percent = 10;
	} else if (skill_difference < 20) {
		// скилл больше сложности, взломать не сложно
		pick_probability_percent = 30;
	} else {
		// скилл намного больше сложности - взлом не проблема
		pick_probability_percent = 60;
	}

	// считаем шанс сломать замок
	int brake_probability = 0;
	if (skill_difference < -20) {
		// слишком большая сложность замка. даже сломать нельзя
		brake_probability = 0;
	} else if (skill_difference < -10) {
		// раница в промежутся от -20 до -11 -> 50% шанс поломать замок
		brake_probability = 50;
	} else if (skill_difference < -5) {
		// раница в промежутся от -10 до -6 -> 6% шанс поломать замок
		brake_probability = 6;
	} else if (skill_difference < 0) {
		// разница в промежутся от -5 до -1 -> 3% шанс поломать замок
		brake_probability = 3;
	} else {
		// скилл больше сложности -> замок не ломается
		brake_probability = 0;
	}

	// выбираем текст в зависимости от уровня скила и сложности замка
	std::string pick_text;
	// если скилл может прокачаться - всегда пишем, что замок очень сложный
	if (skill_train_allowed) {
		pick_text = "Замок очень сложный.";
	} else {
		if (lock_complexity > skill) {
			// сложность замка больше скила
			pick_text = "Вы в жизни не видели подобного замка.";
		} else {
			pick_text = "Простой замок. Эка невидаль.";
		}
	}

	const PickProbabilityInformation pbi {
		pick_text,
		static_cast<unsigned short>(pick_probability_percent),
		static_cast<unsigned short>(brake_probability),
		skill_train_allowed && !complexity_restriction
	};

	if (!IS_NPC(ch)) {
		std::stringstream text_info;
		text_info << std::boolalpha << "Шанс взлома: " << pbi.unlock_probability << "%, шанс сломать замок: " << pbi.brake_lock_probability << "%, возможность прокачки скила: " << pbi.skill_train_allowed << "\r\n";
		text_info << std::boolalpha << "Сложность замка: " << lock_complexity << ", разница между скилом и сложностью замка: " << skill_difference << "\r\n";
		text_info << std::boolalpha << "Возможность прокачки скила (от сложности скила): " << skill_train_allowed << ", ограничение прокачки по сложности скила: "  << complexity_restriction << "\r\n";
		ch->send_to_TC(true, true, true, text_info.str().c_str());
	}

	return pbi;
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
