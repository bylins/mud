#include "msdp_parser.hpp"

#include "config.hpp"
#include "telnet.h"
#include "utils.h"

#include <boost/algorithm/string/classification.hpp>

#include <stdarg.h>

namespace msdp
{
	bool debug_flag = false;
	void debug(const bool on)
	{
		debug_flag = on;
	}

	void log(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		vlog(MSDP_LOG, format, args);
		va_end(args);
	}

	void debug_log(const char* format, ...)
	{
		if (debug_flag)
		{
			va_list args;
			va_start(args, format);
			vlog(MSDP_LOG, format, args);
			va_end(args);
		}
	}

	void hexdump(const char* ptr, const size_t buflen, const char* title)
	{
		if (debug_flag)
		{
			hexdump(MSDP_LOG, ptr, buflen, title);
		}
	}

	size_t CVariable::serialize(char* buffer, size_t size) const
	{
		if (size < required_size())
		{
			return 0;
		}

		*buffer = char(MSDP_VAR);
		memcpy(static_cast<void*>(buffer + 1), m_name.c_str(), m_name.size());
		buffer += 1 + m_name.size();
		m_value->serialize(buffer, size - 1 - m_name.size());
		return required_size();
	}

	void CVariable::dump(const size_t level /*= 0*/) const
	{
		const std::string i = indent(level);
		debug_log("%s(Variable) %s", i.c_str(), m_name.c_str());
		m_value->dump(1 + level);
	}

	size_t CStringValue::serialize(char* buffer, size_t buffer_size) const
	{
		if (buffer_size < required_size())
		{
			return 0;
		}

		buffer[0] = MSDP_VAL;
		memcpy(static_cast<void*>(1 + buffer), m_value.c_str(), required_size() - 1);
		return required_size();
	}

	void CStringValue::dump(const size_t level /*= 0*/) const
	{
		debug_log("%s(String) %s", indent(level).c_str(), m_value.c_str());
	}

	const std::string& CStringValue::escape(const std::string& value)
	{
		static std::string result;

		result.resize(2 * value.size(), '\0');

		size_t j = 0;
		for (size_t i = 0; i < value.size(); ++i)
		{
			result[j++] = value[i];
			if (char(IAC) == value[i])
			{
				result[j++] = value[i];	// double IAC
			}
		}
		result.resize(j);

		return result;
	}

	CTableValue::CTableValue(const table_t& table) : m_size(3)
	{
		for (const auto& t : table)
		{
			m_size += t->required_size();
		}
		m_data = table;
	}

	void CTableValue::add(const CVariable::variable_ptr_t& variable)
	{
		m_data.push_back(variable);
		m_size += variable->required_size();
	}

	size_t CTableValue::serialize(char* buffer, size_t size) const
	{
		if (size < required_size())
		{
			return 0;
		}

		buffer[0] = MSDP_VAL;
		buffer[1] = MSDP_TABLE_OPEN;
		buffer += 2;

		for (const auto& i : m_data)
		{
			i->serialize(buffer, size);
			size -= i->required_size();
			buffer += i->required_size();
		}
		buffer[0] = MSDP_TABLE_CLOSE;

		return required_size();
	}

	void CTableValue::dump(const size_t level /*= 0*/) const
	{
		debug_log("%s(Table)", indent(level).c_str());
		for (const auto& i : m_data)
		{
			i->dump(1 + level);
		}
	}

	CArrayValue::CArrayValue(const array_t& array) : m_size(3)
	{
		for (const auto& i : array)
		{
			m_size += i->required_size();
			m_data.push_back(CValue::value_ptr_t(i));
		}
	}

	size_t CArrayValue::serialize(char* buffer, size_t size) const
	{
		if (size < required_size())
		{
			return 0;
		}

		buffer[0] = MSDP_VAL;
		buffer[1] = MSDP_ARRAY_OPEN;
		buffer += 2;

		for (const auto& i : m_data)
		{
			i->serialize(buffer, size);
			size -= i->required_size();
			buffer += i->required_size();
		}

		buffer[0] = MSDP_ARRAY_CLOSE;

		return required_size();
	}

	void CArrayValue::dump(const size_t level /*= 0*/) const
	{
		debug_log("%s(Table)\n", indent(level).c_str());
		for (const auto& i : m_data)
		{
			i->dump(1 + level);
		}
	}

	inline bool is_stop_byte(const char c)
	{
		return c == char(IAC) || c == MSDP_ARRAY_CLOSE || c == MSDP_TABLE_CLOSE;
	}

	size_t parse_value(const char* buffer, size_t length, CValue::value_ptr_t& value);
	size_t parse_variable(const char* buffer, const size_t length, CVariable::variable_ptr_t& result);

