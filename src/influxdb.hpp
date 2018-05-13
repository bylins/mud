#ifndef __INFLUX_HPP__
#define __INFLUX_HPP__

#include <list>
#include <string>
#include <sstream>

namespace influxdb
{
	class Record
	{
	public:
		Record(const std::string& measurement) : m_measurement(measurement) {}

		template <typename T>
		Record& add_tag(const std::string& name, const T& value) { return add_to_list(m_tags, name, value); }

		template <typename T>
		Record& add_field(const std::string& name, const T&value) { return add_to_list(m_fields, name, value); }

		bool get_data(std::string& data) const;

	private:
		using strings_list_t = std::list<std::string>;

		template <typename T>
		Record& add_to_list(strings_list_t& list, const std::string& name, const T&value);

		std::string m_measurement;
		strings_list_t m_tags;
		strings_list_t m_fields;
	};

	template <typename T>
	Record& Record::add_to_list(strings_list_t& list, const std::string& name, const T&value)
	{
		std::stringstream ss;
		ss << name << "=" << value;
		list.push_back(ss.str());

		return *this;
	}

	class Sender
	{
	public:
		Sender(const std::string& host, const unsigned short port);
		~Sender();

		bool ready() const;
		bool send(const Record& record) const;

	private:
		class SenderImpl* m_implementation;
	};
}

#endif // __INFLUX_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
