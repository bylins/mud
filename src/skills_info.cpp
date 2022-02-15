//#include "skills_info.h"

#include "structs/global_objects.h"
#include "utils/parse.h"
//#include "skills_info.h"

//#include "utils/logger.h"

struct AttackMessages fight_messages[kMaxMessages];

const SkillInfo &SkillsInfo::operator[](const ESkill id) const {
	try {
		return items_->at(id)->second;
	} catch (const std::out_of_range &) {
		err_log("Incorrect id (%d) passed into %s.", to_underlying(id), typeid(this).name());
		return items_->at(ESkill::kUndefined)->second;
	}
}

void SkillsInfo::InitSkill(ESkill id, const std::string &name, const std::string &short_name,
						   ESaving saving, int difficulty, int cap, bool enabled) {
	auto item = std::make_pair(enabled, SkillInfo(name, short_name, saving, difficulty, cap));
	auto it = items_->try_emplace(id, std::make_unique<Pair>(item));
	if (!it.second) {
		err_log("Skill '%s' has already exist. Redundant definition had been ignored.",
				NAME_BY_ITEM<ESkill>(id).c_str());
	}
}

bool SkillsInfo::IsKnown(const ESkill id) {
	return items_->contains(id);
}

bool SkillsInfo::IsValid(const ESkill id) {
	bool validity = IsKnown(id) && id >= ESkill::kFirst && id <= ESkill::kLast;
	if (validity) {
		validity &= IsEnabled(id);
	}
	return validity;
}

bool SkillsInfo::IsEnabled(const ESkill id) {
	return items_->at(id)->first;
}

bool SkillsInfo::IsInitizalized() {
	return (items_->size() > 1);
}

