/**
 \file basic_values.cpp - a part of the Bylins engine.
 \brief issue.basic: чтение cfg/basic.xml в массивы модификаторов из constants.cpp.
 \detail Заменяет старый InitBasicValues() (парсер misc/basic.lst). Массивы обнуляются
         и заполняются из файла; отсутствующие секции/строки логируются один раз при
         загрузке, а соответствующие обращения возвращают 0 (нулевая инициализация).
*/

#include "basic_values.h"

#include "gameplay/core/constants.h"
#include "utils/parse.h"
#include "utils/parser_wrapper.h"
#include "utils/logger.h"

#include <vector>

namespace {

constexpr int kStatModSize = 101;   // индексы 0..100 (как у прежних таблиц)
constexpr int kWisModSize = 51;     // мудрость ограничивается MIN(50, wis)

// Целочисленный атрибут <mod>; 0 при отсутствии/некорректном значении.
int Attr(parser_wrapper::DataNode &node, const char *key) {
	const char *v = node.GetValue(key);
	if (!v || !*v) {
		return 0;
	}
	try {
		return parse::ReadAsInt(v);
	} catch (const std::exception &) {
		return 0;
	}
}

// Читает секцию <name> со строками <mod val="I" .../> и заполняет таблицу через setter.
// Один раз сообщает в errlog, если секции нет или покрыты не все индексы [0, size).
template<typename Setter>
void LoadSection(parser_wrapper::DataNode data, const char *name, int size, Setter setter) {
	auto section = data;
	if (!section.GoToChild(name)) {
		err_log("basic.xml: missing <%s> section -- its lookups will return 0.", name);
		return;
	}
	std::vector<bool> seen(size, false);
	int covered = 0;
	for (auto &mod : section.Children("mod")) {
		const char *vs = mod.GetValue("val");
		if (!vs || !*vs) {
			continue;
		}
		int idx;
		try {
			idx = parse::ReadAsInt(vs);
		} catch (const std::exception &) {
			continue;
		}
		if (idx < 0 || idx >= size) {
			err_log("basic.xml: <%s> val=%d out of range [0,%d) -- skipped.", name, idx, size);
			continue;
		}
		setter(idx, mod);
		if (!seen[idx]) {
			seen[idx] = true;
			++covered;
		}
	}
	if (covered < size) {
		err_log("basic.xml: <%s> has %d of %d rows; missing indices will return 0.",
				name, covered, size);
	}
}

} // namespace

void BasicValuesLoader::Load(parser_wrapper::DataNode data) {
	// Обнуляем таблицы (чтобы reload не оставлял старых значений).
	for (int i = 0; i < kStatModSize; ++i) {
		int_app[i] = IntApplies{};
		cha_app[i] = ChaApplies{};
		size_app[i] = SizeApplies{};
		weapon_app[i] = WeaponApplies{};
	}
	for (int i = 0; i < kWisModSize; ++i) {
		mana[i] = 0;
	}

	LoadSection(data, "cha_mod", kStatModSize, [](int i, parser_wrapper::DataNode &m) {
		cha_app[i].leadership = Attr(m, "leadership");
		cha_app[i].charms = Attr(m, "charms");
		cha_app[i].morale = Attr(m, "luck");
		cha_app[i].illusive = Attr(m, "Illusive");
		cha_app[i].dam_to_hit_rate = Attr(m, "dam_to_hit_rate");
	});

	LoadSection(data, "int_mod", kStatModSize, [](int i, parser_wrapper::DataNode &m) {
		int_app[i].spell_aknowlege = Attr(m, "apell_acknowledge");
		int_app[i].to_skilluse = Attr(m, "to_skill_use");
		int_app[i].mana_per_tic = Attr(m, "mana_per_tic");
		int_app[i].spell_success = Attr(m, "fails");
		int_app[i].improve = Attr(m, "improove_skill");
		int_app[i].observation = Attr(m, "observation");
		int_app[i].mana_gain = Attr(m, "mana_gain");
	});

	LoadSection(data, "size_mod", kStatModSize, [](int i, parser_wrapper::DataNode &m) {
		size_app[i].ac = Attr(m, "ac");
		size_app[i].interpolate = Attr(m, "interpolate");
		size_app[i].initiative = Attr(m, "initiative");
		size_app[i].shocking = Attr(m, "shocking");
	});

	LoadSection(data, "weapon_mod", kStatModSize, [](int i, parser_wrapper::DataNode &m) {
		weapon_app[i].shocking = Attr(m, "shocking");
		weapon_app[i].bashing = Attr(m, "bashing");
		weapon_app[i].parrying = Attr(m, "parrying");
	});

	LoadSection(data, "wiz_mod", kWisModSize, [](int i, parser_wrapper::DataNode &m) {
		mana[i] = Attr(m, "mana_amount");
	});
}

void BasicValuesLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
