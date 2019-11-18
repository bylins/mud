/*************************************************************************
*   File: features.hpp                                 Part of Bylins    *
*   Features code header                                                 *
*                                                                        *
*  $Author$                                                     *
*  $Date$                                          *
*  $Revision$                                                  *
**************************************************************************/
#ifndef __FEATURES_HPP__
#define __FEATURES_HPP__

#include "features.itemset.hpp"
#include "skills.h"
#include "structs.h"
#include "conf.h"

#include <array>
#include <bitset>

using std::bitset;

#define INCORRECT_FEAT			0   //DO NOT USE THIS VALUE
#define BERSERK_FEAT			1   //предсмертная ярость
#define PARRY_ARROW_FEAT		2   //отбить стрелу
#define BLIND_FIGHT_FEAT		3   //слепой бой
#define IMPREGNABLE_FEAT		4   //непробиваемый
#define APPROACHING_ATTACK_FEAT	5   //встречная атака
#define DEFENDER_FEAT			6   //щитоносец
#define DODGER_FEAT				7   //изворотливость
#define LIGHT_WALK_FEAT			8   //легкая поступь
#define WRIGGLER_FEAT			9   //проныра
#define SPELL_SUBSTITUTE_FEAT	10  //подмена заклинания
#define POWER_ATTACK_FEAT		11  //мощная атака
#define WOODEN_SKIN_FEAT		12  //деревянная кожа
#define IRON_SKIN_FEAT			13  //железная кожа
#define CONNOISEUR_FEAT			14  //знаток
#define EXORCIST_FEAT			15  //изгоняющий нежить
#define HEALER_FEAT				16  //целитель
#define LIGHTING_REFLEX_FEAT	17  //мгновенная реакция
#define DRUNKARD_FEAT			18  //пьяница
#define POWER_MAGIC_FEAT		19  //мощь колдовства
#define ENDURANCE_FEAT			20  //выносливость
#define GREAT_FORTITUDE_FEAT	21  //сила духа
#define FAST_REGENERATION_FEAT	22  //быстрое заживление
#define STEALTHY_FEAT			23  //незаметность
#define RELATED_TO_MAGIC_FEAT	24  //магическое родство
#define SPLENDID_HEALTH_FEAT	25  //богатырское здоровье
#define TRACKER_FEAT			26  //следопыт
#define WEAPON_FINESSE_FEAT		27  //ловкий удар
#define COMBAT_CASTING_FEAT		28  //боевое чародейство
#define PUNCH_MASTER_FEAT		29  //кулачный боец
#define CLUBS_MASTER_FEAT		30  //мастер палицы
#define AXES_MASTER_FEAT		31  //мастер секиры
#define LONGS_MASTER_FEAT		32  //мастер меча
#define SHORTS_MASTER_FEAT		33  //мастер кинжала
#define NONSTANDART_MASTER_FEAT		34  //необычное оружие
#define BOTHHANDS_MASTER_FEAT		35  //мастер двуручника
#define PICK_MASTER_FEAT		36  //мастер ножа
#define SPADES_MASTER_FEAT		37  //мастер копья
#define BOWS_MASTER_FEAT		38  //мастер стрельбы
#define FOREST_PATHS_FEAT		39  //лесные тропы
#define MOUNTAIN_PATHS_FEAT		40  //горные тропы
#define LUCKY_FEAT			41  //счастливчик
#define SPIRIT_WARRIOR_FEAT		42  //боевой дух
#define RELIABLE_HEALTH_FEAT		43  //крепкое здоровье
#define EXCELLENT_MEMORY_FEAT		44  //превосходная память
#define ANIMAL_DEXTERY_FEAT		45  //звериная прыть
#define LEGIBLE_WRITTING_FEAT		46  //четкий почерк
#define IRON_MUSCLES_FEAT		47  //стальные мышцы
#define MAGIC_SIGN_FEAT			48  //знак чародея
#define GREAT_ENDURANCE_FEAT		49  //двужильность
#define BEST_DESTINY_FEAT		50  //лучшая доля
#define BREW_POTION_FEAT		51  //травник
#define JUGGLER_FEAT			52  //жонглер
#define NIMBLE_FINGERS_FEAT		53  //ловкач
#define GREAT_POWER_ATTACK_FEAT		54  //улучшенная мощная атака
#define IMMUNITY_FEAT			55  //привычка к яду
#define MOBILITY_FEAT			56  //подвижность
#define NATURAL_STRENGTH_FEAT		57  //силач
#define NATURAL_DEXTERY_FEAT		58  //проворство
#define NATURAL_INTELLECT_FEAT		59  //природный ум
#define NATURAL_WISDOM_FEAT		60  //мудрец
#define NATURAL_CONSTITUTION_FEAT	61  //здоровяк
#define NATURAL_CHARISMA_FEAT		62  //природное обаяние
#define MNEMONIC_ENHANCER_FEAT		63  //отличная память
#define MAGNETIC_PERSONALITY_FEAT	64  //предводитель
#define DAMROLL_BONUS_FEAT		65  //тяжел на руку
#define HITROLL_BONUS_FEAT		66  //твердая рука
#define MAGICAL_INSTINCT_FEAT		67  //магическое чутье
#define PUNCH_FOCUS_FEAT		68  //любимое оружие: голые руки
#define CLUB_FOCUS_FEAT			69  //любимое оружие: палица
#define AXES_FOCUS_FEAT			70  //любимое оружие: секира
#define LONGS_FOCUS_FEAT		71  //любимое оружие: меч
#define SHORTS_FOCUS_FEAT		72  //любимое оружие: нож
#define NONSTANDART_FOCUS_FEAT		73  //любимое оружие: необычное
#define BOTHHANDS_FOCUS_FEAT		74  //любимое оружие: двуручник
#define PICK_FOCUS_FEAT			75  //любимое оружие: кинжал
#define SPADES_FOCUS_FEAT		76  //любимое оружие: копье
#define BOWS_FOCUS_FEAT			77  //любимое оружие: лук
#define AIMING_ATTACK_FEAT		78  //прицельная атака
#define GREAT_AIMING_ATTACK_FEAT	79  //улучшенная прицельная атака
#define DOUBLESHOT_FEAT			80  //двойной выстрел
#define PORTER_FEAT			81  //тяжеловоз
#define RUNE_NEWBIE_FEAT		82  //толкователь рун
#define RUNE_USER_FEAT			83  //тайные руны
#define RUNE_MASTER_FEAT		84  //заветные руны
#define RUNE_ULTIMATE_FEAT		85  //руны богов
#define TO_FIT_ITEM_FEAT		86  //подгонка
#define TO_FIT_CLOTHCES_FEAT		87  //перешить
#define STRENGTH_CONCETRATION_FEAT	88 // концентрация силы
#define DARK_READING_FEAT		89 // кошачий глаз
#define SPELL_CAPABLE_FEAT		90 // зачаровать
#define ARMOR_LIGHT_FEAT		91 // ношение легкого типа доспехов
#define ARMOR_MEDIAN_FEAT		92 // ношение среднего типа доспехов
#define ARMOR_HEAVY_FEAT		93 // ношение тяжелого типа доспехов
#define GEMS_INLAY_FEAT			94 // инкрустация камнями (TODO: не используется)
#define WARRIOR_STR_FEAT		95 // богатырская сила
#define RELOCATE_FEAT			96 // переместиться
#define SILVER_TONGUED_FEAT		97 //сладкоречие
#define BULLY_FEAT				98 //забияка
#define THIEVES_STRIKE_FEAT		99 //воровской удар
#define MASTER_JEWELER_FEAT		100 //искусный ювелир
#define SKILLED_TRADER_FEAT		101 //торговая сметка
#define ZOMBIE_DROVER_FEAT		102 //погонщик умертвий
#define EMPLOYER_FEAT			103 //навык найма
#define MAGIC_USER_FEAT			104 //использование амулетов
#define GOLD_TONGUE_FEAT		105 //златоуст
#define CALMNESS_FEAT			106 //хладнокровие
#define RETREAT_FEAT			107 //отступление
#define SHADOW_STRIKE_FEAT		108 //танцующая тень
#define	THRIFTY_FEAT			109 //запасливость
#define CYNIC_FEAT			    110 // циничность
#define PARTNER_FEAT			111 // напарник
#define HELPDARK_FEAT			112 // помощь тьмы
#define FURYDARK_FEAT           113 // ярость тьмы
#define DARKREGEN_FEAT			114 // темное восстановление
#define SOULLINK_FEAT			115 // родство душ
#define STRONGCLUTCH_FEAT		116 // сильная хватка
#define MAGICARROWS_FEAT		117 // магические стрелы
#define COLLECTORSOULS_FEAT		118 // коллекционер душ
#define DARKDEAL_FEAT			119 // темная сделка
#define DECLINE_FEAT			120 // порча
#define HARVESTLIFE_FEAT		121 // жатва жизни
#define LOYALASSIST_FEAT		122 // верный помощник
#define HAUNTINGSPIRIT_FEAT		123 // блуждающий дух
#define SNEAKRAGE_FEAT			124 // ярость змеи
#define HORSE_STUN				125 // ошеломить на лошади
#define ELDER_TASKMASTER_FEAT	126 // Старший надсмотрщик
#define LORD_UNDEAD_FEAT		127 // Повелитель нежити
#define DARK_WIZARD_FEAT		128 // Темный маг
#define ELDER_PRIEST_FEAT		129 // Старший жрец
#define HIGH_LICH_FEAT			130 // Верховный лич
#define BLACK_RITUAL_FEAT		131 // Темный ритуал
#define TEAMSTER_UNDEAD_FEAT	132 // Погонщик нежити
#define SKIRMISHER_FEAT			133 // Держать строй
#define TACTICIAN_FEAT			134 // Атаман
#define LIVE_SHIELD_FEAT		135 // мультиреск

/*
//новвоведения для татя

#define RESERVED_FEAT_1			132 //
#define RESERVED_FEAT_2			133 //
#define RESERVED_FEAT_3			134 //
#define RESERVED_FEAT_4			135 //
#define RESERVED_FEAT_5			136 //
#define RESERVED_FEAT_6			137 //
*/

#define EVASION_FEAT            138 // скользский тип
#define EXPEDIENT_CUT_FEAT      139 // порез
#define SHOT_FINESSE_FEAT		140 // ловкий выстрел
#define OBJECT_ENCHANTER_FEAT	141 // зачаровывание предметов
#define DEFT_SHOOTER_FEAT		142 // ловкий стрелок
#define MAGIC_SHOOTER_FEAT		143 // магический выстрел
#define THROW_WEAPON_FEAT		144 // метнуть
#define SHADOW_THROW_FEAT		145 // змеево оружие
#define SHADOW_DAGGER_FEAT		146 // змеев кинжал
#define SHADOW_SPEAR_FEAT		147 // змеево копье
#define SHADOW_CLUB_FEAT		148 // змеева палица
#define DOUBLE_THROW_FEAT		149 // двойной бросок
#define TRIPLE_THROW_FEAT		150 // тройной бросок
#define POWER_THROW_FEAT		151 // размах
#define DEADLY_THROW_FEAT		152 // широкий размах
#define TURN_UNDEAD_FEAT		153 // затычка для "изгнать нежить"
#define MULTI_CAST_FEAT			154 // уменьшение штрафа за число циелей для масскастов


// MAX_FEATS определяется в structs.h

#define UNUSED_FTYPE			-1
#define NORMAL_FTYPE			0
#define AFFECT_FTYPE			1
#define SKILL_MOD_FTYPE			2
#define ACTIVATED_FTYPE			3
#define TECHNIQUE_FTYPE			4

/* Константы и формулы, определяющие число способностей у персонажа
   Скорость появления новых слотов можно задавать для каждого класса
      индивидуально, но последний слот персонаж всегда получает на 28 уровне */

//Раз в сколько ремортов появляется новый слот под способность

const int feat_slot_for_remort[NUM_PLAYER_CLASSES] = { 5,6,4,4,4,4,6,6,6,4,4,4,4,5 };
// Количество пар "параметр-значение" у способности
const short MAX_FEAT_AFFECT	= 5;
// Максимально доступное на морте количество не-врожденных способностей
#define MAX_ACC_FEAT(ch)	((int) 1+(LVL_IMMORT-1)*(5+GET_REMORT(ch)/feat_slot_for_remort[(int) GET_CLASS(ch)])/28)

// Поля изменений для способностей (кроме AFFECT_FTYPE, для них используются стардартные поля APPLY)
#define FEAT_TIMER 1
#define FEAT_SKILL 2

extern struct FeatureInfoType feat_info[MAX_FEATS];

