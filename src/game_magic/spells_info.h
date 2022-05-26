#ifndef SPELLS_INFO_H_
#define SPELLS_INFO_H_

#include "boot/cfg_manager.h"
#include "game_classes/classes_constants.h"
#include "game_magic/spells_constants.h"
#include "entities/entities_constants.h"
#include "structs/structs.h"
#include "structs/info_container.h"

namespace spells {

using DataNode = parser_wrapper::DataNode;

/**
 * Загрузчик конфига заклинания.
 */
class SpellsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

/**
 * Класс-описание конкретного заклинания.
 */
class SpellInfo : public info_container::BaseItem<ESpell> {
	std::string name_{"!undefined!"};
	std::string name_eng_{"!undefined!"};
	EPosition min_position_{EPosition::kLast};    // Position for caster   //
	Bitvector flags_{0};
	Bitvector targets_{0};
	long danger_{0};
	EElement element_{EElement::kUndefined};
	bool violent_{false};

	int min_mana_{100};        // Min amount of mana used by a spell (highest lev) //
	int max_mana_{120};        // Max amount of mana used by a spell (lowest lev) //
	int mana_change_{1};    // Change in mana used by spell from lev to lev //

 public:
	SpellInfo() = default;
	SpellInfo(ESpell id, EItemMode mode)
		: BaseItem<ESpell>(id, mode) {};

	friend class SpellInfoBuilder;

	[[nodiscard]] const std::string &GetName() const { return name_; };

	[[nodiscard]] bool IsFlagged(Bitvector flag) const;
	[[nodiscard]] bool AllowTarget(Bitvector target_type) const;
	[[nodiscard]] bool IsViolent() const { return violent_; };
	[[nodiscard]] bool IsBelongedTo(EElement element) const { return element_ == element; };

	[[nodiscard]] long GetDanger() const { return danger_; };
	[[nodiscard]] EPosition GetMinPos() const { return min_position_; };

	[[nodiscard]] int GetMinMana() const { return min_mana_; };
	[[nodiscard]] int GetMaxMana() const { return max_mana_; };
	[[nodiscard]] int GetManaChange() const { return mana_change_; };

	/**
	 * Имя заклинания скилла в виде C-строки. По возможности используйте std::string
	 */
	[[nodiscard]] const char *GetCName() const { return name_.c_str(); };

	void Print(std::ostringstream &buffer) const;
};

/**
 * Класс-билдер описания отдельного умения.
 */
class SpellInfoBuilder : public info_container::IItemBuilder<SpellInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseSpell(DataNode node);
};

using SpellsInfo = info_container::InfoContainer<ESpell, SpellInfo, SpellInfoBuilder>;

}

// ==================================================================================================================
// ==================================================================================================================
// ==================================================================================================================

extern const char *unused_spellname;

struct SpellInfo {
	EPosition min_position;    // Position for caster   //
	int mana_min;        // Min amount of mana used by a spell (highest lev) //
	int mana_max;        // Max amount of mana used by a spell (lowest lev) //
	int mana_change;    // Change in mana used by spell from lev to lev //
	long danger;
	Bitvector routines;
	int violent;
	int targets;
	EElement element;
	const char *name;
	const char *syn;
};

struct SpellCreateItem {
	std::array<int, 3> items;
	int rnumber;
	int min_caster_level;
};

struct SpellCreate {
	struct SpellCreateItem wand;
	struct SpellCreateItem scroll;
	struct SpellCreateItem potion;
	struct SpellCreateItem items;
	struct SpellCreateItem runes;
};

extern std::unordered_map<ESpell, SpellInfo> spell_info;
extern std::unordered_map<ESpell, SpellCreate> spell_create;

void InitSpells();

#endif //SPELLS_INFO_H_
