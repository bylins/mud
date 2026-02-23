//#include "utils_string.h"

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
	return {};
}

std::string RemoveColors(char *string) {
	if (string) {
		int new_length = 0;
		remove_colors_template<char *>(string, new_length);
		string[new_length] = '\0';
		return string;
	}
	return {};
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
	return RemoveColors(string);
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

bool IsEqual(const std::string &abbr, const std::string &words) {
	std::vector<std::string> words_list = utils::Split(words);
	std::vector<std::string> abbr_list = utils::Split(utils::FixDot(abbr));
	auto it = words_list.begin();

	for (auto abr : abbr_list) {
		if (it == words_list.end() || !utils::IsAbbr(abr.c_str(), (*it).c_str()))
			return false;
		++it;
	}
	return true;
}


bool IsEquivalent(const std::string &abbr, const std::string &words) {
	std::vector<std::string> words_list = utils::Split(words);
	std::vector<std::string> abbr_list = utils::Split(utils::FixDot(abbr));
	auto it = words_list.begin();

	for (auto abr : abbr_list) {
		for (; it != words_list.end(); it++) {
			if (utils::IsAbbr(abr.c_str(), (*it).c_str())) {
				break;
			}
		}
		if (it == words_list.end())
			return false;
	}
	return true;
}

std::string ReplaceSymbol(std::string s, const char ToSearch, const char Replacer) {
	for (char &it: s) {
		if (it == ToSearch) {
			it = Replacer;
		}
	}
	return s;
}

std::string ReplaceAny(std::string s, std::string any) {
	if (any.size() == 1)
		return s;
	size_t i = 1;

	while (i < any.size()) {
		s = ReplaceSymbol(s, any[i], any[0]);
		i++;
	}
		return s;
}

std::vector<std::string> SplitAny(const std::string s, std::string any) {
	return Split(ReplaceAny(s, any), any[0]);
}

std::vector<std::string> Split(const std::string s, char delimiter) {
	std::string token;
	std::istringstream tokens_stream(s);
	std::vector<std::string> tokens;

	while (std::getline(tokens_stream, token, delimiter)) {
		if (token.empty())
			continue;
		TrimLeft(token);
		tokens.push_back(token);
	}
	return tokens; //если разделитель не найден вернется 1 элемент содержащий полную строку
}

// первое слово разделенное маской
std::string FirstWordOnString(std::string s, std::string mask) {
	int pos = s.find_first_of(mask);
	if (pos > 0)
		s.erase(pos);
	return s;
}

// аналог one_argument для string
std::string ExtractFirstArgument(const std::string &s, std::string &remains) {
	std::string word;

	size_t space_pos = s.find(" ");
		if (space_pos != std::string::npos) {
			word = s.substr(0, space_pos);
			remains = s.substr(space_pos + 1);
		}
	return word;
}

std::string SubstToLow(std::string s) {
	for (char &it: s) {
		it = LOWER(it);
	}
	return s;
}

void ConvertKtoW(std::string &text) {
	for (char &it: text) {
		it = KtoW(it);
	}
}

void ConvertWtoK(std::string &text) {
	for (char &it: text) {
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
	for (char &it: text) {
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
	for (char &it: s) {
		it = UPPER(it);
	}
	return s;
}

std::string SubstStrToUpper(std::string s) {
	for (char &it: s) {
		it = UPPER(it);
	}
	return s;
}

void TrimLeft(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}
void TrimLeft(char *s) {
	char* f = s;
	while (*f && isspace((unsigned char)*f)) {
		f++;
	}
	if (f != s) {
		memmove(s, f, strlen(f) + 1);
	}
}

void TrimRight(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

void Trim(std::string &s) {
	TrimRight(s);
	TrimLeft(s);
}

void TrimRight(char* string) {
	if (!string || !*string) {
		return;
	}
	size_t len = strlen(string);
	while (len > 0 && (string[len - 1] == ' ')) {
		len--;
	}
	string[len] = '\0';
}

void Trim(char *s) {
	if (!s || !*s)
		return;
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

void TrimLeftIf(std::string &s, const std::string &whitespaces) {
	auto lambda = [&whitespaces] (char c) {
		return (whitespaces.find(c) == std::string::npos);
	};
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), lambda));
}

void TrimRightIf(std::string &s, const std::string &whitespaces) {
	auto lambda = [&whitespaces] (char c) {
		return (whitespaces.find(c) == std::string::npos);
	};
	s.erase(std::find_if(s.rbegin(), s.rend(), lambda).base(), s.end());
}

void TrimIf(std::string &s, const std::string &whitespaces) {
	TrimLeftIf(s, whitespaces);
	TrimRightIf(s, whitespaces);
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

void ReplaceFirst(std::string &s, const std::string &toSearch, const std::string &replacer) {
	size_t pos = s.find(toSearch);
	if (pos != std::string::npos) {
		s.replace(pos, toSearch.size(), replacer);
	}
}

void ReplaceAll(std::string &s, const std::string &toSearch, const std::string &replacer) {
	size_t pos = s.find(toSearch);
	while (pos != std::string::npos) {
		s.replace(pos, toSearch.size(), replacer);
		pos = s.find(toSearch, pos + replacer.size());
	}
}

void ReplaceTrigerNumber(std::string &s, const std::string &toSearch, const std::string &replacer) {
	size_t pos = s.find(toSearch);
	if (pos == 0) {
		s.replace(pos, toSearch.size(), replacer);
		return;
	}
	while (pos != std::string::npos) {
		if (!isdigit(s[pos - 1])) {
			s.replace(pos, toSearch.size(), replacer);
		}
		pos = s.find(toSearch, pos + replacer.size());
	}
}

void EraseAll(std::string &s, const std::string &toSearch) {
	size_t pos = s.find(toSearch);
	size_t len = toSearch.length();
	while (pos != std::string::npos) {
		s.erase(pos, len);
		pos = s.find(toSearch);
	}
}

std::string EraseAllAny(const std::string s, const std::string any) {
	std::string mstr = ReplaceAny(s, any);
	for (size_t k = mstr.find(any[0]); k != mstr.npos;  k = mstr.find(any[0], k )) {
		mstr.erase(k, 1);
	}
	return mstr;
}

std::string CompressSymbol(std::string s, const char ch) {
	std::string c(1, ch);
	std::string::size_type pos = s.find(c + c);

	while (pos != std::string::npos) {
		s.replace(pos, 2, c);
		pos = s.find(c + c, pos);
	}
	return s;
}

const char *first_letter(const char *txt) {
	if (txt) {
		while (*txt && !a_isalpha(*txt)) {
			//Предполагается, что для отправки клиенту используется только управляющий код с цветом
			//На данный момент в коде присутствует только еще один управляющий код для очистки экрана,
			//но он не используется (см. CLEAR_SCREEN)
			if ('\x1B' == *txt) {
				while (*txt && 'm' != *txt) {
					++txt;
				}
				if (!*txt) {
					return txt;
				}
			} else if ('&' == *txt) {
				++txt;
				if (!*txt) {
					return txt;
				}
			}
			++txt;
		}
	}
	return txt;
}

char *colorCAP(char *txt) {
	char *letter = const_cast<char *>(first_letter(txt));
	if (letter && *letter) {
		*letter = UPPER(*letter);
	}
	return txt;
}

std::string &colorCAP(std::string &txt) {
	size_t pos = first_letter(txt.c_str()) - txt.c_str();
	txt[pos] = UPPER(txt[pos]);
	return txt;
}

// rvalue variant
std::string &colorCAP(std::string &&txt) {
	return colorCAP(txt);
}

char *colorLOW(char *txt) {
	char *letter = const_cast<char *>(first_letter(txt));
	if (letter && *letter) {
		*letter = LOWER(*letter);
	}
	return txt;
}

std::string &colorLOW(std::string &txt) {
	size_t pos = first_letter(txt.c_str()) - txt.c_str();
	txt[pos] = LOWER(txt[pos]);
	return txt;
}

// rvalue variant
std::string &colorLOW(std::string &&txt) {
	return colorLOW(txt);
}

char *CAP(char *txt) {
	*txt = UPPER(*txt);
	return (txt);
}

std::string CAP(const std::string txt) {
	std::string tmp_str = txt;
	tmp_str[0] = UPPER(tmp_str[0]);
	return (tmp_str);
}


} //namespace utils

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
