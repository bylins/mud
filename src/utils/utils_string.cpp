#include "utils_string.h"

#include "utils.h"

namespace utils {
template<typename T>
std::string remove_colors_template(T string, int &new_length) {
	int pos = new_length = 0;
	while (string[pos]) {
		if ('&' == string[pos]
			&& string[1 + pos]) {
			++pos;
		} else {
			string[new_length++] = string[pos];
		}
		++pos;
	}
	return string;
}

std::string remove_colors(char *string) {
	if (string) {
		int new_length = 0;
		remove_colors_template<char *>(string, new_length);
		string[new_length] = '\0';
	}
	return string;
}

std::string remove_colors(std::string string) {
	int new_length = 0;
	remove_colors_template<std::string &>(string, new_length);
	string.resize(new_length);
	return string;
}

shared_string_ptr get_string_without_colors(const char *string) {
	shared_string_ptr result;
	if (string) {
#ifdef WIN32
		// map strdup to _strdup to get rid of warning C4996: 'strdup': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _strdup. See online help for details.
#define strdup(x) _strdup(x)
#endif
		result.reset(strdup(string), free);
#ifdef WIN32
#undef strdup
#endif

		remove_colors(result.get());
	}

	return result;
}

std::string get_string_without_colors(const std::string &string) {
	std::string result = string;

	remove_colors(result);

	return result;
}

std::ostream &Padding::output(std::ostream &os) const {
	return pad(os, m_length);
}

std::ostream &Padding::pad(std::ostream &os, const std::size_t length) const {
	for (std::size_t i = 0; i < length; ++i) {
		os << m_padding;
	}

	return os;
}

std::ostream &SpacedPadding::output(std::ostream &os) const {
	os << ' ';

	pad(os, std::max<std::size_t>(1, length()) - 1);

	if (0 < length()) {
		os << ' ';
	}

	return os;
}

void DelegatedStringWriter::set_string(const char *string) {
	const size_t l = strlen(string);
	if (nullptr == m_delegated_string_) {
		CREATE(m_delegated_string_, l + 1);
	} else {
		RECREATE(m_delegated_string_, l + 1);
	}
	strcpy(m_delegated_string_, string);
}

void DelegatedStringWriter::append_string(const char *string) {
	const size_t l = length() + strlen(string);
	if (nullptr == m_delegated_string_) {
		CREATE(m_delegated_string_, l + 1);
		*m_delegated_string_ = '\0';
	} else {
		RECREATE(m_delegated_string_, l + 1);
	}
	strcat(m_delegated_string_, string);
}

void DelegatedStringWriter::clear() {
	if (m_delegated_string_) {
		free(m_delegated_string_);
	}
	m_delegated_string_ = nullptr;
}

bool IsAbbrev(const char *arg1, const char *arg2) {
	if (!*arg1) {
		return false;
	}

	for (; *arg1 && *arg2; arg1++, arg2++) {
		if (LOWER(*arg1) != LOWER(*arg2)) {
			return false;
		}
	}

	if (!*arg1) {
		return true;
	} else {
		return false;
	}
}

// * Конвертация входной строки в нижний регистр
void ConvertToLow(std::string &text) {
	for (char & it : text) {
		it = LOWER(it);
	}
}

// * Конвертация входной строки в нижний регистр
void ConvertToLow(char *text) {
	while (*text) {
		*text = LOWER(*text);
		text++;
	}
}

void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}


void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}


void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

std::string ltrim_copy(std::string s) {
	ltrim(s);
	return s;
}

std::string rtrim_copy(std::string s) {
	rtrim(s);
	return s;
}

std::string trim_copy(std::string s) {
	trim(s);
	return s;
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
