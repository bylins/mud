#include "obj_affects.h"
#include "obj_affects_loader.h"
#include "obj_affect_messages.h"

#include "utils/parser_wrapper.h"
#include "utils/utils_parse.h"        // parse::ReadAsInt
#include "utils/logger.h"
#include "administration/privilege.h"   // issue.obj-affects: IsImmortal (examine visibility)
#include "engine/structs/msg_container.h"
#include "gameplay/abilities/talents_actions.h"   // talents_actions::Actions (per-affect trigger actions)

#include "engine/entities/obj_data.h"
#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"   // world_objects (decay manager), GetObjectPrototype
#include "engine/core/utils_char_obj.inl"
#include "utils/utils.h"                // FormatTimeToStr

#include <array>
#include <set>
#include <memory>
#include <sstream>

// issue.obj-affects: core type layer for affects that live ON an item. See obj_affects.h for the
// design. This TU owns the identity name maps, the legacy ESpell->EObjAffect migration table, and the
// per-affect registry (which extra-flag the affect owns + whether it is player-dispellable). The
// registry is populated from cfg/affects/obj_affects.xml by ObjAffectsLoader (BuildRegistry); until then the
// built-in defaults below apply.

namespace obj_affects {

// --- identity name maps (mirror the EAffect / ERoomAffect maps) -------------------------------------
namespace {
std::map<EObjAffect, std::string> g_obj_affect_name_by_value;
std::map<std::string, EObjAffect> g_obj_affect_value_by_name;
void init_obj_affect_names() {
	g_obj_affect_name_by_value = {
		{EObjAffect::kInvisible, "kInvisible"},
		{EObjAffect::kCurse, "kCurse"},
		{EObjAffect::kPoisoned, "kPoisoned"},
		{EObjAffect::kBless, "kBless"},
		{EObjAffect::kFly, "kFly"},
		{EObjAffect::kLight, "kLight"},
		{EObjAffect::kDartTrap, "kDartTrap"},
		{EObjAffect::kSuppressed, "kSuppressed"},
	};
	for (const auto &[value, token] : g_obj_affect_name_by_value) {
		g_obj_affect_value_by_name.emplace(token, value);
	}
}
}  // namespace

// --- per-affect registry -----------------------------------------------------------------------------
namespace {
struct ObjAffectMeta {
	bool has_flag{false};
	EObjFlag flag{EObjFlag::kGlow};   // meaningful only when has_flag
	bool dispellable{true};
	grammar::ECase msg_case{grammar::ECase::kGen};   // case for "%s" (item name) in lifecycle messages
	EAffect see_affect{EAffect::kDetectMagic};   // affect the viewer needs to SEE it (kUndefined = always)
};

std::array<ObjAffectMeta, to_underlying(EObjAffect::kCount)> g_meta;
bool g_meta_loaded = false;

// Per-affect <actions> (each action gated by its <trigger>), keyed by EObjAffect. Built from
// cfg/affects/obj_affects.xml by BuildRegistry via Actions::Build; empty for affects that declare none.
std::array<talents_actions::Actions, to_underlying(EObjAffect::kCount)> g_obj_affect_actions;

void ensure_meta() {
	if (g_meta_loaded) {
		return;
	}
	// Built-in defaults (cfg/affects/obj_affects.xml overrides these via BuildRegistry).
	using grammar::ECase;
	auto set = [](EObjAffect a, bool has_flag, EObjFlag flag, bool dispellable, ECase c, EAffect see) {
		auto &m = g_meta[to_underlying(a)];
		m.has_flag = has_flag;
		m.flag = flag;
		m.dispellable = dispellable;
		m.msg_case = c;
		m.see_affect = see;
	};
	set(EObjAffect::kInvisible, true, EObjFlag::kInvisible, true, ECase::kGen, EAffect::kDetectInvisible);
	set(EObjAffect::kCurse, true, EObjFlag::kNodrop, true, ECase::kGen, EAffect::kDetectMagic);
	set(EObjAffect::kPoisoned, false, EObjFlag::kPoisoned, true, ECase::kGen, EAffect::kDetectMagic);
	set(EObjAffect::kBless, true, EObjFlag::kBless, true, ECase::kAcc, EAffect::kDetectMagic);
	set(EObjAffect::kFly, true, EObjFlag::kFlying, true, ECase::kGen, EAffect::kUndefined);
	set(EObjAffect::kLight, true, EObjFlag::kGlow, true, ECase::kGen, EAffect::kUndefined);
	set(EObjAffect::kDartTrap, false, EObjFlag::kGlow, true, ECase::kGen, EAffect::kDetectMagic);
	set(EObjAffect::kSuppressed, false, EObjFlag::kGlow, false, ECase::kGen, EAffect::kUndefined);
	g_meta_loaded = true;
}

const ObjAffectMeta &meta(EObjAffect a) {
	ensure_meta();
	return g_meta[to_underlying(a)];
}
}  // namespace

bool HasFlag(EObjAffect affect) { return meta(affect).has_flag; }
EObjFlag Flag(EObjAffect affect) { return meta(affect).flag; }
bool Dispellable(EObjAffect affect) { return meta(affect).dispellable; }
grammar::ECase MsgCase(EObjAffect affect) { return meta(affect).msg_case; }
EAffect SeeAffect(EObjAffect affect) { return meta(affect).see_affect; }

// --- legacy on-disk migration ------------------------------------------------------------------------
bool IsPoisonSpell(ESpell spell) {
	return spell == ESpell::kAconitumPoison
		|| spell == ESpell::kScopolaPoison
		|| spell == ESpell::kBelenaPoison
		|| spell == ESpell::kDaturaPoison;
}

EObjAffect BySpell(ESpell spell) {
	if (IsPoisonSpell(spell)) {
		return EObjAffect::kPoisoned;
	}
	switch (spell) {
		case ESpell::kBless: return EObjAffect::kBless;
		case ESpell::kFly: return EObjAffect::kFly;
		case ESpell::kLight: return EObjAffect::kLight;
		default: return EObjAffect::kUndefined;
	}
}

// --- cfg/affects/obj_affects.xml registry build --------------------------------------------------------------
namespace {
void ValidateRegistry(parser_wrapper::DataNode data) {
	std::set<std::string> seen;
	for (auto &node : data.Children("obj_affect")) {
		const char *id = node.GetValue("id");
		if (!id || !*id) {
			log("SYSERROR: obj_affects.xml: <obj_affect> without an id -- skipped.");
			continue;
		}
		try {
			(void) ITEM_BY_NAME<EObjAffect>(id);
		} catch (const std::out_of_range &) {
			log("SYSERROR: obj_affects.xml: unknown obj-affect id '%s' -- skipped.", id);
			continue;
		}
		seen.insert(id);
	}
	for (const auto &[affect, token] : NAMES_OF<EObjAffect>()) {
		if (seen.find(token) == seen.end()) {
			log("SYSERROR: obj_affects.xml: missing <obj_affect id=\"%s\">.", token.c_str());
		}
	}
}
}  // namespace

void BuildRegistry(parser_wrapper::DataNode data) {
	ensure_meta();  // start from built-in defaults; the file overrides per-affect below.
	for (auto &a : g_obj_affect_actions) { a = talents_actions::Actions{}; }   // clear on (re)load
	for (auto &node : data.Children("obj_affect")) {
		const char *id = node.GetValue("id");
		if (!id || !*id) {
			continue;
		}
		EObjAffect affect;
		try {
			affect = ITEM_BY_NAME<EObjAffect>(id);
		} catch (const std::out_of_range &) {
			continue;  // unknown id already reported by ValidateRegistry
		}
		auto &m = g_meta[to_underlying(affect)];
		// <obj_affect id=".." flag="kBless"> -- the extra-flag this affect owns while active. Absent =
		// the affect owns no flag (e.g. kPoisoned); an empty flag="" explicitly clears it.
		if (const char *flag = node.GetValue("flag"); flag) {
			if (*flag) {
				try {
					m.flag = ITEM_BY_NAME<EObjFlag>(std::string(flag));
					m.has_flag = true;
				} catch (const std::out_of_range &) {
					log("SYSERROR: obj_affects.xml: unknown flag '%s' for obj-affect '%s'.", flag, id);
				}
			} else {
				m.has_flag = false;
			}
		}
		if (const char *disp = node.GetValue("dispellable"); disp && *disp) {
			// accept the Vedun bool editor's Y/N as well as 1/0/true/false
			const char c = *disp;
			m.dispellable = (c == 'Y' || c == 'y' || c == 'T' || c == 't' || c == '1');
		}
		if (const char *mc = node.GetValue("msg_case"); mc && *mc) {
			static const std::map<std::string, grammar::ECase> kCaseByName{
				{"kNom", grammar::ECase::kNom}, {"kGen", grammar::ECase::kGen},
				{"kDat", grammar::ECase::kDat}, {"kAcc", grammar::ECase::kAcc},
				{"kIns", grammar::ECase::kIns}, {"kPre", grammar::ECase::kPre},
			};
			if (auto it = kCaseByName.find(mc); it != kCaseByName.end()) {
				m.msg_case = it->second;
			} else {
				log("SYSERROR: obj_affects.xml: unknown msg_case '%s' for obj-affect '%s'.", mc, id);
			}
		}
		if (const char *sa = node.GetValue("see_affect"); sa && *sa) {
			try {
				m.see_affect = ITEM_BY_NAME<EAffect>(std::string(sa));
			} catch (const std::out_of_range &) {
				log("SYSERROR: obj_affects.xml: unknown see_affect '%s' for obj-affect '%s'.", sa, id);
			}
		}
		// <actions> block (mirrors room_affects.xml): each <action> carries its own <trigger>, parsed
		// by Actions::Build. Read off a copy of the iteration node (the room-affect pattern).
		{
			auto act_node = node;
			if (act_node.GoToChild("actions")) {
				g_obj_affect_actions[to_underlying(affect)].Build(act_node);
			}
		}
	}
}

const talents_actions::Actions &ObjAffectActions(EObjAffect affect) {
	static const talents_actions::Actions kEmpty;
	const auto idx = to_underlying(affect);
	return idx < g_obj_affect_actions.size() ? g_obj_affect_actions[idx] : kEmpty;
}

// --- ObjAffectsLoader (cfg id "obj_affects") ---------------------------------------------------------
void ObjAffectsLoader::Load(parser_wrapper::DataNode data) { ValidateRegistry(data); BuildRegistry(data); }
void ObjAffectsLoader::Reload(parser_wrapper::DataNode data) { ValidateRegistry(data); BuildRegistry(data); }

// issue.obj-affects: in-game editing of cfg/affects/obj_affects.xml (`vedun obj_affects`).
std::string ObjAffectsLoader::EditableWhat() const { return "obj_affects"; }

std::vector<cfg_manager::EditableElement> ObjAffectsLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &[oa, token] : NAMES_OF<EObjAffect>()) {
		if (oa == EObjAffect::kUndefined) {
			continue;
		}
		const std::string &label = ObjAffectMsgRaw(oa, EObjAffectMsgType::kShortDesc);
		out.push_back({token, label.empty() ? token : label});
	}
	return out;
}

