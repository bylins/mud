#include "msdp.hpp"

#include "char.hpp"
#include "constants.h"
#include "db.h"
#include "utils.h"
#include "structs.h"
#include "telnet.h"
#include "msdp.parser.hpp"
#include "msdp.constants.hpp"

#include <string>
#include <deque>
#include <unordered_map>
#include <memory>

namespace msdp
{
	class AbstractReporter
	{
	public:
		using shared_ptr = std::shared_ptr<AbstractReporter>;

		virtual void get(Variable::shared_ptr& response) = 0;

		virtual ~AbstractReporter() {}
	};

	class DescriptorBasedReporter: public AbstractReporter
	{
	public:
		DescriptorBasedReporter(const DESCRIPTOR_DATA* descriptor) : m_descriptor(descriptor) {}

	protected:
		const auto descriptor() const { return m_descriptor; }

	private:
		const DESCRIPTOR_DATA* m_descriptor;
	};

	class RoomReporter : public DescriptorBasedReporter
	{
	public:
		RoomReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<RoomReporter>(descriptor); }
	};

	void RoomReporter::get(Variable::shared_ptr& response)
	{
		const auto rnum = IN_ROOM(descriptor()->character);
		const auto vnum = GET_ROOM_VNUM(rnum);
		if (NOWHERE == vnum)
		{
			return;
		}

		const auto room_descriptor = std::make_shared<TableValue>();

		const auto exits = std::make_shared<TableValue>();
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
					exits->add(std::make_shared<Variable>(direction_commands[i],
						std::make_shared<StringValue>(std::to_string(to_vnum))));
				}
			}
		}

		/* convert string date into client's encoding */
		// output might be more than input up to 4 times (in case of utf-8) plus NULL terminator.
		std::shared_ptr<char> room_name(new char[1 + 4 * strlen(world[rnum]->name)]);
		descriptor()->string_to_client_encoding(world[rnum]->name, room_name.get());

		// output might be more than input up to 4 times (in case of utf-8) plus NULL terminator.
		std::shared_ptr<char> zone_name(new char[4 * strlen(zone_table[world[rnum]->zone].name)]);
		descriptor()->string_to_client_encoding(zone_table[world[rnum]->zone].name, zone_name.get());

		room_descriptor->add(std::make_shared<Variable>("VNUM",
			std::make_shared<StringValue>(std::to_string(vnum))));
		room_descriptor->add(std::make_shared<Variable>("NAME",
			std::make_shared<StringValue>(room_name.get())));
		room_descriptor->add(std::make_shared<Variable>("AREA",
			std::make_shared<StringValue>(zone_name.get())));
		room_descriptor->add(std::make_shared<Variable>("ZONE",
			std::make_shared<StringValue>(std::to_string(vnum / 100))));

		const auto stype = SECTOR_TYPE_BY_VALUE.find(world[rnum]->sector_type);
		if (stype != SECTOR_TYPE_BY_VALUE.end())
		{
			room_descriptor->add(std::make_shared<Variable>("TERRAIN",
				std::make_shared<StringValue>(stype->second)));
		}

		room_descriptor->add(std::make_shared<Variable>("EXITS", exits));

		response = std::make_shared<Variable>("ROOM", room_descriptor);
	}

	class GoldReporter : public DescriptorBasedReporter
	{
	public:
		GoldReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<GoldReporter>(descriptor); }
	};

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

	class MaxHitReporter : public DescriptorBasedReporter
	{
	public:
		MaxHitReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<MaxHitReporter>(descriptor); }
	};

	void MaxHitReporter::get(Variable::shared_ptr& response)
	{
		const auto value = std::to_string(GET_REAL_MAX_HIT(descriptor()->character));
		response = std::make_shared<Variable>(constants::MAX_HIT,
			std::make_shared<StringValue>(value));
	}

	class MaxMoveReporter : public DescriptorBasedReporter
	{
	public:
		MaxMoveReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<MaxMoveReporter>(descriptor); }
	};

	void MaxMoveReporter::get(Variable::shared_ptr& response)
	{
		const auto value = std::to_string(GET_REAL_MAX_MOVE(descriptor()->character));
		response = std::make_shared<Variable>(constants::MAX_MOVE,
			std::make_shared<StringValue>(value));
	}

	class MaxManaReporter : public DescriptorBasedReporter
	{
	public:
		MaxManaReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<MaxManaReporter>(descriptor); }
	};

	void MaxManaReporter::get(Variable::shared_ptr& response)
	{
		const auto max_mana = std::to_string(GET_MAX_MANA(descriptor()->character));
		response = std::make_shared<Variable>("MAX_MANA",
			std::make_shared<StringValue>(max_mana));
	}

	class LevelReporter : public DescriptorBasedReporter
	{
	public:
		LevelReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<LevelReporter>(descriptor); }
	};

	void LevelReporter::get(Variable::shared_ptr& response)
	{
		const auto level = std::to_string(GET_LEVEL(descriptor()->character));
		response = std::make_shared<Variable>(constants::LEVEL,
			std::make_shared<StringValue>(level));
	}

	class ExperienceReporter : public DescriptorBasedReporter
	{
	public:
		ExperienceReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<ExperienceReporter>(descriptor); }
	};

	void ExperienceReporter::get(Variable::shared_ptr& response)
	{
		const auto experience = std::to_string(GET_LEVEL(descriptor()->character));
		response = std::make_shared<Variable>(constants::EXPERIENCE,
			std::make_shared<StringValue>(experience));
	}

	class StateReporter : public DescriptorBasedReporter
	{
	public:
		StateReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<StateReporter>(descriptor); }
	};

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

	class ReporterFactory
	{
	public:
		static AbstractReporter::shared_ptr create(const DESCRIPTOR_DATA* descriptor, const std::string& name);
		static Variable::shared_ptr reportable_variables();

	private:
		using handler_t = std::function<AbstractReporter::shared_ptr(const DESCRIPTOR_DATA*)>;
		using handlers_t = std::unordered_map<std::string, handler_t>;

		static handlers_t s_handlers;
		const static ArrayValue::array_t REPORTABLE_VARIABLES_ARRAY;
		const static Value::shared_ptr REPORTABLE_VARIABLES_VALUE;
		const static Variable::shared_ptr REPORTABLE_VARIABLES;
	};

	msdp::AbstractReporter::shared_ptr ReporterFactory::create(const DESCRIPTOR_DATA* descriptor, const std::string& name)
	{
		const auto reporter = s_handlers.find(name);
		if (reporter != s_handlers.end())
		{
			return reporter->second(descriptor);
		}

		log("SYSERR: Undefined MSDP report requested.");
		return nullptr;
	}

	Variable::shared_ptr ReporterFactory::reportable_variables()
	{
		return REPORTABLE_VARIABLES;
	}

	ReporterFactory::handlers_t ReporterFactory::s_handlers = {
		{ constants::ROOM, std::bind(RoomReporter::create, std::placeholders::_1) },
		{ constants::EXPERIENCE, std::bind(ExperienceReporter::create, std::placeholders::_1) },
		{ constants::GOLD, std::bind(GoldReporter::create, std::placeholders::_1) },
		{ constants::LEVEL, std::bind(LevelReporter::create, std::placeholders::_1) },
		{ constants::MAX_HIT, std::bind(MaxHitReporter::create, std::placeholders::_1) },
		{ constants::MAX_MOVE, std::bind(MaxMoveReporter::create, std::placeholders::_1) },
		{ constants::STATE, std::bind(StateReporter::create, std::placeholders::_1) }
	};

	const ArrayValue::array_t ReporterFactory::REPORTABLE_VARIABLES_ARRAY =
	{
		std::make_shared<StringValue>(constants::ROOM),
		std::make_shared<StringValue>(constants::EXPERIENCE),
		std::make_shared<StringValue>(constants::GOLD),
		std::make_shared<StringValue>(constants::LEVEL),
		std::make_shared<StringValue>(constants::MAX_HIT),
		std::make_shared<StringValue>(constants::MAX_MOVE),
		std::make_shared<StringValue>(constants::STATE)
	};
	const Value::shared_ptr ReporterFactory::REPORTABLE_VARIABLES_VALUE = std::make_shared<ArrayValue>(ReporterFactory::REPORTABLE_VARIABLES_ARRAY);
	const Variable::shared_ptr ReporterFactory::REPORTABLE_VARIABLES = std::make_shared<Variable>("REPORTABLE_VARIABLES", ReporterFactory::REPORTABLE_VARIABLES_VALUE);

	class ConversationHandler
	{
	public:
		ConversationHandler(DESCRIPTOR_DATA* descriptor) : m_descriptor(descriptor) {}
		size_t operator()(const char* buffer, const size_t length);

	private:
		const static ArrayValue::array_t SUPPORTED_COMMANDS_LIST;
		const static Value::shared_ptr SUPPORTED_COMMANDS_ARRAY;

		void handle_list_command(const Variable::shared_ptr& request, Variable::shared_ptr& response);
		void handle_report_command(const Variable::shared_ptr& request);
		void handle_unreport_command(const Variable::shared_ptr& request);
		void handle_send_command(const Variable::shared_ptr& request);
		bool handle_request(const Variable::shared_ptr& request);

		DESCRIPTOR_DATA* m_descriptor;
	};

	size_t ConversationHandler::operator()(const char* buffer, const size_t length)
	{
		size_t actual_length = 0;
		Variable::shared_ptr request;

		std::string i = indent(0);
		debug_log("Conversation from '%s':\n",
			(m_descriptor && m_descriptor->character) ? m_descriptor->character->get_name().c_str() : "<unknown>");
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

		handle_request(request);

		return HEAD_LENGTH + actual_length;
	}

	const ArrayValue::array_t ConversationHandler::SUPPORTED_COMMANDS_LIST = {
		std::make_shared<StringValue>("LIST"),
		std::make_shared<StringValue>("REPORT"),
		std::make_shared<StringValue>("UNREPORT"),
		std::make_shared<StringValue>("SEND")
	};
	const Value::shared_ptr ConversationHandler::SUPPORTED_COMMANDS_ARRAY = std::make_shared<ArrayValue>(ConversationHandler::SUPPORTED_COMMANDS_LIST);
	
	void ConversationHandler::handle_list_command(const Variable::shared_ptr& request, Variable::shared_ptr& response)
	{
		if (Value::EVT_STRING != request->value()->type())
		{
			return;
		}

		const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
		if ("COMMANDS" == string->value())
		{
			log("INFO: '%s' asked for MSDP \"COMMANDS\" list.",
				(m_descriptor && m_descriptor->character) ? m_descriptor->character->get_name().c_str() : "<unknown>");

			response.reset(new Variable("COMMANDS", SUPPORTED_COMMANDS_ARRAY));
		}
		else if ("REPORTABLE_VARIABLES" == string->value())
		{
			log("INFO: Client asked for MSDP \"REPORTABLE_VARIABLES\" list.");

			response = ReporterFactory::reportable_variables();
		}
		else if ("CONFIGURABLE_VARIABLES" == string->value())
		{
			log("INFO: Client asked for MSDP \"CONFIGURABLE_VARIABLES\" list.");

			response = std::make_shared<Variable>("CONFIGURABLE_VARIABLES", std::make_shared<ArrayValue>());
		}
		else
		{
			log("INFO: Client asked for unknown MSDP list \"%s\".", string->value().c_str());
		}
	}

	void ConversationHandler::handle_report_command(const Variable::shared_ptr& request)
	{
		if (Value::EVT_STRING != request->value()->type())
		{
			return;
		}

		const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
		log("INFO: Client asked for report of changing the variable \"%s\".", string->value().c_str());

		m_descriptor->msdp_add_report_variable(string->value());
	}

	void ConversationHandler::handle_unreport_command(const Variable::shared_ptr& request)
	{
		if (Value::EVT_STRING != request->value()->type())
		{
			return;
		}

		const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
		log("INFO: Client asked for unreport of changing the variable \"%s\".", string->value().c_str());

		m_descriptor->msdp_remove_report_variable(string->value());
	}
	
	void ConversationHandler::handle_send_command(const Variable::shared_ptr& request)
	{
		if (Value::EVT_STRING != request->value()->type())
		{
			return;
		}

		const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
		report(m_descriptor, string->value());
	}

	bool ConversationHandler::handle_request(const Variable::shared_ptr& request)
	{
		log("INFO: MSDP request %s.", request->name().c_str());

		Variable::shared_ptr response;
		if ("LIST" == request->name())
		{
			handle_list_command(request, response);
		}
		else if ("REPORT" == request->name())
		{
			handle_report_command(request);
		}
		else if ("UNREPORT" == request->name())
		{
			handle_unreport_command(request);
		}
		else if ("SEND" == request->name())
		{
			handle_send_command(request);
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
		write_to_descriptor_with_options(m_descriptor, buffer.get(), buffer_size, written);

		return true;
	}

	size_t handle_conversation(DESCRIPTOR_DATA* t, const char* buffer, const size_t length)
	{
		ConversationHandler handler(t);
		return handler(buffer, length);
	}

	class ReportSender
	{
	public:
		ReportSender(DESCRIPTOR_DATA* descriptor);
		void send(const AbstractReporter::shared_ptr reporter);

	private:
		DESCRIPTOR_DATA* m_descriptor;
	};

	ReportSender::ReportSender(DESCRIPTOR_DATA* descriptor) : m_descriptor(descriptor)
	{
	}

	void ReportSender::send(const AbstractReporter::shared_ptr reporter)
	{
		if (!m_descriptor->character)
		{
			log("SYSERR: Requested MSDP report for descriptor without character.");
			return;
		}

		if (!reporter)
		{
			log("SYSERR: MSDP reporter was not set at send() function.");
			return;
		}

		Variable::shared_ptr response;
		reporter->get(response);
		if (!response)
		{
			log("SYSERR: MSDP response was not set.");
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
		write_to_descriptor_with_options(m_descriptor, buffer.get(), buffer_size, written);
	}

	void Report::operator()(DESCRIPTOR_DATA* d, const std::string& name)
	{
		ReportSender sender(d);

		const auto reporter = ReporterFactory::create(d, name);
		if (!reporter)
		{
			return;
		}

		sender.send(reporter);
	}

	Report report;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
