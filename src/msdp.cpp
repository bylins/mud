#include "msdp.hpp"

#include "char.hpp"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "utils.h"
#include "structs.h"
#include "telnet.h"

#include <string>
#include <memory>
#include <deque>

#include <boost/algorithm/string/classification.hpp>

namespace msdp
{
	/** MSDP protocol related constants @{ */
	const char MSDP_VAR = 1;
	const char MSDP_VAL = 2;

	const char MSDP_TABLE_OPEN = 3;
	const char MSDP_TABLE_CLOSE = 4;

	const char MSDP_ARRAY_OPEN = 5;
	const char MSDP_ARRAY_CLOSE = 6;

	const size_t HEAD_LENGTH = 3;	// IAC SB TELOPT_MSDP
	const size_t TAIL_LENGTH = 2;	// IAC SE
	const size_t WRAPPER_LENGTH = HEAD_LENGTH + TAIL_LENGTH;
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

		virtual ~CValue() {}
		virtual EValueType type() const = 0;
	};

	class CVariable : public CSerializeable
	{
	public:
		using value_ptr_t = CValue::value_ptr_t;
		using variable_ptr_t = std::shared_ptr<CVariable>;

		CVariable(const std::string& name, const CValue::value_ptr_t& value) : m_name(name), m_value(value), m_size(1 + m_name.size() + m_value->required_size()) {}
		CVariable(const std::string& name, CValue* value) : m_name(name), m_value(value), m_size(1 + m_name.size() + m_value->required_size()) {}
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
		virtual size_t required_size() const override { return 1 + m_value.size(); }
		virtual size_t serialize(char* buffer, size_t size) const override;
		const std::string& value() const { return m_value; }

	private:
		const std::string m_value;
	};

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

	class CTableValue : public CValue
	{
	public:
		using table_t = std::list<CVariable::variable_ptr_t>;
		CTableValue() : m_size(3) {}	// 3 - MSDP_VAL MSDP_TABLE_OPEN ... MSDP_TABLE_CLOSE
		CTableValue(const table_t& table);
		virtual EValueType type() const override { return EVT_TABLE; }
		void add(CVariable* variable) { add(CVariable::variable_ptr_t(variable)); }
		void add(const CVariable::variable_ptr_t& variable);
		virtual size_t required_size() const override { return m_size; }
		virtual size_t serialize(char* buffer, size_t size) const override;

	private:
		table_t m_data;
		size_t m_size;
	};

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

	class CArrayValue : public CValue
	{
	public:
		using array_t = std::list<CValue*>;

		CArrayValue() : m_size(3) {}	// 3 - MSDP_VAL MSDP_ARRAY_OPEN ... MSDP_ARRAY_CLOSE
		CArrayValue(const array_t& array);
		virtual EValueType type() const override { return EVT_TABLE; }
		void add(const value_ptr_t& value) { m_data.push_back(value); }
		virtual size_t required_size() const override { return m_size; }
		virtual size_t serialize(char* buffer, size_t size) const override;

	private:
		std::list<value_ptr_t> m_data;
		size_t m_size;
	};

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
		while (end != pos
			&& !is_stop_byte(*pos))
		{
			pos++;
		}

		if (end == pos)
		{
			return 0;
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

	bool parse_request(const char* buffer, const size_t length, size_t& conversation_length, std::shared_ptr<CVariable>& request)
	{
		size_t var_length = parse_variable(buffer, length, request);

		// check result and remaining space
		if (0 == var_length
			|| TAIL_LENGTH + var_length > length)
		{
			return false;
		}

		auto pos = buffer + var_length;
		// check tail
		if (char(IAC) != pos[0]
			|| char(SE) != pos[1])
		{
			return false;
		}

		conversation_length = var_length + TAIL_LENGTH;
		return true;
	}

	bool handle_request(DESCRIPTOR_DATA* t, std::shared_ptr<CVariable> request)
	{
		log("INFO: MSDP request %s.", request->name().c_str());

		CVariable::variable_ptr_t response;
		if ("LIST" == request->name())
		{
			if (CValue::EVT_STRING == request->value()->type())
			{
				CStringValue* string = dynamic_cast<CStringValue*>(request->value().get());
				if ("COMMANDS" == string->value())
				{
					log("INFO: Client asked for MSDP \"COMMANDS\" list.");

					response.reset(new CVariable("COMMANDS",
						new CArrayValue({
						new CStringValue("LIST"),
						new CStringValue("REPORT"),
						new CStringValue("SEND")
					})));
				}
				else if ("REPORTABLE_VARIABLES" == string->value())
				{
					log("INFO: Client asked for MSDP \"REPORTABLE_VARIABLES\" list.");

					response.reset(new CVariable("REPORTABLE_VARIABLES",
						new CArrayValue({
						new CStringValue("ROOM")
					})));
				}
				else if ("CONFIGURABLE_VARIABLES" == string->value())
				{
					log("INFO: Client asked for MSDP \"CONFIGURABLE_VARIABLES\" list.");

					response.reset(new CVariable("CONFIGURABLE_VARIABLES", new CArrayValue()));
				}
				else
				{
					log("INFO: Client asked for unknown MSDP list \"%s\".", string->value().c_str());
				}
			}
		}
		else if ("REPORT" == request->name())
		{
			if (CValue::EVT_STRING == request->value()->type())
			{
				CStringValue* string = dynamic_cast<CStringValue*>(request->value().get());
				log("INFO: Client asked for report of changing the variable \"%s\".", string->value().c_str());

				t->msdp_add_report_variable(string->value());
			}
		}
		else if ("UNREPORT" == request->name())
		{
			if (CValue::EVT_STRING == request->value()->type())
			{
				CStringValue* string = dynamic_cast<CStringValue*>(request->value().get());
				log("INFO: Client asked for unreport of changing the variable \"%s\".", string->value().c_str());

				t->msdp_remove_report_variable(string->value());
			}
		}

		if (nullptr == response.get())
		{
			return false;
		}

		const size_t buffer_size = WRAPPER_LENGTH + response->required_size();
		std::shared_ptr<char> buffer(new char[1 + buffer_size]);	// 1 byte for NULL terminator
		buffer.get()[0] = char(IAC);
		buffer.get()[1] = char(SB);
		buffer.get()[2] = TELOPT_MSDP;
		response->serialize(HEAD_LENGTH + buffer.get(), buffer_size - WRAPPER_LENGTH);
		buffer.get()[buffer_size - 2] = char(IAC);
		buffer.get()[buffer_size - 1] = char(SE);
		buffer.get()[buffer_size] = '\0';

		int written = 0;
		write_to_descriptor_with_options(t, buffer.get(), 1 + buffer_size, written);	// +1 - including NULL terminator

		return true;
	}

	size_t handle_conversation(DESCRIPTOR_DATA* t, const char* buffer, const size_t length)
	{
		size_t actual_length = 0;
		std::shared_ptr<CVariable> request;
		if (4 > length)
		{
			log("WARNING: Logic error: MSDP block is too small.");
			return 0;
		}

		if (!parse_request(buffer + HEAD_LENGTH, length - HEAD_LENGTH, actual_length, request))
		{
			log("WARNING: Could not parse MSDP request.");
			return 0;
		}

		handle_request(t, request);

		return HEAD_LENGTH + actual_length;
	}

	void report_room(DESCRIPTOR_DATA* d, CVariable::variable_ptr_t& response)
	{
		const auto rnum = IN_ROOM(d->character);
		const auto vnum = GET_ROOM_VNUM(rnum);
		if (NOWHERE == vnum)
		{
			return;
		}

		CTableValue* room_descriptor = new CTableValue();

		CTableValue* exits = new CTableValue();
		const auto directions = world[rnum]->dir_option;
		for (int i = 0; i < NUM_OF_DIRS; ++i)
		{
			if (directions[i]
				&& !EXIT_FLAGGED(directions[i], EX_HIDDEN))
			{
				const static std::string direction_commands[NUM_OF_DIRS] = { "n", "e", "s", "w", "u", "d" };
				const auto to_rnum = directions[i]->to_room;
				const auto to_vnum = GET_ROOM_VNUM(to_rnum);
				exits->add(new CVariable(direction_commands[i],
					new CStringValue(std::to_string(to_vnum))));
			}
		}

		/* convert string date into client's encoding */
		// output might be more than input up to 4 times (in case of utf-8) plus NULL terminator.
		std::shared_ptr<char> room_name(new char[1 + 4*strlen(world[rnum]->name)]);
		d->string_to_client_encoding(world[rnum]->name, room_name.get());

		// output might be more than input up to 4 times (in case of utf-8) plus NULL terminator.
		std::shared_ptr<char> zone_name(new char[4 * strlen(zone_table[world[rnum]->zone].name)]);
		d->string_to_client_encoding(zone_table[world[rnum]->zone].name, zone_name.get());

		room_descriptor->add(new CVariable("VNUM",
			new CStringValue(std::to_string(vnum))));
		room_descriptor->add(new CVariable("NAME",
			new CStringValue(room_name.get())));
		room_descriptor->add(new CVariable("AREA",
			new CStringValue(zone_name.get())));
		room_descriptor->add(new CVariable("ZONE",
			new CStringValue(std::to_string(vnum / 100))));
		room_descriptor->add(new CVariable("EXITS", exits));

		response.reset(new CVariable("ROOM", room_descriptor));
	}

	void msdp_report(DESCRIPTOR_DATA* d, const std::string& name)
	{
		//report
		if (!d->character)
		{
			return;
		}

		CVariable::variable_ptr_t response;
		if ("ROOM" == name)
		{
			report_room(d, response);
		}

		if (nullptr == response.get())
		{
			return;
		}

		const size_t buffer_size = WRAPPER_LENGTH + response->required_size();
		std::shared_ptr<char> buffer(new char[1 + buffer_size]);	// 1 byte for NULL terminator
		buffer.get()[0] = char(IAC);
		buffer.get()[1] = char(SB);
		buffer.get()[2] = TELOPT_MSDP;
		response->serialize(HEAD_LENGTH + buffer.get(), buffer_size - WRAPPER_LENGTH);
		buffer.get()[buffer_size - 2] = char(IAC);
		buffer.get()[buffer_size - 1] = char(SE);
		buffer.get()[buffer_size] = '\0';

		int written = 0;
		write_to_descriptor_with_options(d, buffer.get(), 1 + buffer_size, written);	// +1 - including NULL terminator
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
