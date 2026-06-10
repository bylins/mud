/* ************************************************************************
*   File: spec_assign.cpp                               Part of Bylins    *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "engine/db/obj_prototypes.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/guilds.h"
#include "gameplay/communication/boards/boards_constants.h"
#include "gameplay/communication/boards/boards.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/noob.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/ai/special_messages.h"
#include "gameplay/ai/spec_assign.h"
#include "engine/ui/interpreter.h"
#include "utils/parse.h"
#include "utils/utils_string.h"
#include "utils/logger.h"

#include <string>
#include <set>
#include <unordered_map>

extern int dts_are_dumps;

extern IndexData *mob_index;

int puff(CharData *ch, void *me, int cmd, char *argument);

void assign_kings_castle(void);

typedef int special_f(CharData *, void *, int, char *);

void ASSIGNROOM(RoomVnum room, special_f);
void ASSIGNOBJ(ObjVnum obj, special_f);
void clear_mob_charm(CharData *mob);

// functions to perform assignments

namespace specials {
namespace {
// A mob may carry SEVERAL specials (banker + innkeeper, trainer + merchant); objs/rooms carry one.
std::unordered_map<int, std::set<ESpecial>> g_mob_specials;
std::unordered_map<int, ESpecial> g_obj_specials, g_room_specials;
ESpecial Lookup(const std::unordered_map<int, ESpecial> &m, int vnum) {
	const auto it = m.find(vnum);
	return it == m.end() ? ESpecial::kNone : it->second;
}
} // namespace
void RegisterMob(int vnum, ESpecial s) { if (s == ESpecial::kNone) g_mob_specials.erase(vnum); else g_mob_specials[vnum].insert(s); }
void UnregisterMob(int vnum, ESpecial s) {
	const auto it = g_mob_specials.find(vnum);
	if (it != g_mob_specials.end()) {
		it->second.erase(s);
		if (it->second.empty()) {
			g_mob_specials.erase(it);
		}
	}
}
void RegisterObj(int vnum, ESpecial s) { if (s == ESpecial::kNone) g_obj_specials.erase(vnum); else g_obj_specials[vnum] = s; }
void RegisterRoom(int vnum, ESpecial s) { if (s == ESpecial::kNone) g_room_specials.erase(vnum); else g_room_specials[vnum] = s; }
const std::set<ESpecial> &MobSpecials(int vnum) {
	static const std::set<ESpecial> kEmpty;
	const auto it = g_mob_specials.find(vnum);
	return it == g_mob_specials.end() ? kEmpty : it->second;
}
ESpecial ObjSpecial(int vnum) { return Lookup(g_obj_specials, vnum); }
bool IsMobSpecial(int vnum) { return !MobSpecials(vnum).empty(); }
bool IsMobSpecial(int vnum, ESpecial s) { return MobSpecials(vnum).count(s) > 0; }
bool SharesMobSpecial(int v1, int v2) {
	for (const auto s : MobSpecials(v1)) {
		if (IsMobSpecial(v2, s)) {
			return true;
		}
	}
	return false;
}
} // namespace specials

namespace specials {

// issue.utils-cleaning: "is this mob a <service>-keeper?" -- registry-backed. The func-pointer
// dispatch these used to read (mob_index[].func == shop_ext/postmaster/bank) is gone: mob specials
// are data-driven (cfg/specials.xml -> do_specproc), so mob_index[].func is always null for mobs.
bool IsShopkeeper(const CharData *ch) { return ch->IsNpc() && IsMobSpecial(GET_MOB_VNUM(ch), ESpecial::kShop); }
bool IsPostkeeper(const CharData *ch) { return ch->IsNpc() && IsMobSpecial(GET_MOB_VNUM(ch), ESpecial::kMail); }
bool IsBankkeeper(const CharData *ch) { return ch->IsNpc() && IsMobSpecial(GET_MOB_VNUM(ch), ESpecial::kBank); }
bool IsRentkeeper(const CharData *ch) { return ch->IsNpc() && IsMobSpecial(GET_MOB_VNUM(ch), ESpecial::kRent); }

}  // namespace specials

// Map a spec-proc function to its ESpecial, so ASSIGN* keeps the registry in sync with func.
// Only object/board specials are still assigned via a func pointer (ASSIGNOBJ); every mob spec is
// data-driven (cfg/specials.xml -> registry), so ESpecialForFunc only needs the board handler.
static specials::ESpecial ESpecialForFunc(special_f *f) {
	if (f == Boards::Static::Special) {
		return specials::ESpecial::kBoard;
	}
	return specials::ESpecial::kNone;
}

void ASSIGNOBJ(ObjVnum obj, special_f fname) {
	const ObjRnum rnum = GetObjRnum(obj);

	if (rnum >= 0) {
		obj_proto.func(rnum, fname);
		specials::RegisterObj(obj, ESpecialForFunc(fname));
	} else {
		err_log("Attempt to assign spec to non-existant obj #%d", obj);
	}
}

void ASSIGNROOM(RoomVnum room, special_f fname) {
	const RoomRnum rnum = GetRoomRnum(room);

	if (rnum != kNowhere) {
		world[rnum]->func = fname;
		specials::RegisterRoom(room, ESpecialForFunc(fname));
	} else {
		err_log("Attempt to assign spec to non-existant room #%d", room);
	}
}

void ASSIGNMASTER(MobVnum mob, special_f /*fname*/, int learn_info) {
	MobRnum rnum;

	if ((rnum = GetMobRnum(mob)) >= 0) {
		// do_specproc-only: store the guild vnum + register kGuild; no func pointer.
		mob_index[rnum].stored = learn_info;
		specials::RegisterMob(mob, specials::ESpecial::kGuild);
	} else {
		err_log("Attempt to assign spec to non-existant mob #%d", mob);
	}
}

