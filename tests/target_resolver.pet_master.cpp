// issue #4163: имя игрока может указывать сразу на двоих -- на самого игрока и на его ведомого
// НПС. Двойник носит имя владельца прямо в алиасах ("двойник стрибога Стрибог"), а isname
// сравнивает по началу слова, так что совпадает и родительный падеж. Раньше их разводила точка
// перед именем ("0.<name> means PC with name" в старом get_char_room_vis), в новом резолвере
// правило потерялось, и цель уезжала на моба: ведомые и чужие НПС стоят в списке комнаты раньше
// игрока (#3984).
//
// Здесь закреплено правило, которое пришло вместо точки: нашли ведомого НПС, и тем же именем
// зовут его хозяина-игрока из той же комнаты -- значит искали хозяина. И закреплены границы
// правила: питомца с собственным именем оно не подменяет, а чужого моба (без хозяина) не
// касается вовсе -- этот случай лечится только UID'ом в триггере.

#include <gtest/gtest.h>

#include "engine/core/target_resolver.h"
#include "engine/entities/char_data.h"
#include "engine/db/db.h"

#include <memory>

namespace target_resolver_pet_master_test {

// Резолверу нужна комната со списком people. kNowhere == 0, то есть нулевая комната -- это
// "нигде", поэтому кладём пустышку и работаем во второй. Мир тест подставляет свой и убирает
// за собой.
class RoomFixture {
	std::size_t saved_world_size_ = world.size();

 public:
	RoomRnum room_rn{0};

	RoomFixture() {
		while (world.size() < 2) {
			auto *filler = new RoomData();
			filler->vnum = 100 + static_cast<int>(world.size());
			world.push_back(filler);
		}
		room_rn = 1;
		world[room_rn]->people.clear();
	}

	~RoomFixture() {
		world[room_rn]->people.clear();
		while (world.size() > saved_world_size_) {
			delete world.back();
			world.pop_back();
		}
	}
};

CharData::shared_ptr MakePlayer(const char *name) {
	auto ch = std::make_shared<CharData>();
	ch->SetNpcAttribute(false);
	ch->set_name(name);
	ch->SetCharAliases(name);
	return ch;
}

CharData::shared_ptr MakeNpc(const char *aliases) {
	auto mob = std::make_shared<CharData>();
	mob->SetNpcAttribute(true);
	mob->player_specials = player_special_data::s_for_mobiles;
	mob->set_npc_name(aliases);
	mob->SetCharAliases(aliases);
	return mob;
}

// Поиск без проверки видимости: свет, слепота и невидимость к этому правилу отношения не имеют,
// а поднимать их в тесте значило бы поднимать пол-движка.
CharData *FindByName(CharData *searcher, const std::string &name) {
	target_resolver::Query q;
	q.scopes = {target_resolver::Scope::kRoom};
	q.name = name;
	q.visible_only = false;
	return target_resolver::ResolveChar(searcher, q);
}

TEST(TargetResolver_PetMaster, CloneGivesWayToItsOwner) {
	RoomFixture room;
	auto owner = MakePlayer("Стрибог");
	auto clone = MakeNpc("двойник стрибога Стрибог");
	auto seeker = MakeNpc("сторож");

	clone->set_master(owner.get());
	owner->in_room = clone->in_room = seeker->in_room = room.room_rn;
	// Двойник стоит в списке раньше владельца -- именно тот порядок, в котором правило нужно.
	world[room.room_rn]->people.push_back(clone.get());
	world[room.room_rn]->people.push_back(owner.get());

	EXPECT_EQ(owner.get(), FindByName(seeker.get(), "стрибог")) << "имя увело цель на двойника";
	EXPECT_EQ(owner.get(), FindByName(seeker.get(), ".стрибог")) << "точка перед именем не учтена";
}

TEST(TargetResolver_PetMaster, PetWithItsOwnNameIsStillFound) {
	RoomFixture room;
	auto owner = MakePlayer("Стрибог");
	auto horse = MakeNpc("богатырский конь");
	auto seeker = MakeNpc("сторож");

	horse->set_master(owner.get());
	owner->in_room = horse->in_room = seeker->in_room = room.room_rn;
	world[room.room_rn]->people.push_back(horse.get());
	world[room.room_rn]->people.push_back(owner.get());

	EXPECT_EQ(horse.get(), FindByName(seeker.get(), "конь")) << "правило подменило обычного питомца";
	EXPECT_EQ(owner.get(), FindByName(seeker.get(), "стрибог"));
}

TEST(TargetResolver_PetMaster, OwnerInAnotherRoomDoesNotSubstitute) {
	RoomFixture room;
	auto owner = MakePlayer("Стрибог");
	auto clone = MakeNpc("двойник стрибога Стрибог");
	auto seeker = MakeNpc("сторож");

	clone->set_master(owner.get());
	clone->in_room = seeker->in_room = room.room_rn;
	owner->in_room = 0;    // хозяин не здесь -- подменять им цель нельзя
	world[room.room_rn]->people.push_back(clone.get());

	EXPECT_EQ(clone.get(), FindByName(seeker.get(), "стрибог"))
		<< "цель подменили хозяином из другой комнаты";
}

}  // namespace target_resolver_pet_master_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