void SkillsInfo::Init() {
	if (IsInitizalized()) {
		err_log("This MUD already has an initialized %s. Use 'reload' for re-init.", typeid(this).name());
		return;
	}

	InitSkill(ESkill::kReligion, "!молитва или жертва!", "Error", ESaving::kFirst, 1, 1, false);
	InitSkill(ESkill::kAny, "!any", "Any", ESaving::kFirst, 1, 1, false);
	InitSkill(ESkill::kIncorrect, "!INCORRECT!", "!Icr", ESaving::kFirst, 1, 1, false);
	InitSkill(ESkill::kGlobalCooldown, "!cooldown", "ОЗ", ESaving::kFirst, 1, 1, false);
	InitSkill(ESkill::kBackstab, "заколоть", "Зк", ESaving::kReflex, 25, 1000);
	InitSkill(ESkill::kBash, "сбить", "Сб", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::kHide, "спрятаться", "Сп", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kKick, "пнуть", "Пн", ESaving::kStability, 0, 200);
	InitSkill(ESkill::kPickLock, "взломать", "Вз", ESaving::kReflex, 120, 200);
	InitSkill(ESkill::kPunch, "кулачный бой", "Кб", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::kRescue, "спасти", "Спс", ESaving::kReflex, 130, 1000);
	InitSkill(ESkill::kSneak, "подкрасться", "Пд", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kSteal, "украсть", "Ук", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kTrack, "выследить", "Выс", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::kParry, "парировать", "Пр", ESaving::kReflex, 120, 1000);
	InitSkill(ESkill::kShieldBlock, "блокировать щитом", "Бщ", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::kIntercept, "перехватить атаку", "Пх", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kProtect, "прикрыть", "Пк", ESaving::kReflex, 120, 200);
	InitSkill(ESkill::kTwohands, "двуручники", "Дв", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kLongBlades, "длинные лезвия", "Дл", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kSpades, "копья и рогатины", "Кр", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kShortBlades, "короткие лезвия", "Кл", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kBows, "луки", "Лк", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kClubs, "палицы и дубины", "Пд", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kPicks, "проникающее оружие", "Прн", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kNonstandart, "иное оружие", "Ин", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kAxes, "секиры", "Ск", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kSideAttack, "атака левой рукой", "Ал", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kLooking, "приглядеться", "Прг", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kHearing, "прислушаться", "Прс", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kDisarm, "обезоружить", "Об", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kMorph, "оборотничество", "Об", ESaving::kReflex, 150, 200);
	InitSkill(ESkill::kAddshot, "дополнительный выстрел", "Доп", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::kDisguise, "маскировка", "Мск", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kDodge, "уклониться", "Укл", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kUndercut, "подножка", "Пд", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::kRepair, "ремонт", "Рм", ESaving::kReflex, 180, 200);
	InitSkill(ESkill::kCourage, "ярость", "Яр", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::kIdentify, "опознание", "Оп", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::kPry, "подсмотреть", "Пдм", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kSharpening, "заточить", "Зат", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kArmoring, "укрепить", "Укр", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kHangovering, "опохмелиться", "Опх", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kFirstAid, "лечить", "Лч", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kCampfire, "разжечь костер", "Рк", ESaving::kReflex, 160, 1000);
	InitSkill(ESkill::kLeftHit, "удар левой рукой", "Улр", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kHammer, "богатырский молот", "Бм", ESaving::kStability, 200, 200);
	InitSkill(ESkill::kOverwhelm, "оглушить", "Ог", ESaving::kStability, 200, 200);
	InitSkill(ESkill::kPoisoning, "отравить", "Отр", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::kLeadership, "лидерство", "Лд", ESaving::kReflex, 100, 1000);
	InitSkill(ESkill::kPunctual, "точный стиль", "Тс", ESaving::kCritical, 110, 200);
	InitSkill(ESkill::kAwake, "осторожный стиль", "Ос", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kSense, "найти", "Нйт", ESaving::kReflex, 160, 200);
	InitSkill(ESkill::kRiding, "сражение верхом", "Срв", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kHideTrack, "замести следы", "Зс", ESaving::kReflex, 120, 200);
	InitSkill(ESkill::kSkinning, "освежевать", "Осв", ESaving::kReflex, 120, 200);
	InitSkill(ESkill::kMultiparry, "веерная защита", "Вз", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::kReforging, "перековать", "Прк", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::kThrow, "метнуть", "Мт", ESaving::kReflex, 150, 1000);
	InitSkill(ESkill::kMakeBow, "смастерить лук", "Сл", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::kMakeWeapon, "выковать оружие", "Вкр", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::kMakeArmor, "выковать доспех", "Вкд", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::kMakeWear, "сшить одежду", "Сш", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::kMakeJewel, "смастерить диковинку", "Сд", ESaving::kReflex, 140, 200);
	InitSkill(ESkill::kJinx, "сглазить", "Сг", ESaving::kStability, 100, 200);
	InitSkill(ESkill::kNoParryHit, "скрытый удар", "Сд", ESaving::kCritical, 100, 200);
	InitSkill(ESkill::kTownportal, "врата", "Вр", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kDigging, "горное дело", "Гд", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kJewelry, "ювелир", "Юв", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kWarcry, "боевой клич", "Бк", ESaving::kReflex, 100, 200);
	InitSkill(ESkill::kTurnUndead, "изгнать нежить", "Из", ESaving::kStability, 100, 1000);
	InitSkill(ESkill::kIronwind, "железный ветер", "Жв", ESaving::kReflex, 150, 1000);
	InitSkill(ESkill::kStrangle, "удавить", "Уд", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::kAirMagic, "магия воздуха", "Ма", ESaving::kReflex, 200, 1000);
	InitSkill(ESkill::kFireMagic, "магия огня", "Мо", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::kWaterMagic, "магия воды", "Мвд", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::kEarthMagic, "магия земли", "Мз", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::kLightMagic, "магия света", "Мс", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::kDarkMagic, "магия тьмы", "Мт", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::kMindMagic, "магия разума", "Мр", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::kLifeMagic, "магия жизни", "Мж", ESaving::kReflex, 1000, 1000);
	InitSkill(ESkill::kStun, "ошеломить", "Ош", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::kMakeAmulet, "смастерить оберег", "Со", ESaving::kReflex, 200, 200);
	InitSkill(ESkill::kCreateBow, "!отключено", "!error", ESaving::kFirst, 1, 1, false);
	InitSkill(ESkill::kCreatePotion, "!отключено", "!error", ESaving::kFirst, 1, 1, false);
	InitSkill(ESkill::kCreateScroll, "!отключено", "!error", ESaving::kFirst, 1, 1, false);
	InitSkill(ESkill::kCreateWand, "!отключено", "!error", ESaving::kFirst, 1, 1, false);
	InitSkill(ESkill::kMakeStaff, "!отключено", "!error", ESaving::kFirst, 1, 1, false);
	InitSkill(ESkill::kMakePotion, "!отключено", "!error", ESaving::kFirst, 1, 1, false);

}

using DataNode = parser_wrapper::DataNode;
using Optional = SkillInfoBuilder::ItemOptional;

void SkillsLoader::Load(DataNode data) {
	MUD::NewSkills().Init(data.Children());
}

Optional SkillInfoBuilder::Build(DataNode &node) {
	auto skill_info = MUD::NewSkills().MakeItemOptional();
	skill_info = std::move(ParseDispensableValues(skill_info, node));
	skill_info = std::move(ParseObligatoryValues(skill_info, node));
	return skill_info;
}

Optional &SkillInfoBuilder::ParseDispensableValues(Optional &optional, DataNode &node) {
	try {
		optional.value()->difficulty = parse::ReadAsInt(node.GetValue("difficulty"));
	} catch (std::exception &) {}
	try {
		optional.value()->save_type = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
	} catch (std::exception &) {}
	try {
		optional.value()->mode = parse::ReadAsConstant<EItemMode>(node.GetValue("mode"));
	} catch (std::exception &) {
		optional.value()->mode = EItemMode::kEnabled;
	}
	return optional;
}

Optional &SkillInfoBuilder::ParseObligatoryValues(Optional &optional, DataNode &node) {
	try {
		optional.value()->id = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
		optional.value()->name = parse::ReadAsStr(node.GetValue("name"));
		optional.value()->short_name = parse::ReadAsStr(node.GetValue("abbr"));
		optional.value()->cap = parse::ReadAsInt(node.GetValue("cap"));
	} catch (std::exception &e) {
		err_log("invalid skill description (incorrect value: %s).", e.what());
		optional = std::nullopt;
	}
	return optional;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
