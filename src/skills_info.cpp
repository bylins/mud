#include "skills_info.h"

#include "logger.h"

struct AttackMessages fight_messages[kMaxMessages];

const SkillInfo &SkillsInfo::operator[](const ESkill id) const {
	try {
		return *skills_.at(id);
	} catch (const std::out_of_range &) {
		err_log("Incorrect skill id (%d) passed into MUD::Skills.", to_underlying(id));
		return *skills_.at(ESkill::kUndefined);
	}
}

void SkillsInfo::InitSkill(ESkill id, const std::string &name, const std::string &short_name,
						   ESaving saving, int difficulty, int cap) {
	auto skill = std::make_unique<SkillInfo>(name, short_name, saving, difficulty, cap);
	auto it = skills_.try_emplace(id, std::move(skill));
	if (!it.second) {
		err_log("Skill '%s' has already exist. Redundant definition had been ignored.\n",
				NAME_BY_ITEM<ESkill>(id).c_str());
	}
}

void SkillsInfo::Init() {
	InitSkill(ESkill::kAny, "!any", "Any", ESaving::kReflex, 1, 1);
	InitSkill(ESkill::kUndefined, "!UNDEFINED!", "!Und", ESaving::kReflex, 1, 1);
	InitSkill(ESkill::kIncorrect, "!INCORRECT!", "!Icr", ESaving::kReflex, 1, 1);
	InitSkill(ESkill::SKILL_GLOBAL_COOLDOWN, "!cooldown", "ОЗ", ESaving::kReflex, 1, 1);
	InitSkill(ESkill::SKILL_BACKSTAB, "заколоть", "Зк", ESaving::kReflex, 25, 1000);
	InitSkill(ESkill::SKILL_BASH, "сбить", "Сб", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::SKILL_HIDE, "спрятаться", "Сп", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_KICK, "пнуть", "Пн", ESaving::kStability, 0, 200);
	InitSkill(ESkill::SKILL_PICK_LOCK, "взломать", "Вз", ESaving::kReflex, 120, 200);
	InitSkill(ESkill::SKILL_PUNCH, "кулачный бой", "Кб", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::SKILL_RESCUE, "спасти", "Спс", ESaving::kReflex, 130, 1000);
	InitSkill(ESkill::SKILL_SNEAK, "подкрасться", "Пд", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_STEAL, "украсть", "Ук", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_TRACK, "выследить", "Выс", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::SKILL_PARRY, "парировать", "Пр", ESaving::kReflex, 120, 1000);
	InitSkill(ESkill::SKILL_BLOCK, "блокировать щитом", "Бщ", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::SKILL_TOUCH, "перехватить атаку", "Пх", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_PROTECT, "прикрыть", "Пк", ESaving::kReflex, 120, 200);
	InitSkill(ESkill::SKILL_BOTHHANDS, "двуручники", "Дв", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_LONGS, "длинные лезвия", "Дл", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_SPADES, "копья и рогатины", "Кр", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_SHORTS, "короткие лезвия", "Кл", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_BOWS, "луки", "Лк", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_CLUBS, "палицы и дубины", "Пд", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_PICK, "проникающее оружие", "Прн", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_NONSTANDART, "иное оружие", "Ин", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_AXES, "секиры", "Ск", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_SATTACK, "атака левой рукой", "Ал", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_LOOKING, "приглядеться", "Прг", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_HEARING, "прислушаться", "Прс", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_DISARM, "обезоружить", "Об", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_HEAL, "!heal!", "Hl", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_MORPH, "оборотничество", "Об", ESaving::kReflex, 150, 200);
	InitSkill(ESkill::SKILL_ADDSHOT, "дополнительный выстрел", "Доп", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::SKILL_CAMOUFLAGE, "маскировка", "Мск", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_DEVIATE, "уклониться", "Укл", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_CHOPOFF, "подножка", "Пд", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::SKILL_REPAIR, "ремонт", "Рм", ESaving::kReflex, 180, 200);
	InitSkill(ESkill::SKILL_COURAGE, "ярость", "Яр", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::SKILL_IDENTIFY, "опознание", "Оп", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::SKILL_LOOK_HIDE, "подсмотреть", "Пдм", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_UPGRADE, "заточить", "Зат", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_ARMORED, "укрепить", "Укр", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_DRUNKOFF, "опохмелиться", "Опх", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_AID, "лечить", "Лч", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_FIRE, "разжечь костер", "Рк", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::SKILL_SHIT, "удар левой рукой", "Улр", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_MIGHTHIT, "богатырский молот", "Бм", ESaving::kStability, 200, 200);
	InitSkill(ESkill::SKILL_STUPOR, "оглушить", "Ог", ESaving::kStability, 200, 200);
	InitSkill(ESkill::SKILL_POISONED, "отравить", "Отр", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::SKILL_LEADERSHIP, "лидерство", "Лд", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::SKILL_PUNCTUAL, "точный стиль", "Тс", ESaving::kCritical, 110, 200);
	InitSkill(ESkill::SKILL_AWAKE, "осторожный стиль", "Ос", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_SENSE, "найти", "Нйт", ESaving::kReflex, 160, 200);
	InitSkill(ESkill::SKILL_HORSE, "сражение верхом", "Срв", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_HIDETRACK, "замести следы", "Зс", ESaving::kReflex, 120, 200);
	InitSkill(ESkill::SKILL_RELIGION, "!молитва или жертва!", "Error", ESaving::kReflex, 1, 1);
	InitSkill(ESkill::SKILL_MAKEFOOD, "освежевать", "Осв", ESaving::kReflex, 120, 200);
	InitSkill(ESkill::SKILL_MULTYPARRY, "веерная защита", "Вз", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::SKILL_TRANSFORMWEAPON, "перековать", "Прк", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::SKILL_THROW, "метнуть", "Мт", ESaving::kReflex, 150, 1000);
	InitSkill(ESkill::SKILL_MAKE_BOW, "смастерить лук", "Сл", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::SKILL_MAKE_WEAPON, "выковать оружие", "Вкр", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::SKILL_MAKE_ARMOR, "выковать доспех", "Вкд", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::SKILL_MAKE_WEAR, "сшить одежду", "Сш", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::SKILL_MAKE_JEWEL, "смастерить диковинку", "Сд", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::SKILL_MANADRAIN, "сглазить", "Сг", ESaving::kStability, 100, 200);
	InitSkill(ESkill::SKILL_NOPARRYHIT, "скрытый удар", "Сд", ESaving::kCritical, 100, 200);
	InitSkill(ESkill::SKILL_TOWNPORTAL, "врата", "Вр", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_DIG, "горное дело", "Гд", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_INSERTGEM, "ювелир", "Юв", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_WARCRY, "боевой клич", "Бк", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::SKILL_TURN_UNDEAD, "изгнать нежить", "Из", ESaving::kStability, 100, 1000);
	InitSkill(ESkill::SKILL_IRON_WIND, "железный ветер", "Жв", ESaving::kReflex, 150, 1000);
	InitSkill(ESkill::SKILL_STRANGLE, "удавить", "Уд", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::SKILL_AIR_MAGIC, "магия воздуха", "Ма", ESaving::kReflex, 200, 1000);
	InitSkill(ESkill::SKILL_FIRE_MAGIC, "магия огня", "Мо", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::SKILL_WATER_MAGIC, "магия воды", "Мвд", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::SKILL_EARTH_MAGIC, "магия земли", "Мз", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::SKILL_LIGHT_MAGIC, "магия света", "Мс", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::SKILL_DARK_MAGIC, "магия тьмы", "Мт", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::SKILL_MIND_MAGIC, "магия разума", "Мр", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::SKILL_LIFE_MAGIC, "магия жизни", "Мж", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::SKILL_STUN, "ошеломить", "Ош", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::SKILL_MAKE_AMULET, "смастерить оберег", "Со", ESaving::kReflex, 200, 200);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
