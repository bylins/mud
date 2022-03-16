/* ************************************************************************
*   File: magic.cpp                                     Part of Bylins    *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "magic.h"

#include "action_targeting.h"
#include "affects/affect_handler.h"
#include "entities/world_characters.h"
#include "cmd/hire.h"
#include "corpse.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/mobact.h"
#include "fightsystem/pk.h"
#include "handler.h"
#include "obj_prototypes.h"
#include "utils/random.h"
#include "structs/global_objects.h"

extern int what_sky;
//extern DescriptorData *descriptor_list;
//extern struct spell_create_type spell_create[];
extern int interpolate(int min_value, int pulse);
extern int attack_best(CharData *ch, CharData *victim);

byte saving_throws(int class_num, ESaving saving, int level);    // class.cpp
byte extend_saving_throws(int class_num, ESaving save, int level);
int CheckCharmices(CharData *ch, CharData *victim, int spellnum);
void ReactToCast(CharData *victim, CharData *caster, int spellnum);

//bool material_component_processing(CharacterData *caster, CharacterData *victim, int spellnum);
bool material_component_processing(CharData *caster, int vnum, int spellnum);

bool is_room_forbidden(RoomData *room) {
	for (const auto &af : room->affected) {
		if (af->type == kSpellForbidden && (number(1, 100) <= af->modifier)) {
			return true;
		}
	}
	return false;
}

// * Saving throws are now in class.cpp as of bpl13.

/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */

int CalcAntiSavings(CharData *ch) {
	int modi = 0;

	if (WAITLESS(ch))
		modi = 350;
	else if (GET_GOD_FLAG(ch, GF_GODSLIKE))
		modi = 250;
	else if (GET_GOD_FLAG(ch, GF_GODSCURSE))
		modi = -250;
	else
		modi = GET_CAST_SUCCESS(ch);
	modi += MAX(0, MIN(20, (int) ((GET_REAL_WIS(ch) - 23) * 3 / 2)));
	if (!IS_NPC(ch)) {
		modi *= ch->get_cond_penalty(P_CAST);
	}
//  log("[EXT_APPLY] Name==%s modi==%d",GET_NAME(ch), modi);
	return modi;
}

int CalculateSaving(CharData *killer, CharData *victim, ESaving saving, int ext_apply) {
	int temp_save_stat = 0, temp_awake_mod = 0;

	if (-GET_SAVE(victim, saving) / 10 > number(1, 100)) {
		return 1;
	}

	// NPCs use warrior tables according to some book
	int save;
	int class_sav = GET_CLASS(victim);

	if (IS_NPC(victim)) {
		class_sav = ECharClass::kMob;    // неизвестный класс моба
	} else {
		if (class_sav < 0 || class_sav >= kNumPlayerClasses)
			class_sav = ECharClass::kWarrior;    // неизвестный класс игрока
	}

	// Базовые спасброски профессии/уровня
	save = extend_saving_throws(class_sav, saving, GetRealLevel(victim));

	switch (saving) {
		case ESaving::kReflex:      //3 реакция
			if ((save > 0) && can_use_feat(victim, DODGER_FEAT))
				save >>= 1;
			save -= dex_bonus(GET_REAL_DEX(victim));
			temp_save_stat = dex_bonus(GET_REAL_DEX(victim));
			if (victim->ahorse())
				save += 20;
			break;
		case ESaving::kStability:   //2  стойкость
			save += -GET_REAL_CON(victim);
			if (victim->ahorse())
				save -= 20;
			temp_save_stat = GET_REAL_CON(victim);
			break;
		case ESaving::kWill:        //1  воля
			save += -GET_REAL_WIS(victim);
			temp_save_stat = GET_REAL_WIS(victim);
			break;
		case ESaving::kCritical:   //0   здоровье
			save += -GET_REAL_CON(victim);
			temp_save_stat = GET_REAL_CON(victim);
			break;
	}

	// Ослабление магических атак
	if (saving != ESaving::kReflex) {
		if ((save > 0) &&
			(AFF_FLAGGED(victim, EAffectFlag::AFF_AIRAURA)
				|| AFF_FLAGGED(victim, EAffectFlag::AFF_FIREAURA)
				|| AFF_FLAGGED(victim, EAffectFlag::AFF_EARTHAURA)
				|| AFF_FLAGGED(victim, EAffectFlag::AFF_ICEAURA))) {
			save >>= 1;
		}
	}
	// Учет осторожного стиля
	if (PRF_FLAGGED(victim, PRF_AWAKE)) {
		if (can_use_feat(victim, IMPREGNABLE_FEAT)) {
			save -= MAX(0, victim->get_skill(ESkill::kAwake) - 80) / 2;
			temp_awake_mod = MAX(0, victim->get_skill(ESkill::kAwake) - 80) / 2;
		}
		temp_awake_mod += CalculateSkillAwakeModifier(killer, victim);
		save -= CalculateSkillAwakeModifier(killer, victim);
	}

	save += GET_SAVE(victim, saving);    // одежда
	save += ext_apply;    // внешний модификатор

	if (IS_GOD(victim))
		save = -150;
	else if (GET_GOD_FLAG(victim, GF_GODSLIKE))
		save -= 50;
	else if (GET_GOD_FLAG(victim, GF_GODSCURSE))
		save += 50;
	if (IS_NPC(victim) && !IS_NPC(killer))
		log("SAVING: Caster==%s  Mob==%s vnum==%d Level==%d saving==%d base_save==%d stat_bonus==%d awake_bonus==%d save_ext==%d cast_apply==%d result==%d new_random==%d",
			GET_NAME(killer),
			GET_NAME(victim),
			GET_MOB_VNUM(victim),
			GetRealLevel(victim),
			to_underlying(saving), // Зачем это тут вообще?
			extend_saving_throws(class_sav, saving, GetRealLevel(victim)),
			temp_save_stat,
			temp_awake_mod,
			GET_SAVE(victim, saving),
			ext_apply,
			save,
			number(1, 200));
	// Throwing a 0 is always a failure.
	return save;
}

int CalcGeneralSaving(CharData *killer, CharData *victim, ESaving type, int ext_apply) {
	int save = CalculateSaving(killer, victim, type, ext_apply);
	if (MAX(10, save) <= number(1, 200))
		return (true);

	// Oops, failed. Sorry.
	return (false);
}

int multi_cast_say(CharData *ch) {
	if (!IS_NPC(ch))
		return 1;
	switch (GET_RACE(ch)) {
		case NPC_RACE_EVIL_SPIRIT:
		case NPC_RACE_GHOST:
		case NPC_RACE_HUMAN:
		case NPC_RACE_ZOMBIE:
		case NPC_RACE_SPIRIT: return 1;
	}
	return 0;
}

void show_spell_off(int aff, CharData *ch) {
	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_WRITING))
		return;

	// TODO:" refactor and replace int aff by ESpell
	const std::string &msg = get_wear_off_text(static_cast<ESpell>(aff));
	if (!msg.empty()) {
		act(msg.c_str(), false, ch, nullptr, nullptr, kToChar | kToSleep);
		send_to_char("\r\n", ch);
	}
}

// зависимость длительности закла от скила магии
float func_koef_duration(int spellnum, int percent) {
	switch (spellnum) {
		case kSpellStrength:
		case kSpellDexterity:
		case kSpellGroupBlink:
			[[fallthrough]];
		case kSpellBlink: return 1 + percent / 400.00;
			break;
		default: return 1;
			break;
	}
}

// зависимость модификации спелла от скила магии
float func_koef_modif(int spellnum, int percent) {
	switch (spellnum) {
		case kSpellStrength:
		case kSpellDexterity:
			if (percent > 100)
				return 1;
			return 0;
			break;
		case kSpellMassSlow:
		case kSpellSlowdown: {
			if (percent >= 80) {
				return (percent - 80) / 20.00 + 1.00;
			}
		}
			break;
		case kSpellSonicWave:
			if (percent > 100) {
				return (percent - 80) / 20.00; // после 100% идет прибавка
			}
			return 1;
			break;
		case kSpellFascination:
		case kSpellHypnoticPattern:
			if (percent >= 80) {
				return (percent - 80) / 20.00 + 1.00;
			}
			return 1;
			break;
		default: return 1;
	}
	return 0;
}

int magic_skill_damage_calc(CharData *ch, CharData *victim, int spellnum, int dam) {
	if (IS_NPC(ch)) {
		dam += dam * ((GET_REAL_WIS(ch) - 22) * 5) / 100;
		return (dam);
	}

	auto skill_id = GetMagicSkillId(spellnum);
	if (MUD::Skills().IsValid(skill_id)) {
		float tmp = (1 + std::min(CalcSkillMinCap(ch, skill_id), ch->get_skill(skill_id)) / 500.0);
		dam = (int) dam * tmp;
//	send_to_char(ch, "&CМагДамага магии со скилом %d, бонус %f &n\r\n", dam, tmp);
	}

	if (GET_REAL_WIS(ch) >= 23) {
		float tmp = (1 + (GET_REAL_WIS(ch) - 22) / 200.0);
		dam = (int) dam * tmp;
//		send_to_char(ch, "&CМагДамага магии с мудростью %d, бонус %f &n\r\n", dam, tmp);
	}

	if (!IS_NPC(ch)) {
		dam = (IS_NPC(victim) ? MIN(dam, 6 * GET_REAL_MAX_HIT(ch)) : MIN(dam, 2 * GET_REAL_MAX_HIT(ch)));
	}

	return (dam);
}

int mag_damage(int level, CharData *ch, CharData *victim, int spellnum, ESaving savetype) {
	int dam = 0, rand = 0, count = 1, modi = 0, ndice = 0, sdice = 0, adice = 0, no_savings = false;
	ObjData *obj = nullptr;

	if (victim == nullptr || IN_ROOM(victim) == kNowhere || ch == nullptr)
		return (0);

	if (!pk_agro_action(ch, victim))
		return (0);

	log("[MAG DAMAGE] %s damage %s (%d)", GET_NAME(ch), GET_NAME(victim), spellnum);
	// Magic glass
	if ((spellnum != kSpellLightingBreath && spellnum != kSpellFireBreath && spellnum != kSpellGasBreath
		&& spellnum != kSpellAcidBreath)
		|| ch == victim) {
		if (!IS_SET(SpINFO.routines, kMagWarcry)) {
			if (ch != victim && spellnum <= kSpellCount &&
				((AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICGLASS) && number(1, 100) < (GetRealLevel(victim) / 3)))) {
				act("Магическое зеркало $N1 отразило вашу магию!", false, ch, nullptr, victim, kToChar);
				act("Магическое зеркало $N1 отразило магию $n1!", false, ch, nullptr, victim, kToNotVict);
				act("Ваше магическое зеркало отразило поражение $n1!", false, ch, nullptr, victim, kToVict);
				log("[MAG DAMAGE] Зеркало - полное отражение: %s damage %s (%d)",
					GET_NAME(ch),
					GET_NAME(victim),
					spellnum);
				return (mag_damage(level, ch, ch, spellnum, savetype));
			}
		} else {
			if (ch != victim && spellnum <= kSpellCount && IS_GOD(victim)
				&& (IS_NPC(ch) || GetRealLevel(victim) > GetRealLevel(ch))) {
				act("Звуковой барьер $N1 отразил ваш крик!", false, ch, nullptr, victim, kToChar);
				act("Звуковой барьер $N1 отразил крик $n1!", false, ch, nullptr, victim, kToNotVict);
				act("Ваш звуковой барьер отразил крик $n1!", false, ch, nullptr, victim, kToVict);
				return (mag_damage(level, ch, ch, spellnum, savetype));
			}
		}

		if (!IS_SET(SpINFO.routines, kMagWarcry) && AFF_FLAGGED(victim, EAffectFlag::AFF_SHADOW_CLOAK)
			&& spellnum <= kSpellCount && number(1, 100) < 21) {
			act("Густая тень вокруг $N1 жадно поглотила вашу магию.", false, ch, nullptr, victim, kToChar);
			act("Густая тень вокруг $N1 жадно поглотила магию $n1.", false, ch, nullptr, victim, kToNotVict);
			act("Густая тень вокруг вас поглотила магию $n1.", false, ch, nullptr, victim, kToVict);
			log("[MAG DAMAGE] Мантия  - поглощение урона: %s damage %s (%d)", GET_NAME(ch), GET_NAME(victim), spellnum);
			return (0);
		} 
		// Блочим маг дамагу от директ спелов для Витязей : шанс (скил/20 + вес.щита/2) ~ 30% при 200 скила и 40вес щита (Кудояр)
		if (!IS_SET(SpINFO.routines, kMagWarcry) && !IS_SET(SpINFO.routines, kMagMasses) && !IS_SET(SpINFO.routines, kMagAreas)
			&& (victim->get_skill(ESkill::kShieldBlock) > 100) && GET_EQ(victim, WEAR_SHIELD) && can_use_feat(victim, MAGICAL_SHIELD_FEAT)
			&& ( number(1, 100) < ((victim->get_skill(ESkill::kShieldBlock))/20 + GET_OBJ_WEIGHT(GET_EQ(victim, WEAR_SHIELD))/2)))
			{
				act("Ловким движением $N0 отразил щитом вашу магию.", false, ch, nullptr, victim, kToChar);
				act("Ловким движением $N0 отразил щитом магию $n1.", false, ch, nullptr, victim, kToNotVict);
				act("Вы отразили своим щитом магию $n1.", false, ch, nullptr, victim, kToVict);
				return (0);
			}
	}

	// позиции до начала атаки для расчета модификаторов в damage()
	// в принципе могут меняться какими-то заклами, но пока по дефолту нет
	auto ch_start_pos = GET_POS(ch);
	auto victim_start_pos = GET_POS(victim);

	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		if (can_use_feat(ch, RELATED_TO_MAGIC_FEAT) && !IS_NPC(victim)) {
			modi -= 80; // на игрока бонуса + каст нет
		}
		if (can_use_feat(ch, MAGICAL_INSTINCT_FEAT) && !IS_NPC(victim)) {
			modi -= 30; // на игрока бонуса + каст нет
		}
		if (PRF_FLAGGED(ch, PRF_AWAKE) && !IS_NPC(victim))
			modi = modi - 50; // на игрока бонуса + каст нет
	}
	if (!IS_NPC(ch) && (GetRealLevel(ch) > 10))
		modi += (GetRealLevel(ch) - 10);

	// вводим переменную-модификатор владения школы магии	
	const int ms_mod = func_koef_modif(spellnum, ch->get_skill(GetMagicSkillId(spellnum))); // к кубикам от % владения магии

