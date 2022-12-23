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

std::string RemoveColors(char *string) {
	if (string) {
		int new_length = 0;
		remove_colors_template<char *>(string, new_length);
		string[new_length] = '\0';
	}
	return string;
}

std::string RemoveColors(std::string string) {
	int new_length = 0;
	remove_colors_template<std::string &>(string, new_length);
	string.resize(new_length);
	return string;
}

shared_string_ptr GetStringWithoutColors(const char *string) {
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

		RemoveColors(result.get());
	}

	return result;
}

std::string GetStringWithoutColors(const std::string &string) {
	std::string result = string;

	RemoveColors(result);

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

bool IsAbbr(const char *arg1, const char *arg2) {
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

void Split(std::vector<std::string> &tokens, const std::string& s, char delimiter) {
	std::string token;
	std::istringstream tokens_stream(s);
	while (std::getline(tokens_stream, token, delimiter)) {
		utils::Trim(token);
		tokens.push_back(token);
	}
}


std::string SubstToLow(std::string s) {
	for (char & it : s) {
		it = LOWER(it);
	}
	return s;
}

void ConvertKtoW(std::string &text) {
	for (char & it : text) {
		it = KtoW(it);
	}
}

void ConvertWtoK(std::string &text) {
	for (char & it : text) {
		it = WtoK(it);
	}
}

std::string SubstKtoW(std::string s) {
	ConvertKtoW(s);
	return s;
}

std::string SubstWtoK(std::string s) {
	ConvertWtoK(s);
	return s;
}

void ConvertToLow(std::string &text) {
	for (char & it : text) {
		it = LOWER(it);
	}
}

void ConvertToLow(char *text) {
	while (*text) {
		*text = LOWER(*text);
		text++;
	}
}

std::string SubstStrToLow(std::string s) {
	for (char & it : s) {
		it = UPPER(it);
	}
	return s;
}

std::string SubstStrToUpper(std::string s) {
	for (char & it : s) {
		it = UPPER(it);
	}
	return s;
}

void TrimLeft(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}

void TrimRight(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

void Trim(std::string &s) {
	TrimLeft(s);
	TrimRight(s);
}

std::string TrimLeftCopy(std::string s) {
	TrimLeft(s);
	return s;
}

std::string TrimRightCopy(std::string s) {
	TrimRight(s);
	return s;
}

std::string TrimCopy(std::string s) {
	Trim(s);
	return s;
}

std::string FixDot(std::string s) {
	for (char &it : s) {
		if (('.' == it) || ('_' == it)) {
			it = ' ';
		}
	}
	return s;
}

void SortKoiString(std::vector<std::string> &str) {
	for (auto &it : str) {
		ConvertKtoW(it);
	}
	std::sort(str.begin(), str.end(), std::less<std::string>());
	for (auto &it : str) {
		ConvertWtoK(it);
	}
}


void SortKoiStringReverse(std::vector<std::string> &str) {
	for (auto &it : str) {
		ConvertKtoW(it);
	}
	std::sort(str.begin(), str.end(), std::greater<std::string>());
	for (auto &it : str) {
		ConvertWtoK(it);
	}
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
