#include "craft.commands.hpp"

#include "logger.hpp"
#include "craft.logger.hpp"
#include "craft.static.hpp"
#include "xml_loading_helper.hpp"
#include "craft.hpp"
#include "chars/character.h"
#include "commands.hpp"

#include <sstream>
#include <iomanip>

namespace craft {
using xml::loading::CHelper;

extern CCraftModel model;    ///< to avoid moving #model into headers

/// Contains handlers of craft subcommands
namespace cmd {
const char *CRAFT_COMMAND = "craft";

namespace {
using namespace commands::utils;

// "craft export prototype" command leaf
class ExportPrototype : public CommonCommand {
 public:
  using shared_ptr = std::shared_ptr<ExportPrototype>;

  ExportPrototype() { set_help_line("Allows to export prototype into XML file."); }

  virtual void execute(const CommandContext::shared_ptr &context,
                       const arguments_t &path,
                       const arguments_t &arguments) override;
};

void ExportPrototype::execute(const CommandContext::shared_ptr &context,
                              const arguments_t &,
                              const arguments_t &arguments) {
  if (2 != arguments.size()) {
    usage(context);
    return;
  }

  auto argument_i = arguments.begin();
  const auto vnum_str = argument_i->c_str();
  obj_vnum vnum = 0;
  try {
    CHelper::load_integer(vnum_str, vnum, [&]() { throw std::runtime_error("wrong VNUM value"); });
  }
  catch (...) {
    std::stringstream ss;
    ss << "Could not convert prototype VNUM value '" << vnum_str << "' to integer number.\n";
    send(context, ss.str());
    return;
  }

  ++argument_i;
  const auto filename = argument_i->c_str();
  if (!*filename) {
    send(context, "File name to export is not specified.");
    return;
  }

  if (!model.export_object(vnum, filename)) {
    send(context, "Failed to export prototype.");
  } else {
    std::stringstream ss;
    ss << "Prototype with VNUM " << vnum << " successfully exported into file '" << filename << "'.";
    send(context, ss.str());
  }
}

// "craft list skills" command leaf
class ListSkills : public CommonCommand {
 public:
  using shared_ptr = std::shared_ptr<ListSkills>;

  ListSkills() { set_help_line("Shows skills registered in the craft system."); }

  virtual void execute(const CommandContext::shared_ptr &context,
                       const arguments_t &path,
                       const arguments_t &arguments) override;
};

void ListSkills::execute(const CommandContext::shared_ptr &context, const arguments_t &, const arguments_t &arguments) {
  std::stringstream ss;

  ss << "Listing craft skills..." << std::endl
     << "Arguments: '" << JoinRange<arguments_t>(arguments) << '\'' << std::endl
     << "Count: " << model.skills().size() << std::endl;

  size_t counter = 0;
  for (const auto &s : model.skills()) {
    ++counter;
    ss << std::setw(2) << counter << ". " << s.id() << std::endl;
  }

  send(context, ss.str());
}

// "craft list materials" command leaf
class ListMaterials : public CommonCommand {
 public:
  using shared_ptr = std::shared_ptr<ListMaterials>;

  ListMaterials() { set_help_line("Shows materials registered in the craft system."); }

  virtual void execute(const CommandContext::shared_ptr &context,
                       const arguments_t &path,
                       const arguments_t &arguments) override;
};

void ListMaterials::execute(const CommandContext::shared_ptr &context,
                            const arguments_t &,
                            const arguments_t &arguments) {
  std::stringstream ss;

  ss << "Craft materials..." << std::endl
     << "Arguments: '" << JoinRange<arguments_t>(arguments) << "'" << std::endl;
  size_t number = 0;
  const size_t size = model.materials().size();

  if (0 == size) {
    ss << "No materials loaded." << std::endl;
  }

  for (const auto &material : model.materials()) {
    ++number;
    const size_t classes_count = material->classes().size();
    ss << " &W" << number << ". " << material->get_name() << "&n (&Y" << material->id() << "&n)"
       << (0 == classes_count ? " <&Rmaterial does not have classes&n>" : ":") << std::endl;
    size_t class_number = 0;
    for (const auto &material_class : material->classes()) {
      ++class_number;
      ss << "   &g" << class_number << ". " << material_class.name() << "&n (&B"
         << material_class.id() << "&n)" << std::endl;
    }
  }

  send(context, ss.str());
}

// "craft list recipes" command leaf
class ListRecipes : public CommonCommand {
 public:
  using shared_ptr = std::shared_ptr<ListRecipes>;

  ListRecipes() { set_help_line("Shows recipes registered in the craft system."); }