//расчет на 30 морт 30 левел 90 мудры

	switch (spellnum) {
		// ******** ДЛЯ ВСЕХ МАГОВ ********
		// магическая стрела
		//  мин 2+10 среднее 5+10 макс 8+10
		// нейтрал
		case kSpellMagicMissile: modi += 300;
			ndice = 2;
			sdice = 4;
			adice = 10;
			// если есть фит магическая стрела, то стрелок на 30 уровне будет 6
			if (can_use_feat(ch, MAGICARROWS_FEAT))
				count = (level + 9) / 5;
			else
				count = (level + 9) / 10;
			break;
			// ледяное прикосновение 
			// мин 15+30 среднее 22.5+30 макс 30+30  
		case kSpellChillTouch: savetype = ESaving::kReflex;
			ndice = 15;
			sdice = 2;
			adice = level;
			break;
			// кислота 
			// 6+24 мин 48+24 макс 90+24
			// нейтрал
		case kSpellAcid: savetype = ESaving::kReflex;
			obj = nullptr;
			if (IS_NPC(victim)) {
				rand = number(1, 50);
				if (rand <= WEAR_BOTHS) {
					obj = GET_EQ(victim, rand);
				} else {
					for (rand -= WEAR_BOTHS, obj = victim->carrying; rand && obj;
						 rand--, obj = obj->get_next_content());
				}
			}
			if (obj) {
				ndice = 6;
				sdice = 10;
				adice = level;
				act("Кислота покрыла $o3.", false, victim, obj, nullptr, kToChar);
				alterate_object(obj, number(level * 2, level * 4), 100);
			} else {
				ndice = 6;
				sdice = 15;
				adice = (level - 18) * 2;
			}
			break;

			// землетрясение
			// мин 6+16 среднее 48+16 макс 90+16 (240)
		case kSpellEarthquake: savetype = ESaving::kReflex;
			ndice = 6;
			sdice = 15;
			adice = (level - 22) * 2;
			// если наездник, то считаем не сейвисы, а ESkill::kRiding
			if (ch->ahorse()) {
//		    5% шанс успеха,
				rand = number(1, 100);
				if (rand > 95)
					break;
				// провал - 5% шанс или скилл наездника vs скилл магии кастера на кубике d6
				if (rand < 5 || (CalcCurrentSkill(victim, ESkill::kRiding, nullptr) * number(1, 6))
					< GET_SKILL(ch, ESkill::kEarthMagic) * number(1, 6)) {//фейл
					ch->drop_from_horse();
					break;
				}
			}
			if (GET_POS(victim) > EPosition::kSit && !WAITLESS(victim) && (number(1, 999) > GET_AR(victim) * 10) &&
				(GET_MOB_HOLD(victim) || !CalcGeneralSaving(ch, victim, ESaving::kReflex, CALC_SUCCESS(modi, 30)))) {
				if (IS_HORSE(ch))
					ch->drop_from_horse();
				act("$n3 повалило на землю.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Вас повалило на землю.", false, victim, nullptr, nullptr, kToChar);
				GET_POS(victim) = EPosition::kSit;
				update_pos(victim);
				WAIT_STATE(victim, 2 * kPulseViolence);
			}
			break;
		//звуковая волна зависит от школы
		// мин 10+20 среднее 45+20 макс 80+20
		case kSpellSonicWave: savetype = ESaving::kStability;
			ndice = 5 + ms_mod;
			sdice = 8;
			adice = level/3 + 2*ms_mod;
			if (GET_POS(victim) > EPosition::kSit &&
				!WAITLESS(victim) && (number(1, 999) > GET_AR(victim) * 10) &&
				(GET_MOB_HOLD(victim) || !CalcGeneralSaving(ch, victim, ESaving::kStability, CALC_SUCCESS(modi, 60)))) {
				act("$n3 повалило на землю.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Вас повалило на землю.", false, victim, nullptr, nullptr, kToChar);
				GET_POS(victim) = EPosition::kSit;
				update_pos(victim);
				WAIT_STATE(victim, 2 * kPulseViolence);
			}
			break;

			// ********** ДЛЯ ФРАГЕРОВ **********
			// горящие руки
			// мин 8+10 среднее 16+10 мах 24+10
			// ОГОНЬ
		case kSpellBurningHands: savetype = ESaving::kReflex;
			ndice = 8;
			sdice = 3;
			adice = (level + 2) / 3;
			break;

			// обжигающая хватка
			// мин 10+10 среднее 35+10 макс 60+10)
			// ОГОНЬ
		case kSpellShockingGasp: savetype = ESaving::kReflex;
			ndice = 10;
			sdice = 6;
			adice = (level + 2) / 3;
			break;

			// молния
			// мин 3+6 среднее 9+6 - макс 15+6 
			// ВОЗДУХ
		case kSpellLightingBolt: savetype = ESaving::kReflex;
			ndice = 3;
			sdice = 5;
			count = (level + 6) / 6;
			break;

			// яркий блик нет резиста
			// мин 10+10 среднее 30+10 макс 50+10
			// ОГОНЬ
		case kSpellShineFlash:
			ndice = 10;
			sdice = 5;
			adice = (level + 2) / 3;
			break;

			// шаровая молния ов)
			// мин 37+30 среднее 129.5 +30 макс 222+30
			// ВОЗДУХ
//дебаф
//			af[0].location = APPLY_HITROLL;
//			af[1].location = APPLY_CAST_SUCCESS;

		case kSpellCallLighting: savetype = ESaving::kReflex;
			ndice = 7 + GET_REAL_REMORT(ch);
			sdice = 6;
			adice = level;
			break;

			// ледяные стрелы
			// мин 6+30 среднее 18+30 макс 30+30
			// ОГОНЬ
		case kSpellIceBolts: savetype = ESaving::kStability;
			ndice = 6;
			sdice = 5;
			adice = level;
			break;

			// ледяной ветер
			// мин 10+30 среднее 30+30 макс 50+30
			// ВОДА
		case kSpellColdWind: savetype = ESaving::kStability;
			ndice = 10;
			sdice = 5;
			adice = level;
			break;

			// Огненный шар
			// мин 10+25 среднее 110+25 макс 210+25
			// ОГОНЬ
		case kSpellFireball: savetype = ESaving::kReflex;
			ndice = 10;
			sdice = 21;
			adice = (level - 25) * 5;
			break;

			// Огненный поток 
			// массовое
			// мин 40+30 среднее 80+30 макс 120*30
			// ОГОНЬ, ареа
		case kSpellFireBlast: savetype = ESaving::kStability;
			ndice = 10 + GET_REAL_REMORT(ch);
			sdice = 3;
			adice = level;
			break;

			// метеоритный шторм - уровень 22 круг 7 (4 слота)
			// *** мин 66 макс 80  (240)
			// нейтрал, ареа
/*	case SPELL_METEORSTORM:
		savetype = kReflex;
		ndice = 11+GET_REAL_REMORT(ch);
		sdice = 11;
		adice = (level - 22) * 3;
		break;*/

			// цепь молний
			// массовое
			// мин 32+90 среднее 80+90 макс 128+90
			// ВОЗДУХ, ареа
		case kSpellChainLighting: savetype = ESaving::kStability;
			ndice = 2 + GET_REAL_REMORT(ch);
			sdice = 4;
			adice = (level + GET_REAL_REMORT(ch)) * 2;
			break;

			// гнев богов)
			// мин 10+180 среднее 70+180 макс 130+180
			// ВОДА
		case kSpellGodsWrath: savetype = ESaving::kWill;
			ndice = 10;
			sdice = 13;
			adice = level * 6;
			break;

			// ледяной шторм
			// массовое
			// мин 5+20 среднее 27.5+20 макс 50+20
			// ВОДА, ареа
		case kSpellIceStorm: savetype = ESaving::kStability;
			ndice = 5;
			sdice = 10;
			adice = (level - 26) * 5;
			break;

			// суд богов
			// массовое
			// мин 16+120 среднее 32+150 макс 38+180
			// ВОЗДУХ, ареа
		case kSpellArmageddon: savetype = ESaving::kWill;
			if (!(IS_NPC(ch))) {
				ndice = 10 + ((GET_REAL_REMORT(ch) / 3) - 4);
				sdice = level / 9;
				adice = level * (number(4, 6));
			} else {
				ndice = 12;
				sdice = 3;
				adice = level * 6;
			}
			break;
			// ******* ХАЙЛЕВЕЛ СУПЕРДАМАДЖ МАГИЯ ******
			// каменное проклятие
			//  мин 12+30 среднее 486+30 макс 990+30
		case kSpellStunning:
			if (ch == victim ||
				((number(1, 999) > GET_AR(victim) * 10) &&
					!CalcGeneralSaving(ch, victim, ESaving::kCritical, CALC_SUCCESS(modi, GET_REAL_WIS(ch))))) {
				savetype = ESaving::kStability;
				ndice = GET_REAL_WIS(ch) / 5; 	//18
				sdice = GET_REAL_WIS(ch);		//90
				adice = 5 + (GET_REAL_WIS(ch) - 20) / 6;	//5+11
				int choice_stunning = 750;
				if (can_use_feat(ch, DARKDEAL_FEAT))
					choice_stunning -= GET_REAL_REMORT(ch) * 15;
				if (number(1, 999) > choice_stunning) {
					act("Ваше каменное проклятие отшибло сознание у $N1.", false, ch, nullptr, victim, kToChar);
					act("Каменное проклятие $n1 отшибло сознание у $N1.", false, ch, nullptr, victim, kToNotVict);
					act("У вас отшибло сознание, вам очень плохо...", false, ch, nullptr, victim, kToVict);
					GET_POS(victim) = EPosition::kStun;
					WAIT_STATE(victim, adice * kPulseViolence);
				}
			} else {
				ndice = GET_REAL_WIS(ch) / 7; 	//12
				sdice = GET_REAL_WIS(ch);		//80
				adice = level;
			}
			break;

			// круг пустоты - круг 28 уровень 9 (1)
			// мин 40+17 среднее 1820+17 макс 3600 +17
			// для всех
		case kSpellVacuum: savetype = ESaving::kStability;
			ndice = std::max(1, (GET_REAL_WIS(ch) - 10) / 2);	//40
			sdice = std::max(1, GET_REAL_WIS(ch) - 10);			//90
			adice = 4 + std::max(1, GetRealLevel(ch) + 1 + (GET_REAL_WIS(ch) - 29)) / 7;	//17
			if (ch == victim ||
				(!CalcGeneralSaving(ch, victim, ESaving::kCritical, CALC_SUCCESS(modi, GET_REAL_WIS(ch))) &&
					(number(1, 999) > GET_AR(victim) * 10) &&
					number(0, 1000) <= 500)) {
				GET_POS(victim) = EPosition::kStun;
				WAIT_STATE(victim, adice * kPulseViolence);
			}
			break;

			// ********* СПЕЦИФИЧНАЯ ДЛЯ КЛЕРИКОВ МАГИЯ **********
		case kSpellDamageLight: savetype = ESaving::kCritical;
			ndice = 4;
			sdice = 3;
			adice = (level + 2) / 3;
			break;
		case kSpellDamageSerious: savetype = ESaving::kCritical;
			ndice = 10;
			sdice = 3;
			adice = (level + 1) / 2;
			break;
		case kSpellDamageCritic: savetype = ESaving::kCritical;
			ndice = 15;
			sdice = 4;
			adice = (level + 1) / 2;
			break;
		case kSpellDispelEvil: ndice = 4;
			sdice = 4;
			adice = level;
			if (ch != victim && IS_EVIL(ch) && !WAITLESS(ch) && GET_HIT(ch) > 1) {
				send_to_char("Ваша магия обратилась против вас.", ch);
				GET_HIT(ch) = 1;
			}
			if (!IS_EVIL(victim)) {
				if (victim != ch)
					act("Боги защитили $N3 от вашей магии.", false, ch, nullptr, victim, kToChar);
				return (0);
			};
			break;
		case kSpellDispelGood: ndice = 4;
			sdice = 4;
			adice = level;
			if (ch != victim && IS_GOOD(ch) && !WAITLESS(ch) && GET_HIT(ch) > 1) {
				send_to_char("Ваша магия обратилась против вас.", ch);
				GET_HIT(ch) = 1;
			}
			if (!IS_GOOD(victim)) {
				if (victim != ch)
					act("Боги защитили $N3 от вашей магии.", false, ch, nullptr, victim, kToChar);
				return (0);
			};
			break;
		case kSpellHarm: savetype = ESaving::kCritical;
			ndice = 7;
			sdice = level;
			adice = level * GET_REAL_REMORT(ch) / 4;
			//adice = (level + 4) / 5;
			break;

		case kSpellFireBreath:
		case kSpellFrostBreath:
		case kSpellAcidBreath:
		case kSpellLightingBreath:
		case kSpellGasBreath: savetype = ESaving::kStability;
			if (!IS_NPC(ch))
				return (0);
			ndice = ch->mob_specials.damnodice;
			sdice = ch->mob_specials.damsizedice;
			adice = GetRealDamroll(ch) + str_bonus(GET_REAL_STR(ch), STR_TO_DAM);
			break;
		// высосать жизнь
		case kSpellSacrifice:
			if (WAITLESS(victim))
				break;
			ndice = 8;
			sdice = 8;
			adice = level;
			break;
		// пылевая буря
		// массовое
		// мин 5+120 среднее 17.5+120  макс 30+120
		case kSpellDustStorm: savetype = ESaving::kStability;
			ndice = 5;
			sdice = 6;
			adice = level + GET_REAL_REMORT(ch) * 3;
			if (GET_POS(victim) > EPosition::kSit &&
				!WAITLESS(victim) && (number(1, 999) > GET_AR(victim) * 10) &&
				(!CalcGeneralSaving(ch, victim, ESaving::kReflex, CALC_SUCCESS(modi, 30)))) {
				act("$n3 повалило на землю.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Вас повалило на землю.", false, victim, nullptr, nullptr, kToChar);
				GET_POS(victim) = EPosition::kSit;
				update_pos(victim);
				WAIT_STATE(victim, 2 * kPulseViolence);
			}
			break;
		// камнепад
		// массовое на комнату
		// мин 8+60 среднее 36+60 макс 64+60
		case kSpellEarthfall: savetype = ESaving::kReflex;
			ndice = 8;
			sdice = 8;
			adice = level * 2;
			break;
		// шок
		// массовое на комнату
		// мин 6+90 среднее 48+90 макс 90+90
		case kSpellShock: savetype = ESaving::kReflex;
			ndice = 6;
			sdice = level / 2;
			adice = (level + GET_REAL_REMORT(ch)) * 2;
			break;
		// вопль
		// массовое на комнату
		// мин 10+90 среднее 185+90 макс 360*90
		case kSpellScream: savetype = ESaving::kStability;
			ndice = 10;
			sdice = (level + GET_REAL_REMORT(ch)) / 5;	//36
			adice = level + GET_REAL_REMORT(ch) * 2;	//90
			break;
		// вихрь
		// мин 16+35 среднее 176+40  макс 336+40
		case kSpellWhirlwind: savetype = ESaving::kReflex;
			if (!(IS_NPC(ch))) {
				ndice = 10 + ((GET_REAL_REMORT(ch) / 3) - 4);	//16
				sdice = 18 + (3 - (30 - level) / 3);			//21
				adice = (level + GET_REAL_REMORT(ch) - 25) * (number(1, 3));	//35..40..45
			} else {
				ndice = 10;
				sdice = 21;
				adice = (level - 5) * (number(2, 4));
			}
			break;
		// зубы индрика - без резиста
		// мин 33+61 среднее 82,5+61 макс 132+61
		case kSpellIndriksTeeth:
			ndice = 3 + GET_REAL_REMORT(ch);
			sdice = 4;
			adice = level + GET_REAL_REMORT(ch) + 1;
			break;
		// кислотная стрела
		// мин 20+35 среднее 210+35 макс 400+35
		case kSpellAcidArrow: savetype = ESaving::kReflex;
			ndice = 10 + GET_REAL_REMORT(ch) / 3;
			sdice = 20;
			adice = level + GET_REAL_REMORT(ch) - 25;
			break;
		//громовой камень
		// мин 33+61 среднее 115.5+61 макс 198+61
		case kSpellThunderStone: savetype = ESaving::kReflex;
			ndice = 3 + GET_REAL_REMORT(ch);
			sdice = 6;
			adice = 1 + level + GET_REAL_REMORT(ch);
			break;
		// глыба
		// мин 20+35 среднее 210 +40 макс 400+45
		case kSpellClod: savetype = ESaving::kReflex;
			ndice = 10 + GET_REAL_REMORT(ch) / 3;
			sdice = 20;
			adice = (level + GET_REAL_REMORT(ch) - 25) * (number(1, 3));	//35..40..45
			break;
		// силы света
		// мин 10 + 150 среднее 45+150 макс 80+150
		case kSpellHolystrike:
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_EVILESS)) {
				dam = -1;
				no_savings = true;
				// смерть или диспелл :)
				if (CalcGeneralSaving(ch, victim, ESaving::kWill, modi)) {
					act("Черное облако вокруг вас нейтрализовало действие тумана, растворившись в нем.",
						false, victim, nullptr, nullptr, kToChar);
					act("Черное облако вокруг $n1 нейтрализовало действие тумана.",
						false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
					affect_from_char(victim, kSpellEviless);
				} else {
					dam = MAX(1, GET_HIT(victim) + 1);
					if (IS_NPC(victim))
						dam += 99;    // чтобы насмерть
				}
			} else {
				ndice = 10;
				sdice = 8;
				adice = level * 5;
			}
			break;

		case kSpellWarcryOfRage: ndice = (level + 3) / 4;
			sdice = 6;
			adice = RollDices(GET_REAL_REMORT(ch) / 2, 8);
			break;

		case kSpellWarcryOfThunder: {
			ndice = GET_REAL_REMORT(ch) + (level + 2) / 3;
			sdice = 5;
			if (GET_POS(victim) > EPosition::kSit &&
				!WAITLESS(victim) &&
				(GET_MOB_HOLD(victim) || !CalcGeneralSaving(ch, victim, ESaving::kStability, GET_REAL_CON(ch)))) {
				act("$n3 повалило на землю.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Вас повалило на землю.", false, victim, nullptr, nullptr, kToChar);
				GET_POS(victim) = EPosition::kSit;
				update_pos(victim);
				WAIT_STATE(victim, 2 * kPulseViolence);
			}
			break;
		}
		// стрелы, нет в мире
		// мин 33+61 среднее 82,5+61 макс 132+61
		case kSpellArrowsFire:
		case kSpellArrowsWater:
		case kSpellArrowsEarth:
		case kSpellArrowsAir:
		case kSpellArrowsDeath:
			if (!(IS_NPC(ch))) {
				act("Ваша магическая стрела поразила $N1.", false, ch, nullptr, victim, kToChar);
				act("Магическая стрела $n1 поразила $N1.", false, ch, nullptr, victim, kToNotVict);
				act("Магическая стрела настигла вас.", false, ch, nullptr, victim, kToVict);
				ndice = 3 + GET_REAL_REMORT(ch);
				sdice = 4;
				adice = level + GET_REAL_REMORT(ch) + 1;
			} else {
				ndice = 20;
				sdice = 4;
				adice = level * 3;
			}

			break;

	}            // switch(spellnum)

	if (!dam && !no_savings) {
		double koeff = 1;
		if (IS_NPC(victim)) {
			if (NPC_FLAGGED(victim, NPC_FIRECREATURE)) {
				if (IS_SET(SpINFO.spell_class, kTypeFire))
					koeff /= 2;
				if (IS_SET(SpINFO.spell_class, kTypeWater))
					koeff *= 2;
			}
			if (NPC_FLAGGED(victim, NPC_AIRCREATURE)) {
				if (IS_SET(SpINFO.spell_class, kTypeEarth))
					koeff *= 2;
				if (IS_SET(SpINFO.spell_class, kTypeAir))
					koeff /= 2;
			}
			if (NPC_FLAGGED(victim, NPC_WATERCREATURE)) {
				if (IS_SET(SpINFO.spell_class, kTypeFire))
					koeff *= 2;
				if (IS_SET(SpINFO.spell_class, kTypeWater))
					koeff /= 2;
			}
			if (NPC_FLAGGED(victim, NPC_EARTHCREATURE)) {
				if (IS_SET(SpINFO.spell_class, kTypeEarth))
					koeff /= 2;
				if (IS_SET(SpINFO.spell_class, kTypeAir))
					koeff *= 2;
			}
		}
		dam = RollDices(ndice, sdice) + adice;
		dam = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_EFFECT, dam);

		if (can_use_feat(ch, POWER_MAGIC_FEAT) && IS_NPC(victim)) {
			dam += (int) dam * 0.5;
		}
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DATURA_POISON))
			dam -= dam * GET_POISON(ch) / 100;
		if (!IS_SET(SpINFO.routines, kMagWarcry)) {
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))
				koeff /= 2;
		}
/* // и зачем эта хрень только путает
		if (dam > 0) {
			koeff *= 1000;
			dam = (int) MMAX(1.0, (dam * MMAX(300.0, MMIN(koeff, 2500.0)) / 1000.0)); //капаем максимальный бонус не более х4
		}
*/
		if (ch->add_abils.percent_magdam_add > 0) {
//			send_to_char(ch, "&CМагДамага магии %d &n\r\n", dam);
			auto tmp = ch->add_abils.percent_magdam_add / 100.0;
			dam += (int) dam * tmp;
//			send_to_char(ch, "&CМагДамага c + процентами дамаги== %d, добавили = %d процентов &n\r\n", dam, (int) (tmp * 100));
		}
		//вместо старого учета мудры добавлена обработка с учетом скиллов
		//после коэффициента - так как в самой функции стоит планка по дамагу, пусть и относительная
		dam = magic_skill_damage_calc(ch, victim, spellnum, dam);
	}

	//Голодный кастер меньше дамажит!
	if (!IS_NPC(ch))
		dam *= ch->get_cond_penalty(P_DAMROLL);

	if (number(1, 100) <= MIN(75, GET_MR(victim)))
		dam = 0;

	for (; count > 0 && rand >= 0; count--) {
		if (ch->in_room != kNowhere
			&& IN_ROOM(victim) != kNowhere
			&& GET_POS(ch) > EPosition::kStun
			&& GET_POS(victim) > EPosition::kDead) {
			// инит полей для дамага
			Damage dmg(SpellDmg(spellnum), dam, fight::kMagicDmg);
			dmg.ch_start_pos = ch_start_pos;
			dmg.victim_start_pos = victim_start_pos;

			if (can_use_feat(ch, POWER_MAGIC_FEAT) && IS_NPC(victim)) {
				dmg.flags.set(fight::kIgnoreAbsorbe);
			}
			// отражение магии в кастующего
			if (ch == victim) {
				dmg.flags.set(fight::kMagicReflect);
			}
			if (count <= 1) {
				dmg.flags.reset(fight::kNoFleeDmg);
			} else {
				dmg.flags.set(fight::kNoFleeDmg);
			}
			rand = dmg.Process(ch, victim);
		}
	}
	return rand;
}

int CalcDuration(CharData *ch, int cnst, int level, int level_divisor, int min, int max) {
	int result = 0;
	if (IS_NPC(ch)) {
		result = cnst;
		if (level > 0 && level_divisor > 0)
			level = level / level_divisor;
		else
			level = 0;
		if (min > 0)
			level = std::min(level, min);
		if (max > 0)
			level = std::max(level, max);
		return (level + result);
	}
	result = cnst * SECS_PER_MUD_HOUR;
	if (level > 0 && level_divisor > 0)
		level = level * SECS_PER_MUD_HOUR / level_divisor;
	else
		level = 0;
	if (min > 0)
		level = MIN(level, min * SECS_PER_MUD_HOUR);
	if (max > 0)
		level = MAX(level, max * SECS_PER_MUD_HOUR);
	result = (level + result) / SECS_PER_PLAYER_AFFECT;
	return (result);
}

bool material_component_processing(CharData *caster, CharData *victim, int spellnum) {
	int vnum = 0;
	const char *missing = nullptr, *use = nullptr, *exhausted = nullptr;
	switch (spellnum) {
		case kSpellFascination:
			for (auto i = caster->carrying; i; i = i->get_next_content()) {
				if (GET_OBJ_TYPE(i) == ObjData::ITEM_INGREDIENT && i->get_val(1) == 3000) {
					vnum = GET_OBJ_VNUM(i);
					break;
				}
			}
			use = "Вы взяли череп летучей мыши в левую руку.\r\n";
			missing = "Батюшки светы! А помаду-то я дома забыл$g.\r\n";
			exhausted = "$o рассыпался в ваших руках от неловкого движения.\r\n";
			break;
		case kSpellHypnoticPattern:
			for (auto i = caster->carrying; i; i = i->get_next_content()) {
				if (GET_OBJ_TYPE(i) == ObjData::ITEM_INGREDIENT && i->get_val(1) == 3006) {
					vnum = GET_OBJ_VNUM(i);
					break;
				}
			}
			use = "Вы разожгли палочку заморских благовоний.\r\n";
			missing = "Вы начали суматошно искать свои благовония, но тщетно.\r\n";
			exhausted = "$o дотлели и рассыпались пеплом.\r\n";
			break;
		case kSpellEnchantWeapon:
			for (auto i = caster->carrying; i; i = i->get_next_content()) {
				if (GET_OBJ_TYPE(i) == ObjData::ITEM_INGREDIENT && i->get_val(1) == 1930) {
					vnum = GET_OBJ_VNUM(i);
					break;
				}
			}
			use = "Вы подготовили дополнительные компоненты для зачарования.\r\n";
			missing = "Вы были уверены что положили его в этот карман.\r\n";
			exhausted = "$o вспыхнул голубоватым светом, когда его вставили в предмет.\r\n";
			break;

		default: log("WARNING: wrong spellnum %d in %s:%d", spellnum, __FILE__, __LINE__);
			return false;
	}
	ObjData *tobj = get_obj_in_list_vnum(vnum, caster->carrying);
	if (!tobj) {
		act(missing, false, victim, nullptr, caster, kToChar);
		return (true);
	}
	tobj->dec_val(2);
	act(use, false, caster, tobj, nullptr, kToChar);
	if (GET_OBJ_VAL(tobj, 2) < 1) {
		act(exhausted, false, caster, tobj, nullptr, kToChar);
		obj_from_char(tobj);
		extract_obj(tobj);
	}
	return (false);
}

