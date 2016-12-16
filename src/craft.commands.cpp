#include "craft.commands.hpp"

#include "logger.hpp"
#include "craft.logger.hpp"
#include "craft.static.hpp"
#include "xml_loading_helper.hpp"
#include "craft.hpp"
#include "char.hpp"
#include "commands.hpp"

#include <sstream>
#include <iomanip>

namespace craft
{
	using xml::loading::CHelper;

	extern CCraftModel model;	///< to avoid moving #model into headers

	/// Contains handlers of craft subcommands
	namespace cmd
	{
		/**
		* Returns pointer to the first argument, moves command_line to the beginning of the next argument.
		*
		* \note This routine changes a passed buffer: puts '\0' character after the first argument.
		*/
		const char* first_argument(char*& command_line)
		{
			// skip leading spaces
			while (*command_line && ' ' == *command_line)
			{
				++command_line;
			}

			// getting subcommand
			const char* result = command_line;
			while (*command_line && ' ' != *command_line)
			{
				++command_line;
			}

			// anchor subcommand and move argument pointer to the next one
			if (*command_line)
			{
				*(command_line++) = '\0';
			}

			return result;
		}

		void list_skills(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			send_to_char(ch, "Listing craft skills...\nArguments: '%s'\nCount:%d\n", arguments, model.skills().size());

			size_t counter = 0;
			for (const auto& s : model.skills())
			{
				++counter;
				send_to_char(ch, "%2d. %s\n", counter, s.id().c_str());
			}
		}

		void list_recipes(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			send_to_char(ch, "Listing craft recipes...\nArguments: '%s'\nCount:%d\n", arguments, model.recipes().size());

			size_t counter = 0;
			for (const auto& r : model.recipes())
			{
				++counter;
				send_to_char(ch, "%2d. %s\n", counter, r.id().c_str());
			}
		}

		void list_prototypes(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			send_to_char(ch, "Listing craft prototypes...\nArguments: '%s'\nCount: %d\n", arguments, model.prototypes().size());

			size_t counter = 0;
			for (const auto& p : model.prototypes())
			{
				++counter;
				send_to_char(ch, "%2d. %s\n", counter, p->get_short_description().c_str());
			}
		}

		void list_properties(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			send_to_char(ch, "Craft properties...\nArguments: '%s'\n", arguments);
			send_to_char(ch, "Base count:              &W%4d&n crafts\n", model.base_count());
			send_to_char(ch, "Remorts for count bonus: &W%4d&n remorts\n", model.remort_for_count_bonus());
			send_to_char(ch, "Base top:                &W%4d&n percents\n", model.base_top());
			send_to_char(ch, "Remorts bonus:           &W%4d&n percent\n", model.remorts_bonus());
		}

		void list_materials(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			send_to_char(ch, "Craft materials...\nArguments: '%s'\n", arguments);
			size_t number = 0;
			const size_t size = model.materials().size();

			if (0 == size)
			{
				send_to_char(ch, "No materials loaded.\n");
			}

			for (const auto& material : model.materials())
			{
				++number;
				const size_t classes_count = material.classes().size();
				send_to_char(ch, " &W%d. %s&n (&Y%s&n)%s\n",
					number,
					material.get_name().c_str(),
					material.id().c_str(),
					(0 == classes_count ? " <&Rmaterial does not have classes&n>" : ":"));
				size_t class_number = 0;
				for (const auto& material_class : material.classes())
				{
					++class_number;
					send_to_char(ch, "   &g%d. %s&n (&B%s&n)\n",
						class_number,
						material_class.name().c_str(),
						material_class.id().c_str());
				}
			}
		}

		using subcommand_handler_t = void(*)(CHAR_DATA *, char*, void*);
		using subcommands_t = std::map<std::string, subcommand_handler_t>;

		class CSubcommands
		{
		public:
			enum EProcessResult
			{
				EPR_PROCESSED,		///< Subcommand has been successfully processed
				EPR_UNKNOWN,		///< Unknown subcommand has been provided
				EPR_NO_SUBCOMMAND,	///< No subcommand has been provided
				EPR_WRONG_ARGUMENT	///< Logic error: passed #argument has nullptr value
			};

			static EProcessResult process(const subcommands_t& subcommands_table, CHAR_DATA* ch, char*arguments, void* data);
		};

		CSubcommands::EProcessResult CSubcommands::process(const subcommands_t& subcommands_table, CHAR_DATA* ch, char*arguments, void* /*data*/)
		{
			if (!arguments)
			{
				logger("SYSERROR: argument passed into %s:%d is NULL.", __FILE__, __LINE__);
				send_to_char(ch, "Something went wrong... Send bug report, please. :(\n");

				return EPR_WRONG_ARGUMENT;
			}

			const char* subcommand = first_argument(arguments);
			if (!*subcommand)
			{
				return EPR_NO_SUBCOMMAND;
			}

			subcommands_t::const_iterator i = subcommands_table.lower_bound(subcommand);

			if (i != subcommands_table.end()
				&& i->first.c_str() == strstr(i->first.c_str(), subcommand))
			{
				// found subcommand handler
				i->second(ch, arguments, nullptr);
				return EPR_PROCESSED;
			}

			return EPR_UNKNOWN;
		}

		void do_craft_list(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			static subcommands_t subcommands =
			{
				{ "properties", cmd::list_properties },
				{ "prototypes", cmd::list_prototypes },
				{ "recipes", cmd::list_recipes },
				{ "skills", cmd::list_skills },
				{ "materials", cmd::list_materials }
			};

			const auto result = CSubcommands::process(subcommands, ch, arguments, nullptr);
			switch (result)
			{
			case CSubcommands::EPR_NO_SUBCOMMAND:
				{
					send_to_char(ch, "Listing crafts...\nArguments: '%s'\nCount:%d\n", arguments, model.crafts().size());

					size_t counter = 0;
					for (const auto& c : model.crafts())
					{
						++counter;
						send_to_char(ch, "%2d. %s\n", counter, c.id().c_str());
					}
				}
				break;

			case CSubcommands::EPR_PROCESSED:
				break;

			default:
				send_to_char(ch, "Wrong syntax of the '&glist&n' subcommand.\n");
			}
		}

		void do_craft_export_prototype(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			const auto vnum_str = first_argument(arguments);
			obj_vnum vnum = 0;
			try
			{
				CHelper::load_integer(vnum_str, vnum,
					[&]() { throw std::runtime_error("wrong VNUM value"); });
			}
			catch (...)
			{
				send_to_char(ch, "Could not convert prototype VNUM value '%s' to integer number.\n", vnum_str);
				return;
			}

			const auto filename = first_argument(arguments);
			if (!*filename)
			{
				send_to_char(ch, "File name to export is not specified.");
				return;
			}

			if (!model.export_object(vnum, filename))
			{
				send_to_char(ch, "Failed to export prototype.");
			}
			else
			{
				send_to_char(ch, "Prototype with VNUM %d successfully exported into file '%s'.", vnum, filename);
			}
		}

		void do_craft_export(CHAR_DATA* ch, char* arguments, void* /*data*/)
		{
			static subcommands_t subcommands =
			{
				{ "prototype", do_craft_export_prototype }
			};

			const auto result = CSubcommands::process(subcommands, ch, arguments, nullptr);
			switch (result)
			{
			case CSubcommands::EPR_NO_SUBCOMMAND:
				send_to_char(ch, "Usage: craft export prototype <vnum> <filename>\n");
				break;

			case CSubcommands::EPR_PROCESSED:
				break;

			default:
				send_to_char(ch, "Wrong syntax of the '&gexport&n' subcommand.\n");
				send_to_char(ch, "Usage: craft export prototype <vnum> <filename>\n");
			}
		}

		using namespace commands::utils;

		class CraftContext : public ReplyableContext
		{
		public:
			using shared_ptr = std::shared_ptr<CraftContext>;

			CraftContext(CHAR_DATA* character) : m_character(character) {}

			virtual void reply(const std::string& message) const override;

			static shared_ptr create(CHAR_DATA* character) { return std::make_shared<CraftContext>(character); }

		private:
			CHAR_DATA* m_character;
		};

		void CraftContext::reply(const std::string& message) const
		{
			send_to_char(message, m_character);
		}

		class CommonCraftCommand : public CommandWithHelp
		{
		protected:
			void send(const CommandContext::shared_ptr& context, const std::string& message) const;
			void usage(const CommandContext::shared_ptr& context) const { send(context, get_help()); }
		};

		void CommonCraftCommand::send(const CommandContext::shared_ptr& context, const std::string& message) const
		{
			const auto rcontext = std::dynamic_pointer_cast<ReplyableContext>(context);
			if (rcontext)
			{
				rcontext->reply(message);
			}
		}

		// "craft export prototype" command leaf
		class ExportPrototype : public CommonCraftCommand
		{
		public:
			using shared_ptr = std::shared_ptr<ExportPrototype>;

			virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& arguments) override;

			static auto create() { return std::make_shared<ExportPrototype>(); }
		};

