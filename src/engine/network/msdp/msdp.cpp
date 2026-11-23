#include "msdp.h"

#include "engine/entities/char_data.h"
#include "engine/network/telnet.h"
#include "msdp_parser.h"
#include "msdp_constants.h"
#include "msdp_senders.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace msdp {
namespace {
// Имя переменной приходит от клиента, и в лог его класть как есть нельзя: там может оказаться
// что угодно вплоть до управляющих байтов и килобайта мусора. Оставляем печатные знаки, остальное
// заменяем точкой, длину режем -- в лог нужно ровно столько, чтобы узнать имя.
std::string SafeVariableName(const std::string &name) {
	const static size_t kMaxLoggedLength = 32;

	std::string result;
	result.reserve(std::min(name.length(), kMaxLoggedLength));
	for (const auto c : name) {
		if (result.length() >= kMaxLoggedLength) {
			result += "...";
			break;
		}
		result += (isprint(static_cast<unsigned char>(c)) ? c : '.');
	}

	return result;
}
}   // namespace

class ReporterFactory {
 public:
	static AbstractReporter::shared_ptr create(const DescriptorData *descriptor, const std::string &name);
	const static Variable::shared_ptr &reportable_variables();

 private:
	using handler_t = std::function<AbstractReporter::shared_ptr(const DescriptorData *)>;
	using handlers_t = std::unordered_map<std::string, handler_t>;

	const static handlers_t &handlers();
};

AbstractReporter::shared_ptr ReporterFactory::create(const DescriptorData *descriptor, const std::string &name) {
	const auto reporter = handlers().find(name);
	if (reporter != handlers().end()) {
		return reporter->second(descriptor);
	}

	// Это не наша ошибка, а клиент, спросивший имя, которого мы не отдаем: свой список мы
	// публикуем в REPORTABLE_VARIABLES, но универсальные плагины спрашивают и вслепую. Раньше
	// строка шла как SYSERR и не называла имени -- по ней нельзя было ни понять, чего просят,
	// ни решить, стоит ли это имя заводить.
	log("WARNING: MSDP report requested for unknown variable '%s'.", SafeVariableName(name).c_str());
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
		std::make_shared<StringValue>(constants::GROUP),
		// Клиент, который читает этот список, увидит и принятые имена -- иначе он про них не
		// узнает и будет просить вслепую, как сейчас.
		std::make_shared<StringValue>(constants::HEALTH),
		std::make_shared<StringValue>(constants::HEALTH_MAX),
		std::make_shared<StringValue>(constants::MOVEMENT),
		std::make_shared<StringValue>(constants::MOVEMENT_MAX),
		std::make_shared<StringValue>(constants::MANA),
		std::make_shared<StringValue>(constants::MANA_MAX),
		std::make_shared<StringValue>(constants::CHARACTER_NAME),
		std::make_shared<StringValue>(constants::SERVER_ID),
		std::make_shared<StringValue>(constants::SERVER_TIME),
		std::make_shared<StringValue>(constants::WORLD_TIME),
		std::make_shared<StringValue>(constants::ALIGNMENT),
		std::make_shared<StringValue>(constants::EXPERIENCE_TNL),
		std::make_shared<StringValue>(constants::OPPONENT_NAME),
		std::make_shared<StringValue>(constants::OPPONENT_LEVEL),
		std::make_shared<StringValue>(constants::OPPONENT_HEALTH),
		std::make_shared<StringValue>(constants::OPPONENT_HEALTH_MAX)
	};

	const static Value::shared_ptr
		REPORTABLE_VARIABLES_VALUE = std::make_shared<ArrayValue>(REPORTABLE_VARIABLES_ARRAY);

	const static Variable::shared_ptr REPORTABLE_VARIABLES = std::make_shared<Variable>("REPORTABLE_VARIABLES",
																						REPORTABLE_VARIABLES_VALUE);

	return REPORTABLE_VARIABLES;
}

