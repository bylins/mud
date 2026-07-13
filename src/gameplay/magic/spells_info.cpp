//#include "spells_info.h"

#include "engine/ui/color.h"
#include "gameplay/mechanics/minions.h"
#include "utils/utils_string.h"   // str_cmp
//#include "spells.h"
#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/groups.h"

#include <string_view>

/*
 * Эта структура и все с ней связанное - часть нерабочей, но недовырезанной системы то ли создания свитков,
 * посохов и так далее, то ли их применения. Сейчас на нее завязано применение рун. По уму, после переписывания
 * системы рун надо или вырезать остатки, или наоборот - довести до рабочего состояния и подключить.
 */
std::map<ESpell, SpellCreate> spell_create;

// issue.manual-cast: defined in magic.cpp (global scope); validates <manual_cast> handler names.
bool IsManualHandlerRegistered(const std::string &name);

namespace spells {

using ItemPtr = SpellInfoBuilder::ItemPtr;

void SpellsLoader::Load(DataNode data) {
	MUD::Spells().Init(data.Children());
}

void SpellsLoader::Reload(DataNode data) {
	MUD::Spells().Reload(data.Children());
}

// issue.vedun-editor: editor discovery + safe-commit validation.
std::string SpellsLoader::EditableWhat() const {
	return "spell";
}

std::vector<cfg_manager::EditableElement> SpellsLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &spell : MUD::Spells()) {
		out.push_back({NAME_BY_ITEM<ESpell>(spell.GetId()), spell.GetName()});
	}
	return out;
}

cfg_manager::ValidationResult SpellsLoader::Validate(DataNode &doc) const {
	if (MUD::Spells().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Spell data failed to parse (see syslog for the offending spell/attribute)."};
}

std::string SpellsLoader::CanonicalElementId(const std::string &id) const {
	for (const auto &[value, name] : NAMES_OF<ESpell>()) {
		if (value != ESpell::kUndefined && str_cmp(name, id) == 0) {
			return name;
		}
	}
	return "";
}

parser_wrapper::DataNode SpellsLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	// Minimal skeleton: a kTesting spell with just its id; the editor fills in the rest and the
	// save-time dry-parse validates before it is written.
	auto node = root.AddChild("spell");
	node.SetValue("id", id);
	node.SetValue("mode", "kTesting");
	return node;
}

ItemPtr SpellInfoBuilder::Build(DataNode &node) {
	try {
		return ParseSpell(node);
	} catch (std::exception &e) {
		err_log("Spell parsing error (incorrect value '%s')", e.what());
		return nullptr;
	}
}

ItemPtr SpellInfoBuilder::ParseSpell(DataNode node) {
	auto info = ParseHeader(node);

	ParseName(info, node);
	ParseComponents(info, node);
	ParseMisc(info, node);
	ParseMana(info, node);
	ParseTargets(info, node);
	ParseFlags(info, node);
	ParseTags(info, node);
	if (node.GoToChild("potency_roll")) {
		info->potency_roll_ = talents_actions::Roll(node);
		node.GoToParent();
	}
	if (node.GoToChild("success_roll")) {
		info->success_roll_ = talents_actions::SuccessRoll(node);
		node.GoToParent();
	}
	if (node.GoToChild("caster_conditions")) {
		talents_actions::Actions::ParseCasterConditions(info->caster_conditions_, node);
		node.GoToParent();
	}
	ParseActions(info, node);
	// issue.manual-cast: a <manual_cast> handler name must resolve to a registered handler.
	for (const auto &act : info->actions.list()) {
		const std::string &mh = act.GetManualHandler();
		if (!mh.empty() && !IsManualHandlerRegistered(mh)) {
			char buf[160];
			snprintf(buf, sizeof(buf), "Spell %s: unknown <manual_cast> handler '%s'.",
					 NAME_BY_ITEM(info->GetId()).c_str(), mh.c_str());
			mudlog(buf, CMP, kLvlGod, SYSLOG, true);
		}
	}

	// Cross-validate: kAmbiguous (violent="A") is meaningless for a room paint
	// because there is no per-target relationship to resolve. Clamp to kYes
	// conservatively and SYSERR so the misconfiguration is visible.
	// (issue.ambiguous-spells)
	if (info->violent_ == EViolent::kAmbiguous && info->IsFlagged(kMagRoom)) {
		char buf[128];
		snprintf(buf, sizeof(buf),
				 "Spell %s declared violent=A together with kMagRoom -- clamped to Y.",
				 NAME_BY_ITEM(info->GetId()).c_str());
		mudlog(buf, CMP, kLvlGod, SYSLOG, true);
		info->violent_ = EViolent::kYes;
	}

	return info;
}

