#include "skills_info.h"

const char *kUnusedSkillName = "!UNUSED!";
const char *kUnidentifiedSkillName = "!UNIDENTIFIED!";
const byte kDefaultMinPosition = 0;
const int kDefaultFailPercent = 200;
const unsigned short kDefaultCap = 200;

struct SkillInfoType skill_info[MAX_SKILL_NUM + 1];

const char *skill_name(int num);
void InitSingleSkill(int skill, const char *name, const char *short_name,
					 int save_type, int difficulty, unsigned short cap);
void InitSkills();

const char *skill_name(int num) {
	if (num > 0 && num <= MAX_SKILL_NUM) {
		return (skill_info[num].name);
	} else {
		if (num == -1) {
			return kUnusedSkillName;
		} else {
			return kUnidentifiedSkillName;
		}
	}
}

void InitUnusedSkill(int skill) {
	int i, j;

	for (i = 0; i < NUM_PLAYER_CLASSES; i++) {
		for (j = 0; j < NUM_KIN; j++) {
			skill_info[skill].min_remort[i][j] = MAX_REMORT;
			skill_info[skill].min_level[i][j] = 0;
			skill_info[skill].k_improve[i][j] = 0;
		}
	}
	skill_info[skill].min_position = kDefaultMinPosition;
	skill_info[skill].save_type = SAVING_REFLEX;
	skill_info[skill].difficulty = kDefaultFailPercent;
	skill_info[skill].cap = kDefaultCap;
	skill_info[skill].name = kUnusedSkillName;
}

void InitSingleSkill(int skill, const char *name, const char *short_name,
					 int save_type, int difficulty, unsigned short cap) {
	int i, j;
	for (i = 0; i < NUM_PLAYER_CLASSES; i++) {
		for (j = 0; j < NUM_KIN; j++) {
			skill_info[skill].min_remort[i][j] = MAX_REMORT;
			skill_info[skill].min_level[i][j] = 0;
			skill_info[skill].k_improve[i][j] = 0;
		}
	}
	skill_info[skill].min_position = 0;
	skill_info[skill].name = name;
	skill_info[skill].shortName = short_name;
	skill_info[skill].save_type = save_type;
	skill_info[skill].difficulty = difficulty;
	skill_info[skill].cap = cap;
}

