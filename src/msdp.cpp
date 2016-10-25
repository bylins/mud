#include "msdp.hpp"

#include "char.hpp"
#include "constants.h"
#include "db.h"
#include "utils.h"
#include "structs.h"
#include "telnet.h"
#include "msdp_parser.hpp"

#include <string>
#include <deque>

namespace msdp
{
	bool handle_request(DESCRIPTOR_DATA* t, std::shared_ptr<CVariable> request)
	{
		log("INFO: MSDP request %s.", request->name().c_str());

		CVariable::variable_ptr_t response;
		if ("LIST" == request->name())
		{
			if (CValue::EVT_STRING == request->value()->type())
			{
				CStringValue* string = dynamic_cast<CStringValue*>(request->value().get());
				if ("COMMANDS" == string->value())
				{
					log("INFO: '%s' asked for MSDP \"COMMANDS\" list.",
						(t && t->character) ? t->character->get_name().c_str() : "<unknown>");

					response.reset(new CVariable("COMMANDS",
						new CArrayValue({
						new CStringValue("LIST"),
						new CStringValue("REPORT"),
						new CStringValue("SEND")
					})));
				}
				else if ("REPORTABLE_VARIABLES" == string->value())
				{
					log("INFO: Client asked for MSDP \"REPORTABLE_VARIABLES\" list.");

					response.reset(new CVariable("REPORTABLE_VARIABLES",
						new CArrayValue({
						new CStringValue("ROOM"),
						new CStringValue("OBJ"),
						new CStringValue("STATE"),
						new CStringValue("GOLD"),
						new CStringValue("MAXHIT"),
						new CStringValue("MAXMANA"),
						new CStringValue("MAXMOVE"),
						new CStringValue("EXP"),
						new CStringValue("LEVEL"),
					})));
				}
				else if ("CONFIGURABLE_VARIABLES" == string->value())
				{
					log("INFO: Client asked for MSDP \"CONFIGURABLE_VARIABLES\" list.");

					response.reset(new CVariable("CONFIGURABLE_VARIABLES", new CArrayValue()));
				}
				else
				{
					log("INFO: Client asked for unknown MSDP list \"%s\".", string->value().c_str());
				}
			}
		}
		else if ("REPORT" == request->name())
		{
			if (CValue::EVT_STRING == request->value()->type())
			{
				CStringValue* string = dynamic_cast<CStringValue*>(request->value().get());
				log("INFO: Client asked for report of changing the variable \"%s\".", string->value().c_str());

				t->msdp_add_report_variable(string->value());
			}
		}
		else if ("UNREPORT" == request->name())
		{
			if (CValue::EVT_STRING == request->value()->type())
			{
				CStringValue* string = dynamic_cast<CStringValue*>(request->value().get());
				log("INFO: Client asked for unreport of changing the variable \"%s\".", string->value().c_str());

				t->msdp_remove_report_variable(string->value());
			}
		}

		if (nullptr == response.get())
		{
			return false;
		}

		const size_t buffer_size = WRAPPER_LENGTH + response->required_size();
		std::shared_ptr<char> buffer(new char[buffer_size]);
		buffer.get()[0] = char(IAC);
		buffer.get()[1] = char(SB);
		buffer.get()[2] = TELOPT_MSDP;
		response->serialize(HEAD_LENGTH + buffer.get(), buffer_size - WRAPPER_LENGTH);
		buffer.get()[buffer_size - 2] = char(IAC);
		buffer.get()[buffer_size - 1] = char(SE);

		hexdump(buffer.get(), buffer_size, "MSDP response:");

		int written = 0;
		write_to_descriptor_with_options(t, buffer.get(), buffer_size, written);

		return true;
	}

	size_t handle_conversation(DESCRIPTOR_DATA* t, const char* buffer, const size_t length)
	{
		size_t actual_length = 0;
		std::shared_ptr<CVariable> request;

		std::string i = indent(0);
		debug_log("Conversation from '%s':\n",
			(t && t->character) ? t->character->get_name().c_str() : "<unknown>");
		hexdump(buffer, length);

		if (4 > length)
		{
			log("WARNING: MSDP block is too small.");
			return 0;
		}

		if (!parse_request(buffer + HEAD_LENGTH, length - HEAD_LENGTH, actual_length, request))
		{
			log("WARNING: Could not parse MSDP request.");
			return 0;
		}

		request->dump();

		handle_request(t, request);

		return HEAD_LENGTH + actual_length;
	}


	void report_obj(DESCRIPTOR_DATA* d, OBJ_DATA *obj, bool drop)
	{
		//report
		if (!d->character)
		{
			return;
		}

		CVariable::variable_ptr_t response;
		CTableValue* object = new CTableValue();
		std::shared_ptr<char> obj_name(new char[1 + 4 * strlen(obj->get_PName(0).c_str())]);
		d->string_to_client_encoding(obj->get_PName(0).c_str(), obj_name.get());
		
		object->add(new CVariable("NAME", new CStringValue(obj_name.get())));
		// надо бы, конечно, сделать отдельный тип для bool, но мне лень, поэтому будет вот так
		object->add(new CVariable("DROP", new CStringValue(std::to_string(drop))));
		if (nullptr == response.get())
		{
			return;
		}

		debug_log("Report:");
		response->dump();

		const size_t buffer_size = WRAPPER_LENGTH + response->required_size();
		std::shared_ptr<char> buffer(new char[buffer_size]);
		buffer.get()[0] = char(IAC);
		buffer.get()[1] = char(SB);
		buffer.get()[2] = TELOPT_MSDP;
		response->serialize(HEAD_LENGTH + buffer.get(), buffer_size - WRAPPER_LENGTH);
		buffer.get()[buffer_size - 2] = char(IAC);
		buffer.get()[buffer_size - 1] = char(SE);

		hexdump(buffer.get(), buffer_size, "Response buffer:");

		int written = 0;
		write_to_descriptor_with_options(d, buffer.get(), buffer_size, written);
	}


	void report_room(DESCRIPTOR_DATA* d, CVariable::variable_ptr_t& response)
	{
		const auto rnum = IN_ROOM(d->character);
		const auto vnum = GET_ROOM_VNUM(rnum);
		if (NOWHERE == vnum)
		{
			return;
		}

		CTableValue* room_descriptor = new CTableValue();

		CTableValue* exits = new CTableValue();
		const auto directions = world[rnum]->dir_option;
		for (int i = 0; i < NUM_OF_DIRS; ++i)
		{
			if (directions[i]
				&& !EXIT_FLAGGED(directions[i], EX_HIDDEN))
			{
				const static std::string direction_commands[NUM_OF_DIRS] = { "n", "e", "s", "w", "u", "d" };
				const auto to_rnum = directions[i]->to_room;
				const auto to_vnum = GET_ROOM_VNUM(to_rnum);
				if (NOWHERE != to_vnum)	// Anton Gorev (2016-05-01): Some rooms has exits that  lead to nowhere. It is a workaround.
				{
					exits->add(new CVariable(direction_commands[i],
						new CStringValue(std::to_string(to_vnum))));
				}
			}
		}

		/* convert string date into client's encoding */
		// output might be more than input up to 4 times (in case of utf-8) plus NULL terminator.
		std::shared_ptr<char> room_name(new char[1 + 4*strlen(world[rnum]->name)]);
		d->string_to_client_encoding(world[rnum]->name, room_name.get());

		// output might be more than input up to 4 times (in case of utf-8) plus NULL terminator.
		std::shared_ptr<char> zone_name(new char[4 * strlen(zone_table[world[rnum]->zone].name)]);
		d->string_to_client_encoding(zone_table[world[rnum]->zone].name, zone_name.get());

		room_descriptor->add(new CVariable("VNUM",
			new CStringValue(std::to_string(vnum))));
		room_descriptor->add(new CVariable("NAME",
			new CStringValue(room_name.get())));
		room_descriptor->add(new CVariable("AREA",
			new CStringValue(zone_name.get())));
		room_descriptor->add(new CVariable("ZONE",
			new CStringValue(std::to_string(vnum / 100))));

		const auto stype = SECTOR_TYPE_BY_VALUE.find(world[rnum]->sector_type);
		if (stype != SECTOR_TYPE_BY_VALUE.end())
		{
			room_descriptor->add(new CVariable("TERRAIN",
				new CStringValue(stype->second)));
		}

		room_descriptor->add(new CVariable("EXITS", exits));

		response.reset(new CVariable("ROOM", room_descriptor));
	}

	void report_gold(DESCRIPTOR_DATA* d, CVariable::variable_ptr_t& response)
	{
		CTableValue* gold = new CTableValue();
		gold->add(new CVariable("MONEY", new CStringValue(std::to_string(d->character->get_gold()))));
		gold->add(new CVariable("BANK", new CStringValue(std::to_string(d->character->get_bank()))));
		response.reset(new CVariable("GOLD", gold));
	}

	void report_max_hit(DESCRIPTOR_DATA* d, CVariable::variable_ptr_t& response)
	{
		CTableValue* maxhit = new CTableValue();
		maxhit->add(new CVariable("MAXHIT", new CStringValue(std::to_string(GET_REAL_MAX_HIT(d->character)))));
		response.reset(new CVariable("MAXHIT", maxhit));
	}