		void ExportPrototype::execute(const CommandContext::shared_ptr& context, const arguments_t& arguments)
		{
			if (2 != arguments.size())
			{
				usage(context);
				return;
			}

			auto argument_i = arguments.begin();
			const auto vnum_str = argument_i->c_str();
			obj_vnum vnum = 0;
			try
			{
				CHelper::load_integer(vnum_str, vnum,
					[&]() { throw std::runtime_error("wrong VNUM value"); });
			}
			catch (...)
			{
				std::stringstream ss;
				ss << "Could not convert prototype VNUM value '" << vnum_str << "' to integer number.\n";
				send(context, ss.str());
				return;
			}

			++argument_i;
			const auto filename = argument_i->c_str();
			if (!*filename)
			{
				send(context, "File name to export is not specified.");
				return;
			}

			if (!model.export_object(vnum, filename))
			{
				send(context, "Failed to export prototype.");
			}
			else
			{
				std::stringstream ss;
				ss << "Prototype with VNUM " << vnum << " successfully exported into file '" << filename << "'.";
				send(context, ss.str());
			}
		}

		// "craft list properties" command leaf
		class ListProperties : public CommonCraftCommand
		{
		public:
			using shared_ptr = std::shared_ptr<ListProperties>;

			virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& arguments) override;

			static auto create() { return std::make_shared<ListProperties>(); }
		};

		void ListProperties::execute(const CommandContext::shared_ptr& context, const arguments_t& arguments)
		{
			std::stringstream ss;
			std::string arguments_string;
			joinList(arguments, arguments_string);
			ss << "Craft properties...\nArguments: '" << arguments_string << "'\n"
				<< "Base count:              &W" << std::setw(4) << model.base_count() << "&n crafts\n"
				<< "Remorts for count bonus: &W" << std::setw(4) << model.remort_for_count_bonus() << "&n remorts\n"
				<< "Base top:                &W" << std::setw(4) << model.base_top() << "&n percents\n"
				<< "Remorts bonus:           &W" << std::setw(4) << model.remorts_bonus() << "&n percent\n";
			send(context, ss.str());
		}

		// "craft" with no arguments
		class Root: public CommonCraftCommand
		{
		public:
			using shared_ptr = std::shared_ptr<Root>;

			virtual void execute(const CommandContext::shared_ptr& context, const arguments_t& arguments) override;

			static auto create() { return std::make_shared<Root>(); }
		};

		void Root::execute(const CommandContext::shared_ptr& context, const arguments_t& arguments)
		{
			send(context, "Crafting something... :)\n");
		}

		class CommandsHandlerImplementation : public CommandsHandler
		{
		public:
			virtual void initialize() override;
			virtual void process(CHAR_DATA* character, char* arguments) override;

		private:
			CommandEmbranchment::shared_ptr m_command;
		};

		void CommandsHandlerImplementation::initialize()
		{
			m_command = CommandEmbranchment::create();
			m_command->set_name("craft");
			m_command->set_noargs_handler(Root::create());

			const auto list_command = CommandEmbranchment::create();
			//list_command->add_command("properties", ListProperties::create());
			list_command->set_name("craft list");
			m_command->add_command("list1", list_command);
			m_command->add_command("list2", list_command);
			m_command->add_command("list3", list_command);
			m_command->add_command("list31", list_command);
			m_command->add_command("list32", list_command);

			const auto export_command = CommandEmbranchment::create();
			list_command->set_name("craft prototype");
			//export_command->add_command("prototype", ExportPrototype::create());
			m_command->add_command("export", export_command);
		}

		void CommandsHandlerImplementation::process(CHAR_DATA* character, char* arguments)
		{
			AbstractCommand::arguments_t arguments_list;
			while (*arguments)
			{
				auto next_argument = first_argument(arguments);
				if (*next_argument)
				{
					arguments_list.push_back(next_argument);
				}
			}
			const auto context = CraftContext::create(character);
			m_command->execute(context, arguments_list);
		}

		CommandsHandler::shared_ptr CommandsHandler::create()
		{
			return std::make_shared<CommandsHandlerImplementation>();
		}

		/**
		* "craft" command handler.
		*
		* Syntax:
		* craft list								- list of crafts
		* craft list prototypes						- list of prototypes loaded by craft system.
		* craft list properties						- list of properties loaded by craft system.
		* craft list recipes						- list of recipes loaded by craft system.
		* craft list skills							- list of skills loaded by craft system.
		* craft list materials						- list of materials loaded by craft system.
		* craft export prototype <vnum> <filename>	- exports prototype with <vnum> into file <filename>
		*/
		void do_craft(CHAR_DATA *ch, char *arguments, int /*cmd*/, int /*subcmd*/)
		{
			commands_handler->process(ch, arguments);
		}
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
