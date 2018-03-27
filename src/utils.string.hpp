#ifndef __UTILS_STRING_HPP__
#define __UTILS_STRING_HPP__

#include <memory>
#include <string>

namespace utils
{
	using shared_string_ptr = std::shared_ptr<char>;

	void remove_colors(char* string);
	void remove_colors(std::string& string);
	shared_string_ptr get_string_without_colors(const char* string);
	std::string get_string_without_colors(const std::string& string);

	class Padding
	{
	public:
		Padding(const std::string& string, std::size_t minimal_length, const char padding = ' '):
			m_length(string.size() > minimal_length ? 0 : minimal_length - string.size()),
			m_padding(padding)
		{
		}

		const auto length() const { return m_length; }
		std::ostream& output(std::ostream& os) const;

	protected:
		std::ostream& pad(std::ostream& os, const std::size_t length) const;

	private:
		std::size_t m_length;
		char m_padding;
	};

	inline std::ostream& operator<<(std::ostream& os, const Padding& padding) { return padding.output(os); }

	class SpacedPadding : public Padding
	{
	public:
		SpacedPadding(const std::string& string, std::size_t minimal_length, const char padding = ' ') :
			Padding(string, minimal_length, padding)
		{
		}

		std::ostream& output(std::ostream& os) const;
	};

	inline std::ostream& operator<<(std::ostream& os, const SpacedPadding& padding) { return padding.output(os); }
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
