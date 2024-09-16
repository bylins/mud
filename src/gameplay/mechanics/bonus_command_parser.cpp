#include "bonus_command_parser.h"

#include "engine/ui/interpreter.h"
#include "utils/utils.h"

namespace Bonus {
const char *USAGE_MESSAGE = "Синтаксис команды:\r\nбонус <двойной|тройной|отменить> [оружейный|опыт|урон|обучение] [время]";
class StringStreamFinalizer {
 public:
	explicit StringStreamFinalizer(std::string &destination) : m_destination(destination) {}
	~StringStreamFinalizer() { m_destination = m_stream.str(); }

	template<typename Operand>
	StringStreamFinalizer &operator<<(const Operand &operand);

 private:
	std::stringstream m_stream;
	std::string &m_destination;
};

template<typename Operand>
StringStreamFinalizer &StringStreamFinalizer::operator<<(const Operand &operand) {
	m_stream << operand;
	return *this;
}

ArgumentsParser::ArgumentsParser(const char *arguments, EBonusType type, int duration) :
	m_result(ER_ERROR),
	m_bonus_time(duration),
	m_bonus_multiplier(1),
	m_bonus_type(type) {
	parse_arguments(arguments);
}

void ArgumentsParser::parse() {
	std::stringstream out;
	out << "*** Объявляется ";

	StringStreamFinalizer error_message(m_error_message);
	if (!isname(m_first_argument, "двойной тройной отменить")) {
		error_message << USAGE_MESSAGE << "\r\n";
		return;
	}

	StringStreamFinalizer broadcast_message(m_broadcast_message);
	if (utils::IsAbbr(m_first_argument.c_str(), "отменить")) {
		broadcast_message << "Бонус был отменен.\r\n";
		m_result = ER_STOP;
		return;
	}

	if (m_first_argument.empty()) {
		error_message << USAGE_MESSAGE << "\r\n";
		return;
	}

	if (!m_second_argument.empty()
		&& !isname(m_second_argument, "оружейный опыт урон обучение")) {
		error_message << "Тип бонуса может быть &Wоружейный&n, &Wопыт&n, &Wурон&n или &Wобучение&n.\r\n";
		return;
	}

	if (!m_arguments_remainder.empty()) {
		m_bonus_time = atol(m_arguments_remainder.c_str());
	}

	if (m_bonus_time < 1
		|| m_bonus_time > 60) {
		error_message << "Возможный временной интервал: от 1 до 60 игровых часов.\r\n";
		return;
	}

	if (utils::IsAbbr(m_first_argument.c_str(), "двойной")) {
		out << "двойной бонус";
		m_bonus_multiplier = 2;
	} else if (utils::IsAbbr(m_first_argument.c_str(), "тройной")) {
		out << "тройной бонус";
		m_bonus_multiplier = 3;
	} else {
		// logic error.
	}

	if (utils::IsAbbr(m_second_argument.c_str(), "оружейный")) {
		out << " оружейного опыта";
		m_bonus_type = Bonus::EBonusType::BONUS_WEAPON_EXP;
	} else if (utils::IsAbbr(m_second_argument.c_str(), "опыт")) {
		out << " опыта";
		m_bonus_type = Bonus::EBonusType::BONUS_EXP;
	} else if (utils::IsAbbr(m_second_argument.c_str(), "урон")) {
		out << " увеличенного урона";
		m_bonus_type = Bonus::EBonusType::BONUS_DAMAGE;
	} else if (utils::IsAbbr(m_second_argument.c_str(), "обучение")) {
		out << " ускоренного обучения";
		m_bonus_type = Bonus::EBonusType::BONUS_LEARNING;
	} else {
		// logic error.
	}

	out << " на " << m_bonus_time << " часов. ***";
	m_result = ER_START;
	broadcast_message << "&W" << out.str() << "&n\r\n";
}

void ArgumentsParser::parse_arguments(const char *argument) {
	char buffer[kMaxStringLength];
	argument = one_argument(argument, buffer);
	m_first_argument = buffer;
	argument = one_argument(argument, buffer);
	m_second_argument = buffer;
	m_arguments_remainder = argument;
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
