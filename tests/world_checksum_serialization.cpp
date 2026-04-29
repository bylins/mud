// Part of Bylins http://www.mud.ru
// Regression tests for WorldChecksum::SerializeMob / SerializeRoom.
//
// Background: run_load_tests.sh compares Legacy vs YAML world checksums to
// detect loader regressions. The bug where YAML loader OR'd raw bit indices
// into mob action_flags slipped through silently because SerializeMob did not
// include flags at all. SerializeRoom did emit flag *names*, but stripped
// "UNDEF" tokens -- masking extra/missing high bits the same way.
//
// These tests pin down what the serializer emits so future code cannot quietly
// drop a flag plane from the checksum input again.

#include "engine/db/world_checksum.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/room_data.h"
#include "engine/structs/flag_data.h"

#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>

namespace {

// Format a 4-plane FlagData the same way SerializeMob/Object do:
// "<plane0>,<plane1>,<plane2>,<plane3>".
std::string PlanesString(const FlagData& f) {
	std::ostringstream oss;
	oss << f.get_plane(0) << "," << f.get_plane(1) << ","
	    << f.get_plane(2) << "," << f.get_plane(3);
	return oss.str();
}

}  // namespace

// SerializeMob must include action_flags as raw planes -- the original
// implementation skipped them entirely.
TEST(SerializeMob, IncludesActionFlagPlanes) {
	CharData mob;
	mob.SetNpcAttribute(true);
	mob.SetFlag(EMobFlag::kSentinel);                       // plane 0, bit 1
	mob.SetFlag(EMobFlag::kSwimming);                       // plane 1, bit 0

	const std::string out = WorldChecksum::SerializeMob(42, mob);

	const std::string expected_planes = PlanesString(mob.char_specials.saved.act);
	EXPECT_NE(std::string::npos, out.find(expected_planes))
		<< "SerializeMob must embed action_flags planes; got: " << out;
}

TEST(SerializeMob, IncludesAffectFlagPlanes) {
	CharData mob;
	mob.SetNpcAttribute(true);
	AFF_FLAGS(&mob).set(static_cast<Bitvector>(EAffect::kSanctuary));
	AFF_FLAGS(&mob).set(static_cast<Bitvector>(EAffect::kHelper));   // plane 1

	const std::string out = WorldChecksum::SerializeMob(42, mob);

	const std::string expected_planes =
		PlanesString(mob.char_specials.saved.affected_by);
	EXPECT_NE(std::string::npos, out.find(expected_planes))
		<< "SerializeMob must embed affect_flags planes; got: " << out;
}

TEST(SerializeMob, IncludesNpcFlagPlanes) {
	CharData mob;
	mob.SetNpcAttribute(true);
	mob.mob_specials.npc_flags.set(static_cast<Bitvector>(1u << 5));

	const std::string out = WorldChecksum::SerializeMob(42, mob);

	const std::string expected_planes = PlanesString(mob.mob_specials.npc_flags);
	EXPECT_NE(std::string::npos, out.find(expected_planes))
		<< "SerializeMob must embed npc_flags planes; got: " << out;
}

// Distinct mobs whose ONLY difference is a high-plane action flag must produce
// different checksum-input strings. Before the fix, missing flag-plane fields
// made these collide.
TEST(SerializeMob, MobsDifferingOnlyByActionFlagSerializeDifferently) {
	CharData a, b;
	a.SetNpcAttribute(true);
	b.SetNpcAttribute(true);
	a.SetFlag(EMobFlag::kSentinel);   // plane 0, bit 1

	const std::string sa = WorldChecksum::SerializeMob(42, a);
	const std::string sb = WorldChecksum::SerializeMob(42, b);
	EXPECT_NE(sa, sb)
		<< "Mobs with different action_flags must hash differently";
}

// SerializeRoom must encode all 4 flag planes verbatim -- previously it
// printed flag *names* and stripped "UNDEF", silently masking high bits set
// by buggy loaders.
TEST(SerializeRoom, IncludesRawFlagPlanes) {
	RoomData room;
	room.vnum = 1;
	room.zone_rn = 0;
	room.set_flag(static_cast<Bitvector>(1u << 0));        // plane 0, bit 0
	room.set_flag(kIntOne | static_cast<Bitvector>(1u << 5));   // plane 1, bit 5

	const std::string out = WorldChecksum::SerializeRoom(&room);

	const std::string expected_planes = PlanesString(room.read_flags());
	EXPECT_NE(std::string::npos, out.find(expected_planes))
		<< "SerializeRoom must embed room_flags planes; got: " << out;
}

TEST(SerializeRoom, UndefFlagsAreNotMaskedFromChecksum) {
	RoomData a, b;
	a.vnum = 1; a.zone_rn = 0;
	b.vnum = 1; b.zone_rn = 0;

	// Pick a high-plane bit with no name in room_bits[]. The old serializer
	// rendered such bits as "UNDEF" and stripped the token, so two rooms
	// differing only by such a bit produced identical checksum input --
	// hiding loader regressions where extra/missing high bits drifted.
	a.set_flag(kIntOne | static_cast<Bitvector>(1u << 14));

	const std::string sa = WorldChecksum::SerializeRoom(&a);
	const std::string sb = WorldChecksum::SerializeRoom(&b);
	EXPECT_NE(sa, sb)
		<< "Rooms with different UNDEF high-plane flags must hash differently";
}

// Issue #3218: V-lines on objects (potion spells, refilled-potion proto vnum,
// etc.) live in CObjectPrototype::get_all_values() rather than the four-slot
// values array. Until this commit SerializeObject did not include them, so
// a converter or loader regression that dropped the V-block could pass
// run_load_tests.sh unnoticed. These tests pin the new behaviour down.
//
// Note: we set values via SetPotionValueKey() instead of
// init_values_from_zone() because the latter goes through the text_id
// dictionary, which only gets populated during full-server boot.
TEST(SerializeObject, IncludesExtraValuesFromObjVal) {
	auto obj = std::make_shared<CObjectPrototype>(42);
	obj->set_type(EObjType::kPotion);
	obj->SetPotionValueKey(ObjVal::EValueKey::POTION_SPELL1_NUM, 17);
	obj->SetPotionValueKey(ObjVal::EValueKey::POTION_SPELL1_LVL, 23);

	const std::string out = WorldChecksum::SerializeObject(obj);
	EXPECT_NE(std::string::npos, out.find("0:17"))
		<< "POTION_SPELL1_NUM (key 0) must appear in checksum input; got: " << out;
	EXPECT_NE(std::string::npos, out.find("1:23"))
		<< "POTION_SPELL1_LVL (key 1) must appear in checksum input; got: " << out;
}

TEST(SerializeObject, ObjectsDifferingOnlyByExtraValuesSerializeDifferently) {
	auto a = std::make_shared<CObjectPrototype>(42);
	auto b = std::make_shared<CObjectPrototype>(42);
	a->set_type(EObjType::kPotion);
	b->set_type(EObjType::kPotion);

	// Same base values, same flags, same descriptions -- only ObjVal map differs.
	a->SetPotionValueKey(ObjVal::EValueKey::POTION_SPELL1_NUM, 17);

	const std::string sa = WorldChecksum::SerializeObject(a);
	const std::string sb = WorldChecksum::SerializeObject(b);
	EXPECT_NE(sa, sb)
		<< "Potions with different POTION_SPELL* must hash differently";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