  virtual void execute(const CommandContext::shared_ptr &context,
                       const arguments_t &path,
                       const arguments_t &arguments) override;
};

void ListRecipes::execute(const CommandContext::shared_ptr &context,
                          const arguments_t &,
                          const arguments_t &arguments) {
  std::stringstream ss;

  ss << "Listing craft recipes..." << std::endl
     << "Arguments: '" << JoinRange<arguments_t>(arguments) << '\'' << std::endl
     << "Count: " << model.recipes().size() << std::endl;

  size_t counter = 0;
  for (const auto &r : model.recipes()) {
    ++counter;
    ss << std::setw(2) << counter << ". " << r->id() << std::endl;
  }

  send(context, ss.str());
}

// "craft list prototypes" command leaf
class ListPrototypes : public CommonCommand {
 public:
  using shared_ptr = std::shared_ptr<ListPrototypes>;

  ListPrototypes() { set_help_line("Shows prototypes registered in the craft system."); }

  virtual void execute(const CommandContext::shared_ptr &context,
                       const arguments_t &path,
                       const arguments_t &arguments) override;
};

void ListPrototypes::execute(const CommandContext::shared_ptr &context,
                             const arguments_t &,
                             const arguments_t &arguments) {
  std::stringstream ss;
  ss << "Listing craft prototypes..." << std::endl
     << "Arguments: '" << JoinRange<arguments_t>(arguments) << '\'' << std::endl
     << "Count: " << model.prototypes().size() << std::endl;

  size_t counter = 0;
  for (const auto &p : model.prototypes()) {
    ++counter;
    ss << std::setw(2) << counter << ". " << p->get_short_description() << std::endl;
  }

  send(context, ss.str());
}

// "craft list properties" command leaf
class ListProperties : public CommonCommand {
 public:
  using shared_ptr = std::shared_ptr<ListProperties>;

  ListProperties() { set_help_line("Shows craft system properties."); }

  virtual void execute(const CommandContext::shared_ptr &context,
                       const arguments_t &path,
                       const arguments_t &arguments) override;
};

void ListProperties::execute(const CommandContext::shared_ptr &context,
                             const arguments_t &,
                             const arguments_t &arguments) {
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
class Root : public CommonCommand {
 public:
  using shared_ptr = std::shared_ptr<Root>;

  virtual void execute(const CommandContext::shared_ptr &context,
                       const arguments_t &path,
                       const arguments_t &arguments) override;

  static auto create() { return std::make_shared<Root>(); }
};

void Root::execute(const CommandContext::shared_ptr &context, const arguments_t &, const arguments_t &) {
  send(context, "Crafting something... :)\n");
}

class CommandsHandler : public commands::AbstractCommandsHanler {
 public:
  virtual void initialize() override;
  virtual void process(CHAR_DATA *character, char *arguments) override;

 private:
  CommandEmbranchment::shared_ptr m_command;
};

void CommandsHandler::initialize() {
  m_command = CommandEmbranchment::create();
  const auto export_command = CommandEmbranchment::create("Allows to export craft objects.");
  export_command->add_command("prototype", std::make_shared<ExportPrototype>())
      .rebuild_help();

  const auto list_command = CommandEmbranchment::create("Shows various properties of the craft system.");
  list_command->add_command("properties", std::make_shared<ListProperties>())
      .add_command("prototypes", std::make_shared<ListPrototypes>())
      .add_command("recipes", std::make_shared<ListRecipes>())
      .add_command("skills", std::make_shared<ListSkills>())
      .add_command("materials", std::make_shared<ListMaterials>())
      .add_command("help", std::make_shared<ParentalHelp>(list_command))
      .rebuild_help();

  m_command->set_noargs_handler(Root::create())
      .add_command("export", export_command)
      .add_command("list", list_command)
      .add_command("help", std::make_shared<ParentalHelp>(m_command))
      .rebuild_help();
}

void CommandsHandler::process(CHAR_DATA *character, char *arguments) {
  AbstractCommand::arguments_t arguments_list(arguments);
  const auto context = std::make_shared<ReplyableContext>(character);
  AbstractCommand::arguments_t path;
  path.push_back(craft::cmd::CRAFT_COMMAND);
  m_command->execute(context, path, arguments_list);
}

const commands::AbstractCommandsHanler::shared_ptr &commands_handler() {
  static commands::HandlerInitializer<CommandsHandler> handler;

  return handler();
}
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
void do_craft(CHAR_DATA *ch, char *arguments, int /*cmd*/, int /*subcmd*/) {
  commands_handler()->process(ch, arguments);
}
}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