// Dry-run on save: the .scheme constrains the editor's pick-lists (EObjAffect / EObjAffectFlag / ECase);
// this re-checks the obj-affect identity + owned flag against the enum maps so a bad value never reaches
// the file. (dispellable / msg_case are typed by the scheme and caught by BuildRegistry on reload.)
cfg_manager::ValidationResult ObjAffectsLoader::Validate(parser_wrapper::DataNode &doc) const {
	for (auto &oa : doc.Children("obj_affect")) {
		const char *id = oa.GetValue("id");
		if (!id || !*id) {
			return {false, "<obj_affect> without an id."};
		}
		try {
			(void) ITEM_BY_NAME<EObjAffect>(id);
		} catch (const std::out_of_range &) {
			return {false, std::string("unknown obj affect id '") + id + "'."};
		}
		if (const char *flag = oa.GetValue("flag"); flag && *flag) {
			try {
				(void) ITEM_BY_NAME<EObjFlag>(std::string(flag));
			} catch (const std::out_of_range &) {
				return {false, std::string("unknown obj flag '") + flag + "' for '" + id + "'."};
			}
		}
	}
	return {true, ""};
}

// --- runtime: impose / remove / tick / query (was TimedSpell in magic_objects.cpp) -------------------
namespace {
// Clear the affect's owned extra-flag on the instance, unless the prototype carries it innately (an
// innate prototype flag is a permanent property of the item and must survive the affect).
void clear_owned_flag(ObjData *obj, EObjAffect type) {
	if (!HasFlag(type)) {
		return;
	}
	const EObjFlag f = Flag(type);
	auto proto = GetObjectPrototype(GET_OBJ_VNUM(obj));
	if (!proto || !proto->has_flag(f)) {
		obj->unset_extraflag(f);
	}
}

// Emit an affect lifecycle line to whoever holds/wears the item. The template's "%s" (if any) is
// replaced by the item name in the affect's declared grammatical case.
void emit_holder_message(ObjData *obj, EObjAffect type, EObjAffectMsgType slot) {
	CharData *ch = obj->get_carried_by() ? obj->get_carried_by() : obj->get_worn_by();
	if (!ch) {
		return;
	}
	std::string msg = ObjAffectMsgRaw(type, slot);
	if (msg.empty()) {
		return;
	}
	if (const auto pos = msg.find("%s"); pos != std::string::npos) {
		msg.replace(pos, 2, obj->get_PName(MsgCase(type)));
	}
	SendMsgToChar(ch, "%s", msg.c_str());
}

// Shared removal: clear the flag (prototype-aware), optionally message, erase.
void remove_at(ObjData *obj, ObjAffectIt it, bool message, EObjAffectMsgType slot) {
	const EObjAffect type = (*it)->affect_type;
	if (message) {
		emit_holder_message(obj, type, slot);
	}
	clear_owned_flag(obj, type);
	obj->get_obj_affects().erase(it);
}
}  // namespace