int getModifier(int feat, int location);
int find_feat_num(const char *name, bool alias = false);
void determineFeaturesSpecification(void);
void check_berserk(CHAR_DATA * ch);
void setFeaturesOfRace(CHAR_DATA *ch);
void unsetFeaturesOfRace(CHAR_DATA *ch);
void setAllInbornFeatures(CHAR_DATA *ch);
bool can_use_feat(const CHAR_DATA *ch, int feat);
bool can_get_feat(CHAR_DATA *ch, int feat);
bool tryFlipActivatedFeature(CHAR_DATA *ch, char *argument);
bitvector_t getPRFWithFeatureNumber(int featureNum);

/*
	Класс для удобства вбивания значений в массив affected структуры способности
*/
class CFeatArray
{
public:
	explicit CFeatArray() : _pos(0), i(MAX_FEAT_AFFECT) {}

	int pos(int pos = -1);
	void insert(const int location, sbyte modifier);
	void clear();

	struct CFeatAffect
	{
		int location;
		int modifier;

		CFeatAffect() : location(APPLY_NONE), modifier(0) {}

		CFeatAffect(int __location, int __modifier)
			: location(__location), modifier(__modifier) {}

		bool operator!=(const CFeatAffect &r) const
		{
			return (location != r.location || modifier != r.modifier);
		}
		bool operator==(const CFeatAffect &r) const
		{
			return !(*this != r);
		}
	};

	struct CFeatAffect affected[MAX_FEAT_AFFECT];

private:
	int _pos, i;
};

struct FeatureInfoType {
	int ID;
	int type;
	int minRemort[NUM_PLAYER_CLASSES][NUM_KIN];
	int slot[NUM_PLAYER_CLASSES][NUM_KIN];
	bool classknow[NUM_PLAYER_CLASSES][NUM_KIN];
	bool inbornFeatureOfClass[NUM_PLAYER_CLASSES][NUM_KIN];
	bool up_slot;
	bool usesWeaponSkill;
	bool alwaysAvailable;
	const char *name;
	std::array<CFeatArray::CFeatAffect, MAX_FEAT_AFFECT> affected;
	std::string alias;
	// Параметры для нового базового броска на способность
	// Пока тут, до переписывания системы способностей
	short baseDamageBonusPercent;
	short degreeOfSuccessDamagePercent;
	short dicerollBonus;
	short oppositeSaving;
	short criticalFailThreshold;
	short criticalSuccessThreshold;
	ESkill baseSkill;

	TechniqueItemKitsGroupType techniqueItemKitsGroup;

	int (*getBaseParameter) (const CHAR_DATA* ch);
	int (*getEffectParameter) (const CHAR_DATA* ch);
	float (*calculateSituationalDamageFactor) (CHAR_DATA* /* ch */);
	short (*calculateSituationalRollBonus) (CHAR_DATA* /* ch */, CHAR_DATA* /* enemy */);
};

#endif // __FEATURES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