	size_t parse_table(const char* buffer, size_t length, CValue::value_ptr_t& value)
	{
		if (0 == length)
		{
			return 0;
		}

		if (MSDP_TABLE_OPEN != *buffer)
		{
			return 0;
		}

		CTableValue* table = new CTableValue();
		CValue::value_ptr_t result(table);

		auto pos = buffer + 1;
		auto end = buffer + length;
		while (end != pos
			&& MSDP_TABLE_CLOSE != *pos)
		{
			CVariable::variable_ptr_t v = nullptr;
			auto item_length = parse_variable(pos, end - pos, v);
			if (0 == item_length)
			{
				return 0;
			}
			table->add(v);
			pos += item_length;
		}

		if (end == pos)
		{
			return 0;
		}

		value = result;
		return end - pos;
	}

	size_t parse_array(const char* buffer, size_t length, CValue::value_ptr_t& value)
	{
		if (0 == length)
		{
			return 0;
		}

		if (MSDP_ARRAY_OPEN != *buffer)
		{
			return 0;
		}

		CArrayValue* array = new CArrayValue();
		CValue::value_ptr_t result(array);

		auto pos = buffer + 1;
		auto end = buffer + length;
		while (end != pos
			&& MSDP_ARRAY_CLOSE != *pos)
		{
			CValue::value_ptr_t v = nullptr;
			auto item_length = parse_value(pos, end - pos, v);
			if (0 == item_length)
			{
				return 0;
			}
			array->add(v);
			pos += item_length;
		}

		if (end == pos)
		{
			return 0;
		}

		value = result;
		return end - pos;
	}

	size_t parse_string(const char* buffer, size_t length, CValue::value_ptr_t& value)
	{
		auto pos = buffer;
		auto end = buffer + length;
		while (end != pos)
		{
			if (char(IAC) == *pos
				&& end != 1 + pos
				&& char(IAC) == *(1 + pos))
			{
				pos += 2;	// skip double IAC and continue as usual
				continue;
			}

			if (is_stop_byte(*pos))
			{
				break;
			}

			pos++;
		}

		const auto value_length = pos - buffer;
		value.reset(new CStringValue(std::string(buffer, value_length)));
		return value_length;
	}

	size_t parse_value(const char* buffer, size_t length, CValue::value_ptr_t& value)
	{
		if (3 > length)	// at least one byte for the MSDP_VAL and at least one byte for end-of-value marker
		{
			return 0;
		}

		if (MSDP_VAL != *buffer)
		{
			return 0;
		}

		auto pos = 1 + buffer;
		auto end = buffer + length;

		size_t value_length = 0;
		switch (*pos)
		{
		case MSDP_ARRAY_OPEN:
			value_length = parse_array(pos, end - pos, value);
			break;

		case MSDP_TABLE_OPEN:
			value_length = parse_table(pos, end - pos, value);
			break;

		default:
			value_length = parse_string(pos, end - pos, value);
		}

		return value_length;
	}

	size_t parse_variable(const char* buffer, const size_t length, CVariable::variable_ptr_t& result)
	{
		if (0 == length)
		{
			return 0;
		}

		if (MSDP_VAR != *buffer)
		{
			return 0;
		}

		const size_t MSDP_VAR_LENGTH = 1;	// actually, length of byte MSDP_VAR: always 1.
		const auto end = buffer + length;
		auto name_begin = MSDP_VAR_LENGTH + buffer;
		auto name_end = name_begin;

		while (name_end != end && MSDP_VAL != *name_end)
		{
			++name_end;
		}
		if (end == name_end)
		{
			return 0;
		}
		std::string name(name_begin, name_end - name_begin);

		CVariable::value_ptr_t value = nullptr;
		size_t value_length = parse_value(name_end, end - name_end, value);
		if (0 == value_length)
		{
			return 0;
		}

		result.reset(new CVariable(name, value));

		return MSDP_VAR_LENGTH + value_length + (name_end - buffer);
	}

	bool parse_request(const char* buffer, const size_t length, size_t& conversation_length, parsed_request_t& request, const bool tail_required)
	{
		size_t var_length = parse_variable(buffer, length, request);

		// check result and remaining space
		const size_t actual_length = (tail_required ? TAIL_LENGTH : 0) + var_length;
		if (0 == var_length
			|| actual_length > length)
		{
			return false;
		}

		auto pos = buffer + var_length;
		// check tail
		if (tail_required
			&& (char(IAC) != pos[0]
				|| char(SE) != pos[1]))
		{
			return false;
		}

		conversation_length = actual_length;
		return true;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :