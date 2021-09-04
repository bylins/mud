#include "pick.h"

#include <algorithm>

#include "chars/char.h"
#include "skills_info.h"


PickProbabilityInformation get_pick_probability(CHAR_DATA *ch, int lock_complexity) {
	// для прокачки умения скилл должен быть в промежутке от -10 сложности замка, до сложности замка
	const int allowed_difference = 10;
	const bool skill_train_allowed = (lock_complexity > ch->get_skill(SKILL_PICK_LOCK)) &&
		(ch->get_skill(SKILL_PICK_LOCK) + allowed_difference >= lock_complexity);

	// используем сложность скила для дополнительного ограничения шанса прокачки
	const int skill = CalcCurrentSkill(ch, SKILL_PICK_LOCK, nullptr);
	const int prob_diff = number(1, skill_info[SKILL_PICK_LOCK].difficulty);

	// высчитываем дополнительный шанс ограничения прокачки (от сложности скила)
	const int min_chance = 50;
	const int restriction_percent = std::clamp(skill - prob_diff, min_chance, 100);
	const bool complexity_restriction = number(1, 100) > restriction_percent ? true : false;

	// высчитываем шанс взлома
	const int prob_skill = number(1, skill);
	// если в теории можем сломать замок, то выставляем минимальный шанс взлома
	const int minimum_pick_probability = 15;
	const int pick_probability_by_complexity = prob_skill >= lock_complexity ? std::clamp(prob_skill - lock_complexity, minimum_pick_probability, 100) : minimum_pick_probability;
	// финальный шанс взлома
	const int pick_probability_percent = skill >= lock_complexity ? pick_probability_by_complexity : 0;

	// выбираем текст в зависимости от уровня скила и сложности замка
	std::string pick_text;
	if (lock_complexity > skill) {
		pick_text = "Вы в жизни не видели подобного замка.";
	} else if (lock_complexity + 10 > skill) {
		pick_text = "Замок очень сложный.";
	} else if (lock_complexity + 50 > skill) {
		pick_text = "Сложный замок. Как бы не сломать.";
	} else {
		pick_text = "Простой замок. Эка невидаль.";
	}
	// если скилл может прокачаться - всегда пишем, что замок очень сложный
	if (skill_train_allowed) {
		pick_text = "Замок очень сложный.";
	}

	const PickProbabilityInformation pbi {
		pick_text,
		static_cast<unsigned short>(pick_probability_percent),
		skill_train_allowed && !complexity_restriction
	};

	if (!IS_NPC(ch)) {
		std::stringstream text_info;
		text_info << std::boolalpha << "Шанс взлома: " << pbi.probability << "%, возможность прокачки скила: " << pbi.skill_train_allowed << "\r\n";
		text_info << std::boolalpha << "Возможность прокачки скила (от сложности скила): " << skill_train_allowed << ", ограничение прокачки по сложности скила: "  << complexity_restriction << "\r\n";
		ch->send_to_TC(true, true, true, text_info.str().c_str());
	}

	return pbi;
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
