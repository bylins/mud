#include "skills.info.h"

const char *kUnusedSkillName = "!UNUSED!";
const char *kUnidentifiedSkillName = "!UNIDENTIFIED!";
const byte kDefaultMinPosition = 0;
const int kDefaultFailPercent = 200;
const unsigned short kDefaultCap = 200;

struct SkillInfoType skill_info[MAX_SKILL_NUM + 1];

const char *skill_name(int num);
void InitSingleSkill(int skill, const char *name, const char *short_name, int fail_percent, unsigned short cap);
void InitSkills(void);

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
  skill_info[skill].fail_percent = kDefaultFailPercent;
  skill_info[skill].cap = kDefaultCap;
  skill_info[skill].name = kUnusedSkillName;
}

void InitSingleSkill(int skill, const char *name, const char *short_name, int fail_percent, unsigned short cap) {
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
  skill_info[skill].fail_percent = fail_percent;
  skill_info[skill].cap = cap;
}

void InitSkills(void) {

  for (int i = 0; i <= MAX_SKILL_NUM; i++) {
    InitUnusedSkill(i);
  }

  InitSingleSkill(SKILL_GLOBAL_COOLDOWN, "!глобальная задержка", "ОЗ", 1, 1);
  InitSingleSkill(SKILL_BACKSTAB, "заколоть", "Зк", 180, 1000);
  InitSingleSkill(SKILL_BASH, "сбить", "Сб", 200, 200);
  InitSingleSkill(SKILL_HIDE, "спрятаться", "Сп", 100, 200);
  InitSingleSkill(SKILL_KICK, "пнуть", "Пн", 100, 200);
  InitSingleSkill(SKILL_PICK_LOCK, "взломать", "Вз", 120, 200);
  InitSingleSkill(SKILL_PUNCH, "кулачный бой", "Кб", 100, 1000);
  InitSingleSkill(SKILL_RESCUE, "спасти", "Спс", 130, 1000);
  InitSingleSkill(SKILL_SNEAK, "подкрасться", "Пд", 100, 200);
  InitSingleSkill(SKILL_STEAL, "украсть", "Ук", 100, 200);
  InitSingleSkill(SKILL_TRACK, "выследить", "Выс", 100, 1000);
  InitSingleSkill(SKILL_PARRY, "парировать", "Пр", 120, 1000);
  InitSingleSkill(SKILL_BLOCK, "блокировать щитом", "Бщ", 200, 200);
  InitSingleSkill(SKILL_TOUCH, "перехватить атаку", "Пр", 100, 200);
  InitSingleSkill(SKILL_PROTECT, "прикрыть", "Пр", 120, 200);
  InitSingleSkill(SKILL_BOTHHANDS, "двуручники", "Дв", 160, 1000);
  InitSingleSkill(SKILL_LONGS, "длинные лезвия", "Дл", 160, 1000);
  InitSingleSkill(SKILL_SPADES, "копья и рогатины", "Кр", 160, 1000);
  InitSingleSkill(SKILL_SHORTS, "короткие лезвия", "Кл", 160, 1000);
  InitSingleSkill(SKILL_BOWS, "луки", "Лк", 160, 1000);
  InitSingleSkill(SKILL_CLUBS, "палицы и дубины", "Пд", 160, 1000);
  InitSingleSkill(SKILL_PICK, "проникающее оружие", "Прн", 160, 1000);
  InitSingleSkill(SKILL_NONSTANDART, "иное оружие", "Ин", 160, 1000);
  InitSingleSkill(SKILL_AXES, "секиры", "Ск", 160, 1000);
  InitSingleSkill(SKILL_SATTACK, "атака левой рукой", "Ал", 100, 200);
  InitSingleSkill(SKILL_LOOKING, "приглядеться", "Прг", 100, 200);
  InitSingleSkill(SKILL_HEARING, "прислушаться", "Прс", 100, 200);
  InitSingleSkill(SKILL_DISARM, "обезоружить", "Об", 100, 200);
  InitSingleSkill(SKILL_HEAL, "!heal!", "Hl", 100, 200);
  InitSingleSkill(SKILL_MORPH, "оборотничество", "Об", 150, 200);
  InitSingleSkill(SKILL_ADDSHOT, "дополнительный выстрел", "Доп", 200, 200);
  InitSingleSkill(SKILL_CAMOUFLAGE, "маскировка", "Мск", 100, 200);
  InitSingleSkill(SKILL_DEVIATE, "уклониться", "Укл", 100, 200);
  InitSingleSkill(SKILL_CHOPOFF, "подножка", "Пд", 200, 200);
  InitSingleSkill(SKILL_REPAIR, "ремонт", "Рм", 180, 200);
  InitSingleSkill(SKILL_COURAGE, "ярость", "Яр", 100, 1000);
  InitSingleSkill(SKILL_IDENTIFY, "опознание", "Оп", 100, 1000);
  InitSingleSkill(SKILL_LOOK_HIDE, "подсмотреть", "Пдм", 100, 200);
  InitSingleSkill(SKILL_UPGRADE, "заточить", "Зат", 100, 200);
  InitSingleSkill(SKILL_ARMORED, "укрепить", "Укр", 100, 200);
  InitSingleSkill(SKILL_DRUNKOFF, "опохмелиться", "Опх", 100, 200);
  InitSingleSkill(SKILL_AID, "лечить", "Лч", 100, 200);
  InitSingleSkill(SKILL_FIRE, "разжечь костер", "Рк", 160, 1000);
  InitSingleSkill(SKILL_SHIT, "удар левой рукой", "Улр", 100, 200);
  InitSingleSkill(SKILL_MIGHTHIT, "богатырский молот", "Бм", 200, 200);
  InitSingleSkill(SKILL_STUPOR, "оглушить", "Ог", 200, 200);
  InitSingleSkill(SKILL_POISONED, "отравить", "Отр", 200, 200);
  InitSingleSkill(SKILL_LEADERSHIP, "лидерство", "Лд", 100, 200);
  InitSingleSkill(SKILL_PUNCTUAL, "точный стиль", "Тс", 110, 200);
  InitSingleSkill(SKILL_AWAKE, "осторожный стиль", "Ос", 100, 200);
  InitSingleSkill(SKILL_SENSE, "найти", "Нйт", 160, 200);
  InitSingleSkill(SKILL_HORSE, "сражение верхом", "Срв", 100, 200);
  InitSingleSkill(SKILL_HIDETRACK, "замести следы", "Зс", 120, 200);
  InitSingleSkill(SKILL_RELIGION, "!молитва или жертва!", "Error", 1, 1);
  InitSingleSkill(SKILL_MAKEFOOD, "освежевать", "Осв", 120, 200);
  InitSingleSkill(SKILL_MULTYPARRY, "веерная защита", "Вз", 140, 200);
  InitSingleSkill(SKILL_TRANSFORMWEAPON, "перековать", "Прк", 140, 200);
  InitSingleSkill(SKILL_THROW, "метнуть", "Мт", 150, 1000);
  InitSingleSkill(SKILL_MAKE_BOW, "смастерить лук", "Сл", 140, 200);
  InitSingleSkill(SKILL_MAKE_WEAPON, "выковать оружие", "Вкр", 140, 200);
  InitSingleSkill(SKILL_MAKE_ARMOR, "выковать доспех", "Вкд", 140, 200);
  InitSingleSkill(SKILL_MAKE_WEAR, "сшить одежду", "Сш", 140, 200);
  InitSingleSkill(SKILL_MAKE_JEWEL, "смастерить диковинку", "Сд", 140, 200);
  InitSingleSkill(SKILL_MANADRAIN, "сглазить", "Сг", 100, 200);
  InitSingleSkill(SKILL_NOPARRYHIT, "скрытый удар", "Сд", 100, 200);
  InitSingleSkill(SKILL_TOWNPORTAL, "врата", "Вр", 100, 200);
  InitSingleSkill(SKILL_DIG, "горное дело", "Гд", 100, 200);
  InitSingleSkill(SKILL_INSERTGEM, "ювелир", "Юв", 100, 200);
  InitSingleSkill(SKILL_WARCRY, "боевой клич", "Бк", 100, 200);
  InitSingleSkill(SKILL_TURN_UNDEAD, "изгнать нежить", "Из", 100, 1000);
  InitSingleSkill(SKILL_IRON_WIND, "железный ветер", "Жв", 150, 1000);
  InitSingleSkill(SKILL_STRANGLE, "удавить", "Уд", 200, 200);
  InitSingleSkill(SKILL_AIR_MAGIC, "магия воздуха", "Ма", 200, 1000);
  InitSingleSkill(SKILL_FIRE_MAGIC, "магия огня", "Мо", 1000, 1000);
  InitSingleSkill(SKILL_WATER_MAGIC, "магия воды", "Мвд", 1000, 1000);
  InitSingleSkill(SKILL_EARTH_MAGIC, "магия земли", "Мз", 1000, 1000);
  InitSingleSkill(SKILL_LIGHT_MAGIC, "магия света", "Мс", 1000, 1000);
  InitSingleSkill(SKILL_DARK_MAGIC, "магия тьмы", "Мт", 1000, 1000);
  InitSingleSkill(SKILL_MIND_MAGIC, "магия разума", "Мр", 1000, 1000);
  InitSingleSkill(SKILL_LIFE_MAGIC, "магия жизни", "Мж", 1000, 1000);
  InitSingleSkill(SKILL_STUN, "ошеломить", "Ош", 200, 200);
  InitSingleSkill(SKILL_MAKE_AMULET, "смастерить оберег", "Со", 200, 200);
  InitSingleSkill(SKILL_INDEFINITE, "!неопределен", "Error", 1, 1);
}
