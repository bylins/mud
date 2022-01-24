#include "skills_info.h"

const char *kUnusedSkillName = "!UNUSED!";
const char *kUnidentifiedSkillName = "!UNIDENTIFIED!";
const int kDefaultFailPercent = 200;
const unsigned short kDefaultCap = 200;

struct AttackMessages fight_messages[kMaxMessages];
struct SkillInfoType skill_info[MAX_SKILL_NUM + 1];

void InitSingleSkill(int skill, const char *name, const char *short_name,
					 ESaving saving, int difficulty, unsigned short cap);
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
		for (j = 0; j < kNumKins; j++) {
			skill_info[skill].min_remort[i][j] = kMaxRemort;
			skill_info[skill].min_level[i][j] = 0;
			skill_info[skill].k_improve[i][j] = 0;
		}
	}
	skill_info[skill].save_type = ESaving::kReflex;
	skill_info[skill].difficulty = kDefaultFailPercent;
	skill_info[skill].cap = kDefaultCap;
	skill_info[skill].name = kUnusedSkillName;
}

void InitSingleSkill(int skill, const char *name, const char *short_name,
					 ESaving saving, int difficulty, unsigned short cap) {
	int i, j;
	for (i = 0; i < NUM_PLAYER_CLASSES; i++) {
		for (j = 0; j < kNumKins; j++) {
			skill_info[skill].min_remort[i][j] = kMaxRemort;
			skill_info[skill].min_level[i][j] = 0;
			skill_info[skill].k_improve[i][j] = 0;
		}
	}
	//skill_info[skill].min_position = 0;
	skill_info[skill].name = name;
	skill_info[skill].shortName = short_name;
	skill_info[skill].save_type = saving;
	skill_info[skill].difficulty = difficulty;
	skill_info[skill].cap = cap;
}