bool material_component_processing(CharData *caster, int /*vnum*/, int spellnum) {
	const char *missing = nullptr, *use = nullptr, *exhausted = nullptr;
	switch (spellnum) {
		case kSpellEnchantWeapon: use = "Вы подготовили дополнительные компоненты для зачарования.\r\n";
			missing = "Вы были уверены что положили его в этот карман.\r\n";
			exhausted = "$o вспыхнул голубоватым светом, когда его вставили в предмет.\r\n";
			break;

		default: log("WARNING: wrong spellnum %d in %s:%d", spellnum, __FILE__, __LINE__);
			return false;
	}
	ObjData *tobj = GET_EQ(caster, WEAR_HOLD);
	if (!tobj) {
		act(missing, false, caster, nullptr, caster, kToChar);
		return (true);
	}
	tobj->dec_val(2);
	act(use, false, caster, tobj, nullptr, kToChar);
	if (GET_OBJ_VAL(tobj, 2) < 1) {
		act(exhausted, false, caster, tobj, nullptr, kToChar);
		obj_from_char(tobj);
		extract_obj(tobj);
	}
	return (false);
}

int mag_affects(int level, CharData *ch, CharData *victim, int spellnum, ESaving savetype) {
	bool accum_affect = false, accum_duration = false, success = true;
	bool update_spell = false;
	const char *to_vict = nullptr, *to_room = nullptr;
	int i, modi = 0;
	int rnd = 0;
	int decline_mod = 0;
	if (victim == nullptr
		|| IN_ROOM(victim) == kNowhere
		|| ch == nullptr) {
		return 0;
	}

	// Calculate PKILL's affects
	//   1) "NPC affect PC spellflag"  for victim
	//   2) "NPC affect NPC spellflag" if victim cann't pkill FICHTING(victim)
	if (ch != victim)    //send_to_char("Start\r\n",ch);
	{
		//send_to_char("Start\r\n",victim);
		if (IS_SET(SpINFO.routines, kNpcAffectPc))    //send_to_char("1\r\n",ch);
		{
			//send_to_char("1\r\n",victim);
			if (!pk_agro_action(ch, victim))
				return 0;
		} else if (IS_SET(SpINFO.routines, kNpcAffectNpc) && victim->get_fighting())    //send_to_char("2\r\n",ch);
		{
			//send_to_char("2\r\n",victim);
			if (!pk_agro_action(ch, victim->get_fighting()))
				return 0;
		}
		//send_to_char("Stop\r\n",ch);
		//send_to_char("Stop\r\n",victim);
	}
	// Magic glass
	if (!IS_SET(SpINFO.routines, kMagWarcry)) {
		if (ch != victim
			&& SpINFO.violent
			&& ((!IS_GOD(ch)
				&& AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICGLASS)
				&& (ch->in_room == IN_ROOM(victim)) //зеркало сработает только если оба в одной комнате
				&& number(1, 100) < (GetRealLevel(victim) / 3))
				|| (IS_GOD(victim)
					&& (IS_NPC(ch)
						|| GetRealLevel(victim) > (GetRealLevel(ch)))))) {
			act("Магическое зеркало $N1 отразило вашу магию!", false, ch, nullptr, victim, kToChar);
			act("Магическое зеркало $N1 отразило магию $n1!", false, ch, nullptr, victim, kToNotVict);
			act("Ваше магическое зеркало отразило поражение $n1!", false, ch, nullptr, victim, kToVict);
			mag_affects(level, ch, ch, spellnum, savetype);
			return 0;
		}
	} else {
		if (ch != victim && SpINFO.violent && IS_GOD(victim)
			&& (IS_NPC(ch) || GetRealLevel(victim) > (GetRealLevel(ch) + GET_REAL_REMORT(ch) / 2))) {
			act("Звуковой барьер $N1 отразил ваш крик!", false, ch, nullptr, victim, kToChar);
			act("Звуковой барьер $N1 отразил крик $n1!", false, ch, nullptr, victim, kToNotVict);
			act("Ваш звуковой барьер отразил крик $n1!", false, ch, nullptr, victim, kToVict);
			mag_affects(level, ch, ch, spellnum, savetype);
			return 0;
		}
	}
	//  блочим директ аффекты вредных спелов для Витязей  шанс = (скил/20 + вес.щита/2)  (Кудояр)
	if (ch != victim && SpINFO.violent && !IS_SET(SpINFO.routines, kMagWarcry) && !IS_SET(SpINFO.routines, kMagMasses) && !IS_SET(SpINFO.routines, kMagAreas)
	&& (victim->get_skill(ESkill::kShieldBlock) > 100) && GET_EQ(victim, WEAR_SHIELD) && can_use_feat(victim, MAGICAL_SHIELD_FEAT)
			&& ( number(1, 100) < ((victim->get_skill(ESkill::kShieldBlock))/20 + GET_OBJ_WEIGHT(GET_EQ(victim, WEAR_SHIELD))/2)))
	{
		act("Ваши чары повисли на щите $N1, и затем развеялись.", false, ch, nullptr, victim, kToChar);
		act("Щит $N1 поглотил злые чары $n1.", false, ch, nullptr, victim, kToNotVict);
		act("Ваш щит уберег вас от злых чар $n1.", false, ch, nullptr, victim, kToVict);
		return (0);
	}
	
		
	if (!IS_SET(SpINFO.routines, kMagWarcry) && ch != victim && SpINFO.violent
		&& number(1, 999) <= GET_AR(victim) * 10) {
		send_to_char(NOEFFECT, ch);
		return 0;
	}

	Affect<EApplyLocation> af[kMaxSpellAffects];
	for (i = 0; i < kMaxSpellAffects; i++) {
		af[i].type = spellnum;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].location = APPLY_NONE;
	}

	// decrease modi for failing, increese fo success
	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		if (can_use_feat(ch, RELATED_TO_MAGIC_FEAT) && !IS_NPC(victim)) {
			modi -= 80; //бонуса на непись нету
		}
		if (can_use_feat(ch, MAGICAL_INSTINCT_FEAT) && !IS_NPC(victim)) {
			modi -= 30; //бонуса на непись нету
		}

	}

	if (PRF_FLAGGED(ch, PRF_AWAKE) && !IS_NPC(victim)) {
		modi = modi - 50;
	}

