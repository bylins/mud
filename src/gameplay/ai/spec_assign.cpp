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
#include "gameplay/ai/spec_assign.h"
#include "engine/ui/interpreter.h"
#include "utils/parse.h"
#include "utils/utils_string.h"
#include "utils/logger.h"

#include <string>
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
std::unordered_map<int, ESpecial> g_mob_specials, g_obj_specials, g_room_specials;
ESpecial Lookup(const std::unordered_map<int, ESpecial> &m, int vnum) {
	const auto it = m.find(vnum);
	return it == m.end() ? ESpecial::kNone : it->second;
}
} // namespace
void RegisterMob(int vnum, ESpecial s) { if (s == ESpecial::kNone) g_mob_specials.erase(vnum); else g_mob_specials[vnum] = s; }
void RegisterObj(int vnum, ESpecial s) { if (s == ESpecial::kNone) g_obj_specials.erase(vnum); else g_obj_specials[vnum] = s; }
void RegisterRoom(int vnum, ESpecial s) { if (s == ESpecial::kNone) g_room_specials.erase(vnum); else g_room_specials[vnum] = s; }
ESpecial MobSpecial(int vnum) { return Lookup(g_mob_specials, vnum); }
ESpecial ObjSpecial(int vnum) { return Lookup(g_obj_specials, vnum); }
bool IsMobSpecial(int vnum) { return MobSpecial(vnum) != ESpecial::kNone; }
bool IsMobSpecial(int vnum, ESpecial s) { return MobSpecial(vnum) == s; }
} // namespace specials

bool IsRentkeeper(const CharData *ch) {
	return ch->IsNpc() && specials::MobSpecial(GET_MOB_VNUM(ch)) == specials::ESpecial::kRent;
}

namespace specials {} // namespace specials

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
// issue.specials: handler name -> prototype spec-proc function. Mirrors the old InitSpecProcs chain.
void ParseSpecials(parser_wrapper::DataNode &data) {
	for (auto &node : data.Children("special")) {
		const char *type = node.GetValue("type");
		const char *handler = node.GetValue("handler");
		int vnum;
		try {
			vnum = parse::ReadAsInt(node.GetValue("vnum"));
		} catch (const std::exception &e) {
			err_log("specials: bad or missing 'vnum' (%s) -- entry skipped.", e.what());
			continue;
		}
		if (!type || !handler || !*type || !*handler) {
			err_log("specials: vnum %d missing type/handler -- skipped.", vnum);
			continue;
		}
		if (!str_cmp(type, "mob")) {
			// Mob spec procs are fully data-driven: the handler name maps to an ESpecial registered against
			// the vnum; the command verbs route to do_specproc, which dispatches by ESpecial. No func pointer.
			static const std::unordered_map<std::string, specials::ESpecial> kMobSpecials{
				{"rent", specials::ESpecial::kRent},
				{"mail", specials::ESpecial::kMail},
				{"bank", specials::ESpecial::kBank},
				{"horse", specials::ESpecial::kHorse},
				{"exchange", specials::ESpecial::kExchange},
				{"torc", specials::ESpecial::kTorc},
				{"mercenary", specials::ESpecial::kMercenary},
				{"outfit", specials::ESpecial::kOutfit},
				{"puff", specials::ESpecial::kPuff},
			};
			const auto it = kMobSpecials.find(handler);
			if (it == kMobSpecials.end()) {
				err_log("specials: unknown mob handler '%s' for vnum %d -- skipped.", handler, vnum);
				continue;
			}
			const auto mrnum = GetMobRnum(vnum);
			if (mrnum < 0) {
				err_log("specials: mob vnum %d not found -- skipped.", vnum);
				continue;
			}
			specials::RegisterMob(vnum, it->second);
			if (it->second == specials::ESpecial::kRent) {
				clear_mob_charm(&mob_proto[mrnum]);
			}
		} else if (!str_cmp(type, "obj")) {
			// Object specials (notice boards) are still assigned in code via AssignObjects.
		} else if (!str_cmp(type, "room")) {
			// Room specials are not yet data-driven.
		} else {
			err_log("specials: unknown type '%s' for vnum %d -- skipped.", type, vnum);
		}
	}
}
} // namespace

void SpecialsLoader::Load(parser_wrapper::DataNode data) { ParseSpecials(data); }
void SpecialsLoader::Reload(parser_wrapper::DataNode data) { ParseSpecials(data); }

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
		if (vict->IsNpc() && MobSpecial(GET_MOB_VNUM(vict)) == want) {
			if (fnum == 1 || fnum == specialNum++) {
				if (DispatchSpecial(want, ch, vict, 0, line)) {
					return;
				}
			}
		}
	}
	SendMsgToChar("Это нельзя сделать здесь.\r\n", ch);
}

bool IsMobSpecialInRoom(RoomRnum room, ESpecial s) {
	for (const auto vict : world[room]->people) {
		if (vict->IsNpc() && MobSpecial(GET_MOB_VNUM(vict)) == s) {
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
