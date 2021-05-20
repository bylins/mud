#ifndef __BONUS_BOMMAND_PARSER_HPP__
#define __BONUS_BOMMAND_PARSER_HPP__

#include "bonus.types.hpp"

#include <string>

namespace Bonus
{
	class ArgumentsParser
	{
	public:
		enum EResult
		{
			ER_ERROR,
			ER_START,
			ER_STOP
		};

		ArgumentsParser(const char* arguments, EBonusType type, int duration);

		void parse();

		const auto& error_message() const { return m_error_message; }
		const auto& broadcast_message() const { return m_broadcast_message; }
		const auto result() const { return m_result; }

		const auto time() const { return m_bonus_time; }
		const auto multiplier() const { return m_bonus_multiplier; }
		const auto type() const { return m_bonus_type; }

	private:
		void parse_arguments(const char* argument);

		EResult m_result;

		std::string m_first_argument;
		std::string m_second_argument;
		std::string m_arguments_remainder;

		std::string m_error_message;
		std::string m_broadcast_message;

		long m_bonus_time;
		int m_bonus_multiplier;
		EBonusType m_bonus_type;
	};
}

#endif // __BONUS_BOMMAND_PARSER_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