//  log("[MAG Affect] Modifier value for %s (caster %s) = %d(spell %d)",
//      GET_NAME(victim), GET_NAME(ch), modi, spellnum);

	const int koef_duration = func_koef_duration(spellnum, ch->get_skill(GetMagicSkillId(spellnum)));
	const int koef_modifier = func_koef_modif(spellnum, ch->get_skill(GetMagicSkillId(spellnum)));

	switch (spellnum) {
		case kSpellChillTouch: savetype = ESaving::kStability;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].location = APPLY_STR;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 2, level, 4, 6, 0)) * koef_duration;
			af[0].modifier = -1 - GET_REAL_REMORT(ch) / 2;
			af[0].battleflag = kAfBattledec;
			accum_duration = true;
			to_room = "Боевой пыл $n1 несколько остыл.";
			to_vict = "Вы почувствовали себя слабее!";
			break;

		case kSpellEnergyDrain:
		case kSpellWeaknes: savetype = ESaving::kWill;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			if (affected_by_spell(victim, kSpellStrength)) {
				affect_from_char(victim, kSpellStrength);
				success = false;
				break;
			}
			if (affected_by_spell(victim, kSpellDexterity)) {
				affect_from_char(victim, kSpellDexterity);
				success = false;
				break;
			}
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 4, level, 5, 4, 0)) * koef_duration;
			af[0].location = APPLY_STR;
			if (spellnum == kSpellWeaknes)
				af[0].modifier = -1 * ((level / 6 + GET_REAL_REMORT(ch) / 2));
			else
				af[0].modifier = -2 * ((level / 6 + GET_REAL_REMORT(ch) / 2));
			if (IS_NPC(ch) && level >= (kLvlImmortal))
				af[0].modifier += (kLvlImmortal - level - 1);    //1 str per mob level above 30
			af[0].battleflag = kAfBattledec;
			accum_duration = true;
			to_room = "$n стал$g немного слабее.";
			to_vict = "Вы почувствовали себя слабее!";
			spellnum = kSpellWeaknes;
			break;
		case kSpellStoneWall:
		case kSpellStoneSkin: af[0].location = APPLY_ABSORBE;
			af[0].modifier = (level * 2 + 1) / 3;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "Кожа $n1 покрылась каменными пластинами.";
			to_vict = "Вы стали менее чувствительны к ударам.";
			spellnum = kSpellStoneSkin;
			break;

		case kSpellGeneralRecovery:
		case kSpellFastRegeneration: af[0].location = APPLY_HITREG;
			af[0].modifier = 50 + GET_REAL_REMORT(ch);
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[1].location = APPLY_MOVEREG;
			af[1].modifier = 50 + GET_REAL_REMORT(ch);
			af[1].duration = af[0].duration;
			accum_duration = true;
			to_room = "$n расцвел$g на ваших глазах.";
			to_vict = "Вас наполнила живительная сила.";
			spellnum = kSpellFastRegeneration;
			break;

		case kSpellAirShield:
			if (affected_by_spell(victim, kSpellIceShield))
				affect_from_char(victim, kSpellIceShield);
			if (affected_by_spell(victim, kSpellFireShield))
				affect_from_char(victim, kSpellFireShield);
			af[0].bitvector = to_underlying(EAffectFlag::AFF_AIRSHIELD);
			af[0].battleflag = kAfBattledec;
			if (IS_NPC(victim) || victim == ch)
				af[0].duration = CalcDuration(victim, 10 + GET_REAL_REMORT(ch), 0, 0, 0, 0) * koef_duration;
			else
				af[0].duration = CalcDuration(victim, 4 + GET_REAL_REMORT(ch), 0, 0, 0, 0) * koef_duration;
			to_room = "$n3 окутал воздушный щит.";
			to_vict = "Вас окутал воздушный щит.";
			break;

		case kSpellFireShield:
			if (affected_by_spell(victim, kSpellIceShield))
				affect_from_char(victim, kSpellIceShield);
			if (affected_by_spell(victim, kSpellAirShield))
				affect_from_char(victim, kSpellAirShield);
			af[0].bitvector = to_underlying(EAffectFlag::AFF_FIRESHIELD);
			af[0].battleflag = kAfBattledec;
			if (IS_NPC(victim) || victim == ch)
				af[0].duration = CalcDuration(victim, 10 + GET_REAL_REMORT(ch), 0, 0, 0, 0) * koef_duration;
			else
				af[0].duration = CalcDuration(victim, 4 + GET_REAL_REMORT(ch), 0, 0, 0, 0) * koef_duration;
			to_room = "$n3 окутал огненный щит.";
			to_vict = "Вас окутал огненный щит.";
			break;

		case kSpellIceShield:
			if (affected_by_spell(victim, kSpellFireShield))
				affect_from_char(victim, kSpellFireShield);
			if (affected_by_spell(victim, kSpellAirShield))
				affect_from_char(victim, kSpellAirShield);
			af[0].bitvector = to_underlying(EAffectFlag::AFF_ICESHIELD);
			af[0].battleflag = kAfBattledec;
			if (IS_NPC(victim) || victim == ch)
				af[0].duration = CalcDuration(victim, 10 + GET_REAL_REMORT(ch), 0, 0, 0, 0) * koef_duration;
			else
				af[0].duration = CalcDuration(victim, 4 + GET_REAL_REMORT(ch), 0, 0, 0, 0) * koef_duration;
			to_room = "$n3 окутал ледяной щит.";
			to_vict = "Вас окутал ледяной щит.";
			break;

		case kSpellAirAura: af[0].location = APPLY_RESIST_AIR;
			af[0].modifier = level;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_AIRAURA);
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "$n3 окружила воздушная аура.";
			to_vict = "Вас окружила воздушная аура.";
			break;

		case kSpellEarthAura: af[0].location = APPLY_RESIST_EARTH;
			af[0].modifier = level;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_EARTHAURA);
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "$n глубоко поклонил$u земле.";
			to_vict = "Глубокий поклон тебе, матушка земля.";
			break;

		case kSpellFireAura: af[0].location = APPLY_RESIST_WATER;
			af[0].modifier = level;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_FIREAURA);
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "$n3 окружила огненная аура.";
			to_vict = "Вас окружила огненная аура.";
			break;

		case kSpellIceAura: af[0].location = APPLY_RESIST_FIRE;
			af[0].modifier = level;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_ICEAURA);
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "$n3 окружила ледяная аура.";
			to_vict = "Вас окружила ледяная аура.";
			break;

		case kSpellGroupCloudly:
		case kSpellCloudly: af[0].location = APPLY_AC;
			af[0].modifier = -20;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "Очертания $n1 расплылись и стали менее отчетливыми.";
			to_vict = "Ваше тело стало прозрачным, как туман.";
			spellnum = kSpellCloudly;
			break;

		case kSpellGroupArmor:
		case kSpellArmor: af[0].location = APPLY_AC;
			af[0].modifier = -20;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[1].location = APPLY_SAVING_REFLEX;
			af[1].modifier = -5;
			af[1].duration = af[0].duration;
			af[2].location = APPLY_SAVING_STABILITY;
			af[2].modifier = -5;
			af[2].duration = af[0].duration;
			accum_duration = true;
			to_room = "Вокруг $n1 вспыхнул белый щит и тут же погас.";
			to_vict = "Вы почувствовали вокруг себя невидимую защиту.";
			spellnum = kSpellArmor;
			break;

		case kSpellFascination:
			if (material_component_processing(ch, victim, spellnum)) {
				success = false;
				break;
			}
			af[0].location = APPLY_CHA;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			if (ch == victim)
				af[0].modifier = (level + 9) / 10 + 0.7*koef_modifier;
			else
				af[0].modifier = (level + 14) / 15 + 0.7*koef_modifier;
			accum_duration = true;
			accum_affect = true;
			to_room = "$n0 достал$g из маленькой сумочки какие-то вонючие порошки и отвернул$u, бормоча под нос \r\n\"..так это на ресницы надо, кажется... Эх, только бы не перепутать...\" \r\n";
			to_vict = "Вы попытались вспомнить уроки старой цыганки, что учила вас людям головы морочить.\r\nХотя вы ее не очень то слушали.\r\n";
			spellnum = kSpellFascination;
			break;


		case kSpellGroupBless:
		case kSpellBless: af[0].location = APPLY_SAVING_STABILITY;
			af[0].modifier = -5 - GET_REAL_REMORT(ch) / 3;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_BLESS);
			af[1].location = APPLY_SAVING_WILL;
			af[1].modifier = -5 - GET_REAL_REMORT(ch) / 4;
			af[1].duration = af[0].duration;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_BLESS);
			to_room = "$n осветил$u на миг неземным светом.";
			to_vict = "Боги одарили вас своей улыбкой.";
			spellnum = kSpellBless;
			break;

		case kSpellCallLighting:
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
			}
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -RollDices(1 + level / 8 + GET_REAL_REMORT(ch) / 4, 4);
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 2, level + 7, 8, 0, 0)) * koef_duration;
			af[1].location = APPLY_CAST_SUCCESS;
			af[1].modifier = -RollDices(1 + level / 4 + GET_REAL_REMORT(ch) / 2, 4);
			af[1].duration = af[0].duration;
			spellnum = kSpellMagicBattle;
			to_room = "$n зашатал$u, пытаясь прийти в себя от взрыва шаровой молнии.";
			to_vict = "Взрыв шаровой молнии $N1 отдался в вашей голове громким звоном.";
			break;

		case kSpellColdWind:
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
			}
			af[0].location = APPLY_DEX;
			af[0].modifier = -RollDices(int(MAX(1, ((level - 14) / 7))), 3);
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 9, 0, 0, 0, 0)) * koef_duration;
			to_vict = "Вы покрылись серебристым инеем.";
			to_room = "$n покрыл$u красивым серебристым инеем.";
			break;
		case kSpellGroupAwareness:
		case kSpellAwareness:
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_AWARNESS);
			af[1].location = APPLY_SAVING_REFLEX;
			af[1].modifier = -1 - GET_REAL_REMORT(ch) / 4;
			af[1].duration = af[0].duration;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_AWARNESS);
			to_room = "$n начал$g внимательно осматриваться по сторонам.";
			to_vict = "Вы стали более внимательны к окружающему.";
			spellnum = kSpellAwareness;
			break;

		case kSpellGodsShield: af[0].duration = CalcDuration(victim, 4, 0, 0, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_SHIELD);
			af[0].location = APPLY_SAVING_STABILITY;
			af[0].modifier = -10;
			af[0].battleflag = kAfBattledec;
			af[1].duration = af[0].duration;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_SHIELD);
			af[1].location = APPLY_SAVING_WILL;
			af[1].modifier = -10;
			af[1].battleflag = kAfBattledec;
			af[2].duration = af[0].duration;
			af[2].bitvector = to_underlying(EAffectFlag::AFF_SHIELD);
			af[2].location = APPLY_SAVING_REFLEX;
			af[2].modifier = -10;
			af[2].battleflag = kAfBattledec;

			to_room = "$n покрыл$u сверкающим коконом.";
			to_vict = "Вас покрыл голубой кокон.";
			break;

		case kSpellGroupHaste:
		case kSpellHaste:
			if (affected_by_spell(victim, kSpellSlowdown)) {
				affect_from_char(victim, kSpellSlowdown);
				success = false;
				break;
			}
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_HASTE);
			af[0].location = APPLY_SAVING_REFLEX;
			af[0].modifier = -1 - GET_REAL_REMORT(ch) / 5;
			to_vict = "Вы начали двигаться быстрее.";
			to_room = "$n начал$g двигаться заметно быстрее.";
			spellnum = kSpellHaste;
			break;

		case kSpellShadowCloak: af[0].bitvector = to_underlying(EAffectFlag::AFF_SHADOW_CLOAK);
			af[0].location = APPLY_SAVING_STABILITY;
			af[0].modifier = -(GetRealLevel(ch) / 3 + GET_REAL_REMORT(ch)) / 4;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "$n скрыл$u в густой тени.";
			to_vict = "Густые тени окутали вас.";
			break;

		case kSpellEnlarge:
			if (affected_by_spell(victim, kSpellLessening)) {
				affect_from_char(victim, kSpellLessening);
				success = false;
				break;
			}
			af[0].location = APPLY_SIZE;
			af[0].modifier = 5 + level / 2 + GET_REAL_REMORT(ch) / 3;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "$n начал$g расти, как на дрожжах.";
			to_vict = "Вы стали крупнее.";
			break;

		case kSpellLessening:
			if (affected_by_spell(victim, kSpellEnlarge)) {
				affect_from_char(victim, kSpellEnlarge);
				success = false;
				break;
			}
			af[0].location = APPLY_SIZE;
			af[0].modifier = -(5 + level / 3);
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "$n скукожил$u.";
			to_vict = "Вы стали мельче.";
			break;

		case kSpellMagicGlass:
		case kSpellGroupMagicGlass: af[0].bitvector = to_underlying(EAffectFlag::AFF_MAGICGLASS);
			af[0].duration = CalcDuration(victim, 10, GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "$n3 покрыла зеркальная пелена.";
			to_vict = "Вас покрыло зеркало магии.";
			spellnum = kSpellMagicGlass;
			break;

		case kSpellCloudOfArrows: af[0].duration =
									  CalcDuration(victim, 10, GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_CLOUD_OF_ARROWS);
			af[0].location = APPLY_HITROLL;
			af[0].modifier = level / 6;
			accum_duration = true;
			to_room = "$n3 окружило облако летающих огненных стрел.";
			to_vict = "Вас окружило облако летающих огненных стрел.";
			break;

		case kSpellStoneHands: af[0].bitvector = to_underlying(EAffectFlag::AFF_STONEHAND);
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "Руки $n1 задубели.";
			to_vict = "Ваши руки задубели.";
			break;

		case kSpellGroupPrismaticAura:
		case kSpellPrismaticAura:
			if (!IS_NPC(ch) && !same_group(ch, victim)) {
				send_to_char("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			if (affected_by_spell(victim, kSpellSanctuary)) {
				affect_from_char(victim, kSpellSanctuary);
				success = false;
				break;
			}
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_SANCTUARY)) {
				success = false;
				break;
			}
			af[0].bitvector = to_underlying(EAffectFlag::AFF_PRISMATICAURA);
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
			to_room = "$n3 покрыла призматическая аура.";
			to_vict = "Вас покрыла призматическая аура.";
			spellnum = kSpellPrismaticAura;
			break;

		case kSpellMindless:
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].location = APPLY_MANAREG;
			af[0].modifier = -50;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim,
													  0,
													  GET_REAL_WIS(ch) + GET_REAL_INT(ch),
													  10,
													  0,
													  0))
				* koef_duration;
			af[1].location = APPLY_CAST_SUCCESS;
			af[1].modifier = -50;
			af[1].duration = af[0].duration;
			af[2].location = APPLY_HITROLL;
			af[2].modifier = -5;
			af[2].duration = af[0].duration;

			to_room = "$n0 стал$g слаб$g на голову!";
			to_vict = "Ваш разум помутился!";
			break;

		case kSpellDustStorm:
		case kSpellShineFlash:
		case kSpellMassBlindness:
		case kSpellPowerBlindness:
		case kSpellBlindness: savetype = ESaving::kStability;
			if (MOB_FLAGGED(victim, MOB_NOBLIND) ||
				WAITLESS(victim) ||
				((ch != victim) &&
					!GET_GOD_FLAG(victim, GF_GODSCURSE) && CalcGeneralSaving(ch, victim, savetype, modi))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			switch (spellnum) {
				case kSpellDustStorm:
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 3, level, 6, 0, 0)) * koef_duration;
					break;
				case kSpellShineFlash:
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 2, level + 7, 8, 0, 0))
						* koef_duration;
					break;
				case kSpellMassBlindness:
				case kSpellBlindness:
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 2, level, 8, 0, 0)) * koef_duration;
					break;
				case kSpellPowerBlindness:
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 3, level, 6, 0, 0)) * koef_duration;
					break;
			}
			af[0].bitvector = to_underlying(EAffectFlag::AFF_BLIND);
			af[0].battleflag = kAfBattledec;
			to_room = "$n0 ослеп$q!";
			to_vict = "Вы ослепли!";
			spellnum = kSpellBlindness;
			break;

		case kSpellMadness: savetype = ESaving::kWill;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 3, 0, 0, 0, 0)) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			af[1].location = APPLY_MADNESS;
			af[1].duration = af[0].duration;
			af[1].modifier = level;
			to_room = "Теперь $n не сможет сбежать из боя!";
			to_vict = "Вас обуяло безумие!";
			break;

		case kSpellWeb: savetype = ESaving::kReflex;
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_BROKEN_CHAINS)
				|| (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2 - GET_REAL_REMORT(ch) / 5;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 3, level, 6, 0, 0)) * koef_duration;
			af[0].battleflag = kAfBattledec;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			af[1].location = APPLY_AC;
			af[1].modifier = 20;
			af[1].duration = af[0].duration;
			af[1].battleflag = kAfBattledec;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			to_room = "$n3 покрыла невидимая паутина, сковывая $s движения!";
			to_vict = "Вас покрыла невидимая паутина!";
			break;

		case kSpellMassCurse:
		case kSpellCurse: savetype = ESaving::kWill;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			// если есть фит порча
			if (can_use_feat(ch, DECLINE_FEAT))
				decline_mod += GET_REAL_REMORT(ch);
			af[0].location = APPLY_INITIATIVE;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 1, level, 2, 0, 0)) * koef_duration;
			af[0].modifier = -(5 + decline_mod);
			af[0].bitvector = to_underlying(EAffectFlag::AFF_CURSE);

			af[1].location = APPLY_HITROLL;
			af[1].duration = af[0].duration;
			af[1].modifier = -(level / 6 + decline_mod + GET_REAL_REMORT(ch) / 5);
			af[1].bitvector = to_underlying(EAffectFlag::AFF_CURSE);

			if (level >= 20) {
				af[2].location = APPLY_CAST_SUCCESS;
				af[2].duration = af[0].duration;
				af[2].modifier = -(level / 3 + GET_REAL_REMORT(ch));
				if (IS_NPC(ch) && level >= (kLvlImmortal))
					af[2].modifier += (kLvlImmortal - level - 1);    //1 cast per mob level above 30
				af[2].bitvector = to_underlying(EAffectFlag::AFF_CURSE);
			}
			accum_duration = true;
			accum_affect = true;
			to_room = "Красное сияние вспыхнуло над $n4 и тут же погасло!";
			to_vict = "Боги сурово поглядели на вас.";
			spellnum = kSpellCurse;
			break;

		case kSpellMassSlow:
		case kSpellSlowdown: savetype = ESaving::kStability;
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_BROKEN_CHAINS)
				|| (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi * number(1, koef_modifier / 2)))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			if (affected_by_spell(victim, kSpellHaste)) {
				affect_from_char(victim, kSpellHaste);
				success = false;
				break;
			}

			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 9, 0, 0, 0, 0)) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_SLOW);
			af[1].duration =
				ApplyResist(victim, GetResistType(spellnum), CalcDuration(victim, 9, 0, 0, 0, 0))
					* koef_duration;
			af[1].location = APPLY_DEX;
			af[1].modifier = -koef_modifier;
			to_room = "Движения $n1 заметно замедлились.";
			to_vict = "Ваши движения заметно замедлились.";
			spellnum = kSpellSlowdown;
			break;

		case kSpellGroupSincerity:
		case kSpellDetectAlign:
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_DETECT_ALIGN);
			accum_duration = true;
			to_vict = "Ваши глаза приобрели зеленый оттенок.";
			to_room = "Глаза $n1 приобрели зеленый оттенок.";
			spellnum = kSpellDetectAlign;
			break;

		case kSpellAllSeeingEye:
		case kSpellDetectInvis:
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_DETECT_INVIS);
			accum_duration = true;
			to_vict = "Ваши глаза приобрели золотистый оттенок.";
			to_room = "Глаза $n1 приобрели золотистый оттенок.";
			spellnum = kSpellDetectInvis;
			break;

		case kSpellMagicalGaze:
		case kSpellDetectMagic:
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_DETECT_MAGIC);
			accum_duration = true;
			to_vict = "Ваши глаза приобрели желтый оттенок.";
			to_room = "Глаза $n1 приобрели желтый оттенок.";
			spellnum = kSpellDetectMagic;
			break;

		case kSpellSightOfDarkness:
		case kSpellInfravision:
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_INFRAVISION);
			accum_duration = true;
			to_vict = "Ваши глаза приобрели красный оттенок.";
			to_room = "Глаза $n1 приобрели красный оттенок.";
			spellnum = kSpellInfravision;
			break;

		case kSpellSnakeEyes:
		case kSpellDetectPoison:
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_DETECT_POISON);
			accum_duration = true;
			to_vict = "Ваши глаза приобрели карий оттенок.";
			to_room = "Глаза $n1 приобрели карий оттенок.";
			spellnum = kSpellDetectPoison;
			break;

		case kSpellGroupInvisible:
		case kSpellInvisible:
			if (!victim)
				victim = ch;
			if (affected_by_spell(victim, kSpellGlitterDust)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].modifier = -40;
			af[0].location = APPLY_AC;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_INVISIBLE);
			accum_duration = true;
			to_vict = "Вы стали невидимы для окружающих.";
			to_room = "$n медленно растворил$u в пустоте.";
			spellnum = kSpellInvisible;
			break;

		case kSpellFever: savetype = ESaving::kStability;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].location = APPLY_HITREG;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 0, level, 2, 0, 0)) * koef_duration;
			af[0].modifier = -95;
			af[1].location = APPLY_MANAREG;
			af[1].duration = af[0].duration;
			af[1].modifier = -95;
			af[2].location = APPLY_MOVEREG;
			af[2].duration = af[0].duration;
			af[2].modifier = -95;
			af[3].location = APPLY_PLAQUE;
			af[3].duration = af[0].duration;
			af[3].modifier = level;
			af[4].location = APPLY_WIS;
			af[4].duration = af[0].duration;
			af[4].modifier = -GET_REAL_REMORT(ch) / 5;
			af[5].location = APPLY_INT;
			af[5].duration = af[0].duration;
			af[5].modifier = -GET_REAL_REMORT(ch) / 5;
			af[6].location = APPLY_DEX;
			af[6].duration = af[0].duration;
			af[6].modifier = -GET_REAL_REMORT(ch) / 5;
			af[7].location = APPLY_STR;
			af[7].duration = af[0].duration;
			af[7].modifier = -GET_REAL_REMORT(ch) / 5;
			to_vict = "Вас скрутило в жестокой лихорадке.";
			to_room = "$n3 скрутило в жестокой лихорадке.";
			break;

		case kSpellPoison: savetype = ESaving::kCritical;
			if (ch != victim && (AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD) ||
				CalcGeneralSaving(ch, victim, savetype, modi - GET_REAL_CON(victim) / 2))) {
				if (ch->in_room
					== IN_ROOM(victim)) // Добавлено чтобы яд нанесенный SPELL_POISONED_FOG не спамил чару постоянно
					send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].location = APPLY_STR;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 0, level, 1, 0, 0)) * koef_duration;
			af[0].modifier = -2;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_POISON);
			af[0].battleflag = kAfSameTime;

			af[1].location = APPLY_POISON;
			af[1].duration = af[0].duration;
			af[1].modifier = level + GET_REAL_REMORT(ch) / 2;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_POISON);
			af[1].battleflag = kAfSameTime;

			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";

			break;

		case kSpellProtectFromEvil:
		case kSpellGroupProtectFromEvil:
			if (!IS_NPC(ch) && !same_group(ch, victim)) {
				send_to_char("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			af[0].location = APPLY_RESIST_DARK;
			if (spellnum == kSpellProtectFromEvil) {
				affect_from_char(ch, kSpellGroupProtectFromEvil);
				af[0].modifier = 5;
			} else {
				affect_from_char(ch, kSpellProtectFromEvil);
				af[0].modifier = level;
			}
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_PROTECT_EVIL);
			accum_duration = true;
			to_vict = "Вы подавили в себе страх к тьме.";
			to_room = "$n подавил$g в себе страх к тьме.";
			break;

		case kSpellGroupSanctuary:
		case kSpellSanctuary:
			if (!IS_NPC(ch) && !same_group(ch, victim)) {
				send_to_char("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			if (affected_by_spell(victim, kSpellPrismaticAura)) {
				affect_from_char(victim, kSpellPrismaticAura);
				success = false;
				break;
			}
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_PRISMATICAURA)) {
				success = false;
				break;
			}

			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_SANCTUARY);
			to_vict = "Белая аура мгновенно окружила вас.";
			to_room = "Белая аура покрыла $n3 с головы до пят.";
			spellnum = kSpellSanctuary;
			break;

		case kSpellSleep: savetype = ESaving::kWill;
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_HOLD) || MOB_FLAGGED(victim, MOB_NOSLEEP)
				|| (ch != victim && CalcGeneralSaving(ch, victim, ESaving::kWill, modi))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			};

			if (victim->get_fighting())
				stop_fighting(victim, false);
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 1, level, 6, 1, 6)) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_SLEEP);
			af[0].battleflag = kAfBattledec;
			if (GET_POS(victim) > EPosition::kSleep && success) {
				// add by Pereplut
				if (victim->ahorse())
					victim->drop_from_horse();
				send_to_char("Вы слишком устали... Спать... Спа...\r\n", victim);
				act("$n прилег$q подремать.", true, victim, nullptr, nullptr, kToRoom | kToArenaListen);

				GET_POS(victim) = EPosition::kSleep;
			}
			break;

		case kSpellGroupStrength:
		case kSpellStrength:
			if (affected_by_spell(victim, kSpellWeaknes)) {
				affect_from_char(victim, kSpellWeaknes);
				success = false;
				break;
			}
			af[0].location = APPLY_STR;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			if (ch == victim)
				af[0].modifier = (level + 9) / 10 + koef_modifier + GET_REAL_REMORT(ch) / 5;
			else
				af[0].modifier = (level + 14) / 15 + koef_modifier + GET_REAL_REMORT(ch) / 5;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Вы почувствовали себя сильнее.";
			to_room = "Мышцы $n1 налились силой.";
			spellnum = kSpellStrength;
			break;

		case kSpellDexterity:
			if (affected_by_spell(victim, kSpellWeaknes)) {
				affect_from_char(victim, kSpellWeaknes);
				success = false;
				break;
			}
			af[0].location = APPLY_DEX;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			if (ch == victim)
				af[0].modifier = (level + 9) / 10 + koef_modifier + GET_REAL_REMORT(ch) / 5;
			else
				af[0].modifier = (level + 14) / 15 + koef_modifier + GET_REAL_REMORT(ch) / 5;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Вы почувствовали себя более шустрым.";
			to_room = "$n0 будет двигаться более шустро.";
			spellnum = kSpellDexterity;
			break;

		case kSpellPatronage: af[0].location = APPLY_HIT;
			af[0].duration = CalcDuration(victim, 3, level, 10, 0, 0) * koef_duration;
			af[0].modifier = GetRealLevel(ch) * 2 + GET_REAL_REMORT(ch);
			if (GET_ALIGNMENT(victim) >= 0) {
				to_vict = "Исходящий с небес свет на мгновение озарил вас.";
				to_room = "Исходящий с небес свет на мгновение озарил $n3.";
			} else {
				to_vict = "Вас окутало клубящееся облако Тьмы.";
				to_room = "Клубящееся темное облако на мгновение окутало $n3.";
			}
			break;

		case kSpellEyeOfGods:
		case kSpellSenseLife: to_vict = "Вы способны разглядеть даже микроба.";
			to_room = "$n0 начал$g замечать любые движения.";
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_SENSE_LIFE);
			accum_duration = true;
			spellnum = kSpellSenseLife;
			break;

		case kSpellWaterwalk:
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_WATERWALK);
			accum_duration = true;
			to_vict = "На рыбалку вы можете отправляться без лодки.";
			break;

		case kSpellBreathingAtDepth:
		case kSpellWaterbreath:
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_WATERBREATH);
			accum_duration = true;
			to_vict = "У вас выросли жабры.";
			to_room = "У $n1 выросли жабры.";
			spellnum = kSpellWaterbreath;
			break;

		case kSpellHolystrike:
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_EVILESS)) {
				// все решится в дамадже части спелла
				success = false;
				break;
			}
			// тут break не нужен

			// fall through
		case kSpellMassHold:
		case kSpellPowerHold:
		case kSpellHold:
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_SLEEP)
				|| MOB_FLAGGED(victim, MOB_NOHOLD) || AFF_FLAGGED(victim, EAffectFlag::AFF_BROKEN_CHAINS)
				|| (ch != victim && CalcGeneralSaving(ch, victim, ESaving::kWill, modi))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 spellnum == kSpellPowerHold ? CalcDuration(victim,
																					2,
																					level + 7,
																					8,
																					2,
																					5)
																	 : CalcDuration(victim,
																					1,
																					level + 9,
																					10,
																					1,
																					3)) * koef_duration;

			af[0].bitvector = to_underlying(EAffectFlag::AFF_HOLD);
			af[0].battleflag = kAfBattledec;
			to_room = "$n0 замер$q на месте!";
			to_vict = "Вы замерли на месте, не в силах пошевельнуться.";
			spellnum = kSpellHold;
			break;

		case kSpellWarcryOfRage:
		case kSpellSonicWave:
		case kSpellMassDeafness:
		case kSpellPowerDeafness:
		case kSpellDeafness:
			switch (spellnum) {
				case kSpellWarcryOfRage: savetype = ESaving::kWill;
					modi = GET_REAL_CON(ch);
					break;
				case kSpellSonicWave:
				case kSpellMassDeafness:
				case kSpellPowerDeafness:
				case kSpellDeafness: savetype = ESaving::kStability;
					break;
			}
			if (  //MOB_FLAGGED(victim, MOB_NODEAFNESS) ||
				(ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			switch (spellnum) {
				case kSpellWarcryOfRage:
				case kSpellPowerDeafness:
				case kSpellSonicWave:
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 2, level + 3, 4, 6, 0))
						* koef_duration;
					break;
				case kSpellMassDeafness:
				case kSpellDeafness:
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 2, level + 7, 8, 3, 0))
						* koef_duration;
					break;
			}
			af[0].bitvector = to_underlying(EAffectFlag::AFF_DEAFNESS);
			af[0].battleflag = kAfBattledec;
			to_room = "$n0 оглох$q!";
			to_vict = "Вы оглохли.";
			spellnum = kSpellDeafness;
			break;

		case kSpellMassSilence:
		case kSpellPowerSilence:
		case kSpellSllence: savetype = ESaving::kWill;
			if (MOB_FLAGGED(victim, MOB_NOSIELENCE) ||
				(ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].duration = ApplyResist(victim, GetResistType(spellnum), spellnum == kSpellPowerSilence ?
																		  CalcDuration(victim,
																						 2,
																						 level + 3,
																						 4,
																						 6,
																						 0) : CalcDuration(
					victim,
					2,
					level + 7,
					8,
					3,
					0)) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_SILENCE);
			af[0].battleflag = kAfBattledec;
			to_room = "$n0 прикусил$g язык!";
			to_vict = "Вы не в состоянии вымолвить ни слова.";
			spellnum = kSpellSllence;
			break;

		case kSpellGroupFly:
		case kSpellFly:
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_FLY);
			to_room = "$n0 медленно поднял$u в воздух.";
			to_vict = "Вы медленно поднялись в воздух.";
			spellnum = kSpellFly;
			break;

		case kSpellBrokenChains: af[0].duration = CalcDuration(victim, 10, GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_BROKEN_CHAINS);
			af[0].battleflag = kAfBattledec;
			to_room = "Ярко-синий ореол вспыхнул вокруг $n1 и тут же угас.";
			to_vict = "Волна ярко-синего света омыла вас с головы до ног.";
			break;
		case kSpellGroupBlink:
		case kSpellBlink: af[0].location = APPLY_SPELL_BLINK;
			af[0].modifier = 5 + GET_REAL_REMORT(ch) * 2 / 3.0;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			to_room = "$n начал$g мигать.";
			to_vict = "Вы начали мигать.";
			spellnum = kSpellBlink;
			break;

		case kSpellMagicShield: af[0].location = APPLY_AC;
			af[0].modifier = -GetRealLevel(ch) * 10 / 6;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[1].location = APPLY_SAVING_REFLEX;
			af[1].modifier = -GetRealLevel(ch) / 5;
			af[1].duration = af[0].duration;
			af[2].location = APPLY_SAVING_STABILITY;
			af[2].modifier = -GetRealLevel(ch) / 5;
			af[2].duration = af[0].duration;
			accum_duration = true;
			to_room = "Сверкающий щит вспыхнул вокруг $n1 и угас.";
			to_vict = "Сверкающий щит вспыхнул вокруг вас и угас.";
			break;

		case kSpellNoflee: // "приковать противника"
		case kSpellIndriksTeeth:
		case kSpellSnare: af[0].battleflag = kAfBattledec;
			savetype = ESaving::kWill;
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_BROKEN_CHAINS)
				|| (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 3, level, 4, 4, 0)) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOTELEPORT);
			to_room = "$n0 теперь прикован$a к $N2.";
			to_vict = "Вы не сможете покинуть $N3.";
			break;

		case kSpellLight:
			if (!IS_NPC(ch) && !same_group(ch, victim)) {
				send_to_char("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_HOLYLIGHT);
			to_room = "$n0 начал$g светиться ярким светом.";
			to_vict = "Вы засветились, освещая комнату.";
			break;

		case kSpellDarkness:
			if (!IS_NPC(ch) && !same_group(ch, victim)) {
				send_to_char("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_HOLYDARK);
			to_room = "$n0 погрузил$g комнату во мрак.";
			to_vict = "Вы погрузили комнату в непроглядную тьму.";
			break;
		case kSpellVampirism: af[0].duration = CalcDuration(victim, 10, GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].location = APPLY_DAMROLL;
			af[0].modifier = 0;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_VAMPIRE);
			to_room = "Зрачки $n3 приобрели красный оттенок.";
			to_vict = "Ваши зрачки приобрели красный оттенок.";
			break;

		case kSpellEviless:
			if (!IS_NPC(victim) || victim->get_master() != ch || !MOB_FLAGGED(victim, MOB_CORPSE)) {
				//тихо уходим, т.к. заклинание массовое
				break;
			}
			af[0].duration = CalcDuration(victim, 10, GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].location = APPLY_DAMROLL;
			af[0].modifier = 15 + (GET_REAL_REMORT(ch) > 8 ? (GET_REAL_REMORT(ch) - 8) : 0);
			af[0].bitvector = to_underlying(EAffectFlag::AFF_EVILESS);
			af[1].duration = af[0].duration;
			af[1].location = APPLY_HITROLL;
			af[1].modifier = 7 + (GET_REAL_REMORT(ch) > 8 ? (GET_REAL_REMORT(ch) - 8) : 0);;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_EVILESS);
			af[2].duration = af[0].duration;
			af[2].location = APPLY_HIT;
			af[2].bitvector = to_underlying(EAffectFlag::AFF_EVILESS);

			// иначе, при рекасте, модификатор суммируется с текущим аффектом.
			if (!AFF_FLAGGED(victim, EAffectFlag::AFF_EVILESS)) {
				af[2].modifier = GET_REAL_MAX_HIT(victim);
				// не очень красивый способ передать сигнал на лечение в mag_points
				Affect<EApplyLocation> tmpaf;
				tmpaf.type = kSpellEviless;
				tmpaf.duration = 1;
				tmpaf.modifier = 0;
				tmpaf.location = EApplyLocation::APPLY_NONE;
				tmpaf.battleflag = 0;
				tmpaf.bitvector = to_underlying(EAffectFlag::AFF_EVILESS);
				affect_to_char(ch, tmpaf);
			}
			to_vict = "Черное облако покрыло вас.";
			to_room = "Черное облако покрыло $n3 с головы до пят.";
			break;

		case kSpellWarcryOfThunder:
		case kSpellIceStorm:
		case kSpellEarthfall:
		case kSpellShock: {
			switch (spellnum) {
				case kSpellWarcryOfThunder: savetype = ESaving::kWill;
					modi = GET_REAL_CON(ch) * 3 / 2;
					break;
				case kSpellIceStorm: savetype = ESaving::kReflex;
					modi = CALC_SUCCESS(modi, 30);
					break;
				case kSpellEarthfall: savetype = ESaving::kReflex;
					modi = CALC_SUCCESS(modi, 95);
					break;
				case kSpellShock: savetype = ESaving::kReflex;
					if (GET_CLASS(ch) == ECharClass::kSorcerer) {
						modi = CALC_SUCCESS(modi, 75);
					} else {
						modi = CALC_SUCCESS(modi, 25);
					}
					break;
			}
			if (WAITLESS(victim) || (!WAITLESS(ch) && CalcGeneralSaving(ch, victim, savetype, modi))) {
				success = false;
				break;
			}
			switch (spellnum) {
				case kSpellWarcryOfThunder: af[0].type = kSpellDeafness;
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 2, level + 3, 4, 6, 0)) * koef_duration;
					af[0].duration = complex_spell_modifier(ch, kSpellDeafness, GAPPLY_SPELL_EFFECT, af[0].duration);
					af[0].bitvector = to_underlying(EAffectFlag::AFF_DEAFNESS);
					af[0].battleflag = kAfBattledec;
					to_room = "$n0 оглох$q!";
					to_vict = "Вы оглохли.";

					if ((IS_NPC(victim)
						&& AFF_FLAGGED(victim, static_cast<EAffectFlag>(af[0].bitvector)))
						|| (ch != victim
							&& affected_by_spell(victim, kSpellDeafness))) {
						if (ch->in_room == IN_ROOM(victim))
							send_to_char(NOEFFECT, ch);
					} else {
						affect_join(victim, af[0], accum_duration, false, accum_affect, false);
						act(to_vict, false, victim, nullptr, ch, kToChar);
						act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen);
					}
					break;

				case kSpellIceStorm:
				case kSpellEarthfall: WAIT_STATE(victim, 2 * kPulseViolence);
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 2, 0, 0, 0, 0)) * koef_duration;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_MAGICSTOPFIGHT);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					to_room = "$n3 оглушило.";
					to_vict = "Вас оглушило.";
					spellnum = kSpellMagicBattle;
					break;

				case kSpellShock: WAIT_STATE(victim, 2 * kPulseViolence);
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 2, 0, 0, 0, 0)) * koef_duration;
					af[0].bitvector = to_underlying(EAffectFlag::AFF_MAGICSTOPFIGHT);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					to_room = "$n3 оглушило.";
					to_vict = "Вас оглушило.";
					spellnum = kSpellMagicBattle;
					mag_affects(level, ch, victim, kSpellBlindness, ESaving::kStability);
					break;
			}
			break;
		}

