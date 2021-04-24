#include "skills.info.h"

const char *unused_skillname = "!UNUSED!";
const char *unidentified_skillname = "!UNIDENTIFIED!";

struct skillInfo_t skill_info[MAX_SKILL_NUM + 1];

const char *skill_name(int num);
void unused_skill(int skill);
void initSkill(int skill, const char *name, const char *shortName, int max_percent);
void initSkills(void);

const char *skill_name(int num) {
	if (num > 0 && num <= MAX_SKILL_NUM) {
		return (skill_info[num].name);
	} else {
		if (num == -1) {
			return unused_skillname;
		} else {
			return unidentified_skillname;
		}
	}
}

void initUnusedSkill(int skill) {
	int i, j;

	for (i = 0; i < NUM_PLAYER_CLASSES; i++) {
		for (j = 0; j < NUM_KIN; j++) {
			skill_info[skill].min_remort[i][j] = MAX_REMORT;
			skill_info[skill].min_level[i][j] = 0;
			skill_info[skill].k_improve[i][j] = 0 ;
		}
	}
	skill_info[skill].min_position = 0;
	skill_info[skill].max_percent = 200;
	skill_info[skill].name = unused_skillname;
}

void initSkill(int skill, const char *name, const char *shortName, int max_percent) {
	int i, j;
	for (i = 0; i < NUM_PLAYER_CLASSES; i++) {
		for (j = 0; j < NUM_KIN; j++) {
			skill_info[skill].min_remort[i][j] = MAX_REMORT;
			skill_info[skill].min_level[i][j] = 0 ;
			skill_info[skill].k_improve[i][j] = 0;
		}
	}
	skill_info[skill].min_position = 0;
	skill_info[skill].name = name;
	skill_info[skill].shortName = shortName;
	skill_info[skill].max_percent = max_percent;
}