const ReporterFactory::handlers_t &ReporterFactory::handlers() {
	static const ReporterFactory::handlers_t s_handlers = {{
															   constants::ROOM,
															   std::bind(RoomReporter::create, std::placeholders::_1)
														   },
														   {constants::EXPERIENCE, std::bind(ExperienceReporter::create,
																							 std::placeholders::_1)},
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
															std::bind(GroupReporter::create, std::placeholders::_1)},
														   // Принятые в MSDP имена рядом с нашими
														   // историческими: по старым уже работают клиенты
														   // игроков, новые знает сторонний клиент.
														   {constants::HEALTH,
															std::bind(HealthReporter::create, std::placeholders::_1)},
														   {constants::HEALTH_MAX,
															std::bind(HealthMaxReporter::create, std::placeholders::_1)},
														   {constants::MOVEMENT,
															std::bind(MovementReporter::create, std::placeholders::_1)},
														   {constants::MOVEMENT_MAX,
															std::bind(MovementMaxReporter::create, std::placeholders::_1)},
														   {constants::MANA,
															std::bind(ManaReporter::create, std::placeholders::_1)},
														   {constants::MANA_MAX,
															std::bind(ManaMaxReporter::create, std::placeholders::_1)},
														   {constants::CHARACTER_NAME,
															std::bind(CharacterNameReporter::create, std::placeholders::_1)},
														   {constants::SERVER_ID,
															std::bind(ServerIdReporter::create, std::placeholders::_1)},
														   {constants::SERVER_TIME,
															std::bind(ServerTimeReporter::create, std::placeholders::_1)},
														   {constants::WORLD_TIME,
															std::bind(WorldTimeReporter::create, std::placeholders::_1)},
														   {constants::ALIGNMENT,
															std::bind(AlignmentReporter::create, std::placeholders::_1)},
														   {constants::EXPERIENCE_TNL,
															std::bind(ExperienceTnlReporter::create, std::placeholders::_1)},
														   {constants::OPPONENT_NAME,
															std::bind(OpponentNameReporter::create, std::placeholders::_1)},
														   {constants::OPPONENT_LEVEL,
															std::bind(OpponentLevelReporter::create, std::placeholders::_1)},
														   {constants::OPPONENT_HEALTH,
															std::bind(OpponentHealthReporter::create, std::placeholders::_1)},
														   {constants::OPPONENT_HEALTH_MAX,
															std::bind(OpponentHealthMaxReporter::create, std::placeholders::_1)}};

	return s_handlers;
}

class ConversationHandler {
 public:
	ConversationHandler(DescriptorData *descriptor) : m_descriptor(descriptor) {}
	size_t operator()(const char *buffer, const size_t length);

 private:
	const static ArrayValue::array_t SUPPORTED_COMMANDS_LIST;
	const static Value::shared_ptr SUPPORTED_COMMANDS_ARRAY;

	void handle_list_command(const Variable::shared_ptr &request, Variable::shared_ptr &response);
	void handle_report_command(const Variable::shared_ptr &request);
	void handle_unreport_command(const Variable::shared_ptr &request);
	void handle_send_command(const Variable::shared_ptr &request);
	bool handle_request(const Variable::shared_ptr &request);

	DescriptorData *m_descriptor;
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
		response.reset(new Variable("COMMANDS", SUPPORTED_COMMANDS_ARRAY));
	} else if ("REPORTABLE_VARIABLES" == string->value()) {
		response = ReporterFactory::reportable_variables();
	} else if ("CONFIGURABLE_VARIABLES" == string->value()) {
		response = std::make_shared<Variable>("CONFIGURABLE_VARIABLES", std::make_shared<ArrayValue>());
	}
}

void ConversationHandler::handle_report_command(const Variable::shared_ptr &request) {
	if (Value::EVT_STRING != request->value()->type()) {
		return;
	}

	const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
	m_descriptor->msdp_add_report_variable(string->value());
}

void ConversationHandler::handle_unreport_command(const Variable::shared_ptr &request) {
	if (Value::EVT_STRING != request->value()->type()) {
		return;
	}

	const auto string = std::dynamic_pointer_cast<StringValue>(request->value());
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
	iosystem::write_to_descriptor_with_options(m_descriptor, buffer.get(), buffer_size, written);

	return true;
}

size_t handle_conversation(DescriptorData *t, const char *buffer, const size_t length) {
	ConversationHandler handler(t);
	return handler(buffer, length);
}

void Report::operator()(DescriptorData *d, const std::string &name) {
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