//Заклинание плач. Далим.
		case kSpellCrying: {
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_CRYING)
				|| (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].location = APPLY_HIT;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 4, 0, 0, 0, 0)) * koef_duration;
			af[0].modifier =
				-1 * MAX(1,
						 (MIN(29, GetRealLevel(ch)) - MIN(24, GetRealLevel(victim)) +
							 GET_REAL_REMORT(ch) / 3) * GET_MAX_HIT(victim) / 100);
			af[0].bitvector = to_underlying(EAffectFlag::AFF_CRYING);
			if (IS_NPC(victim)) {
				af[1].location = APPLY_LIKES;
				af[1].duration = ApplyResist(victim, GetResistType(spellnum),
											 CalcDuration(victim, 5, 0, 0, 0, 0));
				af[1].modifier = -1 * MAX(1, ((level + 9) / 2 + 9 - GetRealLevel(victim) / 2));
				af[1].bitvector = to_underlying(EAffectFlag::AFF_CRYING);
				af[1].battleflag = kAfBattledec;
				to_room = "$n0 издал$g протяжный стон.";
				break;
			}
			af[1].location = APPLY_CAST_SUCCESS;
			af[1].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 5, 0, 0, 0, 0));
			af[1].modifier = -1 * MAX(1, (level / 3 + GET_REAL_REMORT(ch) / 3 - GetRealLevel(victim) / 10));
			af[1].bitvector = to_underlying(EAffectFlag::AFF_CRYING);
			af[1].battleflag = kAfBattledec;
			af[2].location = APPLY_MORALE;
			af[2].duration = af[1].duration;
			af[2].modifier = -1 * MAX(1, (level / 3 + GET_REAL_REMORT(ch) / 5 - GetRealLevel(victim) / 5));
			af[2].bitvector = to_underlying(EAffectFlag::AFF_CRYING);
			af[2].battleflag = kAfBattledec;
			to_room = "$n0 издал$g протяжный стон.";
			to_vict = "Вы впали в уныние.";
			break;
		}
			//Заклинания Забвение, Бремя времени. Далим.
		case kSpellOblivion:
		case kSpellBurdenOfTime: {
			if (WAITLESS(victim) || CalcGeneralSaving(ch,
													  victim,
													  ESaving::kReflex,
													  CALC_SUCCESS(modi, (spellnum == kSpellOblivion ? 40 : 90)))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			WAIT_STATE(victim, (level / 10 + 1) * kPulseViolence);
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 3, 0, 0, 0, 0)) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_SLOW);
			af[0].battleflag = kAfBattledec;
			to_room = "Облако забвения окружило $n3.";
			to_vict = "Ваш разум помутился.";
			spellnum = kSpellOblivion;
			break;
		}

		case kSpellPeaceful: {
			if (AFF_FLAGGED(victim, EAffectFlag::AFF_PEACEFUL)
				|| (IS_NPC(victim) && !AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM)) ||
				(ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			if (victim->get_fighting()) {
				stop_fighting(victim, true);
				change_fighting(victim, true);
				WAIT_STATE(victim, 2 * kPulseViolence);
			}
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 2, 0, 0, 0, 0)) * koef_duration;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_PEACEFUL);
			to_room = "Взгляд $n1 потускнел, а сам он успокоился.";
			to_vict = "Ваша душа очистилась от зла и странно успокоилась.";
			break;
		}

		case kSpellStoneBones: {
			if (GET_MOB_VNUM(victim) < kMobSkeleton || GET_MOB_VNUM(victim) > kLastNecroMob) {
				send_to_char(NOEFFECT, ch);
				success = false;
			}
			af[0].location = APPLY_ARMOUR;
			af[0].duration = CalcDuration(victim, 100, level, 1, 0, 0) * koef_duration;
			af[0].modifier = level + 10 + GET_REAL_REMORT(ch) / 2;
			af[1].location = APPLY_SAVING_STABILITY;
			af[1].duration = af[0].duration;
			af[1].modifier = level + 10 + GET_REAL_REMORT(ch) / 2;
			accum_duration = true;
			to_vict = " ";
			to_room = "Кости $n1 обрели твердость кремня.";
			break;
		}

		case kSpellFailure:
		case kSpellMassFailure: {
			savetype = ESaving::kWill;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].location = APPLY_MORALE;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 2, level, 2, 0, 0)) * koef_duration;
			af[0].modifier = -5 - (GetRealLevel(ch) + GET_REAL_REMORT(ch)) / 2;
			af[1].location = static_cast<EApplyLocation>(number(1, 6));
			af[1].duration = af[0].duration;
			af[1].modifier = -(GetRealLevel(ch) + GET_REAL_REMORT(ch) * 3) / 15;
			to_room = "Тяжелое бурое облако сгустилось над $n4.";
			to_vict = "Тяжелые тучи сгустились над вами, и вы почувствовали, что удача покинула вас.";
			break;
		}

		case kSpellGlitterDust: {
			savetype = ESaving::kReflex;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi + 50)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}

			if (affected_by_spell(victim, kSpellInvisible)) {
				affect_from_char(victim, kSpellInvisible);
			}
			if (affected_by_spell(victim, kSpellCamouflage)) {
				affect_from_char(victim, kSpellCamouflage);
			}
			if (affected_by_spell(victim, kSpellHide)) {
				affect_from_char(victim, kSpellHide);
			}
			af[0].location = APPLY_SAVING_REFLEX;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 4, 0, 0, 0, 0)) * koef_duration;
			af[0].modifier = (GetRealLevel(ch) + GET_REAL_REMORT(ch)) / 3;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_GLITTERDUST);
			accum_duration = true;
			accum_affect = true;
			to_room = "Облако ярко блестящей пыли накрыло $n3.";
			to_vict = "Липкая блестящая пыль покрыла вас с головы до пят.";
			break;
		}

		case kSpellScream: {
			savetype = ESaving::kStability;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].bitvector = to_underlying(EAffectFlag::AFF_AFFRIGHT);
			af[0].location = APPLY_SAVING_WILL;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 2, level, 2, 0, 0)) * koef_duration;
			af[0].modifier = (2 * GetRealLevel(ch) + GET_REAL_REMORT(ch)) / 4;

			af[1].bitvector = to_underlying(EAffectFlag::AFF_AFFRIGHT);
			af[1].location = APPLY_MORALE;
			af[1].duration = af[0].duration;
			af[1].modifier = -(GetRealLevel(ch) + GET_REAL_REMORT(ch)) / 6;

			to_room = "$n0 побледнел$g и задрожал$g от страха.";
			to_vict = "Страх сжал ваше сердце ледяными когтями.";
			break;
		}

		case kSpellCatGrace: {
			af[0].location = APPLY_DEX;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			if (ch == victim)
				af[0].modifier = (level + 5) / 10;
			else
				af[0].modifier = (level + 10) / 15;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Ваши движения обрели невиданную ловкость.";
			to_room = "Движения $n1 обрели невиданную ловкость.";
			break;
		}

		case kSpellBullBody: {
			af[0].location = APPLY_CON;
			af[0].duration = CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0);
			if (ch == victim)
				af[0].modifier = (level + 5) / 10;
			else
				af[0].modifier = (level + 10) / 15;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Ваше тело налилось звериной мощью.";
			to_room = "Плечи $n1 раздались вширь, а тело налилось звериной мощью.";
			break;
		}

		case kSpellSnakeWisdom: {
			af[0].location = APPLY_WIS;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].modifier = (level + 6) / 15;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Шелест змеиной чешуи коснулся вашего сознания, и вы стали мудрее.";
			to_room = "$n спокойно и мудро посмотрел$g вокруг.";
			break;
		}

		case kSpellGimmicry: {
			af[0].location = APPLY_INT;
			af[0].duration =
				CalcDuration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REAL_REMORT(ch), 1, 0, 0) * koef_duration;
			af[0].modifier = (level + 6) / 15;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Вы почувствовали, что для вашего ума более нет преград.";
			to_room = "$n хитро прищурил$u и поглядел$g по сторонам.";
			break;
		}

		case kSpellWarcryOfMenace: {
			savetype = ESaving::kWill;
			modi = GET_REAL_CON(ch);
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				send_to_char(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].location = APPLY_MORALE;
			af[0].duration = ApplyResist(victim, GetResistType(spellnum),
										 CalcDuration(victim, 2, level + 3, 4, 6, 0)) * koef_duration;
			af[0].modifier = -RollDices((7 + level) / 8, 3);
			to_vict = "Похоже, сегодня не ваш день.";
			to_room = "Удача покинула $n3.";
			break;
		}

		case kSpellWarcryOfMadness: {
			savetype = ESaving::kStability;
			modi = GET_REAL_CON(ch) * 3 / 2;
			if (ch == victim || !CalcGeneralSaving(ch, victim, savetype, modi)) {
				af[0].location = APPLY_INT;
				af[0].duration = ApplyResist(victim, GetResistType(spellnum),
											 CalcDuration(victim, 2, level + 3, 4, 6, 0)) * koef_duration;
				af[0].modifier = -RollDices((7 + level) / 8, 2);
				to_vict = "Вы потеряли рассудок.";
				to_room = "$n0 потерял$g рассудок.";

				savetype = ESaving::kStability;
				modi = GET_REAL_CON(ch) * 2;
				if (ch == victim || !CalcGeneralSaving(ch, victim, savetype, modi)) {
					af[1].location = APPLY_CAST_SUCCESS;
					af[1].duration = af[0].duration;
					af[1].modifier = -(RollDices((2 + level) / 3, 4) + RollDices(GET_REAL_REMORT(ch) / 2, 5));

					af[2].location = APPLY_MANAREG;
					af[2].duration = af[1].duration;
					af[2].modifier = af[1].modifier;
					to_vict = "Вы обезумели.";
					to_room = "$n0 обезумел$g.";
				}
			} else {
				savetype = ESaving::kStability;
				modi = GET_REAL_CON(ch) * 2;
				if (!CalcGeneralSaving(ch, victim, savetype, modi)) {
					af[0].location = APPLY_CAST_SUCCESS;
					af[0].duration = ApplyResist(victim, GetResistType(spellnum),
												 CalcDuration(victim, 2, level + 3, 4, 6, 0)) * koef_duration;
					af[0].modifier = -(RollDices((2 + level) / 3, 4) + RollDices(GET_REAL_REMORT(ch) / 2, 5));

					af[1].location = APPLY_MANAREG;
					af[1].duration = af[0].duration;
					af[1].modifier = af[0].modifier;
					to_vict = "Вас охватила паника.";
					to_room = "$n0 начал$g сеять панику.";
				} else {
					send_to_char(NOEFFECT, ch);
					success = false;
				}
			}
			update_spell = true;
			break;
		}

		case kSpellWarcryOfLuck: {
			af[0].location = APPLY_MORALE;
			af[0].modifier = MAX(1, ch->get_skill(ESkill::kWarcry) / 20.0);
			af[0].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			to_room = nullptr;
			break;
		}

		case kSpellWatctyOfExpirence: {
			af[0].location = APPLY_PERCENT_EXP;
			af[0].modifier = MAX(1, ch->get_skill(ESkill::kWarcry) / 20.0);
			af[0].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			to_room = nullptr;
			break;
		}

		case kSpellWarcryOfPhysdamage: {
			af[0].location = APPLY_PERCENT_PHYSDAM;
			af[0].modifier = MAX(1, ch->get_skill(ESkill::kWarcry) / 20.0);
			af[0].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			to_room = nullptr;
			break;
		}

		case kSpellWarcryOfBattle: {
			af[0].location = APPLY_AC;
			af[0].modifier = -(10 + MIN(20, 2 * GET_REAL_REMORT(ch)));
			af[0].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			to_room = nullptr;
			break;
		}

		case kSpellWarcryOfDefence: {
			af[0].location = APPLY_SAVING_CRITICAL;
			af[0].modifier -= ch->get_skill(ESkill::kWarcry) / 10.0;
			af[0].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[1].location = APPLY_SAVING_REFLEX;
			af[1].modifier -= ch->get_skill(ESkill::kWarcry) / 10;
			af[1].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[2].location = APPLY_SAVING_STABILITY;
			af[2].modifier -= ch->get_skill(ESkill::kWarcry) / 10;
			af[2].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[3].location = APPLY_SAVING_WILL;
			af[3].modifier -= ch->get_skill(ESkill::kWarcry) / 10;
			af[3].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			//to_vict = nullptr;
			to_room = nullptr;
			break;
		}

		case kSpellWarcryOfPower: {
			af[0].location = APPLY_HIT;
			af[0].modifier = MIN(200, (4 * ch->get_con() + ch->get_skill(ESkill::kWarcry)) / 2);
			af[0].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			to_vict = nullptr;
			to_room = nullptr;
			break;
		}

		case kSpellWarcryOfBless: {
			af[0].location = APPLY_SAVING_STABILITY;
			af[0].modifier = -(4 * ch->get_con() + ch->get_skill(ESkill::kWarcry)) / 24;
			af[0].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[1].location = APPLY_SAVING_WILL;
			af[1].modifier = af[0].modifier;
			af[1].duration = af[0].duration;
			to_vict = nullptr;
			to_room = nullptr;
			break;
		}

		case kSpellWarcryOfCourage: {
			af[0].location = APPLY_HITROLL;
			af[0].modifier = (44 + ch->get_skill(ESkill::kWarcry)) / 45;
			af[0].duration = CalcDuration(victim, 2, ch->get_skill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[1].location = APPLY_DAMROLL;
			af[1].modifier = (29 + ch->get_skill(ESkill::kWarcry)) / 30;
			af[1].duration = af[0].duration;
			to_vict = nullptr;
			to_room = nullptr;
			break;
		}

		case kSpellAconitumPoison: af[0].location = APPLY_ACONITUM_POISON;
			af[0].duration = 7;
			af[0].modifier = level;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_POISON);
			af[0].battleflag = kAfSameTime;
			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";
			break;

		case kSpellScopolaPoison: af[0].location = APPLY_SCOPOLIA_POISON;
			af[0].duration = 7;
			af[0].modifier = 5;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_POISON) | to_underlying(EAffectFlag::AFF_SCOPOLIA_POISON);
			af[0].battleflag = kAfSameTime;
			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";
			break;

		case kSpellBelenaPoison: af[0].location = APPLY_BELENA_POISON;
			af[0].duration = 7;
			af[0].modifier = 5;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_POISON);
			af[0].battleflag = kAfSameTime;
			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";
			break;

		case kSpellDaturaPoison: af[0].location = APPLY_DATURA_POISON;
			af[0].duration = 7;
			af[0].modifier = 5;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_POISON);
			af[0].battleflag = kAfSameTime;
			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";
			break;

		case kSpellLucky: af[0].duration = CalcDuration(victim, 6, 0, 0, 0, 0);
			af[0].bitvector = to_underlying(EAffectFlag::AFF_LACKY);
			//Polud пробный обработчик аффектов
			af[0].handler.reset(new LackyAffectHandler());
			af[0].type = kSpellLucky;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = 0;
			to_room = "$n вдохновенно выпятил$g грудь.";
			to_vict = "Вы почувствовали вдохновение.";
			break;

		case kSpellArrowsFire:
		case kSpellArrowsWater:
		case kSpellArrowsEarth:
		case kSpellArrowsAir:
		case kSpellArrowsDeath: {
			//Додати обработчик
			break;
		}
		case kSpellPaladineInspiration:
			/*
         * групповой спелл, развешивающий рандомные аффекты, к сожалению
         * не может быть применен по принципа "сгенерили рандом - и применили"
         * поэтому на каждого члена группы применяется свой аффект, а кастер еще и полечить может
         * */

			if (ch == victim && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
				rnd = number(1, 5);
			else
				rnd = number(1, 4);
			af[0].type = kSpellPaladineInspiration;
			af[0].battleflag = kAfBattledec | kAfPulsedec;
			switch (rnd) {
				case 1:af[0].location = APPLY_PERCENT_PHYSDAM;
					af[0].duration = CalcDuration(victim, 5, 0, 0, 0, 0);
					af[0].modifier = GET_REAL_REMORT(ch) / 5 * 2 + GET_REAL_REMORT(ch);
					break;
				case 2:af[0].location = APPLY_CAST_SUCCESS;
					af[0].duration = CalcDuration(victim, 3, 0, 0, 0, 0);
					af[0].modifier = GET_REAL_REMORT(ch) / 5 * 2 + GET_REAL_REMORT(ch);
					break;
				case 3:af[0].location = APPLY_MANAREG;
					af[0].duration = CalcDuration(victim, 10, 0, 0, 0, 0);
					af[0].modifier = GET_REAL_REMORT(ch) / 5 * 2 + GET_REAL_REMORT(ch) * 5;
					break;
				case 4:af[0].location = APPLY_PERCENT_MAGDAM;
					af[0].duration = CalcDuration(victim, 5, 0, 0, 0, 0);
					af[0].modifier = GET_REAL_REMORT(ch) / 5 * 2 + GET_REAL_REMORT(ch);
					break;
				case 5:CallMagic(ch, ch, nullptr, nullptr, kSpellGroupHeal, GetRealLevel(ch));
					break;
				default:break;
			}
	}


	//проверка на обкаст мобов, имеющих от рождения встроенный аффкект
	//чтобы этот аффект не очистился, при спадении спелла
	if (IS_NPC(victim) && success) {
		for (i = 0; i < kMaxSpellAffects && success; ++i) {
			if (AFF_FLAGGED(&mob_proto[victim->get_rnum()], static_cast<EAffectFlag>(af[i].bitvector))) {
				if (ch->in_room == IN_ROOM(victim)) {
					send_to_char(NOEFFECT, ch);
				}
				success = false;
			}
		}
	}
	// позитивные аффекты - продлеваем, если они уже на цели
	if (!SpINFO.violent && affected_by_spell(victim, spellnum) && success) {
		update_spell = true;
	}
	// вот такой оригинальный способ запретить рекасты негативных аффектов - через флаг апдейта
	if ((ch != victim) && affected_by_spell(victim, spellnum) && success && (!update_spell)) {
		if (ch->in_room == IN_ROOM(victim))
			send_to_char(NOEFFECT, ch);
		success = false;
	}

	for (i = 0; success && i < kMaxSpellAffects; i++) {
		af[i].type = spellnum;
		if (af[i].bitvector || af[i].location != APPLY_NONE) {
			af[i].duration = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_EFFECT, af[i].duration);
			if (update_spell)
				affect_join_fspell(victim, af[i]);
			else
				affect_join(victim, af[i], accum_duration, false, accum_affect, false);
		}
		// тут мы ездим по циклу 16 раз, хотя аффектов 1-3...