// ********************************************************************
// *  Assignments                                                     *
// ********************************************************************

/**
* Спешиалы на мобов сюда писать не нужно, пишите в lib/misc/specials.lst,
* TODO: вообще убирать надо это тоже в конфиг, всеравно без конфигов мад
* не запустится, толку в коде держать даже этот минимальный набор.
*/
void AssignMobiles(void) {
	// HOTEL: data-driven (registry kRent, do_specproc). 4022 comes from cfg/specials.xml;
	// 106 is hardcoded here (not in cfg). Rentkeepers get charm flags cleared.
	if (const auto rnum = GetMobRnum(106); rnum >= 0) {
		specials::RegisterMob(106, specials::ESpecial::kRent);
		clear_mob_charm(&mob_proto[rnum]);
	}

	// POSTMASTER 4002: data-driven (registry kMail, do_specproc); 4070 comes from cfg/specials.xml.
	specials::RegisterMob(4002, specials::ESpecial::kMail);

	// BANK is data-driven now (cfg/specials.xml -> kBank registry, do_specproc dispatch).

	// HORSEKEEPER is data-driven now (cfg/specials.xml -> kHorse, do_specproc).
}

// assign special procedures to objects //
void AssignObjects(void) {
	special_f *const function = Boards::Static::Special;
	ASSIGNOBJ(Boards::GODGENERAL_BOARD_OBJ, function);
	ASSIGNOBJ(Boards::GENERAL_BOARD_OBJ, function);
	ASSIGNOBJ(Boards::GODCODE_BOARD_OBJ, function);
	ASSIGNOBJ(Boards::GODPUNISH_BOARD_OBJ, function);
	ASSIGNOBJ(Boards::GODBUILD_BOARD_OBJ, function);
}

// assign special procedures to rooms //
void AssignRooms() {
/*
	RoomRnum i;

	if (dts_are_dumps)
		for (i = kFirstRoom; i <= top_of_world; i++)
			if (ROOM_FLAGGED(i, ERoomFlag::kDeathTrap))
				world[i]->func = dump;
*/
}

// * Снятие нежелательных флагов у рентеров и продавцов.
void clear_mob_charm(CharData *mob) {
	if (mob && !mob->purged()) {
		mob->UnsetFlag(EMobFlag::kMounting);
		mob->SetFlag(EMobFlag::kNoCharm);
		mob->SetFlag(EMobFlag::kNoResurrection);
		mob->mob_specials.npc_flags.unset(ENpcFlag::kHelped);
	} else {
		log("SYSERROR: mob = %s (%s:%d)",
			mob ? (mob->purged() ? "purged" : "true") : "false",
			__FILE__, __LINE__);
	}
}

namespace {
// issue.specials: special id token -> ESpecial. The file groups mobs UNDER each special:
//   <special id="kRent"><mob vnum="4022"/><mob vnum="4001"/></special>
// id is the ESpecial enum token (kRent/kBank/...); the verbs route to do_specproc, which dispatches
// by ESpecial -- no func pointer. kShop comes from shop config, kGuild from guilds.xml, kBoard from code.
const std::unordered_map<std::string, specials::ESpecial> &HandlerMap() {
	static const std::unordered_map<std::string, specials::ESpecial> kMobSpecials{
		{"kRent", specials::ESpecial::kRent},
		{"kMail", specials::ESpecial::kMail},
		{"kBank", specials::ESpecial::kBank},
		{"kHorse", specials::ESpecial::kHorse},
		{"kExchange", specials::ESpecial::kExchange},
		{"kTorc", specials::ESpecial::kTorc},
		{"kMercenary", specials::ESpecial::kMercenary},
		{"kOutfit", specials::ESpecial::kOutfit},
		{"kPuff", specials::ESpecial::kPuff},
	};
	return kMobSpecials;
}
// Resolve an id token (case-insensitively) to its ESpecial, or kNone if it is not a mob-handler id.
specials::ESpecial IdToSpecial(const char *id) {
	if (id && *id) {
		for (const auto &[name, sp] : HandlerMap()) {
			if (!str_cmp(name.c_str(), id)) {
				return sp;
			}
		}
	}
	return specials::ESpecial::kNone;
}

// Read the nested <specials> tree (<special id><mob vnum/></special>) into the registry. A mob may
// appear under several <special> blocks (banker + innkeeper); RegisterMob accumulates into a set, so no
// search is needed. `out`, when given, collects one EditableElement per <special> block (id -> joined
// mob vnums) for ListElements. NOTE: iterate the UNFILTERED Children() and name-check -- a node copied
// out of a keyed Children("x") range carries that filter, breaking its own later Children().
void ParseSpecials(parser_wrapper::DataNode &data, std::vector<cfg_manager::EditableElement> *out) {
	for (auto &group : data.Children()) {
		if (std::string(group.GetName()) != "special") {
			continue;
		}
		const char *id = group.GetValue("id");
		const auto special = IdToSpecial(id);
		if (special == specials::ESpecial::kNone) {
			err_log("specials: unknown/empty special id '%s' -- block skipped.", id ? id : "");
			continue;
		}
		std::string vnums;
		for (auto &mob : group.Children()) {
			if (std::string(mob.GetName()) != "mob") {
				continue;
			}
			int vnum;
			try {
				vnum = parse::ReadAsInt(mob.GetValue("vnum"));
			} catch (const std::exception &e) {
				err_log("specials: %s <mob> with bad or missing 'vnum' (%s) -- skipped.", id, e.what());
				continue;
			}
			if (vnum <= 0) {
				err_log("specials: %s <mob> vnum %d is not a positive integer -- skipped.", id, vnum);
				continue;
			}
			const auto mrnum = GetMobRnum(vnum);
			if (mrnum < 0) {
				err_log("specials: %s mob vnum %d not found -- skipped.", id, vnum);
				continue;
			}
			specials::RegisterMob(vnum, special);
			if (special == specials::ESpecial::kRent) {
				clear_mob_charm(&mob_proto[mrnum]);
			}
			if (!vnums.empty()) {
				vnums += ", ";
			}
			vnums += std::to_string(vnum);
		}
		if (out) {
			out->push_back({id, vnums.empty() ? std::string("(no mobs)") : vnums});
		}
	}
}
} // namespace

void SpecialsLoader::Load(parser_wrapper::DataNode data) { elements_.clear(); ParseSpecials(data, &elements_); }
void SpecialsLoader::Reload(parser_wrapper::DataNode data) { elements_.clear(); ParseSpecials(data, &elements_); }

// --- issue.specials: Vedun in-game editing of specials.xml ------------------------------------------
// A <special> block (keyed by its ESpecial id token, e.g. kRent) is the editable element; the user
// adds/removes its <mob vnum=> children. id is validated to a mob-handler token; vnum to a positive int.
std::string SpecialsLoader::EditableWhat() const { return "special"; }

std::vector<cfg_manager::EditableElement> SpecialsLoader::ListElements() const { return elements_; }

