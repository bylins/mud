#ifndef __MSDP_PARSER_HPP__
#define __MSDP_PARSER_HPP__

#include "sysdep.h"

#include <memory>
#include <string>
#include <list>

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

	void debug(const bool on);
	void log(const char* format, ...) __attribute__((format(printf, 1, 2)));
	void debug_log(const char* format, ...) __attribute__((format(printf, 1, 2)));
	void hexdump(const char* ptr, const size_t buflen, const char* title = nullptr);

	inline std::string indent(size_t level) { return std::string(level, '\t'); }

	class CSerializeable
	{
	public:
		virtual size_t required_size() const = 0;
		virtual size_t serialize(char* buffer, size_t size) const = 0;
		virtual void dump(const size_t level = 0) const = 0;
	};

	class CValue: public CSerializeable
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

	class CVariable: public CSerializeable
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
		virtual void dump(const size_t level = 0) const override;

	private:
		std::string m_name;
		const value_ptr_t m_value;
		const size_t m_size;
	};

	class CStringValue: public CValue
	{
	public:
		/**
		** \note Seychas nam ne nuzhno ekranirovat' znachenie: ono nechayanno ekraniruetsya pri convertirovanii.
		** Eto esteticheski ne ochen' horosho, no menyat' seichas ne hochu.
		**/
		CStringValue(const std::string& value) : m_value(value) {}
		virtual EValueType type() const override { return EVT_STRING; }
		virtual size_t required_size() const override { return 1 + m_value.size(); }
		virtual size_t serialize(char* buffer, size_t size) const override;
		const std::string& value() const { return m_value; }
		virtual void dump(const size_t level = 0) const override;

	private:
		static const std::string& escape(const std::string& value);

		std::string m_value;
	};

	class CTableValue: public CValue
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
		virtual void dump(const size_t level = 0) const override;

	private:
		table_t m_data;
		size_t m_size;
	};

	class CArrayValue: public CValue
	{
	public:
		using array_t = std::list<CValue*>;

		CArrayValue() : m_size(3) {}	// 3 - MSDP_VAL MSDP_ARRAY_OPEN ... MSDP_ARRAY_CLOSE
		CArrayValue(const array_t& array);
		virtual EValueType type() const override { return EVT_TABLE; }
		void add(const value_ptr_t& value) { m_data.push_back(value); }
		virtual size_t required_size() const override { return m_size; }
		virtual size_t serialize(char* buffer, size_t size) const override;
		virtual void dump(const size_t level = 0) const override;

	private:
		std::list<value_ptr_t> m_data;
		size_t m_size;
	};

	using parsed_request_t = std::shared_ptr<CVariable>;
	bool parse_request(const char* buffer, const size_t length, size_t& conversation_length, parsed_request_t& request, const bool tail_required);
	inline bool parse_request(const char* buffer, const size_t length, size_t& conversation_length, parsed_request_t& request) { return parse_request(buffer, length, conversation_length, request, true); }
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :