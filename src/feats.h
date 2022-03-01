/*************************************************************************
*   File: features.hpp                                 Part of Bylins    *
*   Features code header                                                 *
*                                                                        *
*  $Author$                                                     *
*  $Date$                                          *
*  $Revision$                                                  *
**************************************************************************/
#ifndef FILE_FEATURES_H_INCLUDED
#define FILE_FEATURES_H_INCLUDED

#include "abilities/abilities_items_set.h"
#include "game_skills/skills.h"
#include "structs/structs.h"
#include "conf.h"
#include "game_classes/classes_constants.h"

#include <array>
#include <bitset>

const int INCORRECT_FEAT = 0;			//DO NOT USE
const int  BERSERK_FEAT = 1;			//предсмертная ярость
const int PARRY_ARROW_FEAT = 2;			//отбить стрелу
const int BLIND_FIGHT_FEAT = 3;			//слепой бой
const int IMPREGNABLE_FEAT = 4;			//непробиваемый
const int APPROACHING_ATTACK_FEAT = 5;	//встречная атака
const int DEFENDER_FEAT = 6;			//щитоносец
const int DODGER_FEAT = 7;				//изворотливость
const int LIGHT_WALK_FEAT = 8;			//легкая поступь
const int WRIGGLER_FEAT = 9;			//проныра
const int SPELL_SUBSTITUTE_FEAT = 10;	//подмена заклинания
const int POWER_ATTACK_FEAT = 11;		//мощная атака
const int WOODEN_SKIN_FEAT = 12;		//деревянная кожа
const int IRON_SKIN_FEAT = 13;			//железная кожа
const int CONNOISEUR_FEAT = 14;			//знаток
const int EXORCIST_FEAT = 15;			//изгоняющий нежить
const int HEALER_FEAT = 16;				//целитель
const int LIGHTING_REFLEX_FEAT = 17;	//мгновенная реакция
const int DRUNKARD_FEAT = 18;			//пьяница
const int POWER_MAGIC_FEAT = 19;		//мощь колдовства
const int ENDURANCE_FEAT = 20;			//выносливость
const int GREAT_FORTITUDE_FEAT = 21;	//сила духа
const int FAST_REGENERATION_FEAT = 22;	//быстрое заживление
const int STEALTHY_FEAT = 23;			//незаметность
const int RELATED_TO_MAGIC_FEAT = 24;	//магическое родство
const int SPLENDID_HEALTH_FEAT = 25;	//богатырское здоровье
const int TRACKER_FEAT = 26;			//следопыт
const int WEAPON_FINESSE_FEAT = 27;		//ловкий удар
const int COMBAT_CASTING_FEAT = 28;		//боевое чародейство
const int PUNCH_MASTER_FEAT = 29;		//кулачный боец
const int CLUBS_MASTER_FEAT = 30;		//мастер палицы
const int AXES_MASTER_FEAT = 31;		//мастер секиры
const int LONGS_MASTER_FEAT = 32;		//мастер меча
const int SHORTS_MASTER_FEAT = 33;		//мастер кинжала
const int NONSTANDART_MASTER_FEAT = 34;	//необычное оружие
const int BOTHHANDS_MASTER_FEAT = 35;	//мастер двуручника
const int PICK_MASTER_FEAT = 36;		//мастер ножа
const int SPADES_MASTER_FEAT = 37;		//мастер копья
const int BOWS_MASTER_FEAT = 38;		//мастер стрельбы
const int FOREST_PATHS_FEAT = 39;		//лесные тропы
const int MOUNTAIN_PATHS_FEAT = 40;		//горные тропы
const int LUCKY_FEAT = 41;				//счастливчик
const int SPIRIT_WARRIOR_FEAT = 42;		//боевой дух
const int RELIABLE_HEALTH_FEAT = 43;	//крепкое здоровье
const int EXCELLENT_MEMORY_FEAT = 44;	//превосходная память
const int ANIMAL_DEXTERY_FEAT = 45;		//звериная прыть
const int LEGIBLE_WRITTING_FEAT = 46;	//четкий почерк
const int IRON_MUSCLES_FEAT = 47;		//стальные мышцы
const int MAGIC_SIGN_FEAT = 48;			//знак чародея
const int GREAT_ENDURANCE_FEAT = 49;	//двужильность
const int BEST_DESTINY_FEAT = 50;		//лучшая доля
const int BREW_POTION_FEAT = 51;		//травник
const int JUGGLER_FEAT = 52;			//жонглер
const int NIMBLE_FINGERS_FEAT = 53;		//ловкач
const int GREAT_POWER_ATTACK_FEAT = 54;	//улучшенная мощная атака
const int IMMUNITY_FEAT = 55;			//привычка к яду
const int MOBILITY_FEAT = 56;			//подвижность
const int NATURAL_STRENGTH_FEAT = 57;	//силач
const int NATURAL_DEXTERY_FEAT = 58;	//проворство
const int NATURAL_INTELLECT_FEAT = 59;	//природный ум
const int NATURAL_WISDOM_FEAT = 60;		//мудрец
const int NATURAL_CONSTITUTION_FEAT = 61;//здоровяк
const int NATURAL_CHARISMA_FEAT = 62;	//природное обаяние
const int MNEMONIC_ENHANCER_FEAT = 63;	//отличная память
const int MAGNETIC_PERSONALITY_FEAT = 64;//предводитель
const int DAMROLL_BONUS_FEAT = 65;		//тяжел на руку
const int HITROLL_BONUS_FEAT = 66;		//твердая рука
const int MAGICAL_INSTINCT_FEAT = 67;	//магическое чутье
const int PUNCH_FOCUS_FEAT = 68;		//любимое оружие: голые руки
const int CLUB_FOCUS_FEAT = 69;			//любимое оружие: палица
const int AXES_FOCUS_FEAT = 70;			//любимое оружие: секира
const int LONGS_FOCUS_FEAT = 71;		//любимое оружие: меч
const int SHORTS_FOCUS_FEAT = 72;		//любимое оружие: нож
const int NONSTANDART_FOCUS_FEAT = 73;	//любимое оружие: необычное
const int BOTHHANDS_FOCUS_FEAT = 74;	//любимое оружие: двуручник
const int PICK_FOCUS_FEAT = 75;			//любимое оружие: кинжал
const int SPADES_FOCUS_FEAT = 76;		//любимое оружие: копье
const int BOWS_FOCUS_FEAT = 77;			//любимое оружие: лук
const int AIMING_ATTACK_FEAT = 78;		//прицельная атака
const int GREAT_AIMING_ATTACK_FEAT = 79;//улучшенная прицельная атака
const int DOUBLESHOT_FEAT = 80;			//двойной выстрел
const int PORTER_FEAT = 81;				//тяжеловоз
const int SECRET_RUNES_FEAT = 82;		//тайные руны
/*
Удалены, номера можно использовать.
const int UNUSED_FEAT = 83;
const int UNUSED_FEAT = 84;
const int UNUSED_FEAT = 85;
*/
const int TO_FIT_ITEM_FEAT = 86;		//подгонка
const int TO_FIT_CLOTHCES_FEAT = 87;	//перешить
const int STRENGTH_CONCETRATION_FEAT = 88; // концентрация силы
const int DARK_READING_FEAT = 89;		// кошачий глаз
const int SPELL_CAPABLE_FEAT = 90;		// зачаровать
const int ARMOR_LIGHT_FEAT = 91;		// ношение легкого типа доспехов
const int ARMOR_MEDIAN_FEAT = 92;		// ношение среднего типа доспехов
const int ARMOR_HEAVY_FEAT = 93;		// ношение тяжелого типа доспехов
const int GEMS_INLAY_FEAT = 94;			// инкрустация камнями (TODO: не используется)
const int WARRIOR_STR_FEAT = 95;		// богатырская сила
const int RELOCATE_FEAT = 96;			// переместиться
const int SILVER_TONGUED_FEAT = 97;		//сладкоречие
const int BULLY_FEAT = 98;				//забияка
const int THIEVES_STRIKE_FEAT = 99;		//воровской удар
const int MASTER_JEWELER_FEAT = 100;	//искусный ювелир
const int SKILLED_TRADER_FEAT = 101;	//торговая сметка
const int ZOMBIE_DROVER_FEAT = 102;		//погонщик умертвий
const int EMPLOYER_FEAT = 103;			//навык найма
const int MAGIC_USER_FEAT = 104;		//использование амулетов
const int GOLD_TONGUE_FEAT = 105;		//златоуст
const int CALMNESS_FEAT = 106;			//хладнокровие
const int RETREAT_FEAT = 107;			//отступление
const int SHADOW_STRIKE_FEAT = 108;		//танцующая тень
const int THRIFTY_FEAT = 109;			//запасливость
const int CYNIC_FEAT = 110;				//циничность
const int PARTNER_FEAT = 111;			//напарник
const int HELPDARK_FEAT = 112;			//помощь тьмы
const int FURYDARK_FEAT = 113;			//ярость тьмы
const int DARKREGEN_FEAT = 114;			//темное восстановление
const int SOULLINK_FEAT = 115;			//родство душ
const int STRONGCLUTCH_FEAT = 116;		//сильная хватка
const int MAGICARROWS_FEAT = 117;		//магические стрелы
const int COLLECTORSOULS_FEAT = 118;	//коллекционер душ
const int DARKDEAL_FEAT = 119;			//темная сделка
const int DECLINE_FEAT = 120;			//порча
const int HARVESTLIFE_FEAT = 121;		//жатва жизни
const int LOYALASSIST_FEAT = 122;		//верный помощник
const int HAUNTINGSPIRIT_FEAT = 123;	//блуждающий дух
const int SNEAKRAGE_FEAT = 124;			//ярость змеи
const int HORSE_STUN = 125;				//ошеломить на лошади
const int ELDER_TASKMASTER_FEAT = 126;	//Старший надсмотрщик
const int LORD_UNDEAD_FEAT = 127;		//Повелитель нежити
const int DARK_WIZARD_FEAT = 128;		//Темный маг
const int ELDER_PRIEST_FEAT = 129;		//Старший жрец
const int HIGH_LICH_FEAT = 130;			//Верховный лич
const int BLACK_RITUAL_FEAT = 131;		//Темный ритуал
const int TEAMSTER_UNDEAD_FEAT = 132;	//Погонщик нежити
const int SKIRMISHER_FEAT = 133;		//Держать строй
const int TACTICIAN_FEAT = 134;			//Атаман
const int LIVE_SHIELD_FEAT = 135;		//мультиреск

/*
//новвоведения для татя

const int RESERVED_FEAT_1 = 132;		//
const int RESERVED_FEAT_2 = 133;		//
const int RESERVED_FEAT_3 = 134;		//
const int RESERVED_FEAT_4 = 135;		//
const int RESERVED_FEAT_5 = 136;		//
const int RESERVED_FEAT_6 = 137;		//
*/

const int EVASION_FEAT = 138;			//скользский тип
const int EXPEDIENT_CUT_FEAT = 139;		//порез
const int SHOT_FINESSE_FEAT = 140;		//ловкий выстрел
const int OBJECT_ENCHANTER_FEAT = 141;	//зачаровывание предметов
const int DEFT_SHOOTER_FEAT = 142;		//ловкий стрелок
const int MAGIC_SHOOTER_FEAT = 143;		//магический выстрел
const int THROW_WEAPON_FEAT = 144;		//метнуть
const int SHADOW_THROW_FEAT = 145;		//змеево оружие
const int SHADOW_DAGGER_FEAT = 146;		//змеев кинжал
const int SHADOW_SPEAR_FEAT = 147;		//змеево копье
const int SHADOW_CLUB_FEAT = 148;		//змеева палица
const int DOUBLE_THROW_FEAT = 149;		//двойной бросок
const int TRIPLE_THROW_FEAT = 150;		//тройной бросок
const int POWER_THROW_FEAT = 151;		//размах
const int DEADLY_THROW_FEAT = 152;		//широкий размах
const int TURN_UNDEAD_FEAT = 153;		//затычка для "изгнать нежить"
const int MULTI_CAST_FEAT = 154;		//уменьшение штрафа за число циелей для масскастов
const int MAGICAL_SHIELD_FEAT = 155;	//заговоренный щит" - фит для витязей, защита от директ спеллов
const int ANIMAL_MASTER_FEAT = 156;		//хозяин животных


// kMaxFeats определяется в structs.h

const int UNUSED_FTYPE = -1;
const int NORMAL_FTYPE = 0;
const int AFFECT_FTYPE = 1;
const int SKILL_MOD_FTYPE = 2;
const int ACTIVATED_FTYPE = 3;
const int TECHNIQUE_FTYPE = 4;

/* Константы и формулы, определяющие число способностей у персонажа
   Скорость появления новых слотов можно задавать для каждого класса
      индивидуально, но последний слот персонаж всегда получает на 28 уровне */

//Раз в сколько ремортов появляется новый слот под способность

const int feat_slot_for_remort[kNumPlayerClasses] = {5, 6, 4, 4, 4, 4, 6, 6, 6, 4, 4, 4, 4, 5};
// Количество пар "параметр-значение" у способности
const int kMaxFeatAffect = 5;
// Максимально доступное на морте количество не-врожденных способностей
#define MAX_ACC_FEAT(ch)    ((int) 1+(kLvlImmortal-1)*(5+GET_REMORT(ch)/feat_slot_for_remort[(int) GET_CLASS(ch)])/28)

// Поля изменений для способностей (кроме AFFECT_FTYPE, для них используются стардартные поля APPLY)
const int FEAT_TIMER = 1;
const int FEAT_SKILL = 2;

