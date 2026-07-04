#ifndef SPELLS_INFO_H_
#define SPELLS_INFO_H_

#include "engine/boot/cfg_manager.h"
#include "gameplay/classes/classes_constants.h"
#include "spells_constants.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/abilities/talents_actions.h"
#include "engine/structs/structs.h"
#include "engine/structs/info_container.h"
#include "gameplay/abilities/feats_constants.h"   // issue.perk-action-patching: EFeat

#include <set>

namespace feats { struct TalentPatch; }   // issue.perk-action-patching (avoid feats.h<->spells_info.h cycle)

class CharData;

namespace spells {

using DataNode = parser_wrapper::DataNode;

// Three-state aggression flag for a spell, parsed from the <misc violent> attribute.
// (issue.ambiguous-spells)
//   kNo        -- a purely helpful spell ("N" / "0"), can be cast freely on anyone.
//   kYes       -- always violent ("Y" / "1"), triggers PK / retaliation / saving-shield logic.
//   kAmbiguous -- "A": helpful when cast on self / charmed pets / groupmates, but treated as
//                 fully aggressive (including PK liability) when cast on outsiders. NPC<->NPC
//                 mob casts always resolve to the helpful side; charmed pets resolve to their
//                 PC master's relationship.
// Backwards-compat note: IsViolent() returns true ONLY for kYes -- A is conservatively reported
// as "not always violent" for the call sites that still ask the static question. Sites that
// need the per-target answer call IsViolentAgainst(caster, target).
enum class EViolent : uint8_t { kNo, kYes, kAmbiguous };

/**
 * Загрузчик конфига заклинания.
 */
class SpellsLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	// issue.vedun-editor:
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

/**
 * Класс-описание конкретного заклинания.
 */
// issue.perk-action-patching: one talent patch that targets a spell (which feat + the patch payload).
struct TalentPatchRef {
	EFeat feat{EFeat::kUndefined};
	const feats::TalentPatch *patch{nullptr};
};

class SpellInfo : public info_container::BaseItem<ESpell> {
	std::string name_{"!undefined!"};
	std::string name_eng_{"!undefined!"};
	EPosition min_position_{EPosition::kLast};    // Position for caster   //
	Bitvector flags_{0};
	Bitvector targets_{0};
	long danger_{0};
	EElement element_{EElement::kUndefined};
	// issue.perk-action-patching (Phase 3 selectors): semantic category tags (e.g. "curse", "shield"),
	// an open content taxonomy authored in <tags val="a|b"/>. A talent_patch category= selector matches
	// spells carrying the tag. Free strings (not a typed enum) so authors add categories without a recompile;
	// a selector typo is caught by BuildTalentPatchIndex's "matched no spell" warning.
	std::set<std::string> tags_;
	EViolent violent_{EViolent::kNo};

	int min_mana_{100};        // Min amount of mana used by a spell (highest lev) //
	int max_mana_{120};        // Max amount of mana used by a spell (lowest lev) //
	int mana_change_{1};    // Change in mana used by spell from lev to lev //

	// Potency roll parameters of the spell (dice + skill/stat bonuses), shared by
	// all of the spell's effects (issue #3332). Filled from the <potency_roll>
	// section; spells without one keep the default-constructed roll.
	talents_actions::Roll potency_roll_;
	// Success roll parameters, filled from the <success_roll> section (issue.success-roll).
	// Dice-less; base_skill/base_stat mirror potency_roll, plus optional <bonus>/<thresholds>.
	// Parsed and stored only for now; not yet consumed (the success mechanic is future work).
	talents_actions::SuccessRoll success_roll_;
	// Spell-level caster gate (issue.spell-unification): a spell is castable by the
	// caster or not. Filled from the optional <caster_conditions> block (after
	// success_roll); empty by default. Checked once in CallMagic.
	talents_actions::CasterConditions caster_conditions_;
	// Material component requirements (issue.spellcomponents). Filled from the
	// optional <components>...</components> block; spells without one keep an
	// empty container, which ProcessMatComponents treats as "no component
	// required, cast proceeds".
	talents_actions::Components components_;

