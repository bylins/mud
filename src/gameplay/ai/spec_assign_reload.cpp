#include "gameplay/ai/spec_assign.h"

#include "engine/db/db.h"
#include "engine/db/obj_prototypes.h"
#include "engine/entities/char_data.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/communication/boards/boards.h"
#include "gameplay/mechanics/noob.h"

using special_f = int(CharData *, void *, int, char *);

int dump(CharData *ch, void *me, int cmd, char *argument);
int puff(CharData *ch, void *me, int cmd, char *argument);

namespace {

bool IsManagedMobSpecial(const special_f *func) {
	return func == puff
		|| func == receptionist
		|| func == postmaster
		|| func == bank
		|| func == horse_keeper
		|| func == exchange
		|| func == torc
		|| func == Noob::outfit
		|| func == mercenary;
}

bool IsManagedObjectSpecial(const special_f *func) {
	return func == Boards::Static::Special;
}

bool IsManagedRoomSpecial(const special_f *func) {
	return func == dump;
}

void ClearManagedSpecProcs() {
	for (MobRnum i = 0; i <= top_of_mobt; ++i) {
		if (IsManagedMobSpecial(mob_index[i].func)) {
			mob_index[i].func = nullptr;
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
	InitSpecProcs();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
