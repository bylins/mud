#include "msdp_reporters.h"

#include "entities/char_data.h"
#include "entities/entities_constants.h"
#include "game_magic/magic.h"
#include "msdp_constants.h"
#include "entities/zone.h"

namespace msdp {
void RoomReporter::get(Variable::shared_ptr &response) {
	const auto rnum = IN_ROOM(descriptor()->character);
	const auto vnum = GET_ROOM_VNUM(rnum);
	const auto from_rnum = descriptor()->character->get_from_room();
	if ((from_rnum == vnum) || (kNowhere == vnum)) {
		//добавил проверку если перемещаемся из неоткуда
		return;
	}

	if (blockReport()) {
		return;
	}

	const auto room_descriptor = std::make_shared<TableValue>();

	const auto exits = std::make_shared<TableValue>();
	const auto directions = world[rnum]->dir_option;
	std::string from_direction = "-";

	for (int i = 0; i < kDirMaxNumber; ++i) {
		if (directions[i]
			&& !EXIT_FLAGGED(directions[i], EX_HIDDEN)) {
			const static std::string direction_commands[kDirMaxNumber] = {"n", "e", "s", "w", "u", "d"};
			const auto to_rnum = directions[i]->to_room();
			if (to_rnum == from_rnum) {
				from_direction = direction_commands[i];
			}
			const auto to_vnum = GET_ROOM_VNUM(to_rnum);
			if (kNowhere
				!= to_vnum)    // Anton Gorev (2016-05-01): Some rooms has exits that  lead to nowhere. It is a workaround.
			{
				exits->add(std::make_shared<Variable>(direction_commands[i],
						std::make_shared<StringValue>(std::to_string(to_vnum))));
			}
		}
	}

	/* convert string date into client's encoding */
	// output might be more than input up to 4 times (in case of utf-8) plus NULL terminator.
	std::shared_ptr<char> room_name(new char[1 + 4 * strlen(world[rnum]->name)], std::default_delete<char[]>());
	descriptor()->string_to_client_encoding(world[rnum]->name, room_name.get());

	// output might be more than input up to 4 times (in case of utf-8) plus NULL terminator.
	std::shared_ptr<char>
		zone_name(new char[4 * strlen(zone_table[world[rnum]->zone_rn].name)], std::default_delete<char[]>());
	descriptor()->string_to_client_encoding(zone_table[world[rnum]->zone_rn].name, zone_name.get());

	room_descriptor->add(std::make_shared<Variable>("VNUM", std::make_shared<StringValue>(std::to_string(vnum))));
	room_descriptor->add(std::make_shared<Variable>("NAME", std::make_shared<StringValue>(room_name.get())));
	room_descriptor->add(std::make_shared<Variable>("AREA", std::make_shared<StringValue>(zone_name.get())));
	room_descriptor->add(std::make_shared<Variable>("ZONE", std::make_shared<StringValue>(std::to_string(vnum / 100))));

	const auto from_vnum = GET_ROOM_VNUM(from_rnum);
	if (from_vnum != kNowhere) {
		room_descriptor->add(std::make_shared<Variable>("FROM_ROOM",
				std::make_shared<StringValue>(std::to_string(from_vnum))));
	}

	if (from_direction != "-") {
		room_descriptor->add(std::make_shared<Variable>("FROM_DIRECTION",
				std::make_shared<StringValue>(from_direction)));
	}

	const auto stype = SECTOR_TYPE_BY_VALUE.find(world[rnum]->sector_type);
	if (stype != SECTOR_TYPE_BY_VALUE.end()) {
		room_descriptor->add(std::make_shared<Variable>("TERRAIN",
				std::make_shared<StringValue>(stype->second)));
	}

	room_descriptor->add(std::make_shared<Variable>("EXITS", exits));

	response = std::make_shared<Variable>(constants::ROOM, room_descriptor);
}

bool RoomReporter::blockReport() const {
	bool nomapper = true;
	const auto blind = (PRF_FLAGGED(descriptor()->character, EPrf::kBlindMode)) //В режиме слепого игрока карта недоступна
		|| (AFF_FLAGGED((descriptor()->character), EAffect::kBlind));  //Слепому карта не поможет!
	const auto cannot_see_in_dark = (is_dark(IN_ROOM(descriptor()->character)) && !CAN_SEE_IN_DARK(descriptor()->character));
	if (descriptor()->character->in_room != kNowhere)
		nomapper = ROOM_FLAGGED(descriptor()->character->in_room, ROOM_NOMAPPER);
	const auto scriptwriter = PLR_FLAGGED(descriptor()->character, EPlrFlag::kScriptWriter); // скриптеру не шлем

	return blind || cannot_see_in_dark || scriptwriter || nomapper;
}

void GoldReporter::get(Variable::shared_ptr &response) {
	const auto gold = std::make_shared<TableValue>();

	const auto pocket_money = std::to_string(descriptor()->character->get_gold());
	gold->add(std::make_shared<Variable>("POCKET",
			std::make_shared<StringValue>(pocket_money)));

	const auto bank_money = std::to_string(descriptor()->character->get_bank());
	gold->add(std::make_shared<Variable>("BANK",
										 std::make_shared<StringValue>(bank_money)));

	response = std::make_shared<Variable>(constants::GOLD, gold);
}

void MaxHitReporter::get(Variable::shared_ptr &response) {
	const auto value = std::to_string(GET_REAL_MAX_HIT(descriptor()->character));
	response = std::make_shared<Variable>(constants::MAX_HIT,
										  std::make_shared<StringValue>(value));
}

void MaxMoveReporter::get(Variable::shared_ptr &response) {
	const auto value = std::to_string(GET_REAL_MAX_MOVE(descriptor()->character));
	response = std::make_shared<Variable>(constants::MAX_MOVE,
										  std::make_shared<StringValue>(value));
}

void MaxManaReporter::get(Variable::shared_ptr &response) {
	const auto max_mana = std::to_string(GET_MAX_MANA((descriptor()->character).get()));
	response = std::make_shared<Variable>("MAX_MANA",
										  std::make_shared<StringValue>(max_mana));
}

void LevelReporter::get(Variable::shared_ptr &response) {
	const auto level = std::to_string(GetRealLevel(descriptor()->character));
	response = std::make_shared<Variable>(constants::LEVEL,
										  std::make_shared<StringValue>(level));
}

void ExperienceReporter::get(Variable::shared_ptr &response) {
	const auto experience = std::to_string(GET_EXP(descriptor()->character));
	response = std::make_shared<Variable>(constants::EXPERIENCE,
										  std::make_shared<StringValue>(experience));
}

void StateReporter::get(Variable::shared_ptr &response) {
	const auto state = std::make_shared<TableValue>();

	const auto current_hp = std::to_string(GET_HIT(descriptor()->character));
	state->add(std::make_shared<Variable>("CURRENT_HP",
										  std::make_shared<StringValue>(current_hp)));
	const auto current_move = std::to_string(GET_MOVE(descriptor()->character));
	state->add(std::make_shared<Variable>("CURRENT_MOVE",
										  std::make_shared<StringValue>(current_move)));

	if (IS_MANA_CASTER(descriptor()->character)) {
		const auto current_mana = std::to_string(descriptor()->character->mem_queue.stored);
		state->add(std::make_shared<Variable>("CURRENT_MANA",
											  std::make_shared<StringValue>(current_mana)));
	}

	response = std::make_shared<Variable>(constants::STATE, state);
}

void GroupReporter::append_char(const std::shared_ptr<ArrayValue> &group,
								const CharData *ch,
								const CharData *character,
								const bool leader) {
	if (PRF_FLAGGED(ch, EPrf::kNoClones)
		&& character->is_npc()
		&& (MOB_FLAGGED(character, EMobFlag::kClone)
			|| GET_MOB_VNUM(character) == kMobKeeper)) {
		return;
	}
	const auto member = std::make_shared<TableValue>();

	char buffer[kMaxInputLength] = {0};
	descriptor()->string_to_client_encoding(GET_NAME(character), buffer);
	member->add(std::make_shared<Variable>("NAME",
										   std::make_shared<StringValue>(buffer)));
	const auto hp_percents =
		std::to_string(posi_value(GET_HIT(character), GET_REAL_MAX_HIT(character)) * 10);    // *10 to show percents
	member->add(std::make_shared<Variable>("HEALTH", std::make_shared<StringValue>(hp_percents)));

	const auto move_percents =
		std::to_string(posi_value(GET_MOVE(character), GET_REAL_MAX_MOVE(character)) * 10);// *10 to show percents
	member->add(std::make_shared<Variable>("MOVE", std::make_shared<StringValue>(move_percents)));

	const bool same_room = ch->in_room == IN_ROOM(character);
	member->add(std::make_shared<Variable>("IS_HERE", std::make_shared<StringValue>(same_room ? "1" : "0")));

	const int memory = get_mem(character);
	member->add(std::make_shared<Variable>("MEM_TIME", std::make_shared<StringValue>(std::to_string(memory))));

	std::string affects;
	affects += AFF_FLAGGED(character, EAffect::kSanctuary)
			   ? "О"
			   : (AFF_FLAGGED(character, EAffect::kPrismaticAura) ? "П" : "");
	if (AFF_FLAGGED(character, EAffect::kWaterBreath)) {
		affects += "Д";
	}

	if (AFF_FLAGGED(character, EAffect::kInvisible)) {
		affects += "Н";
	}

	if (AFF_FLAGGED(character, EAffect::kSingleLight)
		|| AFF_FLAGGED(character, EAffect::kHolyLight)
		|| (GET_EQ(character, EEquipPos::kLight)
			&& GET_OBJ_VAL(GET_EQ(character, EEquipPos::kLight), 2))) {
		affects += "С";
	}

	if (AFF_FLAGGED(character, EAffect::kFly)) {
		affects += "Л";
	}

	if (!character->is_npc() && character->ahorse()) {
		affects += "В";
	}

	if (character->is_npc()
		&& character->low_charm()) {
		affects += "Т";
	}

	descriptor()->string_to_client_encoding(affects.c_str(), buffer);
	member->add(std::make_shared<Variable>("AFFECTS", std::make_shared<StringValue>(buffer)));

	const auto leader_value = leader ? "leader" : (character->is_npc() ? "npc" : "pc");
	member->add(std::make_shared<Variable>("ROLE", std::make_shared<StringValue>(leader_value)));

	char position[kMaxInputLength];
	sprinttype(static_cast<int>(GET_POS(character)), position_types, position);
	descriptor()->string_to_client_encoding(position, buffer);
	member->add(std::make_shared<Variable>("POSITION", std::make_shared<StringValue>(buffer)));

	group->add(member);
}

int GroupReporter::get_mem(const CharData *character) const {
	int result = 0;
	int div = 0;
	if (!character->is_npc()
		&& ((!IS_MANA_CASTER(character) && !character->mem_queue.Empty())
			|| (IS_MANA_CASTER(character) && character->mem_queue.stored < GET_MAX_MANA(character)))) {
		div = mana_gain(character);
		if (div > 0) {
			if (!IS_MANA_CASTER(character)) {
				result = std::max(0, 1 + character->mem_queue.total - character->mem_queue.stored);
				result = result * 60 / div;
			} else {
				result = std::max(0, 1 + GET_MAX_MANA(character) - character->mem_queue.stored);
				result = result / div;
			}
		} else {
			result = -1;
		}
	}

	return result;
}

void GroupReporter::get(Variable::shared_ptr &response) {
	/*
	format: array of tables of
	{
	"NAME" : <name>;
	"HEALTH" : <hit_percent>;
	"MOVE" : <move_percent>;
	"IS_HERE" : (1|0);
	"MEM_TIME" : <mem_time>;
	"AFFECTS" : affects;
	"ROLE" : (leader/pc/npc);
	"POSITION" : <position>
	}
	*/
	const auto group_descriptor = std::make_shared<ArrayValue>();
	const auto ch = descriptor()->character.get();
	const auto master = ch->has_master() ? ch->get_master() : ch;
	append_char(group_descriptor, ch, master, true);
	for (auto f = master->followers; f; f = f->next) {
		if (!AFF_FLAGGED(f->ch, EAffect::kGroup)
			&& !(AFF_FLAGGED(f->ch, EAffect::kCharmed)
				|| MOB_FLAGGED(f->ch, EMobFlag::kTutelar)
				|| MOB_FLAGGED(f->ch, EMobFlag::kMentalShadow))) {
			continue;
		}

		append_char(group_descriptor, ch, f->ch, false);

		// followers of a ch
		if (!AFF_FLAGGED(f->ch, EAffect::kGroup)) {
			continue;
		}

		for (auto ff = f->ch->followers; ff; ff = ff->next) {
			if (!(AFF_FLAGGED(ff->ch, EAffect::kCharmed)
				|| MOB_FLAGGED(ff->ch, EMobFlag::kTutelar)
				|| MOB_FLAGGED(ff->ch, EMobFlag::kMentalShadow))) {
				continue;
			}

			append_char(group_descriptor, ch, ff->ch, false);
		}
	}

	response = std::make_shared<Variable>(constants::GROUP, group_descriptor);
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