void Impose(ObjData *obj, EObjAffect type, int duration, int modifier, long caster_id, float potency, int charges) {
	if (type == EObjAffect::kUndefined || type == EObjAffect::kCount) {
		log("SYSERROR: obj_affects::Impose bad type=%d on obj vnum=%d",
			to_underlying(type), GET_OBJ_VNUM(obj));
		return;
	}
	auto &list = obj->get_obj_affects();
	// Refresh an existing same-type affect in place (also enforces one-poison-per-weapon, since all
	// poisons share the kPoisoned identity -- the new poison's modifier just replaces the old one).
	ObjAffect::shared_ptr aff;
	for (auto &existing : list) {
		if (existing->affect_type == type) {
			aff = existing;
			break;
		}
	}
	if (!aff) {
		aff = std::make_shared<ObjAffect>();
		aff->affect_type = type;
		aff->location = EObjApply::kNone;
		list.push_back(aff);
	}
	aff->duration = duration;
	aff->modifier = modifier;
	aff->caster_id = caster_id;
	aff->potency = potency;
	aff->charges = charges;
	if (HasFlag(type)) {
		obj->set_extra_flag(Flag(type));
	}
	if (duration > 0) {
		world_objects.decay_manager().add_periodic_obj(obj);
	}
}

void Remove(ObjData *obj, EObjAffect type, bool message) {
	auto it = Find(obj, type);
	if (it != obj->get_obj_affects().end()) {
		remove_at(obj, it, message, EObjAffectMsgType::kAffDispelledToChar);
	}
}