struct TimedFeat {
	int feat{INCORRECT_FEAT};	// Used feature //
	ubyte time{0};				// Time for next using //
	struct TimedFeat *next{nullptr};
};

extern struct FeatureInfo feat_info[kMaxFeats];

const char *GetFeatName(int num);
int GetModifier(int feat, int location);
int FindFeatNum(const char *name, bool alias = false);
void InitFeatures();
void CheckBerserk(CharData *ch);
void SetRaceFeats(CharData *ch);
void UnsetRaceFeats(CharData *ch);
void SetInbornFeats(CharData *ch);
bool can_use_feat(const CharData *ch, int feat);
bool can_get_feat(CharData *ch, int feat);
bool TryFlipActivatedFeature(CharData *ch, char *argument);
Bitvector GetPrfWithFeatNumber(int feat_num);

/*
	Класс для удобства вбивания значений в массив affected структуры способности
*/
class CFeatArray {
 public:
	explicit CFeatArray() :
		pos_{0} {}

	int pos(int pos = -1);
	void insert(int location, int modifier);
	void clear();

	struct CFeatAffect {
		int location;
		int modifier;

		CFeatAffect() :
			location{APPLY_NONE}, modifier{0} {}

		CFeatAffect(int location, int modifier) :
			location(location), modifier(modifier) {}

		bool operator!=(const CFeatAffect &r) const {
			return (location != r.location || modifier != r.modifier);
		}
		bool operator==(const CFeatAffect &r) const {
			return !(*this != r);
		}
	};

	struct CFeatAffect affected[kMaxFeatAffect];

 private:
	int pos_;
};

struct FeatureInfo {
	int id;
	int type;
	int min_remort[kNumPlayerClasses][kNumKins];
	int slot[kNumPlayerClasses][kNumKins];
	bool is_known[kNumPlayerClasses][kNumKins];
	bool is_inborn[kNumPlayerClasses][kNumKins];
	bool up_slot;
	bool uses_weapon_skill;
	bool always_available;
	const char *name;
	std::array<CFeatArray::CFeatAffect, kMaxFeatAffect> affected;
	std::string alias;
	// Параметры для нового базового броска на способность
	// Пока тут, до переписывания системы способностей
	int damage_bonus;
	int success_degree_damage_bonus;
	int diceroll_bonus;
	int critfail_threshold;
	int critsuccess_threshold;
	ESaving saving;
	ESkill base_skill;

	TechniqueItemKitsGroup item_kits;

	int (*GetBaseParameter)(const CharData *ch);
	int (*GetEffectParameter)(const CharData *ch);
	float (*CalcSituationalDamageFactor)(CharData * /* ch */);
	int (*CalcSituationalRollBonus)(CharData * /* ch */, CharData * /* enemy */);
};

#endif // __FEATURES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
