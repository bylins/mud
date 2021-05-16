#include "msdp.hpp"

#include "chars/character.h"
#include "telnet.h"
#include "msdp.parser.hpp"
#include "msdp.constants.hpp"
#include "msdp.senders.hpp"
#include "msdp.reporters.hpp"

#include <string>
#include <deque>
#include <unordered_map>
#include <memory>

namespace msdp {
class ReporterFactory {
 public:
  static AbstractReporter::shared_ptr create(const DESCRIPTOR_DATA *descriptor, const std::string &name);
  const static Variable::shared_ptr &reportable_variables();

 private:
  using handler_t = std::function<AbstractReporter::shared_ptr(const DESCRIPTOR_DATA *)>;
  using handlers_t = std::unordered_map<std::string, handler_t>;

  const static handlers_t &handlers();
};

AbstractReporter::shared_ptr ReporterFactory::create(const DESCRIPTOR_DATA *descriptor, const std::string &name) {
  const auto reporter = handlers().find(name);
  if (reporter != handlers().end()) {
    return reporter->second(descriptor);
  }

  log("SYSERR: Undefined MSDP report requested.");
  return nullptr;
}

const Variable::shared_ptr &ReporterFactory::reportable_variables() {
  const static ArrayValue::array_t REPORTABLE_VARIABLES_ARRAY = {
      std::make_shared<StringValue>(constants::ROOM),
      std::make_shared<StringValue>(constants::EXPERIENCE),
      std::make_shared<StringValue>(constants::GOLD),
      std::make_shared<StringValue>(constants::LEVEL),
      std::make_shared<StringValue>(constants::MAX_HIT),
      std::make_shared<StringValue>(constants::MAX_MOVE),
      std::make_shared<StringValue>(constants::STATE),
      std::make_shared<StringValue>(constants::GROUP)
  };

  const static Value::shared_ptr REPORTABLE_VARIABLES_VALUE = std::make_shared<ArrayValue>(REPORTABLE_VARIABLES_ARRAY);

  const static Variable::shared_ptr REPORTABLE_VARIABLES = std::make_shared<Variable>("REPORTABLE_VARIABLES",
                                                                                      REPORTABLE_VARIABLES_VALUE);

  return REPORTABLE_VARIABLES;
}

const ReporterFactory::handlers_t &ReporterFactory::handlers() {
  static const ReporterFactory::handlers_t s_handlers = {{
                                                             constants::ROOM,
                                                             std::bind(RoomReporter::create, std::placeholders::_1)
                                                         },
                                                         {constants::EXPERIENCE,
                                                          std::bind(ExperienceReporter::create, std::placeholders::_1)},
                                                         {constants::GOLD,
                                                          std::bind(GoldReporter::create, std::placeholders::_1)},
                                                         {constants::LEVEL,
                                                          std::bind(LevelReporter::create, std::placeholders::_1)},
                                                         {constants::MAX_HIT,
                                                          std::bind(MaxHitReporter::create, std::placeholders::_1)},
                                                         {constants::MAX_MOVE,
                                                          std::bind(MaxMoveReporter::create, std::placeholders::_1)},
                                                         {constants::STATE,
                                                          std::bind(StateReporter::create, std::placeholders::_1)},
                                                         {constants::GROUP,
                                                          std::bind(GroupReporter::create, std::placeholders::_1)}};

  return s_handlers;
}

class ConversationHandler {
 public:
  ConversationHandler(DESCRIPTOR_DATA *descriptor) : m_descriptor(descriptor) {}
  size_t operator()(const char *buffer, const size_t length);

 private:
  const static ArrayValue::array_t SUPPORTED_COMMANDS_LIST;
  const static Value::shared_ptr SUPPORTED_COMMANDS_ARRAY;

  void handle_list_command(const Variable::shared_ptr &request, Variable::shared_ptr &response);
  void handle_report_command(const Variable::shared_ptr &request);
  void handle_unreport_command(const Variable::shared_ptr &request);
  void handle_send_command(const Variable::shared_ptr &request);
  bool handle_request(const Variable::shared_ptr &request);