//		ch->send_to_TC(true, true, true, "Applied affect type %i\r\n", af[i].type);
	}

	if (success) {
		// вот некрасиво же тут это делать...
		if (spellnum == kSpellPoison)
			victim->Poisoner = GET_ID(ch);
		if (to_vict != nullptr)
			act(to_vict, false, victim, nullptr, ch, kToChar);
		if (to_room != nullptr)
			act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen);
		return 1;
	}
	return 0;
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

// * These use act(), don't put the \r\n.
const char *mag_summon_msgs[] =
	{
		"\r\n",
		"$n сделал$g несколько изящних пассов - вы почувствовали странное дуновение!",
		"$n поднял$g труп!",
		"$N появил$U из клубов голубого дыма!",
		"$N появил$U из клубов зеленого дыма!",
		"$N появил$U из клубов красного дыма!",
		"$n сделал$g несколько изящных пассов - вас обдало порывом холодного ветра.",
		"$n сделал$g несколько изящных пассов, от чего ваши волосы встали дыбом.",
		"$n сделал$g несколько изящных пассов, обдав вас нестерпимым жаром.",
		"$n сделал$g несколько изящных пассов, вызвав у вас приступ тошноты.",
		"$n раздвоил$u!",
		"$n оживил$g труп!",
		"$n призвал$g защитника!",
		"Огненный хранитель появился из вихря пламени!"
	};

// * Keep the \r\n because these use send_to_char.
const char *mag_summon_fail_msgs[] =
	{
		"\r\n",
		"Нет такого существа в мире.\r\n",
		"Жаль, сорвалось...\r\n",
		"Ничего.\r\n",
		"Черт! Ничего не вышло.\r\n",
		"Вы не смогли сделать этого!\r\n",
		"Ваша магия провалилась.\r\n",
		"У вас нет подходящего трупа!\r\n"
	};

int mag_summons(int level, CharData *ch, ObjData *obj, int spellnum, int savetype) {
	CharData *tmp_mob, *mob = nullptr;
	ObjData *tobj, *next_obj;
	struct Follower *k;
	int pfail = 0, msg = 0, fmsg = 0, handle_corpse = false, keeper = false, cha_num = 0, modifier = 0;
	MobVnum mob_num;

	if (ch == nullptr) {
		return 0;
	}

	switch (spellnum) {
		case kSpellClone: msg = 10;
			fmsg = number(3, 5);    // Random fail message.
			mob_num = kMobDouble;
			pfail =
				50 - GET_CAST_SUCCESS(ch) - GET_REAL_REMORT(ch) * 5;    // 50% failure, should be based on something later.
			keeper = true;
			break;

		case kSpellSummonKeeper: msg = 12;
			fmsg = number(2, 6);
			mob_num = kMobKeeper;
			if (ch->get_fighting())
				pfail = 50 - GET_CAST_SUCCESS(ch) - GET_REAL_REMORT(ch);
			else
				pfail = 0;
			keeper = true;
			break;

		case kSpellSummonFirekeeper: msg = 13;
			fmsg = number(2, 6);
			mob_num = kMobFirekeeper;
			if (ch->get_fighting())
				pfail = 50 - GET_CAST_SUCCESS(ch) - GET_REAL_REMORT(ch);
			else
				pfail = 0;
			keeper = true;
			break;

		case kSpellAnimateDead:
			if (obj == nullptr || !IS_CORPSE(obj)) {
				act(mag_summon_fail_msgs[7], false, ch, nullptr, nullptr, kToChar);
				return 0;
			}
			mob_num = GET_OBJ_VAL(obj, 2);
			if (mob_num <= 0)
				mob_num = kMobSkeleton;
			else {
				const int real_mob_num = real_mobile(mob_num);
				tmp_mob = (mob_proto + real_mob_num);
				tmp_mob->set_normal_morph();
				pfail = 10 + tmp_mob->get_con() * 2
					- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - GET_REAL_REMORT(ch) * 5;

				int corpse_mob_level = GetRealLevel(mob_proto + real_mob_num);
				if (corpse_mob_level <= 5) {
					mob_num = kMobSkeleton;
				} else if (corpse_mob_level <= 10) {
					mob_num = kMobZombie;
				} else if (corpse_mob_level <= 15) {
					mob_num = kMobBonedog;
				} else if (corpse_mob_level <= 20) {
					mob_num = kMobBonedragon;
				} else if (corpse_mob_level <= 25) {
					mob_num = kMobBonespirit;
				} else if (corpse_mob_level <= 34) {
					mob_num = kMobNecrotank;
				} else {
					int rnd = number(1, 100);
					mob_num = kMobNecrodamager;
					if (rnd > 50) {
						mob_num = kMobNecrobreather;
					}
				}

				// kMobNecrocaster disabled, cant cast

				if (GetRealLevel(ch) + GET_REAL_REMORT(ch) + 4 < 15 && mob_num > kMobZombie) {
					mob_num = kMobZombie;
				} else if (GetRealLevel(ch) + GET_REAL_REMORT(ch) + 4 < 25 && mob_num > kMobBonedog) {
					mob_num = kMobBonedog;
				} else if (GetRealLevel(ch) + GET_REAL_REMORT(ch) + 4 < 32 && mob_num > kMobBonedragon) {
					mob_num = kMobBonedragon;
				}
			}

			handle_corpse = true;
			msg = number(1, 9);
			fmsg = number(2, 6);
			break;

		case kSpellResurrection:
			if (obj == nullptr || !IS_CORPSE(obj)) {
				act(mag_summon_fail_msgs[7], false, ch, nullptr, nullptr, kToChar);
				return 0;
			}
			if ((mob_num = GET_OBJ_VAL(obj, 2)) <= 0) {
				send_to_char("Вы не можете поднять труп этого монстра!\r\n", ch);
				return 0;
			}

			handle_corpse = true;
			msg = 11;
			fmsg = number(2, 6);

			tmp_mob = mob_proto + real_mobile(mob_num);
			tmp_mob->set_normal_morph();

			pfail = 10 + tmp_mob->get_con() * 2
				- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - GET_REAL_REMORT(ch) * 5;
			break;

		default: return 0;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
		send_to_char("Вы слишком зависимы, чтобы искать себе последователей!\r\n", ch);
		return 0;
	}
	// при перке помощь тьмы гораздо меньше шанс фейла
	if (!IS_IMMORTAL(ch) && number(0, 101) < pfail && savetype) {
		if (can_use_feat(ch, HELPDARK_FEAT)) {
			if (number(0, 3) == 0) {
				send_to_char(mag_summon_fail_msgs[fmsg], ch);
				if (handle_corpse)
					extract_obj(obj);
				return 0;
			}
		} else {
			send_to_char(mag_summon_fail_msgs[fmsg], ch);
			if (handle_corpse)
				extract_obj(obj);
			return 0;
		}
	}

	if (!(mob = read_mobile(-mob_num, VIRTUAL))) {
		send_to_char("Вы точно не помните, как создать данного монстра.\r\n", ch);
		return 0;
	}
	// очищаем умения у оживляемого моба
	if (spellnum == kSpellResurrection) {
		clear_char_skills(mob);
		// Меняем именование.
		sprintf(buf2, "умертвие %s %s", GET_PAD(mob, 1), GET_NAME(mob));
		mob->set_pc_name(buf2);
		sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
		mob->set_npc_name(buf2);
		mob->player_data.long_descr = "";
		sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
		mob->player_data.PNames[0] = std::string(buf2);
		sprintf(buf2, "умертвию %s", GET_PAD(mob, 1));
		mob->player_data.PNames[2] = std::string(buf2);
		sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
		mob->player_data.PNames[3] = std::string(buf2);
		sprintf(buf2, "умертвием %s", GET_PAD(mob, 1));
		mob->player_data.PNames[4] = std::string(buf2);
		sprintf(buf2, "умертвии %s", GET_PAD(mob, 1));
		mob->player_data.PNames[5] = std::string(buf2);
		sprintf(buf2, "умертвия %s", GET_PAD(mob, 1));
		mob->player_data.PNames[1] = std::string(buf2);
		mob->set_sex(ESex::kNeutral);
		MOB_FLAGS(mob).set(MOB_RESURRECTED);    // added by Pereplut
		// если есть фит ярость тьмы, то прибавляем к хп и дамролам
		if (can_use_feat(ch, FURYDARK_FEAT)) {
			GET_DR(mob) = GET_DR(mob) + GET_DR(mob) * 0.20;
			GET_MAX_HIT(mob) = GET_MAX_HIT(mob) + GET_MAX_HIT(mob) * 0.20;
			GET_HIT(mob) = GET_MAX_HIT(mob);
			GET_HR(mob) = GET_HR(mob) + GET_HR(mob) * 0.20;
		}
	}
	
	if (!IS_IMMORTAL(ch) && (AFF_FLAGGED(mob, EAffectFlag::AFF_SANCTUARY) || MOB_FLAGGED(mob, MOB_PROTECT))) {
		send_to_char("Оживляемый был освящен Богами и противится этому!\r\n", ch);
		extract_char(mob, false);
		return 0;
	}
	if (!IS_IMMORTAL(ch)
		&& (GET_MOB_SPEC(mob) || MOB_FLAGGED(mob, MOB_NORESURRECTION) || MOB_FLAGGED(mob, MOB_AREA_ATTACK))) {
		send_to_char("Вы не можете обрести власть над этим созданием!\r\n", ch);
		extract_char(mob, false);
		return 0;
	}
	if (!IS_IMMORTAL(ch) && AFF_FLAGGED(mob, EAffectFlag::AFF_SHIELD)) {
		send_to_char("Боги защищают это существо даже после смерти.\r\n", ch);
		extract_char(mob, false);
		return 0;
	}
	if (MOB_FLAGGED(mob, MOB_MOUNTING)) {
		MOB_FLAGS(mob).unset(MOB_MOUNTING);
	}
	if (IS_HORSE(mob)) {
		send_to_char("Это был боевой скакун, а не хухры-мухры.\r\n", ch);
		extract_char(mob, false);
		return 0;
	}

	if (spellnum == kSpellAnimateDead && mob_num >= kMobNecrodamager && mob_num <= kLastNecroMob) {
		// add 10% mob health by remort
		mob->set_max_hit(mob->get_max_hit() * (1.0 + GET_REAL_REMORT(ch) / 10.0));
		mob->set_hit(mob->get_max_hit());
		int player_charms_value = get_player_charms(ch, spellnum);
		int mob_cahrms_value = get_reformed_charmice_hp(ch, mob, spellnum);
		int damnodice = 1;
		mob->mob_specials.damnodice = damnodice;
		// look for count dice to maximize damage on player_charms_value. max 255.
		while (player_charms_value > mob_cahrms_value && damnodice <= 255) {
			damnodice++;
			mob->mob_specials.damnodice = damnodice;
			mob_cahrms_value = get_reformed_charmice_hp(ch, mob, spellnum);
		}
		damnodice--;

		mob->mob_specials.damnodice = damnodice; // get prew damnodice for match with player_charms_value
		if (damnodice == 255) {
			// if damnodice == 255 mob damage not maximized. damsize too small
			send_to_room("Темные искры пробежали по земле... И исчезли...", ch->in_room, 0);
		} else {
			// mob damage maximazed.
			send_to_room("Темные искры пробежали по земле. Кажется сама СМЕРТЬ наполняет это тело силой!",
						 ch->in_room,
						 0);
		}
	}

	if (!CheckCharmices(ch, mob, spellnum)) {
		extract_char(mob, false);
		if (handle_corpse)
			extract_obj(obj);
		return 0;
	}

	mob->set_exp(0);
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;
	mob->set_gold(0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;
	const auto days_from_full_moon =
		(weather_info.moon_day < 14) ? (14 - weather_info.moon_day) : (weather_info.moon_day - 14);
	const auto duration = CalcDuration(mob, GET_REAL_WIS(ch) + number(0, days_from_full_moon), 0, 0, 0, 0);
	Affect<EApplyLocation> af;
	af.type = kSpellCharm;
	af.duration = duration;
	af.modifier = 0;
	af.location = EApplyLocation::APPLY_NONE;
	af.bitvector = to_underlying(EAffectFlag::AFF_CHARM);
	af.battleflag = 0;
	affect_to_char(mob, af);
	if (keeper) {
		af.bitvector = to_underlying(EAffectFlag::AFF_HELPER);
		affect_to_char(mob, af);
		mob->set_skill(ESkill::kRescue, 100);
	}

	MOB_FLAGS(mob).set(MOB_CORPSE);
	if (spellnum == kSpellClone) {
		sprintf(buf2, "двойник %s %s", GET_PAD(ch, 1), GET_NAME(ch));
		mob->set_pc_name(buf2);
		sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
		mob->set_npc_name(buf2);
		mob->player_data.long_descr = "";
		sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
		mob->player_data.PNames[0] = std::string(buf2);
		sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
		mob->player_data.PNames[1] = std::string(buf2);
		sprintf(buf2, "двойнику %s", GET_PAD(ch, 1));
		mob->player_data.PNames[2] = std::string(buf2);
		sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
		mob->player_data.PNames[3] = std::string(buf2);
		sprintf(buf2, "двойником %s", GET_PAD(ch, 1));
		mob->player_data.PNames[4] = std::string(buf2);
		sprintf(buf2, "двойнике %s", GET_PAD(ch, 1));
		mob->player_data.PNames[5] = std::string(buf2);

		mob->set_str(ch->get_str());
		mob->set_dex(ch->get_dex());
		mob->set_con(ch->get_con());
		mob->set_wis(ch->get_wis());
		mob->set_int(ch->get_int());
		mob->set_cha(ch->get_cha());

		mob->set_level(GetRealLevel(ch));
		GET_HR(mob) = -20;
		GET_AC(mob) = GET_AC(ch);
		GET_DR(mob) = GET_DR(ch);

		GET_MAX_HIT(mob) = GET_MAX_HIT(ch);
		GET_HIT(mob) = GET_MAX_HIT(ch);
		mob->mob_specials.damnodice = 0;
		mob->mob_specials.damsizedice = 0;
		mob->set_gold(0);
		GET_GOLD_NoDs(mob) = 0;
		GET_GOLD_SiDs(mob) = 0;
		mob->set_exp(0);

		GET_POS(mob) = EPosition::kStand;
		GET_DEFAULT_POS(mob) = EPosition::kStand;
		mob->set_sex(ESex::kMale);

		mob->set_class(ch->get_class());
		GET_WEIGHT(mob) = GET_WEIGHT(ch);
		GET_HEIGHT(mob) = GET_HEIGHT(ch);
		GET_SIZE(mob) = GET_SIZE(ch);
		MOB_FLAGS(mob).set(MOB_CLONE);
		MOB_FLAGS(mob).unset(MOB_MOUNTING);
	}
	act(mag_summon_msgs[msg], false, ch, nullptr, mob, kToRoom | kToArenaListen);

	char_to_room(mob, ch->in_room);
	ch->add_follower(mob); 
	
	if (spellnum == kSpellClone) {
		// клоны теперь кастятся все вместе // ужасно некрасиво сделано
		for (k = ch->followers; k; k = k->next) {
			if (AFF_FLAGGED(k->ch, EAffectFlag::AFF_CHARM)
				&& k->ch->get_master() == ch) {
				cha_num++;
			}
		}
		cha_num = MAX(1, (GetRealLevel(ch) + 4) / 5 - 2) - cha_num;
		if (cha_num < 1)
			return 0;
		mag_summons(level, ch, obj, spellnum, 0);
	}
	if (spellnum == kSpellAnimateDead) {
		MOB_FLAGS(mob).set(MOB_RESURRECTED);
		if (mob_num == kMobSkeleton && can_use_feat(ch, LOYALASSIST_FEAT))
			mob->set_skill(ESkill::kRescue, 100);

		if (mob_num == kMobBonespirit && can_use_feat(ch, HAUNTINGSPIRIT_FEAT))
			mob->set_skill(ESkill::kRescue, 120);

		// даем всем поднятым, ну наверное не будет чернок 75+ мудры вызывать зомби в щите.
		float eff_wis = get_effective_wis(ch, spellnum);
		if (eff_wis >= 65) {
			// пока не даем, если надо включите
			//af.bitvector = to_underlying(EAffectFlag::AFF_MAGICGLASS);
			//affect_to_char(mob, af);
		}
		if (eff_wis >= 75) {
			Affect<EApplyLocation> af;
			af.type = kSpellNoSpell;
			af.duration = duration * (1 + GET_REAL_REMORT(ch));
			af.modifier = 0;
			af.location = EApplyLocation::APPLY_NONE;
			af.bitvector = to_underlying(EAffectFlag::AFF_ICESHIELD);
			af.battleflag = 0;
			affect_to_char(mob, af);
		}

	}

	if (spellnum == kSpellSummonKeeper) {
		// Svent TODO: не забыть перенести это в ability
		mob->set_level(GetRealLevel(ch));
		int rating = (ch->get_skill(ESkill::kLightMagic) + GET_REAL_CHA(ch)) / 2;
		GET_MAX_HIT(mob) = GET_HIT(mob) = 50 + RollDices(10, 10) + rating * 6;
		mob->set_skill(ESkill::kPunch, 10 + rating * 1.5);
		mob->set_skill(ESkill::kRescue, 50 + rating);
		mob->set_str(3 + rating / 5);
		mob->set_dex(10 + rating / 5);
		mob->set_con(10 + rating / 5);
		GET_HR(mob) = rating / 2 - 4;
		GET_AC(mob) = 100 - rating * 2.65;
	}

	if (spellnum == kSpellSummonFirekeeper) {
		Affect<EApplyLocation> af;
		af.type = kSpellCharm;
		af.duration = duration;
		af.modifier = 0;
		af.location = EApplyLocation::APPLY_NONE;
		af.battleflag = 0;
		if (get_effective_cha(ch) >= 30) {
			af.bitvector = to_underlying(EAffectFlag::AFF_FIRESHIELD);
			affect_to_char(mob, af);
		} else {
			af.bitvector = to_underlying(EAffectFlag::AFF_FIREAURA);
			affect_to_char(mob, af);
		}

		modifier = VPOSI((int) get_effective_cha(ch) - 20, 0, 30);

		GET_DR(mob) = 10 + modifier * 3 / 2;
		GET_NDD(mob) = 1;
		GET_SDD(mob) = modifier / 5 + 1;
		mob->mob_specials.ExtraAttack = 0;

		GET_MAX_HIT(mob) = GET_HIT(mob) = 300 + number(modifier * 12, modifier * 16);
		mob->set_skill(ESkill::kAwake, 50 + modifier * 2);
		PRF_FLAGS(mob).set(PRF_AWAKE);
	}
	MOB_FLAGS(mob).set(MOB_NOTRAIN);
	// А надо ли это вообще делать???
	if (handle_corpse) {
		for (tobj = obj->get_contains(); tobj;) {
			next_obj = tobj->get_next_content();
			obj_from_obj(tobj);
			obj_to_room(tobj, ch->in_room);
			if (!obj_decay(tobj) && tobj->get_in_room() != kNowhere) {
				act("На земле остал$U лежать $o.", false, ch, tobj, nullptr, kToRoom | kToArenaListen);
			}
			tobj = next_obj;
		}
		extract_obj(obj);
	}
	return 1;
}

int CastToPoints(int level, CharData *ch, CharData *victim, int spellnum, ESaving /*savetype*/ ) {
	int hit = 0; //если выставить больше нуля, то лечит
	int move = 0; //если выставить больше нуля, то реген хп
	bool extraHealing = false; //если true, то лечит сверх макс.хп

	if (victim == nullptr) {
		log("MAG_POINTS: Ошибка! Не указана цель, спелл spellnum: %d!\r\n", spellnum);
		return 0;
	}

	switch (spellnum) {
		case kSpellCureLight:
			hit = GET_REAL_MAX_HIT(victim) / 100 * GET_REAL_INT(ch) / 3 + ch->get_skill(ESkill::kLifeMagic) / 2;
			send_to_char("Вы почувствовали себя немножко лучше.\r\n", victim);
			break;
		case kSpellCureSerious:
			hit = GET_REAL_MAX_HIT(victim) / 100 * GET_REAL_INT(ch) / 2 + ch->get_skill(ESkill::kLifeMagic) / 2;
			send_to_char("Вы почувствовали себя намного лучше.\r\n", victim);
			break;
		case kSpellCureCritic:
			hit = int (GET_REAL_MAX_HIT(victim) / 100 * GET_REAL_INT(ch) / 1.5) + ch->get_skill(ESkill::kLifeMagic) / 2;
			send_to_char("Вы почувствовали себя значительно лучше.\r\n", victim);
			break;
		case kSpellHeal:
		case kSpellGroupHeal:
			hit = GET_REAL_MAX_HIT(victim) - GET_HIT(victim);
			send_to_char("Вы почувствовали себя здоровым.\r\n", victim);
			break;
		case kSpellPatronage:
			hit = (GetRealLevel(victim) + GET_REAL_REMORT(victim)) * 2;
			break;
		case kSpellWarcryOfPower:
			hit = MIN(200, (4 * ch->get_con() + ch->get_skill(ESkill::kWarcry)) / 2);
			send_to_char("По вашему телу начала струиться живительная сила.\r\n", victim);
			break;
		case kSpellExtraHits:
			extraHealing = true;
			hit = RollDices(10, level / 3) + level;
			send_to_char("По вашему телу начала струиться живительная сила.\r\n", victim);
			break;
		case kSpellEviless:
			//лечим только умертвия-чармисы
			if (!IS_NPC(victim) || victim->get_master() != ch || !MOB_FLAGGED(victim, MOB_CORPSE))
				return 1;
			//при рекасте - не лечим
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_EVILESS)) {
				hit = GET_REAL_MAX_HIT(victim) - GET_HIT(victim);
				affect_from_char(ch, kSpellEviless); //сбрасываем аффект с хозяина
			}
			break;
		case kSpellResfresh:
		case kSpellGroupRefresh: move = GET_REAL_MAX_MOVE(victim) - GET_MOVE(victim);
			send_to_char("Вы почувствовали себя полным сил.\r\n", victim);
			break;
		case kSpellFullFeed:
		case kSpellCommonMeal: {
			if (GET_COND(victim, THIRST) > 0)
				GET_COND(victim, THIRST) = 0;
			if (GET_COND(victim, FULL) > 0)
				GET_COND(victim, FULL) = 0;
			send_to_char("Вы полностью насытились.\r\n", victim);
		}
			break;
		default: log("MAG_POINTS: Ошибка! Передан не определенный лечащий спелл spellnum: %d!\r\n", spellnum);
			return 0;
			break;
	}

	hit = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_EFFECT, hit);

	if (hit && victim->get_fighting() && ch != victim) {
		if (!pk_agro_action(ch, victim->get_fighting()))
			return 0;
	}
	// лечение
	if (GET_HIT(victim) < kMaxHits && hit != 0) {
		// просто лечим
		if (!extraHealing && GET_HIT(victim) < GET_REAL_MAX_HIT(victim))
			GET_HIT(victim) = MIN(GET_HIT(victim) + hit, GET_REAL_MAX_HIT(victim));
		// добавляем фикс.хиты сверх максимума
		if (extraHealing) {
			// если макс.хп отрицательные - доводим до 1
			if (GET_REAL_MAX_HIT(victim) <= 0)
				GET_HIT(victim) = MAX(GET_HIT(victim), MIN(GET_HIT(victim) + hit, 1));
			else
				// лимит в треть от макс.хп сверху
				GET_HIT(victim) = MAX(GET_HIT(victim),
									  MIN(GET_HIT(victim) + hit,
										  GET_REAL_MAX_HIT(victim) + GET_REAL_MAX_HIT(victim) * 33 / 100));
		}
	}
	if (move != 0 && GET_MOVE(victim) < GET_REAL_MAX_MOVE(victim))
		GET_MOVE(victim) = MIN(GET_MOVE(victim) + move, GET_REAL_MAX_MOVE(victim));
	update_pos(victim);

	return 1;
}