	void report_max_move(DESCRIPTOR_DATA* d, CVariable::variable_ptr_t& response)
	{
		CTableValue* maxmove = new CTableValue();
		maxmove->add(new CVariable("MAXMOVE", new CStringValue(std::to_string(GET_REAL_MAX_MOVE(d->character)))));
		response.reset(new CVariable("MAXMOVE", maxmove));
	}

	void report_max_mana(DESCRIPTOR_DATA* d, CVariable::variable_ptr_t& response)
	{
		CTableValue* maxmana = new CTableValue();
		maxmana->add(new CVariable("MAXMANA", new CStringValue(std::to_string(GET_MAX_MANA(d->character)))));
		response.reset(new CVariable("MAXMANA", maxmana));
	}

	void report_level(DESCRIPTOR_DATA* d, CVariable::variable_ptr_t& response)
	{
		CTableValue* level = new CTableValue();
		level->add(new CVariable("LEVEL", new CStringValue(std::to_string(GET_LEVEL(d->character)))));
		response.reset(new CVariable("LEVEL", level));
	}

	void report_exp(DESCRIPTOR_DATA* d, CVariable::variable_ptr_t& response)
	{
		CTableValue* exp = new CTableValue();
		exp->add(new CVariable("EXP", new CStringValue(std::to_string(GET_LEVEL(d->character)))));
		response.reset(new CVariable("EXP", exp));
	}


	void report_state(DESCRIPTOR_DATA* d, CVariable::variable_ptr_t& response)
	{
		CTableValue* state = new CTableValue();
		state->add(new CVariable("CURHP", new CStringValue(std::to_string(GET_HIT(d->character)))));
		//state->add(new CVariable("MAXHP", new CStringValue(std::to_string(GET_REAL_MAX_HIT(d->character)))));
		//state->add(new CVariable("LEVEL", new CStringValue(std::to_string(GET_LEVEL(d->character)))));
		state->add(new CVariable("CURMOV", new CStringValue(std::to_string(GET_MOVE(d->character)))));
		//state->add(new CVariable("MAXMOV", new CStringValue(std::to_string(GET_REAL_MAX_MOVE(d->character)))));
		if (IS_MANA_CASTER(d->character))
		{
			state->add(new CVariable("CURMANA", new CStringValue(std::to_string(GET_MANA_STORED(d->character)))));
			//state->add(new CVariable("MAXMANA", new CStringValue(std::to_string(GET_MAX_MANA(d->character)))));
		}
		//state->add(new CVariable("MONEY", new CStringValue(std::to_string(d->character->get_gold()))));
		//state->add(new CVariable("EXP", new CStringValue(std::to_string(GET_EXP(d->character)))));

		response.reset(new CVariable("STATE", state));
	}

	void report(DESCRIPTOR_DATA* d, const std::string& name)
	{
		//report
		if (!d->character)
		{
			return;
		}

		CVariable::variable_ptr_t response;
		if ("ROOM" == name)
		{
			report_room(d, response);
		}

		if ("STATE" == name)
		{
			report_state(d, response);
		}
		if ("GOLD" == name)
		{
			report_gold(d, response);
		}
		if ("MAXHIT" == name)
		{
			report_max_hit(d, response);
		}
		if ("MAXMOVE" == name)
		{
			report_max_move(d, response);
		}
		if ("MAXMANA" == name)
		{
			report_max_mana(d, response);
		}
		if ("LEVEL" == name)
		{
			report_level(d, response);
		}
		if ("GOLD" == name)
		{
			report_gold(d, response);
		}
		if ("EXP" == name)
		{
			report_exp(d, response);
		}


		if (nullptr == response.get())
		{
			return;
		}

		debug_log("Report:");
		response->dump();

		const size_t buffer_size = WRAPPER_LENGTH + response->required_size();
		std::shared_ptr<char> buffer(new char[buffer_size]);
		buffer.get()[0] = char(IAC);
		buffer.get()[1] = char(SB);
		buffer.get()[2] = TELOPT_MSDP;
		response->serialize(HEAD_LENGTH + buffer.get(), buffer_size - WRAPPER_LENGTH);
		buffer.get()[buffer_size - 2] = char(IAC);
		buffer.get()[buffer_size - 1] = char(SE);

		hexdump(buffer.get(), buffer_size, "Response buffer:");

		int written = 0;
		write_to_descriptor_with_options(d, buffer.get(), buffer_size, written);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