void Tick(ObjData *obj, int minutes, bool tick_suppressions) {
	auto &list = obj->get_obj_affects();
	for (auto it = list.begin(); it != list.end(); /* advanced in body */) {
		auto &aff = *it;
		if (aff->duration == -1) {   // permanent affect: never ticks down
			++it;
			continue;
		}
		if (!tick_suppressions && aff->affect_type == EObjAffect::kSuppressed) {
			// issue.obj-suppressor-affect: equipment-affect suppression counts PLAY time only, so the
			// offline catch-up (dec_timer with the rented hours) must not decrement or expire it. Ticked
			// online by process_periodic_effects instead.
			++it;
			continue;
		}
		aff->duration -= minutes;
		if (aff->duration <= 0) {
			const EObjAffect type = aff->affect_type;
			emit_holder_message(obj, type, EObjAffectMsgType::kAffExpiredToChar);
			clear_owned_flag(obj, type);
			it = list.erase(it);
		} else {
			++it;
		}
	}
}

void SpendCharge(ObjData *obj, EObjAffect type) {
	auto it = Find(obj, type);
	if (it == obj->get_obj_affects().end()) {
		return;
	}
	if ((*it)->charges == -1) {
		return;   // unlimited charges (the default): nothing to spend
	}
	if (--(*it)->charges <= 0) {
		remove_at(obj, it, true, EObjAffectMsgType::kAffExpiredToChar);   // consumed like an expiry
	}
}

ObjAffectIt Find(ObjData *obj, EObjAffect type) {
	auto &list = obj->get_obj_affects();
	for (auto it = list.begin(); it != list.end(); ++it) {
		if ((*it)->affect_type == type) {
			return it;
		}
	}
	return list.end();
}

bool Has(const ObjData *obj, EObjAffect type) {
	for (const auto &aff : obj->get_obj_affects()) {
		if (aff->affect_type == type) {
			return true;
		}
	}
	return false;
}

// --- equipment-affect suppression (issue.obj-suppressor-affect) ---------------------------------------
void SuppressEquipAffect(ObjData *obj, EAffect aff, int hours) {
	if (hours <= 0) {
		return;
	}
	const int mod = to_underlying(aff);
	auto &list = obj->get_obj_affects();
	for (auto &existing : list) {   // refresh an existing suppression of the same EAffect in place
		if (existing->affect_type == EObjAffect::kSuppressed && existing->modifier == mod) {
			existing->duration = hours;
			world_objects.decay_manager().add_periodic_obj(obj);
			return;
		}
	}
	auto a = std::make_shared<ObjAffect>();   // one kSuppressed instance per suppressed EAffect
	a->affect_type = EObjAffect::kSuppressed;
	a->location = EObjApply::kNone;
	a->duration = hours;
	a->modifier = mod;
	list.push_back(a);
	world_objects.decay_manager().add_periodic_obj(obj);
}

bool IsEquipAffectSuppressed(const ObjData *obj, EAffect aff) {
	const int mod = to_underlying(aff);
	for (const auto &a : obj->get_obj_affects()) {
		if (a->affect_type == EObjAffect::kSuppressed && a->modifier == mod) {
			return true;
		}
	}
	return false;
}

std::vector<std::pair<EAffect, int>> SuppressedEquipAffects(const ObjData *obj) {
	std::vector<std::pair<EAffect, int>> out;
	for (const auto &a : obj->get_obj_affects()) {
		if (a->affect_type == EObjAffect::kSuppressed) {
			out.emplace_back(static_cast<EAffect>(a->modifier), a->duration);
		}
	}
	return out;
}

ESpell PoisonSpell(const ObjData *obj) {
	for (const auto &aff : obj->get_obj_affects()) {
		if (aff->affect_type == EObjAffect::kPoisoned) {
			return static_cast<ESpell>(aff->modifier);
		}
	}
	return ESpell::kUndefined;
}

std::string Diag(const ObjData *obj, const CharData *viewer) {
	std::string out;
	for (const auto &aff : obj->get_obj_affects()) {
		const EObjAffect type = aff->affect_type;
		if (type == EObjAffect::kSuppressed) {   // meta-affect: never a displayable item property
			continue;
		}
		// visibility: a normal inspector sees the affect only if they carry SeeAffect(type) (kUndefined =
		// always, e.g. fly/light). A null viewer (god `stat`) or an immortal sees every affect.
		const EAffect need = SeeAffect(type);
		if (viewer && need != EAffect::kUndefined
				&& !privilege::IsImmortal(viewer) && !AFF_FLAGGED(viewer, need)) {
			continue;
		}
		std::string line = ObjAffectMsgRaw(type, EObjAffectMsgType::kDiagToChar);
		if (line.empty()) {
			const std::string &sd = ObjAffectMsgRaw(type, EObjAffectMsgType::kShortDesc);
			line = sd.empty() ? NAME_BY_ITEM<EObjAffect>(type) : sd;
		}
		while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
			line.pop_back();
		}
		if (aff->duration > 0) {
			line += " (" + FormatTimeToStr(aff->duration, true) + ")";
		}
		out += line + "\r\n";
	}
	return out;
}

std::string Serialize(const ObjData *obj) {
	const auto &list = obj->get_obj_affects();
	if (list.empty()) {
		return "";
	}
	std::stringstream out;
	out << "OAff: ";
	for (const auto &aff : list) {
		out << NAME_BY_ITEM<EObjAffect>(aff->affect_type) << " "
			<< aff->duration << " " << aff->modifier << "\n";
	}
	out << "~\n";
	return out.str();
}

// --- obj-affect message container (cfg/messages/ru/obj_affect_msg.xml) --------------------------------
namespace {
msg_container::MsgContainer<EObjAffect, EObjAffectMsgType> &ObjAffectMsgContainer() {
	static msg_container::MsgContainer<EObjAffect, EObjAffectMsgType> container;
	return container;
}
}  // namespace

const std::string &ObjAffectMsg(EObjAffect affect, EObjAffectMsgType slot) {
	return ObjAffectMsgContainer().GetMessage(affect, slot);
}
const std::string &ObjAffectMsgRaw(EObjAffect affect, EObjAffectMsgType slot) {
	static const std::string kEmpty;
	auto &c = ObjAffectMsgContainer();
	return c.IsKnown(affect) ? c[affect].GetMessage(slot) : kEmpty;
}
void ObjAffectMessagesLoader::Load(parser_wrapper::DataNode data) {
	ObjAffectMsgContainer().Init(data.Children());
}
void ObjAffectMessagesLoader::Reload(parser_wrapper::DataNode data) {
	ObjAffectMsgContainer().Reload(data.Children());
}

}  // namespace obj_affects

// --- meta_enum specializations (global scope, like ERoomAffect) --------------------------------------
template<>
const std::string &NAME_BY_ITEM<obj_affects::EObjAffect>(obj_affects::EObjAffect item) {
	if (obj_affects::g_obj_affect_name_by_value.empty()) { obj_affects::init_obj_affect_names(); }
	return obj_affects::g_obj_affect_name_by_value.at(item);
}
template<>
obj_affects::EObjAffect ITEM_BY_NAME<obj_affects::EObjAffect>(const std::string &name) {
	if (obj_affects::g_obj_affect_value_by_name.empty()) { obj_affects::init_obj_affect_names(); }
	return obj_affects::g_obj_affect_value_by_name.at(name);
}
template<>
const std::map<obj_affects::EObjAffect, std::string> &NAMES_OF<obj_affects::EObjAffect>() {
	if (obj_affects::g_obj_affect_name_by_value.empty()) { obj_affects::init_obj_affect_names(); }
	return obj_affects::g_obj_affect_name_by_value;
}

// --- EObjAffectMsgType name map (mirrors kRoomAffectMsgTypeNames) -------------------------------------
namespace {
const std::map<obj_affects::EObjAffectMsgType, std::string> kObjAffectMsgTypeNames{
	{obj_affects::EObjAffectMsgType::kUndefined, "kUndefined"},
	{obj_affects::EObjAffectMsgType::kShortDesc, "kShortDesc"},
	{obj_affects::EObjAffectMsgType::kDiagToChar, "kDiagToChar"},
	{obj_affects::EObjAffectMsgType::kAffImposedToChar, "kAffImposedToChar"},
	{obj_affects::EObjAffectMsgType::kAffImposedToRoom, "kAffImposedToRoom"},
	{obj_affects::EObjAffectMsgType::kAffExpiredToChar, "kAffExpiredToChar"},
	{obj_affects::EObjAffectMsgType::kAffDispelledToChar, "kAffDispelledToChar"},
	{obj_affects::EObjAffectMsgType::kTriggerToChar, "kTriggerToChar"},
	{obj_affects::EObjAffectMsgType::kTriggerToRoom, "kTriggerToRoom"},
	{obj_affects::EObjAffectMsgType::kDamageToChar, "kDamageToChar"},
	{obj_affects::EObjAffectMsgType::kDamageToVict, "kDamageToVict"},
	{obj_affects::EObjAffectMsgType::kDamageToRoom, "kDamageToRoom"},
};
}  // namespace

template<>
const std::string &NAME_BY_ITEM<obj_affects::EObjAffectMsgType>(obj_affects::EObjAffectMsgType item) {
	return kObjAffectMsgTypeNames.at(item);
}
template<>
const std::map<obj_affects::EObjAffectMsgType, std::string> &NAMES_OF<obj_affects::EObjAffectMsgType>() {
	return kObjAffectMsgTypeNames;
}
template<>
obj_affects::EObjAffectMsgType ITEM_BY_NAME<obj_affects::EObjAffectMsgType>(const std::string &name) {
	static std::map<std::string, obj_affects::EObjAffectMsgType> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kObjAffectMsgTypeNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