bool CheckNodispel(const Affect<EApplyLocation>::shared_ptr &affect) {
	return !affect
		|| !spell_info[affect->type].name
		|| *spell_info[affect->type].name == '!'
		|| affect->bitvector == to_underlying(EAffectFlag::AFF_CHARM)
		|| affect->type == kSpellCharm
		|| affect->type == kSpellQUest
		|| affect->type == kSpellPatronage
		|| affect->type == kSpellSolobonus
		|| affect->type == kSpellEviless;
}

int CastUnaffects(int/* level*/, CharData *ch, CharData *victim, int spellnum, ESaving/* type*/) {
	int spell = 0, remove = 0;
	const char *to_vict = nullptr, *to_room = nullptr;

	if (victim == nullptr) {
		return 0;
	}

	switch (spellnum) {
		case kSpellCureBlind: spell = kSpellBlindness;
			to_vict = "К вам вернулась способность видеть.";
			to_room = "$n прозрел$g.";
			break;
		case kSpellRemovePoison: spell = kSpellPoison;
			to_vict = "Тепло заполнило ваше тело.";
			to_room = "$n выглядит лучше.";
			break;
		case kSpellCureFever: spell = kSpellFever;
			to_vict = "Лихорадка прекратилась.";
			break;
		case kSpellRemoveCurse: spell = kSpellCurse;
			to_vict = "Боги вернули вам свое покровительство.";
			break;
		case kSpellRemoveHold: spell = kSpellHold;
			to_vict = "К вам вернулась способность двигаться.";
			break;
		case kSpellRemoveSilence: spell = kSpellSllence;
			to_vict = "К вам вернулась способность разговаривать.";
			break;
		case kSpellRemoveDeafness: spell = kSpellDeafness;
			to_vict = "К вам вернулась способность слышать.";
			break;
		case kSpellDispellMagic:
			if (!IS_NPC(ch)
				&& !same_group(ch, victim)) {
				send_to_char("Только на себя или одногруппника!\r\n", ch);

				return 0;
			}

			{
				const auto affects_count = victim->affected.size();
				if (0 == affects_count) {
					send_to_char(NOEFFECT, ch);
					return 0;
				}

				spell = 1;
				const auto rspell = number(1, static_cast<int>(affects_count));
				auto affect_i = victim->affected.begin();
				while (spell < rspell) {
					++affect_i;
					++spell;
				}

				if (CheckNodispel(*affect_i)) {
					send_to_char(NOEFFECT, ch);

					return 0;
				}

				spell = (*affect_i)->type;
			}

			remove = true;
			break;

		default: log("SYSERR: unknown spellnum %d passed to CastUnaffects.", spellnum);
			return 0;
	}

	if (spellnum == kSpellRemovePoison && !affected_by_spell(victim, spell)) {
		if (affected_by_spell(victim, kSpellAconitumPoison))
			spell = kSpellAconitumPoison;
		else if (affected_by_spell(victim, kSpellScopolaPoison))
			spell = kSpellScopolaPoison;
		else if (affected_by_spell(victim, kSpellBelenaPoison))
			spell = kSpellBelenaPoison;
		else if (affected_by_spell(victim, kSpellDaturaPoison))
			spell = kSpellDaturaPoison;
	}

	if (!affected_by_spell(victim, spell)) {
		if (spellnum != kSpellHeal)    // 'cure blindness' message.
			send_to_char(NOEFFECT, ch);
		return 0;
	}
	spellnum = spell;
	if (ch != victim && !remove) {
		if (IS_SET(SpINFO.routines, kNpcAffectNpc)) {
			if (!pk_agro_action(ch, victim))
				return 0;
		} else if (IS_SET(SpINFO.routines, kNpcAffectPc) && victim->get_fighting()) {
			if (!pk_agro_action(ch, victim->get_fighting()))
				return 0;
		}
	}
//Polud затычка для закла !удалить яд!. По хорошему нужно его переделать с параметром - тип удаляемого яда
	if (spell == kSpellPoison) {
		affect_from_char(victim, kSpellAconitumPoison);
		affect_from_char(victim, kSpellDaturaPoison);
		affect_from_char(victim, kSpellScopolaPoison);
		affect_from_char(victim, kSpellBelenaPoison);
	}
	affect_from_char(victim, spell);
	if (to_vict != nullptr)
		act(to_vict, false, victim, nullptr, ch, kToChar);
	if (to_room != nullptr)
		act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen);

	return 1;
}

int CastToAlterObjs(int/* level*/, CharData *ch, ObjData *obj, int spellnum, ESaving /* savetype*/) {
	const char *to_char = nullptr;

	if (obj == nullptr) {
		return 0;
	}

	if (obj->get_extra_flag(EExtraFlag::ITEM_NOALTER)) {
		act("$o устойчив$A к вашей магии.", true, ch, obj, nullptr, kToChar);
		return 0;
	}

	switch (spellnum) {
		case kSpellBless:
			if (!obj->get_extra_flag(EExtraFlag::ITEM_BLESS)
				&& (GET_OBJ_WEIGHT(obj) <= 5 * GetRealLevel(ch))) {
				obj->set_extra_flag(EExtraFlag::ITEM_BLESS);
				if (obj->get_extra_flag(EExtraFlag::ITEM_NODROP)) {
					obj->unset_extraflag(EExtraFlag::ITEM_NODROP);
					if (GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON) {
						obj->inc_val(2);
					}
				}
				obj->add_maximum(MAX(GET_OBJ_MAX(obj) >> 2, 1));
				obj->set_current_durability(GET_OBJ_MAX(obj));
				to_char = "$o вспыхнул$G голубым светом и тут же погас$Q.";
				obj->add_timed_spell(kSpellBless, -1);
			}
			break;

		case kSpellCurse:
			if (!obj->get_extra_flag(EExtraFlag::ITEM_NODROP)) {
				obj->set_extra_flag(EExtraFlag::ITEM_NODROP);
				if (GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON) {
					if (GET_OBJ_VAL(obj, 2) > 0) {
						obj->dec_val(2);
					}
				} else if (ObjSystem::is_armor_type(obj)) {
					if (GET_OBJ_VAL(obj, 0) > 0) {
						obj->dec_val(0);
					}
					if (GET_OBJ_VAL(obj, 1) > 0) {
						obj->dec_val(1);
					}
				}
				to_char = "$o вспыхнул$G красным светом и тут же погас$Q.";
			}
			break;

		case kSpellInvisible:
			if (!obj->get_extra_flag(EExtraFlag::ITEM_NOINVIS)
				&& !obj->get_extra_flag(EExtraFlag::ITEM_INVISIBLE)) {
				obj->set_extra_flag(EExtraFlag::ITEM_INVISIBLE);
				to_char = "$o растворил$U в пустоте.";
			}
			break;

		case kSpellPoison:
			if (!GET_OBJ_VAL(obj, 3)
				&& (GET_OBJ_TYPE(obj) == ObjData::ITEM_DRINKCON
					|| GET_OBJ_TYPE(obj) == ObjData::ITEM_FOUNTAIN
					|| GET_OBJ_TYPE(obj) == ObjData::ITEM_FOOD)) {
				obj->set_val(3, 1);
				to_char = "$o отравлен$G.";
			}
			break;

		case kSpellRemoveCurse:
			if (obj->get_extra_flag(EExtraFlag::ITEM_NODROP)) {
				obj->unset_extraflag(EExtraFlag::ITEM_NODROP);
				if (GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON) {
					obj->inc_val(2);
				}
				to_char = "$o вспыхнул$G розовым светом и тут же погас$Q.";
			}
			break;

		case kSpellEnchantWeapon: {
			if (ch == nullptr || obj == nullptr) {
				return 0;
			}

			// Either already enchanted or not a weapon.
			if (GET_OBJ_TYPE(obj) != ObjData::ITEM_WEAPON) {
				to_char = "Еще раз ударьтесь головой об стену, авось зрение вернется...";
				break;
			} else if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_MAGIC)) {
				to_char = "Вам не под силу зачаровать магическую вещь.";
				break;
			}

			if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF)) {
				send_to_char(ch, "Сетовый предмет не может быть заколдован.\r\n");
				break;
			}

			auto reagobj = GET_EQ(ch, WEAR_HOLD);
			if (reagobj
				&& (get_obj_in_list_vnum(GlobalDrop::MAGIC1_ENCHANT_VNUM, reagobj)
					|| get_obj_in_list_vnum(GlobalDrop::MAGIC2_ENCHANT_VNUM, reagobj)
					|| get_obj_in_list_vnum(GlobalDrop::MAGIC3_ENCHANT_VNUM, reagobj))) {
				// у нас имеется доп символ для зачарования
				obj->set_enchant(ch->get_skill(ESkill::kLightMagic), reagobj);
				material_component_processing(ch, reagobj->get_rnum(), spellnum); //может неправильный вызов
			} else {
				obj->set_enchant(ch->get_skill(ESkill::kLightMagic));
			}
			if (GET_RELIGION(ch) == kReligionMono) {
				to_char = "$o вспыхнул$G на миг голубым светом и тут же потух$Q.";
			} else if (GET_RELIGION(ch) == kReligionPoly) {
				to_char = "$o вспыхнул$G на миг красным светом и тут же потух$Q.";
			} else {
				to_char = "$o вспыхнул$G на миг желтым светом и тут же потух$Q.";
			}
			break;
		}
		case kSpellRemovePoison:
			if (GET_OBJ_RNUM(obj) < 0) {
				to_char = "Ничего не случилось.";
				char message[100];
				sprintf(message,
						"неизвестный прототип объекта : %s (VNUM=%d)",
						GET_OBJ_PNAME(obj, 0).c_str(),
						obj->get_vnum());
				mudlog(message, BRF, kLvlBuilder, SYSLOG, 1);
				break;
			}
			if (obj_proto[GET_OBJ_RNUM(obj)]->get_val(3) > 1 && GET_OBJ_VAL(obj, 3) == 1) {
				to_char = "Содержимое $o1 протухло и не поддается магии.";
				break;
			}
			if ((GET_OBJ_VAL(obj, 3) == 1)
				&& ((GET_OBJ_TYPE(obj) == ObjData::ITEM_DRINKCON)
					|| GET_OBJ_TYPE(obj) == ObjData::ITEM_FOUNTAIN
					|| GET_OBJ_TYPE(obj) == ObjData::ITEM_FOOD)) {
				obj->set_val(3, 0);
				to_char = "$o стал$G вполне пригодным к применению.";
			}
			break;

		case kSpellFly: obj->add_timed_spell(kSpellFly, -1);
			obj->set_extra_flag(EExtraFlag::ITEM_FLYING);
			to_char = "$o вспыхнул$G зеленоватым светом и тут же погас$Q.";
			break;

		case kSpellAcid: alterate_object(obj, number(GetRealLevel(ch) * 2, GetRealLevel(ch) * 4), 100);
			break;

		case kSpellRepair: obj->set_current_durability(GET_OBJ_MAX(obj));
			to_char = "Вы полностью восстановили $o3.";
			break;

		case kSpellTimerRestore:
			if (GET_OBJ_RNUM(obj) != kNothing) {
				obj->set_current_durability(GET_OBJ_MAX(obj));
				obj->set_timer(obj_proto.at(GET_OBJ_RNUM(obj))->get_timer());
				to_char = "Вы полностью восстановили $o3.";
				log("%s used magic repair", GET_NAME(ch));
			} else {
				return 0;
			}
			break;

		case kSpellRestoration: {
			if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_MAGIC)
				&& (GET_OBJ_RNUM(obj) != kNothing)) {
				if (obj_proto.at(GET_OBJ_RNUM(obj))->get_extra_flag(EExtraFlag::ITEM_MAGIC)) {
					return 0;
				}
				obj->unset_enchant();
			} else {
				return 0;
			}
			to_char = "$o осветил$U на миг внутренним светом и тут же потух$Q.";
		}
			break;

		case kSpellLight: obj->add_timed_spell(kSpellLight, -1);
			obj->set_extra_flag(EExtraFlag::ITEM_GLOW);
			to_char = "$o засветил$U ровным зеленоватым светом.";
			break;

		case kSpellDarkness:
			if (obj->timed_spell().check_spell(kSpellLight)) {
				obj->del_timed_spell(kSpellLight, true);
				return 1;
			}
			break;
	} // switch

	if (to_char == nullptr) {
		send_to_char(NOEFFECT, ch);
	} else {
		act(to_char, true, ch, obj, nullptr, kToChar);
	}

	return 1;
}

int CastCreation(int/* level*/, CharData *ch, int spellnum) {
	ObjVnum z;

	if (ch == nullptr) {
		return 0;
	}
	// level = MAX(MIN(level, kLevelImplementator), 1); - Hm, not used.

	switch (spellnum) {
		case kSpellCreateFood: z = START_BREAD;
			break;

		case kSpellCreateLight: z = CREATE_LIGHT;
			break;

		default: send_to_char("Spell unimplemented, it would seem.\r\n", ch);
			return 0;
			break;
	}

	const auto tobj = world_objects.create_from_prototype_by_vnum(z);
	if (!tobj) {
		send_to_char("Что-то не видно образа для создания.\r\n", ch);
		log("SYSERR: spell_creations, spell %d, obj %d: obj not found", spellnum, z);
		return 0;
	}

	act("$n создал$g $o3.", false, ch, tobj.get(), nullptr, kToRoom | kToArenaListen);
	act("Вы создали $o3.", false, ch, tobj.get(), nullptr, kToChar);
	load_otrigger(tobj.get());

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
		send_to_char("Вы не сможете унести столько предметов.\r\n", ch);
		obj_to_room(tobj.get(), ch->in_room);
		obj_decay(tobj.get());
	} else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(tobj) > CAN_CARRY_W(ch)) {
		send_to_char("Вы не сможете унести такой вес.\r\n", ch);
		obj_to_room(tobj.get(), ch->in_room);
		obj_decay(tobj.get());
	} else {
		obj_to_char(tobj.get(), ch);
	}

	return 1;
}

int CastManual(int level, CharData *caster, CharData *cvict, ObjData *ovict, int spellnum, ESaving /*saving*/) {
	switch (spellnum) {
		case kSpellGroupRecall:
		case kSpellWorldOfRecall:
			SpellRecall(level, caster, cvict, ovict);
			break;
		case kSpellTeleport:
			SpellTeleport(level, caster, cvict, ovict);
			break;
		case kSpellControlWeather:
			SpellControlWeather(level, caster, cvict, ovict);
			break;
		case kSpellCreateWater:
			SpellCreateWater(level, caster, cvict, ovict);
			break;
		case kSpellLocateObject:
			SpellLocateObject(level, caster, cvict, ovict);
			break;
		case kSpellSummon:
			SpellSummon(level, caster, cvict, ovict);
			break;
		case kSpellPortal:
			SpellPortal(level, caster, cvict, ovict);
			break;
		case kSpellCreateWeapon:
			SpellCreateWeapon(level, caster, cvict, ovict);
			break;
		case kSpellRelocate:
			SpellRelocate(level, caster, cvict, ovict);
			break;
		case kSpellCharm:
			SpellCharm(level, caster, cvict, ovict);
			break;
		case kSpellEnergyDrain:
			SpellEnergydrain(level, caster, cvict, ovict);
			break;
		case kSpellMassFear:
		case kSpellFear:
			SpellFear(level, caster, cvict, ovict);
			break;
		case kSpellSacrifice:
			SpellSacrifice(level, caster, cvict, ovict);
			break;
		case kSpellIdentify:
			SpellIdentify(level, caster, cvict, ovict);
			break;
		case kSpellFullIdentify:
			SpellFullIdentify(level, caster, cvict, ovict);
			break;
		case kSpellHolystrike:
			SpellHolystrike(level, caster, cvict, ovict);
			break;
		case kSpellSumonAngel:
			SpellSummonAngel(level, caster, cvict, ovict);
			break;
		case kSpellVampirism:
			SpellVampirism(level, caster, cvict, ovict);
			break;
		case kSpellMentalShadow:
			SpellMentalShadow(level, caster, cvict, ovict);
			break;
		default: return 0;
			break;
	}
	return 1;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************

int CheckMobList(CharData *ch) {
	for (const auto &vict : character_list) {
		if (vict.get() == ch) {
			return (true);
		}
	}

	return (false);
}

void ReactToCast(CharData *victim, CharData *caster, int spellnum) {
	if (caster == victim)
		return;

	if (!CheckMobList(victim) || !SpINFO.violent)
		return;

	if (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM) ||
		AFF_FLAGGED(victim, EAffectFlag::AFF_SLEEP) ||
		AFF_FLAGGED(victim, EAffectFlag::AFF_BLIND) ||
		AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT) ||
		AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT) || AFF_FLAGGED(victim, EAffectFlag::AFF_HOLD)
		|| IS_HORSE(victim))
		return;

	if (IS_NPC(caster)
		&& GET_MOB_RNUM(caster) == real_mobile(DG_CASTER_PROXY))
		return;

	if (CAN_SEE(victim, caster) && MAY_ATTACK(victim) && IN_ROOM(victim) == IN_ROOM(caster)) {
		if (IS_NPC(victim))
			attack_best(victim, caster);
		else
			hit(victim, caster, ESkill::kUndefined, fight::kMainHand);
	} else if (CAN_SEE(victim, caster) && !IS_NPC(caster) && IS_NPC(victim) && MOB_FLAGGED(victim, MOB_MEMORY)) {
		mobRemember(victim, caster);
	}

	if (caster->purged()) {
		return;
	}
	if (!CAN_SEE(victim, caster) && (GET_REAL_INT(victim) > 25 || GET_REAL_INT(victim) > number(10, 25))) {
		if (!AFF_FLAGGED(victim, EAffectFlag::AFF_DETECT_INVIS)
			&& GET_SPELL_MEM(victim, kSpellDetectInvis) > 0)
			CastSpell(victim, victim, 0, 0, kSpellDetectInvis, kSpellDetectInvis);
		else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_SENSE_LIFE)
			&& GET_SPELL_MEM(victim, kSpellSenseLife) > 0)
			CastSpell(victim, victim, 0, 0, kSpellSenseLife, kSpellSenseLife);
		else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_INFRAVISION)
			&& GET_SPELL_MEM(victim, kSpellLight) > 0)
			CastSpell(victim, victim, 0, 0, kSpellLight, kSpellLight);
	}
}