ItemPtr SpellInfoBuilder::ParseHeader(DataNode &node) {
	auto id{ESpell::kUndefined};
	try {
		id = parse::ReadAsConstant<ESpell>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect spell id (%s).", e.what());
		throw;
	}
	auto mode = SpellInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	auto info = std::make_shared<SpellInfo>(id, mode);
	try {
		info->element_ = parse::ReadAsConstant<EElement>(node.GetValue("element"));
	} catch (std::exception &) {
	}

	return info;
}

void SpellInfoBuilder::ParseName(ItemPtr &info, DataNode &node) {
	// issue.thing-names: the English name stays in spells.xml as an admin label; the Russian display
	// name now lives in spell_msg.xml and is read from the message container (loaded just before spells).
	if (node.GoToChild("name")) {
		try {
			info->name_eng_ = parse::ReadAsStr(node.GetValue("val"));
		} catch (std::exception &e) {
			err_log("Incorrect 'name' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
	info->name_ = MUD::SpellMessages().GetName(info->GetId());
}

// Parses the <components>...</components> spell-level block placed right after
// <name> (issue.spellcomponents). Errors bubble up via Components' ctor;
// material entries with invalid vnums abort spell loading just like Roll/Actions
// errors do today. Spells without a <components> tag keep the default empty
// container (cast proceeds without any material requirement).
void SpellInfoBuilder::ParseComponents(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("components")) {
		try {
			info->components_ = talents_actions::Components(node);
		} catch (std::exception &e) {
			err_log("Incorrect 'components' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

// Parses the <misc violent> attribute as the three-state EViolent enum
// (issue.ambiguous-spells). Accepts "Y"/"1" (kYes), "N"/"0"/empty (kNo),
// "A" (kAmbiguous); anything else falls back to kNo with a CMP-level mudlog
// so the misconfiguration is visible without breaking the load.
static EViolent ParseViolent(const std::string &raw) {
	if (raw.empty()) return EViolent::kNo;
	if (raw == "Y" || raw == "y" || raw == "1") return EViolent::kYes;
	if (raw == "N" || raw == "n" || raw == "0") return EViolent::kNo;
	if (raw == "A" || raw == "a") return EViolent::kAmbiguous;
	char buf[128];
	snprintf(buf, sizeof(buf), "Unknown 'violent' value '%s' -- assuming N.", raw.c_str());
	mudlog(buf, CMP, kLvlGod, SYSLOG, true);
	return EViolent::kNo;
}

void SpellInfoBuilder::ParseMisc(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("misc")) {
		try {
			info->min_position_ = parse::ReadAsConstant<EPosition>(node.GetValue("pos"));
			info->violent_ = ParseViolent(node.GetValue("violent"));
			info->danger_ = parse::ReadAsInt(node.GetValue("danger"));
		} catch (std::exception &e) {
			err_log("Incorrect 'misc' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

void SpellInfoBuilder::ParseMana(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("mana")) {
		try {
			info->min_mana_ = parse::ReadAsInt(node.GetValue("min"));
			info->max_mana_ = parse::ReadAsInt(node.GetValue("max"));
			info->mana_change_ = parse::ReadAsInt(node.GetValue("change"));
		} catch (std::exception &e) {
			err_log("Incorrect 'mana' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

void SpellInfoBuilder::ParseTargets(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("targets")) {
		try {
			info->targets_ = parse::ReadAsConstantsBitvector<ETarget>(node.GetValue("val"));
		} catch (std::exception &e) {
			err_log("Incorrect 'targets' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

void SpellInfoBuilder::ParseFlags(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("flags")) {
		try {
			info->flags_ = parse::ReadAsConstantsBitvector<EMagic>(node.GetValue("val"));
		} catch (std::exception &e) {
			err_log("Incorrect 'flags' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

// issue.perk-action-patching (Phase 3): <tags val="curse|shield"/> -> the spell's semantic category set.
void SpellInfoBuilder::ParseTags(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("tags")) {
		if (const char *v = node.GetValue("val"); v && *v) {
			for (const auto &t : utils::Split(v, '|')) {
				if (!t.empty()) { info->tags_.insert(t); }
			}
		}
		node.GoToParent();
	}
}

void SpellInfoBuilder::ParseActions(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("talent_actions")) {
		info->actions.Build(node);
		node.GoToParent();
	}
}

bool SpellInfo::IsFlagged(const Bitvector flag) const {
	return IS_SET(flags_, flag);
}

bool SpellInfo::AllowTarget(const Bitvector target_type) const {
	return IS_SET(targets_, target_type);
}

// Walks up the charm-chain so a charmed NPC resolves to its PC master for
// alliance purposes (issue.ambiguous-spells). The loop is bounded at a depth
// of 4 so a pathological charm cycle can't hang the cast pipeline.
static const CharData *AllyRoot(const CharData *ch) {
	if (!ch) return nullptr;
	for (int depth = 0; depth < 4 && IsCharmice(ch) && ch->has_master(); ++depth) {
		const auto *next = ch->get_master();
		if (!next || next == ch) break;
		ch = next;
	}
	return ch;
}

bool SpellInfo::IsViolentAgainst(const CharData *caster, const CharData *target) const {
	if (violent_ == EViolent::kYes) return true;
	if (violent_ == EViolent::kNo) return false;
	// kAmbiguous: relationship-dependent.
	if (!caster || !target || caster == target) return false;
	const auto *cr = AllyRoot(caster);
	const auto *tr = AllyRoot(target);
	if (!cr || !tr || cr == tr) return false;
	// Two NPCs whose roots are both NPCs: kvirund's "all aggressive mobs are
	// conditionally allied" rule -- A spells between mobs always resolve to the
	// helpful side. A PC's charmed pet has a PC root and falls into the cases below.
	if (cr->IsNpc() && tr->IsNpc()) return false;
	if (group::same_group(const_cast<CharData *>(cr), const_cast<CharData *>(tr))) return false;
	return true;
}

void SpellInfo::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << "Print spell:" << "\r\n"
		   << " Id: " << kColorGrn << NAME_BY_ITEM<ESpell>(GetId()) << kColorNrm
		   << " Mode: " << kColorGrn << NAME_BY_ITEM<EItemMode>(GetMode()) << kColorNrm << "\r\n"
		   << " Name (rus): " << kColorGrn << name_ << kColorNrm << "\r\n"
		   << " Name (eng): " << kColorGrn << name_eng_ << kColorNrm << "\r\n"
		   << " Element: " << kColorGrn << NAME_BY_ITEM<EElement>(element_) << kColorNrm << "\r\n"
		   << " Min position: " << kColorGrn << NAME_BY_ITEM<EPosition>(min_position_) << kColorNrm << "\r\n"
		   << " Violent: " << kColorGrn
		   << (violent_ == EViolent::kYes ? "Yes" : violent_ == EViolent::kAmbiguous ? "Ambiguous" : "No")
		   << kColorNrm << "\r\n"
		   << " Danger: " << kColorGrn << danger_ << kColorNrm << "\r\n"
		   << " Mana min: " << kColorGrn << min_mana_ << kColorNrm
		   << " Mana max: " << kColorGrn << max_mana_ << kColorNrm
		   << " Mana change: " << kColorGrn << mana_change_ << kColorNrm << "\r\n"
		   << " Flags: " << kColorGrn << parse::BitvectorToString<EMagic>(flags_) << kColorNrm << "\r\n"
		   << " Targets: " << kColorGrn << parse::BitvectorToString<ETarget>(targets_) << kColorNrm << "\r\n\r\n";

	potency_roll_.Print(ch, buffer);
	success_roll_.Print(ch, buffer);
	if (!caster_conditions_.empty()) {
		buffer << " CasterConditions:\r\n";
	}
	actions.Print(ch, buffer);
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