void InitSkills() {

	for (int i = 0; i <= MAX_SKILL_NUM; i++) {
		InitUnusedSkill(i);
	}

	InitSingleSkill(SKILL_GLOBAL_COOLDOWN, "!global cooldown", "ОЗ", ESaving::kReflex, 1, 1);
	InitSingleSkill(SKILL_BACKSTAB, "заколоть", "Зк", ESaving::kReflex, 25, 1000);
	InitSingleSkill(SKILL_BASH, "сбить", "Сб", ESaving::kReflex, 200, 200);
	InitSingleSkill(SKILL_HIDE, "спрятаться", "Сп", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_KICK, "пнуть", "Пн", ESaving::kStability, 0, 200);
	InitSingleSkill(SKILL_PICK_LOCK, "взломать", "Вз", ESaving::kReflex, 120, 200);
	InitSingleSkill(SKILL_PUNCH, "кулачный бой", "Кб", ESaving::kReflex, 100, 1000);
	InitSingleSkill(SKILL_RESCUE, "спасти", "Спс", ESaving::kReflex, 130, 1000);
	InitSingleSkill(SKILL_SNEAK, "подкрасться", "Пд", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_STEAL, "украсть", "Ук", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_TRACK, "выследить", "Выс", ESaving::kReflex, 100, 1000);
	InitSingleSkill(SKILL_PARRY, "парировать", "Пр", ESaving::kReflex, 120, 1000);
	InitSingleSkill(SKILL_BLOCK, "блокировать щитом", "Бщ", ESaving::kReflex, 200, 200);
	InitSingleSkill(SKILL_TOUCH, "перехватить атаку", "Пр", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_PROTECT, "прикрыть", "Пр", ESaving::kReflex, 120, 200);
	InitSingleSkill(SKILL_BOTHHANDS, "двуручники", "Дв", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_LONGS, "длинные лезвия", "Дл", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_SPADES, "копья и рогатины", "Кр", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_SHORTS, "короткие лезвия", "Кл", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_BOWS, "луки", "Лк", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_CLUBS, "палицы и дубины", "Пд", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_PICK, "проникающее оружие", "Прн", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_NONSTANDART, "иное оружие", "Ин", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_AXES, "секиры", "Ск", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_SATTACK, "атака левой рукой", "Ал", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_LOOKING, "приглядеться", "Прг", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_HEARING, "прислушаться", "Прс", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_DISARM, "обезоружить", "Об", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_HEAL, "!heal!", "Hl", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_MORPH, "оборотничество", "Об", ESaving::kReflex, 150, 200);
	InitSingleSkill(SKILL_ADDSHOT, "дополнительный выстрел", "Доп", ESaving::kReflex, 200, 200);
	InitSingleSkill(SKILL_CAMOUFLAGE, "маскировка", "Мск", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_DEVIATE, "уклониться", "Укл", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_CHOPOFF, "подножка", "Пд", ESaving::kReflex, 200, 200);
	InitSingleSkill(SKILL_REPAIR, "ремонт", "Рм", ESaving::kReflex, 180, 200);
	InitSingleSkill(SKILL_COURAGE, "ярость", "Яр", ESaving::kReflex, 100, 1000);
	InitSingleSkill(SKILL_IDENTIFY, "опознание", "Оп", ESaving::kReflex, 100, 1000);
	InitSingleSkill(SKILL_LOOK_HIDE, "подсмотреть", "Пдм", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_UPGRADE, "заточить", "Зат", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_ARMORED, "укрепить", "Укр", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_DRUNKOFF, "опохмелиться", "Опх", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_AID, "лечить", "Лч", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_FIRE, "разжечь костер", "Рк", ESaving::kReflex, 160, 1000);
	InitSingleSkill(SKILL_SHIT, "удар левой рукой", "Улр", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_MIGHTHIT, "богатырский молот", "Бм", ESaving::kStability, 200, 200);
	InitSingleSkill(SKILL_STUPOR, "оглушить", "Ог", ESaving::kStability, 200, 200);
	InitSingleSkill(SKILL_POISONED, "отравить", "Отр", ESaving::kReflex, 200, 200);
	InitSingleSkill(SKILL_LEADERSHIP, "лидерство", "Лд", ESaving::kReflex, 100, 1000);
	InitSingleSkill(SKILL_PUNCTUAL, "точный стиль", "Тс", ESaving::kCritical, 110, 200);
	InitSingleSkill(SKILL_AWAKE, "осторожный стиль", "Ос", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_SENSE, "найти", "Нйт", ESaving::kReflex, 160, 200);
	InitSingleSkill(SKILL_HORSE, "сражение верхом", "Срв", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_HIDETRACK, "замести следы", "Зс", ESaving::kReflex, 120, 200);
	InitSingleSkill(SKILL_RELIGION, "!молитва или жертва!", "Error", ESaving::kReflex, 1, 1);
	InitSingleSkill(SKILL_MAKEFOOD, "освежевать", "Осв", ESaving::kReflex, 120, 200);
	InitSingleSkill(SKILL_MULTYPARRY, "веерная защита", "Вз", ESaving::kReflex, 140, 200);
	InitSingleSkill(SKILL_TRANSFORMWEAPON, "перековать", "Прк", ESaving::kReflex, 140, 200);
	InitSingleSkill(SKILL_THROW, "метнуть", "Мт", ESaving::kReflex, 150, 1000);
	InitSingleSkill(SKILL_MAKE_BOW, "смастерить лук", "Сл", ESaving::kReflex, 140, 200);
	InitSingleSkill(SKILL_MAKE_WEAPON, "выковать оружие", "Вкр", ESaving::kReflex, 140, 200);
	InitSingleSkill(SKILL_MAKE_ARMOR, "выковать доспех", "Вкд", ESaving::kReflex, 140, 200);
	InitSingleSkill(SKILL_MAKE_WEAR, "сшить одежду", "Сш", ESaving::kReflex, 140, 200);
	InitSingleSkill(SKILL_MAKE_JEWEL, "смастерить диковинку", "Сд", ESaving::kReflex, 140, 200);
	InitSingleSkill(SKILL_MANADRAIN, "сглазить", "Сг", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_NOPARRYHIT, "скрытый удар", "Сд", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_TOWNPORTAL, "врата", "Вр", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_DIG, "горное дело", "Гд", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_INSERTGEM, "ювелир", "Юв", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_WARCRY, "боевой клич", "Бк", ESaving::kReflex, 100, 200);
	InitSingleSkill(SKILL_TURN_UNDEAD, "изгнать нежить", "Из", ESaving::kReflex, 100, 1000);
	InitSingleSkill(SKILL_IRON_WIND, "железный ветер", "Жв", ESaving::kReflex, 150, 1000);
	InitSingleSkill(SKILL_STRANGLE, "удавить", "Уд", ESaving::kReflex, 200, 200);
	InitSingleSkill(SKILL_AIR_MAGIC, "магия воздуха", "Ма", ESaving::kReflex, 200, 1000);
	InitSingleSkill(SKILL_FIRE_MAGIC, "магия огня", "Мо", ESaving::kReflex, 1000, 1000);
	InitSingleSkill(SKILL_WATER_MAGIC, "магия воды", "Мвд", ESaving::kReflex, 1000, 1000);
	InitSingleSkill(SKILL_EARTH_MAGIC, "магия земли", "Мз", ESaving::kReflex, 1000, 1000);
	InitSingleSkill(SKILL_LIGHT_MAGIC, "магия света", "Мс", ESaving::kReflex, 1000, 1000);
	InitSingleSkill(SKILL_DARK_MAGIC, "магия тьмы", "Мт", ESaving::kReflex, 1000, 1000);
	InitSingleSkill(SKILL_MIND_MAGIC, "магия разума", "Мр", ESaving::kReflex, 1000, 1000);
	InitSingleSkill(SKILL_LIFE_MAGIC, "магия жизни", "Мж", ESaving::kReflex, 1000, 1000);
	InitSingleSkill(SKILL_STUN, "ошеломить", "Ош", ESaving::kReflex, 200, 200);
	InitSingleSkill(SKILL_MAKE_AMULET, "смастерить оберег", "Со", ESaving::kReflex, 200, 200);
	InitSingleSkill(SKILL_INDEFINITE, "!неопределен", "Error", ESaving::kReflex, 1, 1);
}
