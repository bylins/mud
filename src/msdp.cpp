#include "msdp.hpp"

#include "utils.h"
#include "structs.h"
#include "telnet.h"

#include <string>
#include <memory>

class CValue
{
};

class CStringValue: public CValue
{
public:
};

class CVariable
{
public:
	using value_ptr_t = std::shared_ptr<CValue>;

	CVariable(const std::string& name, const value_ptr_t value) : m_name(name), m_value(value) {}
	value_ptr_t value() const { return m_value; }

private:
	std::string m_name;
	const std::shared_ptr<CValue> m_value;
};

namespace msdp
{
	/** MSDP protocol related constants @{ */
	const char MSDP_VAR = 1;
	const char MSDP_VAL = 2;

	const char MSDP_TABLE_OPEN = 3;
	const char MSDP_TABLE_CLOSE = 4;

	const char MSDP_ARRAY_OPEN = 5;
	const char MSDP_ARRAY_CLOSE = 6;
	/** @} */

	bool parse_request(const char* buffer, const size_t length, size_t& conversation_length)
	{
		auto pos = buffer;
		const auto end = buffer + length;

		switch (*pos)
		{
		case MSDP_VAR:
			{
				auto var_end = pos;
				while (var_end != end && MSDP_VAL != *var_end)
				{
					++var_end;
				}
				if (end == var_end)
				{
					return false;
				}
				std::string name(pos, var_end - pos);
			}
			break;

		default:
			return false;
		}

		conversation_length = buffer - pos;
		return true;
	}

	size_t handle_conversation(DESCRIPTOR_DATA* t, const char* buffer, const size_t length)
	{
		size_t actual_length = 0;
		if (!parse_request(buffer + 2, length - 2, actual_length))
		{
			log("WARNING: Could not parse MSDP request.\n");
			return 0;
		}

		return actual_length;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