void initSkills(void) {

	for (int i = 0; i <= MAX_SKILL_NUM; i++) {
		initUnusedSkill(i);
	}

	initSkill(SKILL_GLOBAL_COOLDOWN, "!глобальная задержка", "ОЗ",  1);
	initSkill(SKILL_BACKSTAB, "заколоть", "Зк", 180);
	initSkill(SKILL_BASH, "сбить", "Сб", 200);
	initSkill(SKILL_HIDE, "спрятаться", "Сп", 100);
	initSkill(SKILL_KICK, "пнуть", "Пн", 100);
	initSkill(SKILL_PICK_LOCK, "взломать", "Вз", 120);
	initSkill(SKILL_PUNCH, "кулачный бой", "Кб", 100);
	initSkill(SKILL_RESCUE, "спасти", "Спс", 130);
	initSkill(SKILL_SNEAK, "подкрасться", "Пд", 100);
	initSkill(SKILL_STEAL, "украсть", "Ук", 100);
	initSkill(SKILL_TRACK, "выследить", "Выс", 100);
	initSkill(SKILL_PARRY, "парировать", "Пр", 120);
	initSkill(SKILL_BLOCK, "блокировать щитом", "Бщ", 200);
	initSkill(SKILL_TOUCH, "перехватить атаку", "Пр", 100);
	initSkill(SKILL_PROTECT, "прикрыть", "Пр", 120);
	initSkill(SKILL_BOTHHANDS, "двуручники", "Дв", 160);
	initSkill(SKILL_LONGS, "длинные лезвия", "Дл", 160);
	initSkill(SKILL_SPADES, "копья и рогатины", "Кр", 160);
	initSkill(SKILL_SHORTS, "короткие лезвия", "Кл", 160);
	initSkill(SKILL_BOWS, "луки", "Лк", 160);
	initSkill(SKILL_CLUBS, "палицы и дубины", "Пд", 160);
	initSkill(SKILL_PICK, "проникающее оружие", "Прн", 160);
	initSkill(SKILL_NONSTANDART, "иное оружие", "Ин", 160);
	initSkill(SKILL_AXES, "секиры", "Ск", 160);
	initSkill(SKILL_SATTACK, "атака левой рукой", "Ал", 100);
	initSkill(SKILL_LOOKING, "приглядеться", "Прг", 100);
	initSkill(SKILL_HEARING, "прислушаться", "Прс", 100);
	initSkill(SKILL_DISARM, "обезоружить", "Об", 100);
	initSkill(SKILL_HEAL, "!heal!", "Hl", 100);
	initSkill(SKILL_MORPH, "оборотничество", "Об", 150);
	initSkill(SKILL_ADDSHOT, "дополнительный выстрел", "Доп", 200);
	initSkill(SKILL_CAMOUFLAGE, "маскировка", "Мск", 100);
	initSkill(SKILL_DEVIATE, "уклониться", "Укл", 100);
	initSkill(SKILL_CHOPOFF, "подножка", "Пд", 200);
	initSkill(SKILL_REPAIR, "ремонт", "Рм", 180);
	initSkill(SKILL_COURAGE, "ярость", "Яр", 100);
	initSkill(SKILL_IDENTIFY, "опознание", "Оп", 100);
	initSkill(SKILL_LOOK_HIDE, "подсмотреть", "Пдм", 100);
	initSkill(SKILL_UPGRADE, "заточить", "Зат", 100);
	initSkill(SKILL_ARMORED, "укрепить", "Укр", 100);
	initSkill(SKILL_DRUNKOFF, "опохмелиться", "Опх", 100);
	initSkill(SKILL_AID, "лечить", "Лч", 100);
	initSkill(SKILL_FIRE, "разжечь костер", "Рк", 160);
	initSkill(SKILL_SHIT, "удар левой рукой", "Улр", 100);
	initSkill(SKILL_MIGHTHIT, "богатырский молот", "Бм", 200);
	initSkill(SKILL_STUPOR, "оглушить", "Ог", 200);
	initSkill(SKILL_POISONED, "отравить", "Отр", 200);
	initSkill(SKILL_LEADERSHIP, "лидерство", "Лд", 100);
	initSkill(SKILL_PUNCTUAL, "точный стиль", "Тс", 110);
	initSkill(SKILL_AWAKE, "осторожный стиль", "Ос", 100);
	initSkill(SKILL_SENSE, "найти", "Нйт", 160);
	initSkill(SKILL_HORSE, "сражение верхом", "Срв", 100);
	initSkill(SKILL_HIDETRACK, "замести следы", "Зс", 120);
	initSkill(SKILL_RELIGION, "!молитва или жертва!", "Error", 1);
	initSkill(SKILL_MAKEFOOD, "освежевать", "Осв", 120);
	initSkill(SKILL_MULTYPARRY, "веерная защита", "Вз", 140);
	initSkill(SKILL_TRANSFORMWEAPON, "перековать", "Прк", 140);
	initSkill(SKILL_THROW, "метнуть", "Мт", 150);
	initSkill(SKILL_MAKE_BOW, "смастерить лук", "Сл", 140);
	initSkill(SKILL_MAKE_WEAPON, "выковать оружие", "Вкр", 140);
	initSkill(SKILL_MAKE_ARMOR, "выковать доспех", "Вкд", 140);
	initSkill(SKILL_MAKE_WEAR, "сшить одежду", "Сш", 140);
	initSkill(SKILL_MAKE_JEWEL, "смастерить диковинку", "Сд", 140);
	initSkill(SKILL_MANADRAIN, "сглазить", "Сг", 100);
	initSkill(SKILL_NOPARRYHIT, "скрытый удар", "Сд", 100);
	initSkill(SKILL_TOWNPORTAL, "врата", "Вр", 100);
	initSkill(SKILL_DIG, "горное дело", "Гд", 100);
	initSkill(SKILL_INSERTGEM, "ювелир", "Юв", 100);
	initSkill(SKILL_WARCRY, "боевой клич", "Бк", 100);
	initSkill(SKILL_TURN_UNDEAD, "изгнать нежить", "Из", 100);
	initSkill(SKILL_IRON_WIND, "железный ветер", "Жв", 150);
	initSkill(SKILL_STRANGLE, "удавить", "Уд", 200);
	initSkill(SKILL_AIR_MAGIC, "магия воздуха", "Ма", 200);
	initSkill(SKILL_FIRE_MAGIC, "магия огня", "Мо", 1000);
	initSkill(SKILL_WATER_MAGIC, "магия воды", "Мвд", 1000);
	initSkill(SKILL_EARTH_MAGIC, "магия земли", "Мз", 1000);
	initSkill(SKILL_LIGHT_MAGIC, "магия света", "Мс", 1000);
	initSkill(SKILL_DARK_MAGIC, "магия тьмы", "Мт", 1000);
	initSkill(SKILL_MIND_MAGIC, "магия разума", "Мр", 1000);
	initSkill(SKILL_LIFE_MAGIC, "магия жизни", "Мж", 1000);
	initSkill(SKILL_STUN, "ошеломить", "Ош", 200);
	initSkill(SKILL_MAKE_AMULET, "смастерить оберег", "Со", 200);
	initSkill(SKILL_INDEFINITE, "!неопределен", "Error", 1);
}
