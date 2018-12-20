#include "msdp.reporters.hpp"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "char.hpp"
#include "magic.h"
#include "msdp.constants.hpp"
#include "zone.table.hpp"

namespace msdp
{
	void RoomReporter::get(Variable::shared_ptr& response)
	{
		const auto rnum = IN_ROOM(descriptor()->character);		
		const auto vnum = GET_ROOM_VNUM(rnum);
		const auto from_rnum = descriptor()->character->get_from_room();
		if ((from_rnum == vnum)||(NOWHERE == vnum))
		{
			//добавил проверку если перемещаемся из неоткуда
			return;
		}
		
		const auto from_vnum = GET_ROOM_VNUM(from_rnum);

		if ((PRF_FLAGGED(descriptor()->character, PRF_BLIND))||    //В режиме слепого игрока карта недоступна
				(AFF_FLAGGED((descriptor()->character), EAffectFlag::AFF_BLIND))||  //Слепому карта не поможет!
				(is_dark(IN_ROOM(descriptor()->character)) && !CAN_SEE_IN_DARK(descriptor()->character)))
		{	
			return;
		}
		
		const auto room_descriptor = std::make_shared<TableValue>();

		const auto exits = std::make_shared<TableValue>();
		const auto directions = world[rnum]->dir_option;
		std::string from_direction = "-";

		for (int i = 0; i < NUM_OF_DIRS; ++i)
		{
			if (directions[i]
				&& !EXIT_FLAGGED(directions[i], EX_HIDDEN))
			{
				const static std::string direction_commands[NUM_OF_DIRS] = { "n", "e", "s", "w", "u", "d" };
				const auto to_rnum = directions[i]->to_room();
				if (to_rnum == from_rnum)
				{
					from_direction = direction_commands[i];
				}
				const auto to_vnum = GET_ROOM_VNUM(to_rnum);
				if (NOWHERE != to_vnum)	// Anton Gorev (2016-05-01): Some rooms has exits that  lead to nowhere. It is a workaround.
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
		std::shared_ptr<char> zone_name(new char[4 * strlen(zone_table[world[rnum]->zone].name)], std::default_delete<char[]>());
		descriptor()->string_to_client_encoding(zone_table[world[rnum]->zone].name, zone_name.get());

		room_descriptor->add(std::make_shared<Variable>("VNUM",
			std::make_shared<StringValue>(std::to_string(vnum))));
		room_descriptor->add(std::make_shared<Variable>("NAME",
			std::make_shared<StringValue>(room_name.get())));
		room_descriptor->add(std::make_shared<Variable>("AREA",
			std::make_shared<StringValue>(zone_name.get())));
		room_descriptor->add(std::make_shared<Variable>("ZONE",
			std::make_shared<StringValue>(std::to_string(vnum / 100))));
		if (from_vnum != NOWHERE)
			room_descriptor->add(std::make_shared<Variable>("FROM_ROOM",
				std::make_shared<StringValue>(std::to_string(from_vnum))));
		if (from_direction != "-")
			room_descriptor->add(std::make_shared<Variable>("FROM_DIRECTION",
				std::make_shared<StringValue>(from_direction)));
		

		const auto stype = SECTOR_TYPE_BY_VALUE.find(world[rnum]->sector_type);
		if (stype != SECTOR_TYPE_BY_VALUE.end())
		{
			room_descriptor->add(std::make_shared<Variable>("TERRAIN",
				std::make_shared<StringValue>(stype->second)));
		}

		room_descriptor->add(std::make_shared<Variable>("EXITS", exits));

		response = std::make_shared<Variable>(constants::ROOM, room_descriptor);
	}

	void GoldReporter::get(Variable::shared_ptr& response)
	{
		const auto gold = std::make_shared<TableValue>();

		const auto pocket_money = std::to_string(descriptor()->character->get_gold());
		gold->add(std::make_shared<Variable>("POCKET",
			std::make_shared<StringValue>(pocket_money)));

		const auto bank_money = std::to_string(descriptor()->character->get_bank());
		gold->add(std::make_shared<Variable>("BANK",
			std::make_shared<StringValue>(bank_money)));

		response = std::make_shared<Variable>(constants::GOLD, gold);
	}

	void MaxHitReporter::get(Variable::shared_ptr& response)
	{
		const auto value = std::to_string(GET_REAL_MAX_HIT(descriptor()->character));
		response = std::make_shared<Variable>(constants::MAX_HIT,
			std::make_shared<StringValue>(value));
	}

	void MaxMoveReporter::get(Variable::shared_ptr& response)
	{
		const auto value = std::to_string(GET_REAL_MAX_MOVE(descriptor()->character));
		response = std::make_shared<Variable>(constants::MAX_MOVE,
			std::make_shared<StringValue>(value));
	}

	void MaxManaReporter::get(Variable::shared_ptr& response)
	{
		const auto max_mana = std::to_string(GET_MAX_MANA(descriptor()->character));
		response = std::make_shared<Variable>("MAX_MANA",
			std::make_shared<StringValue>(max_mana));
	}

	void LevelReporter::get(Variable::shared_ptr& response)
	{
		const auto level = std::to_string(GET_LEVEL(descriptor()->character));
		response = std::make_shared<Variable>(constants::LEVEL,
			std::make_shared<StringValue>(level));
	}

	void ExperienceReporter::get(Variable::shared_ptr& response)
	{
		const auto experience = std::to_string(GET_EXP(descriptor()->character));
		response = std::make_shared<Variable>(constants::EXPERIENCE,
			std::make_shared<StringValue>(experience));
	}

	void StateReporter::get(Variable::shared_ptr& response)
	{
		const auto state = std::make_shared<TableValue>();

		const auto current_hp = std::to_string(GET_HIT(descriptor()->character));
		state->add(std::make_shared<Variable>("CURRENT_HP",
			std::make_shared<StringValue>(current_hp)));
		const auto current_move = std::to_string(GET_MOVE(descriptor()->character));
		state->add(std::make_shared<Variable>("CURRENT_MOVE",
			std::make_shared<StringValue>(current_move)));

		if (IS_MANA_CASTER(descriptor()->character))
		{
			const auto current_mana = std::to_string(GET_MANA_STORED(descriptor()->character));
			state->add(std::make_shared<Variable>("CURRENT_MANA",
				std::make_shared<StringValue>(current_mana)));
		}

		response = std::make_shared<Variable>(constants::STATE, state);
	}

	void GroupReporter::append_char(const std::shared_ptr<ArrayValue>& group, const CHAR_DATA* ch, const CHAR_DATA* character, const bool leader)
	{
		if (PRF_FLAGGED(ch, PRF_NOCLONES)
			&& IS_NPC(character)
			&& (MOB_FLAGGED(character, MOB_CLONE)
				|| GET_MOB_VNUM(character) == MOB_KEEPER))
		{
			return;
		}
		const auto member = std::make_shared<TableValue>();

		char buffer[MAX_INPUT_LENGTH] = { 0 };
		descriptor()->string_to_client_encoding(GET_NAME(character), buffer);
		member->add(std::make_shared<Variable>("NAME",
			std::make_shared<StringValue>(buffer)));
		const auto hp_percents = std::to_string(posi_value(GET_HIT(character), GET_REAL_MAX_HIT(character)) * 10);	// *10 to show percents
		member->add(std::make_shared<Variable>("HEALTH", std::make_shared<StringValue>(hp_percents)));

		const auto move_percents = std::to_string(posi_value(GET_MOVE(character), GET_REAL_MAX_MOVE(character)) * 10);// *10 to show percents
		member->add(std::make_shared<Variable>("MOVE", std::make_shared<StringValue>(move_percents)));

		const bool same_room = ch->in_room == IN_ROOM(character);
		member->add(std::make_shared<Variable>("IS_HERE", std::make_shared<StringValue>(same_room ? "1" : "0")));

		const int memory = get_mem(character);
		member->add(std::make_shared<Variable>("MEM_TIME", std::make_shared<StringValue>(std::to_string(memory))));

		std::string affects;
		affects += AFF_FLAGGED(character, EAffectFlag::AFF_SANCTUARY)
			? "О"
			: (AFF_FLAGGED(character, EAffectFlag::AFF_PRISMATICAURA) ? "П" : "");
		if (AFF_FLAGGED(character, EAffectFlag::AFF_WATERBREATH))
		{
			affects += "Д";
		}

		if (AFF_FLAGGED(character, EAffectFlag::AFF_INVISIBLE))
		{
			affects += "Н";
		}

		if (AFF_FLAGGED(character, EAffectFlag::AFF_SINGLELIGHT)
			|| AFF_FLAGGED(character, EAffectFlag::AFF_HOLYLIGHT)
			|| (GET_EQ(character, WEAR_LIGHT)
				&& GET_OBJ_VAL(GET_EQ(character, WEAR_LIGHT), 2)))
		{
			affects += "С";
		}

		if (AFF_FLAGGED(character, EAffectFlag::AFF_FLY))
		{
			affects += "Л";
		}

		if (!IS_NPC(character)
			&& on_horse(character))
		{
			affects += "В";
		}

		if (IS_NPC(character)
			&& character->low_charm())
		{
			affects += "Т";
		}

		descriptor()->string_to_client_encoding(affects.c_str(), buffer);
		member->add(std::make_shared<Variable>("AFFECTS", std::make_shared<StringValue>(buffer)));

		const auto leader_value = leader ? "leader" : (IS_NPC(character) ? "npc" : "pc");
		member->add(std::make_shared<Variable>("ROLE", std::make_shared<StringValue>(leader_value)));

		char position[MAX_INPUT_LENGTH];
		sprinttype(GET_POS(character), position_types, position);
		descriptor()->string_to_client_encoding(position, buffer);
		member->add(std::make_shared<Variable>("POSITION", std::make_shared<StringValue>(buffer)));

		group->add(member);
	}

	int GroupReporter::get_mem(const CHAR_DATA* character) const
	{
		int result = 0;
		int div = 0;
		if (!IS_NPC(character)
			&& ((!IS_MANA_CASTER(character) && !MEMQUEUE_EMPTY(character))
				|| (IS_MANA_CASTER(character) && GET_MANA_STORED(character) < GET_MAX_MANA(character))))
		{
			div = mana_gain(character);
			if (div > 0)
			{
				if (!IS_MANA_CASTER(character))
				{
					result = MAX(0, 1 + GET_MEM_TOTAL(character) - GET_MEM_COMPLETED(character));
					result = result * 60 / div;
				}
				else
				{
					result = MAX(0, 1 + GET_MAX_MANA(character) - GET_MANA_STORED(character));
					result = result / div;
				}
			}
			else
			{
				result = -1;
			}
		}

		return result;
	}

	void GroupReporter::get(Variable::shared_ptr& response)
	{
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
		for (auto f = master->followers; f; f = f->next)
		{
			if (!AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
				&& !(AFF_FLAGGED(f->follower, EAffectFlag::AFF_CHARM)
					|| MOB_FLAGGED(f->follower, MOB_ANGEL)
					|| MOB_FLAGGED(f->follower, MOB_GHOST)))
			{
				continue;
			}

			append_char(group_descriptor, ch, f->follower, false);

			// followers of a follower
			if (!AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP))
			{
				continue;
			}

			for (auto ff = f->follower->followers; ff; ff = ff->next)
			{
				if (!(AFF_FLAGGED(ff->follower, EAffectFlag::AFF_CHARM)
					|| MOB_FLAGGED(ff->follower, MOB_ANGEL)
					|| MOB_FLAGGED(ff->follower, MOB_GHOST)))
				{
					continue;
				}

				append_char(group_descriptor, ch, ff->follower, false);
			}
		}

		response = std::make_shared<Variable>(constants::GROUP, group_descriptor);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
