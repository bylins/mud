#ifndef SPELLS_INFO_H_
#define SPELLS_INFO_H_

#include "boot/cfg_manager.h"
#include "game_classes/classes_constants.h"
#include "game_magic/spells_constants.h"
#include "entities/entities_constants.h"
#include "game_abilities/talents_actions.h"
#include "structs/structs.h"
#include "structs/messages_data.h"
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

	MessagesData<ESpellMsg> messages_;

 public:
	SpellInfo() = default;
	SpellInfo(ESpell id, EItemMode mode)
		: BaseItem<ESpell>(id, mode) {};

	friend class SpellInfoBuilder;

	talents_actions::Actions actions;

	[[nodiscard]] const std::string &GetName() const { return name_; };
	/**
 	* Имя заклинания скилла в виде C-строки. По возможности используйте std::string
 	*/
	[[nodiscard]] const char *GetCName() const { return name_.c_str(); };
	[[nodiscard]] const std::string &GetEngName() const { return name_eng_; };
	/**
 	* Имя заклинания скилла в виде C-строки. По возможности используйте std::string
 	*/
	[[nodiscard]] const char *GetEngCName() const { return name_eng_.c_str(); };
	[[nodiscard]] const MessagesData<ESpellMsg> &Messages() const { return messages_; }
	const std::string &GetMsg(ESpellMsg id) const { return messages_.GetMsg(id); }

	[[nodiscard]] bool IsFlagged(Bitvector flag) const;
	[[nodiscard]] bool AllowTarget(Bitvector target_type) const;
	[[nodiscard]] bool IsViolent() const { return violent_; };

	[[nodiscard]] long GetDanger() const { return danger_; };
	[[nodiscard]] EPosition GetMinPos() const { return min_position_; };
	[[nodiscard]] EElement GetElement() const { return element_; };

	[[nodiscard]] int GetMinMana() const { return min_mana_; };
	[[nodiscard]] int GetMaxMana() const { return max_mana_; };
	[[nodiscard]] int GetManaChange() const { return mana_change_; };

	void Print(CharData *ch, std::ostringstream &buffer) const;
};

/**
 * Класс-билдер описания отдельного умения.
 */
class SpellInfoBuilder : public info_container::IItemBuilder<SpellInfo> {
 public:
	ItemPtr Build(DataNode &node) final;
 private:
	static ItemPtr ParseSpell(DataNode node);
	static ItemPtr ParseHeader(DataNode &node);
	static void ParseName(ItemPtr &info, DataNode &node);
	static void ParseMisc(ItemPtr &info, DataNode &node);
	static void ParseMana(ItemPtr &info, DataNode &node);
	static void ParseTargets(ItemPtr &info, DataNode &node);
	static void ParseFlags(ItemPtr &info, DataNode &node);
	static void ParseActions(ItemPtr &info, DataNode &node);
	static void ParseMessages(ItemPtr &info, DataNode &node);
};

using SpellsInfo = info_container::InfoContainer<ESpell, SpellInfo, SpellInfoBuilder>;

void InitSpellsCreate();

}

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

extern std::unordered_map<ESpell, SpellCreate> spell_create;


#endif //SPELLS_INFO_H_