// Применение заклинания к одной цели
//---------------------------------------------------------
int CastToSingleTarget(int level, CharData *caster, CharData *cvict, ObjData *ovict, int spellnum, ESaving saving) {
	/*
	if (IS_SET(SpINFO.routines, 0)){
			send_to_char(NOEFFECT, caster);
			return (-1);
	}
	*/

	//туповато конечно, но подобные проверки тут как счупалцьа перепутаны
	//и чтоб сделать по человечески надо треть пары модулей перелопатить
	if (cvict && (caster != cvict))
		if (IS_GOD(cvict) || (((GetRealLevel(cvict) / 2) > (GetRealLevel(caster) + (GET_REAL_REMORT(caster) / 2)))
			&& !IS_NPC(caster))) // при разнице уровня более чем в 2 раза закл фэйл
		{
			send_to_char(NOEFFECT, caster);
			return (-1);
		}
	if (!cast_mtrigger(cvict, caster, spellnum)) {
		return -1;
		}
	if (IS_SET(SpINFO.routines, kMagWarcry) && cvict && IS_UNDEAD(cvict))
		return 1;

	if (IS_SET(SpINFO.routines, kMagDamage))
		if (mag_damage(level, caster, cvict, spellnum, saving) == -1)
			return (-1);    // Successful and target died, don't cast again.

	if (IS_SET(SpINFO.routines, kMagAffects))
		mag_affects(level, caster, cvict, spellnum, saving);

	if (IS_SET(SpINFO.routines, kMagUnaffects))
		CastUnaffects(level, caster, cvict, spellnum, saving);

	if (IS_SET(SpINFO.routines, kMagPoints))
		CastToPoints(level, caster, cvict, spellnum, saving);

	if (IS_SET(SpINFO.routines, kMagAlterObjs))
		CastToAlterObjs(level, caster, ovict, spellnum, saving);

	if (IS_SET(SpINFO.routines, kMagSummons))
		mag_summons(level, caster, ovict, spellnum, 1);    // saving =1 -- ВРЕМЕННО, показатель что фэйлить надо

	if (IS_SET(SpINFO.routines, kMagCreations))
		CastCreation(level, caster, spellnum);

	if (IS_SET(SpINFO.routines, kMagManual))
		CastManual(level, caster, cvict, ovict, spellnum, saving);

	ReactToCast(cvict, caster, spellnum);
	return 1;
}

typedef struct {
	int spell;
	const char *to_char;
	const char *to_room;
	const char *to_vict;
	float castSuccessPercentDecay;
	int skillDivisor;
	int diceSize;
	int minTargetsAmount;
	int maxTargetsAmount;
	int freeTargets;
	int castLevelDecay;
} spl_message;

// Svent TODO Перенести эту порнографию в спеллпарсер
const spl_message mag_messages[] =
	{
		{kSpellPaladineInspiration, // групповой
		 "Ваш точный удар воодушевил и придал новых сил!",
		 "Точный удар $n1 воодушевил и придал новых сил!",
		 nullptr,
		 0.0, 20, 2, 1, 20, 3, 0},
		{kSpellEviless,
		 "Вы запросили помощи у Чернобога. Долг перед темными силами стал чуточку больше..",
		 "Внезапно появившееся чёрное облако скрыло $n3 на мгновение от вашего взгляда.",
		 nullptr,
		 0.0, 20, 2, 1, 20, 3, 0},
		{kSpellMassBlindness,
		 "У вас над головой возникла яркая вспышка, которая ослепила все живое.",
		 "Вдруг над головой $n1 возникла яркая вспышка.",
		 "Вы невольно взглянули на вспышку света, вызванную $n4, и ваши глаза заслезились.",
		 0.05, 20, 2, 5, 20, 3, 2},
		{kSpellMassHold,
		 "Вы сжали зубы от боли, когда из вашего тела вырвалось множество невидимых каменных лучей.",
		 nullptr,
		 "В вас попал каменный луч, исходящий от $n1.",
		 0.05, 20, 2, 5, 20, 3, 2},
		{kSpellMassCurse,
		 "Медленно оглянувшись, вы прошептали древние слова.",
		 nullptr,
		 "$n злобно посмотрел$g на вас и начал$g шептать древние слова.",
		 0.05, 20, 3, 5, 20, 3, 2},
		{kSpellMassSilence,
		 "Поведя вокруг грозным взглядом, вы заставили всех замолчать.",
		 nullptr,
		 "Вы встретились взглядом с $n4, и у вас появилось ощущение, что горлу чего-то не хватает.",
		 0.05, 20, 2, 5, 20, 3, 2},
		{kSpellDeafness,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.05, 10, 3, 5, 20, 3, 2},
		{kSpellMassDeafness,
		 "Вы нахмурились, склонив голову, и громкий хлопок сотряс воздух.",
		 "Как только $n0 склонил$g голову, раздался оглушающий хлопок.",
		 nullptr,
		 0.05, 10, 3, 5, 20, 3, 2},
		{kSpellMassSlow,
		 "Положив ладони на землю, вы вызвали цепкие корни,\r\nопутавшие существ, стоящих рядом с вами.",
		 nullptr,
		 "$n вызвал$g цепкие корни, опутавшие ваши ноги.",
		 0.05, 10, 3, 5, 20, 3, 2},
		{kSpellArmageddon,
		 "Вы сплели руки в замысловатом жесте, и все потускнело!",
		 "$n сплел$g руки в замысловатом жесте, и все потускнело!",
		 nullptr,
		 0.05, 25, 2, 5, 20, 3, 2},
		{kSpellEarthquake,
		 "Вы опустили руки, и земля начала дрожать вокруг вас!",
		 "$n опустил$g руки, и земля задрожала!",
		 nullptr,
		 0.05, 25, 2, 5, 20, 5, 2},
		{kSpellThunderStone,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.05, 25, 2, 3, 15, 3, 4},
		{kSpellColdWind,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.05, 40, 2, 2, 5, 5, 4},
		{kSpellAcid,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.05, 20, 2, 3, 8, 4, 4},
		{kSpellLightingBolt,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.05, 15, 3, 3, 6, 4, 4},
		{kSpellCallLighting,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.05, 15, 3, 3, 5, 4, 4},
		{kSpellWhirlwind,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.05, 40, 1, 1, 3, 3, 4},
		{kSpellDamageSerious,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.05, 20, 3, 1, 6, 6, 4},
		{kSpellFireBlast,
		 "Вы вызвали потоки подземного пламени!",
		 "$n0 вызвал$g потоки пламени из глубин земли!",
		 nullptr,
		 0.05, 20, 2, 5, 20, 3, 3},
		{kSpellIceStorm,
		 "Вы воздели руки к небу, и тысячи мелких льдинок хлынули вниз!",
		 "$n воздел$g руки к небу, и тысячи мелких льдинок хлынули вниз!",
		 nullptr,
		 0.05, 30, 2, 5, 20, 3, 4},
		{kSpellDustStorm,
		 "Вы взмахнули руками и вызвали огромное пылевое облако,\r\nскрывшее все вокруг.",
		 "Вас поглотила пылевая буря, вызванная $n4.",
		 nullptr,
		 0.05, 15, 2, 5, 20, 3, 2},
		{kSpellMassFear,
		 "Вы оглядели комнату устрашающим взглядом, заставив всех содрогнуться.",
		 "$n0 оглядел$g комнату устрашающим взглядом.",
		 nullptr,
		 0.05, 15, 2, 5, 20, 3, 3},
		{kSpellGlitterDust,
		 "Вы слегка прищелкнули пальцами, и вокруг сгустилось облако блестящей пыли.",
		 "$n0 сотворил$g облако блестящей пыли, медленно осевшее на землю.",
		 nullptr,
		 0.05, 15, 3, 5, 20, 5, 3},
		{kSpellSonicWave,
		 "Вы оттолкнули от себя воздух руками, и он плотным кольцом стремительно двинулся во все стороны!",
		 "$n махнул$g руками, и огромное кольцо сжатого воздуха распостранилось во все стороны!",
		 nullptr,
		 0.05, 20, 2, 5, 20, 3, 3},
		{kSpellChainLighting,
		 "Вы подняли руки к небу и оно осветилось яркими вспышками!",
		 "$n поднял$g руки к небу и оно осветилось яркими вспышками!",
		 nullptr,
		 0.05, 10, 3, 1, 8, 1, 5},
		{kSpellEarthfall,
		 "Вы высоко подбросили комок земли и он, увеличиваясь на глазах, обрушился вниз.",
		 "$n высоко подбросил$g комок земли, который, увеличиваясь на глазах, стал падать вниз.",
		 nullptr,
		 0.05, 20, 2, 1, 3, 1, 8},
		{kSpellShock,
		 "Яркая вспышка слетела с кончиков ваших пальцев и с оглушительным грохотом взорвалась в воздухе.",
		 "Выпущенная $n1 яркая вспышка с оглушительным грохотом взорвалась в воздухе.",
		 nullptr,
		 0.05, 35, 2, 1, 4, 2, 8},
		{kSpellBurdenOfTime,
		 "Вы скрестили руки на груди, вызвав яркую вспышку синего света.",
		 "$n0 скрестил$g руки на груди, вызвав яркую вспышку синего света.",
		 nullptr,
		 0.05, 20, 2, 5, 20, 3, 8},
		{kSpellFailure,
		 "Вы простерли руки над головой, вызвав череду раскатов грома.",
		 "$n0 вызвал$g череду раскатов грома, заставивших все вокруг содрогнуться.",
		 nullptr,
		 0.05, 15, 2, 1, 5, 3, 8},
		{kSpellScream,
		 "Вы испустили кошмарный вопль, едва не разорвавший вам горло.",
		 "$n0 испустил$g кошмарный вопль, отдавшийся в вашей душе замогильным холодом.",
		 nullptr,
		 0.05, 40, 3, 1, 8, 3, 5},
		{kSpellBurningHands,
		 "С ваших ладоней сорвался поток жаркого пламени.",
		 "$n0 испустил$g поток жаркого багрового пламени!",
		 nullptr,
		 0.05, 20, 2, 5, 20, 3, 7},
		{kSpellIceBolts,
		 "Из ваших рук вылетел сноп ледяных стрел.",
		 "$n0 метнул$g во врагов сноп ледяных стрел.",
		 nullptr,
		 0.05, 30, 2, 1, 5, 3, 7},
		{kSpellWarcryOfChallenge,
		 nullptr,
		 "Вы не стерпели насмешки, и бросились на $n1!",
		 nullptr,
		 0.01, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfMenace,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.01, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfRage,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.01, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfMadness,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.01, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfThunder,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.01, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfDefence,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.01, 20, 2, 5, 20, 3, 0},
		{kSpellGroupHeal,
		 "Вы подняли голову вверх и ощутили яркий свет, ласково бегущий по вашему телу.\r\n",
		 nullptr,
		 nullptr,
		 0.01, 20, 2, 5, 20, 3, 0},
		{kSpellGroupArmor,
		 "Вы создали защитную сферу, которая окутала вас и пространство рядом с вами.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupRecall,
		 "Вы выкрикнули заклинание и хлопнули в ладоши.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupStrength,
		 "Вы призвали мощь Вселенной.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupBless,
		 "Прикрыв глаза, вы прошептали таинственную молитву.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupHaste,
		 "Разведя руки в стороны, вы ощутили всю мощь стихии ветра.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupFly,
		 "Ваше заклинание вызвало белое облако, которое разделилось, подхватывая вас и товарищей.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupInvisible,
		 "Вы вызвали прозрачный туман, поглотивший все дружественное вам.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupMagicGlass,
		 "Вы произнесли несколько резких слов, и все вокруг засеребрилось.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupSanctuary,
		 "Вы подняли руки к небу и произнесли священную молитву.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupPrismaticAura,
		 "Силы духа, призванные вами, окутали вас и окружающих голубоватым сиянием.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellFireAura,
		 "Силы огня пришли к вам на помощь и защитили вас.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellAirAura,
		 "Силы воздуха пришли к вам на помощь и защитили вас.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellIceAura,
		 "Силы холода пришли к вам на помощь и защитили вас.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupRefresh,
		 "Ваша магия наполнила воздух зеленоватым сиянием.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfDefence,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfBattle,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfPower,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfBless,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfCourage,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellSightOfDarkness,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupSincerity,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellMagicalGaze,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellAllSeeingEye,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellEyeOfGods,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellBreathingAtDepth,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGeneralRecovery,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellCommonMeal,
		 "Вы услышали гомон невидимых лакеев, готовящих трапезу.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellStoneWall,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellSnakeEyes,
		 nullptr,
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellEarthAura,
		 "Земля одарила вас своей защитой.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupProtectFromEvil,
		 "Сила света подавила в вас страх к тьме.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupBlink,
		 "Очертания вас и соратников замерцали в такт биения сердца, став прозрачней.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupCloudly,
		 "Пелена тумана окутала вас и окружающих, скрыв очертания.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellGroupAwareness,
		 "Произнесенные слова обострили ваши чувства и внимательность ваших соратников.\r\n",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellWatctyOfExpirence,
		 "Вы приготовились к обретению нового опыта.",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfLuck,
		 "Вы ощутили, что вам улыбнулась удача.",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellWarcryOfPhysdamage,
		 "Боевой клич придал вам сил!",
		 nullptr,
		 nullptr,
		 0.0, 20, 2, 5, 20, 3, 0},
		{kSpellMassFailure,
		 "Вняв вашему призыву, Змей Велес коснулся недобрым взглядом ваших супостатов.\r\n",
		 nullptr,
		 "$n провыл$g несколько странно звучащих слов и от тяжелого взгляда из-за края мира у вас подкосились ноги.",
		 0.03, 25, 2, 3, 15, 4, 6},
		{kSpellSnare,
		 "Вы соткали магические тенета, опутавшие ваших врагов.\r\n",
		 nullptr,
		 "$n что-то прошептал$g, странно скрючив пальцы, и взлетевшие откуда ни возьмись ловчие сети опутали вас",
		 0.03, 25, 2, 3, 15, 5, 6},
		{-1, nullptr, nullptr, nullptr, 0.01, 1, 1, 1, 1, 1, 0}
	};

int FindIndexOfMsg(int spellNumber) {
	int i = 0;
	for (; mag_messages[i].spell != -1; ++i) {
		if (mag_messages[i].spell == spellNumber) {
			return i;
		}
	}
	return i;
}

int TrySendCastMessages(CharData *ch, CharData *victim, RoomData *room, int spellnum) {
	int msgIndex = FindIndexOfMsg(spellnum);
	if (mag_messages[msgIndex].spell < 0) {
		sprintf(buf, "ERROR: Нет сообщений в mag_messages для заклинания с номером %d.", spellnum);
		mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
		return msgIndex;
	}
	if (room && world[ch->in_room] == room) {
		if (multi_cast_say(ch)) {
			if (mag_messages[msgIndex].to_char != nullptr) {
				// вот тут надо воткнуть проверку на группу.
				act(mag_messages[msgIndex].to_char, false, ch, nullptr, victim, kToChar);
			}
			if (mag_messages[msgIndex].to_room != nullptr) {
				act(mag_messages[msgIndex].to_room, false, ch, nullptr, victim, kToRoom | kToArenaListen);
			}
		}
	}
	return msgIndex;
};

int CalcAmountOfTargets(const CharData *ch, const int &msgIndex, const int &spellnum) {
	int amount = ch->get_skill(GetMagicSkillId(spellnum));
	amount = RollDices(amount / mag_messages[msgIndex].skillDivisor, mag_messages[msgIndex].diceSize);
	return mag_messages[msgIndex].minTargetsAmount + MIN(amount, mag_messages[msgIndex].maxTargetsAmount);
}

int CallMagicToArea(CharData *ch, CharData *victim, RoomData *room, int spellnum, int level) {
	if (ch == nullptr || IN_ROOM(ch) == kNowhere) {
		return 0;
	}

	ActionTargeting::FoesRosterType
		roster{ch, victim, [](CharData *, CharData *target) { return !IS_HORSE(target); }};
	int msgIndex = TrySendCastMessages(ch, victim, room, spellnum);
	int targetsAmount = CalcAmountOfTargets(ch, msgIndex, spellnum);
	int targetsCounter = 1;
	float castDecay = 0.0;
	int levelDecay = 0;
	if (can_use_feat(ch, MULTI_CAST_FEAT)) {
		castDecay = mag_messages[msgIndex].castSuccessPercentDecay * 0.6;
		levelDecay = MAX(MIN(1, mag_messages[msgIndex].castLevelDecay), mag_messages[msgIndex].castLevelDecay - 1);
	} else {
		castDecay = mag_messages[msgIndex].castSuccessPercentDecay;
		levelDecay = mag_messages[msgIndex].castLevelDecay;
	}
	const int CASTER_CAST_SUCCESS = GET_CAST_SUCCESS(ch);

	for (const auto &target : roster) {
		if (mag_messages[msgIndex].to_vict != nullptr && target->desc) {
			act(mag_messages[msgIndex].to_vict, false, ch, nullptr, target, kToVict);
		}
		CastToSingleTarget(level, ch, target, nullptr, spellnum, ESaving::kStability);
		if (ch->purged()) {
			return 1;
		}
		if (!IS_NPC(ch)) {
			++targetsCounter;
			if (targetsCounter > mag_messages[msgIndex].freeTargets) {
				int tax = CASTER_CAST_SUCCESS * castDecay * (targetsCounter - mag_messages[msgIndex].freeTargets);
				GET_CAST_SUCCESS(ch) = MAX(-200, CASTER_CAST_SUCCESS - tax);
				level = MAX(1, level - levelDecay);
				if (PRF_FLAGGED(ch, PRF_TESTER)) {
					send_to_char(ch,
								 "&GМакс. целей: %d, Каст: %d, Уровень заклинания: %d.&n\r\n",
								 targetsAmount,
								 GET_CAST_SUCCESS(ch),
								 level);
				}
			};
		};
		if (targetsCounter >= targetsAmount) {
			break;
		}
	}

	GET_CAST_SUCCESS(ch) = CASTER_CAST_SUCCESS;
	return 1;
}

// Применение заклинания к группе в комнате
//---------------------------------------------------------
int CallMagicToGroup(int level, CharData *ch, int spellnum) {
	if (ch == nullptr) {
		return 0;
	}

	TrySendCastMessages(ch, nullptr, world[IN_ROOM(ch)], spellnum);

	ActionTargeting::FriendsRosterType roster{ch, ch};
	roster.flip();
	for (const auto target : roster) {
		CastToSingleTarget(level, ch, target, nullptr, spellnum, ESaving::kStability);
	}
	return 1;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
