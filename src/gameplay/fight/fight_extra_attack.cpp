#include "fight_extra_attack.h"
#include "engine/entities/char_data.h"
#include "gameplay/magic/spells.h"
#include "utils/utils.h"

WeaponMagicalAttack::WeaponMagicalAttack(CharacterData *ch) { ch_ = ch; }

void WeaponMagicalAttack::set_attack(CharacterData *ch, CharacterData *victim) {
	ObjectData *mag_cont;

	mag_cont = ch->equipment[WEAR_QUIVER];
	if (GET_OBJ_VAL(mag_cont, 2) <= 0) {
		act("Эх какой выстрел мог бы быть, а так колчан пуст.", false, ch, 0, 0, TO_CHAR);
		act("$n пошарил$g в колчане и достал$g оттуда мышь.", false, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		mag_cont->set_val(2, 0);
		return;
	}

	mag_single_target(GetRealLevel(ch), ch, victim, nullptr, GET_OBJ_VAL(mag_cont, 0), ESaving::SAVING_REFLEX);
}

bool WeaponMagicalAttack::set_count_attack(CharacterData *ch) {
	ObjectData *mag_cont;
	mag_cont = ch->equipment[WEAR_QUIVER];
	//sprintf(buf, "Количество выстрелов %d", get_count());
	//act(buf, true, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	//выстрел из колчана
	if ((ch->equipment[WEAR_BOTHS] && ch->equipment[WEAR_BOTHS]->get_type() == EObjType::kWeapon))
		&& (GET_OBJ_SKILL(ch->equipment[WEAR_BOTHS]) == SKILL_BOWS)
		&& (ch->equipment[WEAR_QUIVER])) {
		//если у нас в руках лук и носим колчан
		set_count(get_count() + 1);

		//по договоренности кадый 4 выстрел магический
		if (get_count() >= 4) {
			set_count(0);
			mag_cont->add_val(2, -1);
			return true;

		} else if ((CanFeat(ch, EFeat::kDeftShooter)) && (get_count() == 3)) {
			set_count(0);
			mag_cont->add_val(2, -1);
			return true;
		}
		return false;
	}
	set_count(0);
	return false;
}
