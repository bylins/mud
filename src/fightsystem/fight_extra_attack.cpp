#include "fight_extra_attack.h"

#include "entities/char.h"
#include "magic/spells.h"
#include "utils/utils.h"

#include <boost/algorithm/string.hpp>

WeaponMagicalAttack::WeaponMagicalAttack(CharacterData *ch) { ch_ = ch; }

void WeaponMagicalAttack::set_attack(CharacterData *ch, CharacterData *victim) {
	ObjectData *mag_cont;

	mag_cont = GET_EQ(ch, WEAR_QUIVER);
	if (GET_OBJ_VAL(mag_cont, 2) <= 0) {
		act("Эх какой выстрел мог бы быть, а так колчан пуст.", false, ch, 0, 0, TO_CHAR);
		act("$n пошарил$g в колчане и достал$g оттуда мышь.", false, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		mag_cont->set_val(2, 0);
		return;
	}

	mag_single_target(GET_REAL_LEVEL(ch), ch, victim, nullptr, GET_OBJ_VAL(mag_cont, 0), ESaving::SAVING_REFLEX);
}

bool WeaponMagicalAttack::set_count_attack(CharacterData *ch) {
	ObjectData *mag_cont;
	mag_cont = GET_EQ(ch, WEAR_QUIVER);
	//sprintf(buf, "Количество выстрелов %d", get_count());
	//act(buf, true, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	//выстрел из колчана
	if ((GET_EQ(ch, WEAR_BOTHS) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS)) == ObjectData::ITEM_WEAPON))
		&& (GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS)) == SKILL_BOWS)
		&& (GET_EQ(ch, WEAR_QUIVER))) {
		//если у нас в руках лук и носим колчан
		set_count(get_count() + 1);

		//по договоренности кадый 4 выстрел магический
		if (get_count() >= 4) {
			set_count(0);
			mag_cont->add_val(2, -1);
			return true;

		} else if ((can_use_feat(ch, DEFT_SHOOTER_FEAT)) && (get_count() == 3)) {
			set_count(0);
			mag_cont->add_val(2, -1);
			return true;
		}
		return false;
	}
	set_count(0);
	return false;
}
