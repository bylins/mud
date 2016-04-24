#include "msdp.hpp"

#include "utils.h"
#include "structs.h"
#include "telnet.h"

#include <string>
#include <memory>
#include <deque>

#include <boost\algorithm\string\classification.hpp>

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

	class CSerializeable
	{
	public:
		virtual size_t required_size() const = 0;
		virtual size_t serialize(char* buffer, size_t size) const = 0;
	};

	class CValue : public CSerializeable
	{
	public:
		using value_ptr_t = std::shared_ptr<CValue>;

		enum EValueType
		{
			EVT_UNDEFINED,
			EVT_STRING,
			EVT_TABLE,
			EVT_ARRAY
		};

		virtual EValueType type() const = 0;
	};

	class CVariable : public CSerializeable
	{
	public:
		using value_ptr_t = CValue::value_ptr_t;
		using variable_ptr_t = std::shared_ptr<CVariable>;

		CVariable(const std::string& name, const value_ptr_t value) : m_name(name), m_value(value), m_size(1 + m_name.size() + 1 + m_value->required_size()) {}
		const std::string& name() const { return m_name; }
		value_ptr_t value() const { return m_value; }
		virtual size_t required_size() const override { return m_size; }
		virtual size_t serialize(char* buffer, size_t size) const override;

	private:
		std::string m_name;
		const value_ptr_t m_value;
		const size_t m_size;
	};

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

	class CStringValue : public CValue
	{
	public:
		CStringValue(const std::string& value) : m_value(value) {}
		virtual EValueType type() const override { return EVT_STRING; }
		virtual size_t required_size() const override { return m_value.size(); }
		virtual size_t serialize(char* buffer, size_t size) const override;

	private:
		const std::string m_value;
	};

	size_t CStringValue::serialize(char* buffer, size_t buffer_size) const
	{
		if (buffer_size < required_size())
		{
			return 0;
		}

		memcpy(static_cast<void*>(buffer), m_value.c_str(), required_size());
		return required_size();
	}

	class CTableValue : public CValue
	{
	public:
		CTableValue() : m_size(0) {}
		virtual EValueType type() const override { return EVT_TABLE; }
		void add(const CVariable::variable_ptr_t& variable);
		virtual size_t required_size() const override { return m_size; }
		virtual size_t serialize(char* buffer, size_t size) const override;

	private:
		std::list<CVariable::variable_ptr_t> m_data;
		size_t m_size;
	};

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

		for (const auto& i : m_data)
		{
			i->serialize(buffer, size);
			size -= i->required_size();
			buffer += i->required_size();
		}

		return required_size();
	}

	class CArrayValue : public CValue
	{
	public:
		CArrayValue() : m_size(0) {}
		virtual EValueType type() const override { return EVT_TABLE; }
		void add(const value_ptr_t& value) { m_data.push_back(value); }
		virtual size_t required_size() const override { return m_size; }
		virtual size_t serialize(char* buffer, size_t size) const override;

	private:
		std::list<value_ptr_t> m_data;
		size_t m_size;
	};

	size_t CArrayValue::serialize(char* buffer, size_t size) const
	{
		if (size < required_size())
		{
			return 0;
		}

		for (const auto& i : m_data)
		{
			i->serialize(buffer, size);
			size -= i->required_size();
			buffer += i->required_size();
		}

		return required_size();
	}

	inline bool is_stop_byte(const char c)
	{
		return c == IAC || c == MSDP_ARRAY_CLOSE || c == MSDP_TABLE_CLOSE;
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
		while (end != pos
			&& !is_stop_byte(*pos))
		{
			pos++;
		}

		if (end == pos)
		{
			return 0;
		}

		const auto value_length = buffer - pos;
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
			parse_string(pos, end - pos, value);
		}

		return false;
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

		const auto end = buffer + length;
		auto name_end = buffer;

		while (name_end != end && MSDP_VAL != *name_end)
		{
			++name_end;
		}
		if (end == name_end)
		{
			return 0;
		}
		std::string name(buffer, name_end - buffer);

		CVariable::value_ptr_t value = nullptr;
		size_t value_length = parse_value(name_end, end - name_end, value);
		if (0 == value_length)
		{
			return 0;
		}

		result.reset(new CVariable(name, value));

		return buffer - (name_end + value_length);
	}

	bool parse_request(const char* buffer, const size_t length, size_t& conversation_length, std::shared_ptr<CVariable>& request)
	{
		size_t var_length = parse_variable(buffer, length, request);

		// check result and remaining space
		if (0 == conversation_length
			|| 2 + conversation_length > length)
		{
			return false;
		}

		auto pos = buffer + conversation_length;
		// check tail
		if (IAC != pos[0]
			|| SE != pos[1])
		{
			return false;
		}

		conversation_length = var_length + 2;
		return true;
	}

	bool handle_request(DESCRIPTOR_DATA* t, const std::shared_ptr<CVariable> request)
	{
		log("MSDP request ignored.");

		return true;
	}

	size_t handle_conversation(DESCRIPTOR_DATA* t, const char* buffer, const size_t length)
	{
		size_t actual_length = 0;
		std::shared_ptr<CVariable> request;
		if (3 > length)
		{
			log("WARNING: Logic error: MSDP block is too small.\n");
			return 0;
		}

		if (!parse_request(buffer + 2, length - 2, actual_length, request))
		{
			log("WARNING: Could not parse MSDP request.\n");
			return 0;
		}

		if (!handle_request(t, request))
		{
			log("WARNING: failed to handle MSDP request.\n");
			return 0;
		}

		return actual_length;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
