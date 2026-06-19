#include "gameplay/ai/spec_assign.h"

#include "engine/db/global_objects.h"

#include "engine/db/db.h"
#include "engine/db/obj_prototypes.h"
#include "engine/entities/char_data.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/communication/boards/boards.h"
#include "gameplay/mechanics/noob.h"

using special_f = int(CharData *, void *, int, char *);

namespace {

// The cfg/specials.xml + AssignMobiles-driven mob specials. Dropped before the reassign below so
// that entries removed from cfg actually disappear; kShop (shop config) and kGuild (guilds) are
// managed elsewhere and left intact. (Mobs are data-driven now -- no func pointer to inspect.)
const specials::ESpecial kReloadableMobSpecials[] = {
	specials::ESpecial::kRent, specials::ESpecial::kMail, specials::ESpecial::kBank,
	specials::ESpecial::kHorse, specials::ESpecial::kExchange, specials::ESpecial::kTorc,
	specials::ESpecial::kMercenary, specials::ESpecial::kOutfit, specials::ESpecial::kPuff,
};

bool IsManagedObjectSpecial(const special_f *func) {
	return func == Boards::Static::Special;
}

bool IsManagedRoomSpecial(const special_f * /*func*/) {
	return false;  // dump (the only room special) removed
}

void ClearManagedSpecProcs() {
	for (MobRnum i = 0; i <= top_of_mobt; ++i) {
		for (const auto sp : kReloadableMobSpecials) {
			specials::UnregisterMob(mob_index[i].vnum, sp);
		}
	}

	for (size_t i = 0; i < obj_proto.size(); ++i) {
		if (IsManagedObjectSpecial(obj_proto.func(i))) {
			obj_proto.func(i, nullptr);
		}
	}

	for (RoomRnum i = kFirstRoom; i <= top_of_world; ++i) {
		if (IsManagedRoomSpecial(world[i]->func)) {
			world[i]->func = nullptr;
		}
	}
}

} // namespace

void ReloadSpecProcs() {
	ClearManagedSpecProcs();
	AssignMobiles();
	AssignObjects();
	AssignRooms();
	MUD::CfgManager().ReloadCfg("specials");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
