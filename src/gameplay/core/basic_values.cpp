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

constexpr int kStatModSize = kBaseStatTableSize;   // индексы 0..100
constexpr int kWisModSize = kBaseStatTableSize;    // мудрость 0..100

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
// Пропуски и хвост за последним заданным индексом доращиваются значением предыдущего
// уровня (copy_prev), так что запрос значения выше максимального в таблице вернет
// последнее заданное, а дырки (напр. 31..34 при заданных 30 и 35) заполнятся из 30.
// В errlog сообщается, только если секции нет или в ней нет ни одной строки.
template<typename Setter, typename CopyPrev>
void LoadSection(parser_wrapper::DataNode data, const char *name, int size,
				 Setter setter, CopyPrev copy_prev) {
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
	if (covered == 0) {
		err_log("basic.xml: <%s> has no rows -- its lookups will return 0.", name);
		return;
	}
	// Доращиваем пропуски и хвост значением предыдущего индекса. Ведущие индексы до
	// первого заданного (если val=0 не задан) остаются нулевыми -- доращивать неоткуда.
	for (int i = 1; i < size; ++i) {
		if (!seen[i]) {
			copy_prev(i, i - 1);
			seen[i] = true;
		}
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
	}, [](int d, int s) { cha_app[d] = cha_app[s]; });

	LoadSection(data, "int_mod", kStatModSize, [](int i, parser_wrapper::DataNode &m) {
		int_app[i].spell_aknowlege = Attr(m, "apell_acknowledge");
		int_app[i].to_skilluse = Attr(m, "to_skill_use");
		int_app[i].mana_per_tic = Attr(m, "mana_per_tic");
		int_app[i].spell_success = Attr(m, "fails");
		int_app[i].improve = Attr(m, "improove_skill");
		int_app[i].observation = Attr(m, "observation");
		int_app[i].mana_gain = Attr(m, "mana_gain");
	}, [](int d, int s) { int_app[d] = int_app[s]; });

	LoadSection(data, "size_mod", kStatModSize, [](int i, parser_wrapper::DataNode &m) {
		size_app[i].ac = Attr(m, "ac");
		size_app[i].interpolate = Attr(m, "interpolate");
		size_app[i].initiative = Attr(m, "initiative");
		size_app[i].shocking = Attr(m, "shocking");
	}, [](int d, int s) { size_app[d] = size_app[s]; });

	LoadSection(data, "weapon_mod", kStatModSize, [](int i, parser_wrapper::DataNode &m) {
		weapon_app[i].shocking = Attr(m, "shocking");
		weapon_app[i].bashing = Attr(m, "bashing");
		weapon_app[i].parrying = Attr(m, "parrying");
	}, [](int d, int s) { weapon_app[d] = weapon_app[s]; });

	LoadSection(data, "wiz_mod", kWisModSize, [](int i, parser_wrapper::DataNode &m) {
		mana[i] = Attr(m, "mana_amount");
	}, [](int d, int s) { mana[d] = mana[s]; });
}

void BasicValuesLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
