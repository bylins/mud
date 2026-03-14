#ifndef __MSDP_PARSER_HPP__
#define __MSDP_PARSER_HPP__

#include "engine/core/sysdep.h"

#include <memory>
#include <string>
#include <list>

namespace msdp {
/** MSDP protocol related constants @{ */
constexpr char MSDP_VAR = 1;
constexpr char MSDP_VAL = 2;

constexpr char MSDP_TABLE_OPEN = 3;
constexpr char MSDP_TABLE_CLOSE = 4;

constexpr char MSDP_ARRAY_OPEN = 5;
constexpr char MSDP_ARRAY_CLOSE = 6;

constexpr size_t HEAD_LENGTH = 3;    // IAC SB TELOPT_MSDP
constexpr size_t TAIL_LENGTH = 2;    // IAC SE
constexpr size_t WRAPPER_LENGTH = HEAD_LENGTH + TAIL_LENGTH;
/** @} */

void debug(const bool on);
void log(const char *format, ...) __attribute__((format(gnu_printf, 1, 2)));
void debug_log(const char *format, ...) __attribute__((format(gnu_printf, 1, 2)));
void hexdump(const char *ptr, const size_t buflen, const char *title = nullptr);

inline std::string indent(size_t level) { return std::string(level, '\t'); }

class Serializeable {
 public:
	virtual size_t required_size() const = 0;
	virtual size_t serialize(char *buffer, size_t size) const = 0;
	virtual void dump(const size_t level = 0) const = 0;
};

class Value : public Serializeable {
 public:
	using shared_ptr = std::shared_ptr<Value>;

	enum EValueType {
		EVT_UNDEFINED,
		EVT_STRING,
		EVT_TABLE,
		EVT_ARRAY
	};

	virtual ~Value() {}
	virtual EValueType type() const = 0;
};

class Variable : public Serializeable {
 public:
	using shared_ptr = std::shared_ptr<Variable>;

	Variable(const std::string &name, const Value::shared_ptr &value)
		: m_name(name), m_value(value), m_size(1 + m_name.size() + m_value->required_size()) {}

	virtual size_t required_size() const override { return m_size; }
	virtual size_t serialize(char *buffer, size_t size) const override;
	virtual void dump(const size_t level = 0) const override;

	const std::string &name() const { return m_name; }
	auto value() const { return m_value; }

 private:
	std::string m_name;
	const Value::shared_ptr m_value;
	const size_t m_size;
};

class StringValue : public Value {
 public:
	/**
	** \note Seychas nam ne nuzhno ekranirovat' znachenie: ono nechayanno ekraniruetsya pri convertirovanii.
	** Eto esteticheski ne ochen' horosho, no menyat' seichas ne hochu.
	**/
	StringValue(const std::string &value) : m_value(value) {}
	StringValue(const std::string &&value) : m_value(value) {}

	virtual EValueType type() const override { return EVT_STRING; }
	virtual size_t required_size() const override { return 1 + m_value.size(); }
	virtual size_t serialize(char *buffer, size_t size) const override;
	const std::string &value() const { return m_value; }
	virtual void dump(const size_t level = 0) const override;

 private:
	static const std::string &escape(const std::string &value);

	std::string m_value;
};

class TableValue : public Value {
 public:
	using table_t = std::list<Variable::shared_ptr>;

	TableValue() : m_size(3) {}    // 3 - MSDP_VAL MSDP_TABLE_OPEN ... MSDP_TABLE_CLOSE
	TableValue(const table_t &table);

	virtual EValueType type() const override { return EVT_TABLE; }
	virtual size_t required_size() const override { return m_size; }
	virtual size_t serialize(char *buffer, size_t size) const override;
	virtual void dump(const size_t level = 0) const override;

	void add(Variable *variable) { add(Variable::shared_ptr(variable)); }
	void add(const Variable::shared_ptr &variable);

 private:
	table_t m_data;
	size_t m_size;
};

class ArrayValue : public Value {
 public:
	using array_t = std::list<Value::shared_ptr>;

	ArrayValue() : m_size(3) {}    // 3 - MSDP_VAL MSDP_ARRAY_OPEN ... MSDP_ARRAY_CLOSE
	ArrayValue(const array_t &array);

	virtual EValueType type() const override { return EVT_ARRAY; }
	virtual size_t required_size() const override { return m_size; }
	virtual size_t serialize(char *buffer, size_t size) const override;
	virtual void dump(const size_t level = 0) const override;

	void add(const shared_ptr &value);

 private:
	std::list<shared_ptr> m_data;
	size_t m_size;
};

using parsed_request_t = std::shared_ptr<Variable>;

bool parse_request(const char *buffer,
				   const size_t length,
				   size_t &conversation_length,
				   parsed_request_t &request,
				   const bool tail_required);
inline bool parse_request(const char *buffer,
						  const size_t length,
						  size_t &conversation_length,
						  parsed_request_t &request) {
	return parse_request(buffer,
						 length,
						 conversation_length,
						 request,
						 true);
}
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