cfg_manager::ValidationResult SpecialsLoader::Validate(parser_wrapper::DataNode &doc) const {
	for (auto &group : doc.Children()) {
		if (std::string(group.GetName()) != "special") {
			continue;
		}
		const char *id = group.GetValue("id");
		if (IdToSpecial(id) == specials::ESpecial::kNone) {
			return {false, std::string("Unknown special id '") + (id ? id : "") +
					"' (valid: kRent kMail kBank kHorse kExchange kTorc kMercenary kOutfit kPuff)."};
		}
		for (auto &mob : group.Children()) {
			if (std::string(mob.GetName()) != "mob") {
				continue;
			}
			int vnum;
			try {
				vnum = parse::ReadAsInt(mob.GetValue("vnum"));
			} catch (const std::exception &) {
				return {false, std::string("A <mob> under ") + id + " has a missing or non-integer vnum."};
			}
			if (vnum <= 0) {
				return {false, std::string("A <mob> under ") + id + " has a non-positive vnum."};
			}
		}
	}
	return {true, ""};
}

parser_wrapper::DataNode SpecialsLoader::FindElementNode(parser_wrapper::DataNode root,
														   const std::string &id) const {
	// A <special> is keyed by its `id` token. Iterate ALL children with a name check (not
	// Children("special")) so the returned node keeps a working Children() for its <mob> list.
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "special" && id == child.GetValue("id")) {
			return child;
		}
	}
	return parser_wrapper::DataNode{};
}

std::string SpecialsLoader::CanonicalElementId(const std::string &id) const {
	// Valid only if it is a mob-handler special token; return the canonical (proper-cased) token.
	for (const auto &[name, sp] : HandlerMap()) {
		(void) sp;
		if (!str_cmp(name.c_str(), id.c_str())) {
			return name;
		}
	}
	return "";
}

parser_wrapper::DataNode SpecialsLoader::CreateElementNode(parser_wrapper::DataNode root,
															 const std::string &id) const {
	// A fresh special block with no mobs yet; the user adds <mob vnum=> children in the editor.
	auto node = root.AddChild("special");
	node.SetValue("id", id);
	return node;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

// issue.specials 1.2b: data-driven dispatch. DispatchSpecial calls a special handler by its ESpecial.
// do_specproc is the target of the entity-verb commands (shop/магазин, ...): it finds the room carrier
// registered with that ESpecial (honouring fnum) and dispatches, passing the FULL argument (the action
// word plus its parameters). No carrier present gives the standard wrong-place reply.
static int DispatchSpecial(specials::ESpecial s, CharData *ch, void *me, int cmd, char *arg) {
	switch (s) {
		case specials::ESpecial::kShop: return shop_ext(ch, me, cmd, arg);
		case specials::ESpecial::kBank: return bank(ch, me, cmd, arg);
		case specials::ESpecial::kHorse: return horse_keeper(ch, me, cmd, arg);
		case specials::ESpecial::kMail: return postmaster(ch, me, cmd, arg);
		case specials::ESpecial::kGuild: return guilds::GuildInfo::DoGuildLearn(ch, me, cmd, arg);
		case specials::ESpecial::kRent: return RentReceptionist(ch, me, cmd, arg);
		case specials::ESpecial::kTorc: return TorcExchange(ch, me, cmd, arg);
		case specials::ESpecial::kMercenary: return mercenary(ch, me, cmd, arg);
		default: return 0;
	}
}

namespace specials {
void RunSpecCommand(CharData *ch, ESpecial want, char *line) {
	const int fnum = GetSpecprocFnum();
	int specialNum = 1;
	for (const auto vict : world[ch->in_room]->people) {
		if (vict->IsNpc() && IsMobSpecial(GET_MOB_VNUM(vict), want)) {
			if (fnum == 1 || fnum == specialNum++) {
				if (DispatchSpecial(want, ch, vict, 0, line)) {
					return;
				}
			}
		}
	}
	SendMsgToChar((specials::SpecialMsg(specials::ESpecialMsg::kNotHere) + "\r\n").c_str(), ch);
}

bool IsMobSpecialInRoom(RoomRnum room, ESpecial s) {
	for (const auto vict : world[room]->people) {
		if (vict->IsNpc() && IsMobSpecial(GET_MOB_VNUM(vict), s)) {
			return true;
		}
	}
	return false;
}
} // namespace specials

void do_specproc(CharData *ch, char *argument, int /*cmd*/, int subcmd) {
	specials::RunSpecCommand(ch, static_cast<specials::ESpecial>(subcmd), argument);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