	//std::unordered_map<effects::EEffect, effects::EffectPtr> effects_;

 public:
	SpellInfo() = default;
	SpellInfo(ESpell id, EItemMode mode)
		: BaseItem<ESpell>(id, mode) {};

	friend class SpellInfoBuilder;

	talents_actions::Actions actions;

	// issue.perk-action-patching: talent patches targeting this spell, filled once at boot by
	// feats::BuildTalentPatchIndex(). Empty for nearly every spell -> the cast fast-path skips patching.
	// mutable: a post-load index on an otherwise const-at-runtime object.
	mutable std::vector<TalentPatchRef> talent_patches;

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

	[[nodiscard]] bool IsFlagged(Bitvector flag) const;
	[[nodiscard]] bool AllowTarget(Bitvector target_type) const;
	// Strict / static accessor: returns true only for spells declared <misc violent="Y">.
	// kAmbiguous reports false here -- call sites that need the per-target verdict use
	// IsViolentAgainst() below. (issue.ambiguous-spells)
	[[nodiscard]] bool IsViolent() const { return violent_ == EViolent::kYes; };
	[[nodiscard]] EViolent GetViolent() const { return violent_; };
	// Resolves an A (kAmbiguous) spell against a concrete caster<->target relationship.
	// Returns true iff this cast on this target should be treated as aggression
	// (PK rules apply, NPCs retaliate, saving-shields fire, dispel needs a strength
	// contest, etc.). For kYes always true; for kNo always false. For kAmbiguous:
	// allies (self / charmed-pet chain / same group, with NPC<->NPC mob casts defaulting
	// to allied) -> false; outsiders -> true. (issue.ambiguous-spells)
	[[nodiscard]] bool IsViolentAgainst(const CharData *caster, const CharData *target) const;

	[[nodiscard]] long GetDanger() const { return danger_; };
	[[nodiscard]] EPosition GetMinPos() const { return min_position_; };
	[[nodiscard]] EElement GetElement() const { return element_; };
	[[nodiscard]] bool HasTag(const std::string &tag) const { return tags_.count(tag) > 0; };

	[[nodiscard]] int GetMinMana() const { return min_mana_; };
	[[nodiscard]] int GetMaxMana() const { return max_mana_; };
	[[nodiscard]] int GetManaChange() const { return mana_change_; };

	[[nodiscard]] const talents_actions::Roll &GetPotencyRoll() const { return potency_roll_; };
	[[nodiscard]] const talents_actions::SuccessRoll &GetSuccessRoll() const { return success_roll_; };
	[[nodiscard]] const talents_actions::CasterConditions &GetCasterConditions() const { return caster_conditions_; };
	[[nodiscard]] const talents_actions::Components &GetComponents() const { return components_; };
	// Convenience: spell has a <verbal/> child in its <components> block.
	// Speech-blocking effects (kSilence) only stop verbal spells; non-verbal
	// spells can be cast under kSilence. See do_cast / CastSpell /
	// process_player_attack / SaySpell guards.
	[[nodiscard]] bool IsVerbal() const { return components_.HasVerbalComponent(); };

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
	static void ParseComponents(ItemPtr &info, DataNode &node);
	static void ParseMisc(ItemPtr &info, DataNode &node);
	static void ParseMana(ItemPtr &info, DataNode &node);
	static void ParseTargets(ItemPtr &info, DataNode &node);
	static void ParseFlags(ItemPtr &info, DataNode &node);
	static void ParseTags(ItemPtr &info, DataNode &node);
	static void ParseActions(ItemPtr &info, DataNode &node);
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

extern std::map<ESpell, SpellCreate> spell_create;


#endif //SPELLS_INFO_H_