  DESCRIPTOR_DATA *m_descriptor;
};

size_t ConversationHandler::operator()(const char *buffer, const size_t length) {
  size_t actual_length = 0;
  Variable::shared_ptr request;

  debug_log("Conversation from '%s':\n",
            (m_descriptor && m_descriptor->character) ? m_descriptor->character->get_name().c_str() : "<unknown>");
  hexdump(buffer, length);

  if (4 > length) {
    log("WARNING: MSDP block is too small.");
    return 0;
  }

  if (!parse_request(buffer + HEAD_LENGTH, length - HEAD_LENGTH, actual_length, request)) {
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

const Value::shared_ptr ConversationHandler::SUPPORTED_COMMANDS_ARRAY =
    std::make_shared<ArrayValue>(ConversationHandler::SUPPORTED_COMMANDS_LIST);

void ConversationHandler::handle_list_command(const Variable::shared_ptr &request, Variable::shared_ptr &response) {
  if (Value::EVT_STRING != request->value()->type()) {
    return;
  }

  const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
  if ("COMMANDS" == string->value()) {
    log("INFO: '%s' asked for MSDP \"COMMANDS\" list.",
        (m_descriptor && m_descriptor->character) ? m_descriptor->character->get_name().c_str() : "<unknown>");

    response.reset(new Variable("COMMANDS", SUPPORTED_COMMANDS_ARRAY));
  } else if ("REPORTABLE_VARIABLES" == string->value()) {
    log("INFO: Client asked for MSDP \"REPORTABLE_VARIABLES\" list.");

    response = ReporterFactory::reportable_variables();
  } else if ("CONFIGURABLE_VARIABLES" == string->value()) {
    log("INFO: Client asked for MSDP \"CONFIGURABLE_VARIABLES\" list.");

    response = std::make_shared<Variable>("CONFIGURABLE_VARIABLES", std::make_shared<ArrayValue>());
  } else {
    log("INFO: Client asked for unknown MSDP list \"%s\".", string->value().c_str());
  }
}

void ConversationHandler::handle_report_command(const Variable::shared_ptr &request) {
  if (Value::EVT_STRING != request->value()->type()) {
    return;
  }

  const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
  log("INFO: Client asked for report of changing the variable \"%s\".", string->value().c_str());

  m_descriptor->msdp_add_report_variable(string->value());
}

void ConversationHandler::handle_unreport_command(const Variable::shared_ptr &request) {
  if (Value::EVT_STRING != request->value()->type()) {
    return;
  }

  const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
  log("INFO: Client asked for unreport of changing the variable \"%s\".", string->value().c_str());

  m_descriptor->msdp_remove_report_variable(string->value());
}

void ConversationHandler::handle_send_command(const Variable::shared_ptr &request) {
  if (Value::EVT_STRING != request->value()->type()) {
    return;
  }

  const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
  report(m_descriptor, string->value());
}

bool ConversationHandler::handle_request(const Variable::shared_ptr &request) {
  log("INFO: MSDP request %s.", request->name().c_str());

  Variable::shared_ptr response;
  if ("LIST" == request->name()) {
    handle_list_command(request, response);
  } else if ("REPORT" == request->name()) {
    handle_report_command(request);
  } else if ("UNREPORT" == request->name()) {
    handle_unreport_command(request);
  } else if ("SEND" == request->name()) {
    handle_send_command(request);
  }

  if (nullptr == response.get()) {
    return false;
  }

  const size_t buffer_size = WRAPPER_LENGTH + response->required_size();
  std::shared_ptr<char> buffer(new char[buffer_size], std::default_delete<char[]>());
  buffer.get()[0] = char(IAC);
  buffer.get()[1] = char(SB);
  buffer.get()[2] = constants::TELOPT_MSDP;
  response->serialize(HEAD_LENGTH + buffer.get(), buffer_size - WRAPPER_LENGTH);
  buffer.get()[buffer_size - 2] = char(IAC);
  buffer.get()[buffer_size - 1] = char(SE);

  hexdump(buffer.get(), buffer_size, "MSDP response:");

  int written = 0;
  write_to_descriptor_with_options(m_descriptor, buffer.get(), buffer_size, written);

  return true;
}

size_t handle_conversation(DESCRIPTOR_DATA *t, const char *buffer, const size_t length) {
  ConversationHandler handler(t);
  return handler(buffer, length);
}

void Report::operator()(DESCRIPTOR_DATA *d, const std::string &name) {
  ReportSender sender(d);

  const auto reporter = ReporterFactory::create(d, name);
  if (!reporter) {
    return;
  }

  sender.send(reporter);
}

Report report;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