void InitSkills() {

	for (int i = 0; i <= MAX_SKILL_NUM; i++) {
		InitUnusedSkill(i);
	}

	InitSingleSkill(SKILL_GLOBAL_COOLDOWN, "!global cooldown", "ОЗ", SAVING_REFLEX, 1, 1);
	InitSingleSkill(SKILL_BACKSTAB, "заколоть", "Зк", SAVING_REFLEX, 180, 1000);
	InitSingleSkill(SKILL_BASH, "сбить", "Сб", SAVING_REFLEX, 200, 200);
	InitSingleSkill(SKILL_HIDE, "спрятаться", "Сп", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_KICK, "пнуть", "Пн", SAVING_STABILITY, 100, 200);
	InitSingleSkill(SKILL_PICK_LOCK, "взломать", "Вз", SAVING_REFLEX, 120, 200);
	InitSingleSkill(SKILL_PUNCH, "кулачный бой", "Кб", SAVING_REFLEX, 100, 1000);
	InitSingleSkill(SKILL_RESCUE, "спасти", "Спс", SAVING_REFLEX, 130, 1000);
	InitSingleSkill(SKILL_SNEAK, "подкрасться", "Пд", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_STEAL, "украсть", "Ук", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_TRACK, "выследить", "Выс", SAVING_REFLEX, 100, 1000);
	InitSingleSkill(SKILL_PARRY, "парировать", "Пр", SAVING_REFLEX, 120, 1000);
	InitSingleSkill(SKILL_BLOCK, "блокировать щитом", "Бщ", SAVING_REFLEX, 200, 200);
	InitSingleSkill(SKILL_TOUCH, "перехватить атаку", "Пр", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_PROTECT, "прикрыть", "Пр", SAVING_REFLEX, 120, 200);
	InitSingleSkill(SKILL_BOTHHANDS, "двуручники", "Дв", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_LONGS, "длинные лезвия", "Дл", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_SPADES, "копья и рогатины", "Кр", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_SHORTS, "короткие лезвия", "Кл", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_BOWS, "луки", "Лк", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_CLUBS, "палицы и дубины", "Пд", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_PICK, "проникающее оружие", "Прн", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_NONSTANDART, "иное оружие", "Ин", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_AXES, "секиры", "Ск", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_SATTACK, "атака левой рукой", "Ал", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_LOOKING, "приглядеться", "Прг", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_HEARING, "прислушаться", "Прс", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_DISARM, "обезоружить", "Об", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_HEAL, "!heal!", "Hl", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_MORPH, "оборотничество", "Об", SAVING_REFLEX, 150, 200);
	InitSingleSkill(SKILL_ADDSHOT, "дополнительный выстрел", "Доп", SAVING_REFLEX, 200, 200);
	InitSingleSkill(SKILL_CAMOUFLAGE, "маскировка", "Мск", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_DEVIATE, "уклониться", "Укл", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_CHOPOFF, "подножка", "Пд", SAVING_REFLEX, 200, 200);
	InitSingleSkill(SKILL_REPAIR, "ремонт", "Рм", SAVING_REFLEX, 180, 200);
	InitSingleSkill(SKILL_COURAGE, "ярость", "Яр", SAVING_REFLEX, 100, 1000);
	InitSingleSkill(SKILL_IDENTIFY, "опознание", "Оп", SAVING_REFLEX, 100, 1000);
	InitSingleSkill(SKILL_LOOK_HIDE, "подсмотреть", "Пдм", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_UPGRADE, "заточить", "Зат", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_ARMORED, "укрепить", "Укр", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_DRUNKOFF, "опохмелиться", "Опх", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_AID, "лечить", "Лч", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_FIRE, "разжечь костер", "Рк", SAVING_REFLEX, 160, 1000);
	InitSingleSkill(SKILL_SHIT, "удар левой рукой", "Улр", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_MIGHTHIT, "богатырский молот", "Бм", SAVING_STABILITY, 200, 200);
	InitSingleSkill(SKILL_STUPOR, "оглушить", "Ог", SAVING_STABILITY, 200, 200);
	InitSingleSkill(SKILL_POISONED, "отравить", "Отр", SAVING_REFLEX, 200, 200);
	InitSingleSkill(SKILL_LEADERSHIP, "лидерство", "Лд", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_PUNCTUAL, "точный стиль", "Тс", SAVING_CRITICAL, 110, 200);
	InitSingleSkill(SKILL_AWAKE, "осторожный стиль", "Ос", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_SENSE, "найти", "Нйт", SAVING_REFLEX, 160, 200);
	InitSingleSkill(SKILL_HORSE, "сражение верхом", "Срв", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_HIDETRACK, "замести следы", "Зс", SAVING_REFLEX, 120, 200);
	InitSingleSkill(SKILL_RELIGION, "!молитва или жертва!", "Error", SAVING_REFLEX, 1, 1);
	InitSingleSkill(SKILL_MAKEFOOD, "освежевать", "Осв", SAVING_REFLEX, 120, 200);
	InitSingleSkill(SKILL_MULTYPARRY, "веерная защита", "Вз", SAVING_REFLEX, 140, 200);
	InitSingleSkill(SKILL_TRANSFORMWEAPON, "перековать", "Прк", SAVING_REFLEX, 140, 200);
	InitSingleSkill(SKILL_THROW, "метнуть", "Мт", SAVING_REFLEX, 150, 1000);
	InitSingleSkill(SKILL_MAKE_BOW, "смастерить лук", "Сл", SAVING_REFLEX, 140, 200);
	InitSingleSkill(SKILL_MAKE_WEAPON, "выковать оружие", "Вкр", SAVING_REFLEX, 140, 200);
	InitSingleSkill(SKILL_MAKE_ARMOR, "выковать доспех", "Вкд", SAVING_REFLEX, 140, 200);
	InitSingleSkill(SKILL_MAKE_WEAR, "сшить одежду", "Сш", SAVING_REFLEX, 140, 200);
	InitSingleSkill(SKILL_MAKE_JEWEL, "смастерить диковинку", "Сд", SAVING_REFLEX, 140, 200);
	InitSingleSkill(SKILL_MANADRAIN, "сглазить", "Сг", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_NOPARRYHIT, "скрытый удар", "Сд", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_TOWNPORTAL, "врата", "Вр", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_DIG, "горное дело", "Гд", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_INSERTGEM, "ювелир", "Юв", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_WARCRY, "боевой клич", "Бк", SAVING_REFLEX, 100, 200);
	InitSingleSkill(SKILL_TURN_UNDEAD, "изгнать нежить", "Из", SAVING_REFLEX, 100, 1000);
	InitSingleSkill(SKILL_IRON_WIND, "железный ветер", "Жв", SAVING_REFLEX, 150, 1000);
	InitSingleSkill(SKILL_STRANGLE, "удавить", "Уд", SAVING_REFLEX, 200, 200);
	InitSingleSkill(SKILL_AIR_MAGIC, "магия воздуха", "Ма", SAVING_REFLEX, 200, 1000);
	InitSingleSkill(SKILL_FIRE_MAGIC, "магия огня", "Мо", SAVING_REFLEX, 1000, 1000);
	InitSingleSkill(SKILL_WATER_MAGIC, "магия воды", "Мвд", SAVING_REFLEX, 1000, 1000);
	InitSingleSkill(SKILL_EARTH_MAGIC, "магия земли", "Мз", SAVING_REFLEX, 1000, 1000);
	InitSingleSkill(SKILL_LIGHT_MAGIC, "магия света", "Мс", SAVING_REFLEX, 1000, 1000);
	InitSingleSkill(SKILL_DARK_MAGIC, "магия тьмы", "Мт", SAVING_REFLEX, 1000, 1000);
	InitSingleSkill(SKILL_MIND_MAGIC, "магия разума", "Мр", SAVING_REFLEX, 1000, 1000);
	InitSingleSkill(SKILL_LIFE_MAGIC, "магия жизни", "Мж", SAVING_REFLEX, 1000, 1000);
	InitSingleSkill(SKILL_STUN, "ошеломить", "Ош", SAVING_REFLEX, 200, 200);
	InitSingleSkill(SKILL_MAKE_AMULET, "смастерить оберег", "Со", SAVING_REFLEX, 200, 200);
	InitSingleSkill(SKILL_INDEFINITE, "!неопределен", "Error", SAVING_REFLEX, 1, 1);
}
