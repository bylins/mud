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
// пропускает ведущие пробелы, возвращает первое слово, в remains остаток после пробела
// безопасно вызывать как ExtractFirstArgument(str, str) - нет проблем с алиасингом
std::string ExtractFirstArgument(const std::string &s, std::string &remains) {
	auto start = s.find_first_not_of(' ');
	if (start == std::string::npos) {
		remains.clear();
		return {};
	}
	auto space_pos = s.find(' ', start);
	if (space_pos != std::string::npos) {
		std::string word = s.substr(start, space_pos - start);
		remains = s.substr(space_pos + 1);
		return word;
	}
	std::string word = s.substr(start);
	remains.clear();
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

// Moved from utils.cpp
void PruneCrlf(char *txt) {
	size_t i = strlen(txt) - 1;
	while (txt[i] == '\n' || txt[i] == '\r') {
		txt[i--] = '\0';
	}
}

const char *first_letter(const char *txt) {
	if (txt) {
		while (*txt && !a_isalpha(*txt)) {
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

// Moved from interpreter.cpp
int is_number(const char *str) {
	while (*str) {
		if (!a_isdigit(*(str++))) {
			return 0;
		}
	}
	return 1;
}

char *delete_doubledollar(char *string) {
	char *read, *write;
	if ((write = strchr(string, '$')) == nullptr)
		return (string);
	read = write;
	while (*read)
		if ((*(write++) = *(read++)) == '$')
			if (*read == '$')
				read++;
	*write = '\0';
	return (string);
}

// Moved from utils.cpp
int str_cmp(const char *arg1, const char *arg2) {
	int chk, i;
	if (arg1 == nullptr || arg2 == nullptr) {
		log("SYSERR: str_cmp() passed a nullptr pointer, %p or %p.", arg1, arg2);
		return (0);
	}
	for (i = 0; arg1[i] || arg2[i]; i++)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);
	return (0);
}

int str_cmp(const std::string &arg1, const char *arg2) {
	int chk;
	std::string::size_type i;
	if (arg2 == nullptr) {
		log("SYSERR: str_cmp() passed a NULL pointer, %p.", arg2);
		return (0);
	}
	for (i = 0; i != arg1.length() && *arg2; i++, arg2++)
		if ((chk = LOWER(arg1[i]) - LOWER(*arg2)) != 0)
			return (chk);
	if (i == arg1.length() && !*arg2)
		return (0);
	if (*arg2)
		return (LOWER('\0') - LOWER(*arg2));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}

int str_cmp(const char *arg1, const std::string &arg2) {
	int chk;
	std::string::size_type i;
	if (arg1 == nullptr) {
		log("SYSERR: str_cmp() passed a NULL pointer, %p.", arg1);
		return (0);
	}
	for (i = 0; *arg1 && i != arg2.length(); i++, arg1++)
		if ((chk = LOWER(*arg1) - LOWER(arg2[i])) != 0)
			return (chk);
	if (!*arg1 && i == arg2.length())
		return (0);
	if (*arg1)
		return (LOWER(*arg1) - LOWER('\0'));
	else
		return (LOWER('\0') - LOWER(arg2[i]));
}

int str_cmp(const std::string &arg1, const std::string &arg2) {
	int chk;
	std::string::size_type i;
	for (i = 0; i != arg1.length() && i != arg2.length(); i++)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);
	if (arg1.length() == arg2.length())
		return (0);
	if (i == arg1.length())
		return (LOWER('\0') - LOWER(arg2[i]));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}

int strn_cmp(const char *arg1, const char *arg2, size_t n) {
	int chk, i;
	if (arg1 == nullptr || arg2 == nullptr) {
		log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}
	for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);
	return (0);
}

int strn_cmp(const std::string &arg1, const char *arg2, size_t n) {
	int chk;
	std::string::size_type i;
	if (arg2 == nullptr) {
		log("SYSERR: strn_cmp() passed a NULL pointer, %p.", arg2);
		return (0);
	}
	for (i = 0; i != arg1.length() && *arg2 && (n > 0); i++, arg2++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(*arg2)) != 0)
			return (chk);
	if (i == arg1.length() && (!*arg2 || n == 0))
		return (0);
	if (*arg2)
		return (LOWER('\0') - LOWER(*arg2));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}

int strn_cmp(const char *arg1, const std::string &arg2, size_t n) {
	int chk;
	std::string::size_type i;
	if (arg1 == nullptr) {
		log("SYSERR: strn_cmp() passed a NULL pointer, %p.", arg1);
		return (0);
	}
	for (i = 0; *arg1 && i != arg2.length() && (n > 0); i++, arg1++, n--)
		if ((chk = LOWER(*arg1) - LOWER(arg2[i])) != 0)
			return (chk);
	if (!*arg1 && (i == arg2.length() || n == 0))
		return (0);
	if (*arg1)
		return (LOWER(*arg1) - LOWER('\0'));
	else
		return (LOWER('\0') - LOWER(arg2[i]));
}

int strn_cmp(const std::string &arg1, const std::string &arg2, size_t n) {
	int chk;
	std::string::size_type i;
	for (i = 0; i != arg1.length() && i != arg2.length() && (n > 0); i++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);
	if (arg1.length() == arg2.length() || (n == 0))
		return (0);
	if (i == arg1.length())
		return (LOWER('\0') - LOWER(arg2[i]));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}

void StringReplace(std::string &buffer, char s, const std::string &d) {
	for (size_t index = 0; index = buffer.find(s, index), index != std::string::npos;) {
		buffer.replace(index, 1, d);
		index += d.length();
	}
}

std::string &format_news_message(std::string &text) {
	StringReplace(text, '\n', "\n   ");
	utils::Trim(text);
	text.insert(0, "   ");
	text += '\n';
	return text;
}

size_t strl_cpy(char *dst, const char *src, size_t siz) {
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}
	if (n == 0) {
		if (siz != 0)
			*d = '\0';
		while (*s++);
	}
	return (s - src - 1);
}

std::string PrintNumberByDigits(long long num, const char separator) {
	const int digits_num{3};
	bool negative{false};
	if (num < 0) {
		num = -num;
		negative = true;
	}
	std::string buffer;
	try {
		buffer = std::to_string(num);
	} catch (std::bad_alloc &) {
		log("SYSERROR : string.at() (%s:%d)", __FILE__, __LINE__);
		return "<Out Of Range>";
	}
	fmt::memory_buffer out;
	if (negative) {
		out.push_back('-');
	}
	if (digits_num >= static_cast<int>(buffer.size())) {
		fmt::format_to(std::back_inserter(out), "{}", buffer);
	} else {
		auto modulo = buffer.size() % digits_num;
		if (modulo != 0) {
			fmt::format_to(std::back_inserter(out), "{}{}", buffer.substr(0, modulo), separator);
		}
		unsigned pos = modulo;
		while (pos < buffer.size() - digits_num) {
			fmt::format_to(std::back_inserter(out), "{}{}", buffer.substr(pos, digits_num), separator);
			pos += digits_num;
		}
		fmt::format_to(std::back_inserter(out), "{}", buffer.substr(pos, digits_num));
	}
	return to_string(out);
}

std::string thousands_sep(long long n) {
	bool negative = false;
	if (n < 0) {
		n = -n;
		negative = true;
	}
	int size = 50;
	int curr_pos = size - 1;
	const int comma = ',';
	std::string buffer;
	buffer.resize(size);
	int i = 0;
	try {
		do {
			if (i % 3 == 0 && i != 0) {
				buffer.at(--curr_pos) = comma;
			}
			buffer.at(--curr_pos) = '0' + n % 10;
			n /= 10;
			i++;
		} while (n != 0);
		if (negative) {
			buffer.at(--curr_pos) = '-';
		}
	}
	catch (...) {
		log("SYSERROR : string.at() (%s:%d)", __FILE__, __LINE__);
		return "<OutOfRange>";
	}
	buffer = buffer.substr(curr_pos, size - 1);
	return buffer;
}

void skip_dots(char **string) {
	for (; **string && (strchr(" .", **string) != nullptr); (*string)++);
}

char *str_str(const char *cs, const char *ct) {
	if (!cs || !ct) {
		return nullptr;
	}
	while (*cs) {
		const char *t = ct;
		while (*cs && (LOWER(*cs) != LOWER(*t))) {
			cs++;
		}
		char *s = (char*)cs;
		while (*t && *cs && (LOWER(*cs) == LOWER(*t))) {
			t++;
			cs++;
		}
		if (!*t) {
			return s;
		}
	}
	return nullptr;
}

void kill_ems(char *str) {
	char *ptr1, *ptr2;
	ptr1 = str;
	ptr2 = str;
	while (*ptr1) {
		if ('\r' != *ptr1) {
			*ptr2 = *ptr1;
			++ptr2;
		}
		++ptr1;
	}
	*ptr2 = '\0';
}

void cut_one_word(std::string &str, std::string &word) {
	if (str.empty()) {
		word.clear();
		return;
	}
	bool process = false;
	unsigned begin = 0, end = 0;
	for (unsigned i = 0; i < str.size(); ++i) {
		if (!process && a_isalnum(str.at(i))) {
			process = true;
			begin = i;
			continue;
		}
		if (process && !a_isalnum(str.at(i))) {
			end = i;
			break;
		}
	}
	if (process) {
		if (!end || end >= str.size()) {
			word = str.substr(begin);
			str.clear();
		} else {
			word = str.substr(begin, end - begin);
			str.erase(0, end);
		}
		return;
	}
	str.clear();
	word.clear();
}

void ReadEndString(std::ifstream &file) {
	char c;
	while (file.get(c)) {
		if (c == '\n') {
			return;
		}
	}
}

bool IsValidEmail(const char *address) {
	int count = 0;
	static std::string special_symbols("\r\n ()<>,;:\\\"[]|/&'`$");
	std::string addr = address;
	std::string::size_type dog_pos = 0, pos = 0;
	if (addr.find_first_of(special_symbols) != std::string::npos) {
		return false;
	}
	size_t size = addr.size();
	for (size_t i = 0; i < size; i++) {
		if (addr[i] <= ' ' || addr[i] >= 127) {
			return false;
		}
	}
	while ((pos = addr.find_first_of('@', pos)) != std::string::npos) {
		dog_pos = pos;
		++count;
		++pos;
	}
	if (count != 1 || dog_pos == 0) {
		return false;
	}
	if (size - dog_pos <= 3) {
		return false;
	}
	if (addr[dog_pos + 1] == '.' || addr[size - 1] == '.' || addr.find('.', dog_pos) == std::string::npos) {
		return false;
	}
	return true;
}

bool isname(const char *str, const char *namelist) {
	bool once_ok = false;
	const char *curname, *curstr, *laststr;
	if (!namelist || !*namelist || !str) {
		return false;
	}
	for (curstr = str; !a_isalnum(*curstr); curstr++) {
		if (!*curstr) {
			return once_ok;
		}
	}
	laststr = curstr;
	curname = namelist;
	for (;;) {
		once_ok = false;
		for (;; curstr++, curname++) {
			if (!*curstr) {
				return once_ok;
			}
			if (*curstr == '!') {
				if (a_isalnum(*curname)) {
					curstr = laststr;
					break;
				}
			}
			if (!a_isalnum(*curstr)) {
				for (; !a_isalnum(*curstr); curstr++) {
					if (!*curstr) {
						return once_ok;
					}
				}
				laststr = curstr;
				break;
			}
			if (!*curname) {
				return false;
			}
			if (!a_isalnum(*curname)) {
				curstr = laststr;
				break;
			}
			if (LOWER(*curstr) != LOWER(*curname)) {
				curstr = laststr;
				break;
			} else {
				once_ok = true;
			}
		}
		for (; a_isalnum(*curname); curname++);
		for (; !a_isalnum(*curname); curname++) {
			if (!*curname) {
				return false;
			}
		}
	}
}

const char *one_word(const char *argument, char *first_arg) {
	char *begin = first_arg;
	skip_spaces(&argument);
	first_arg = begin;
	if (*argument == '\"') {
		argument++;
		while (*argument && *argument != '\"') {
			*(first_arg++) = a_lcc(*argument);
			argument++;
		}
		argument++;
	} else {
		while (*argument && !a_isspace(*argument)) {
			*(first_arg++) = a_lcc(*argument);
			argument++;
		}
	}
	*first_arg = '\0';
	return argument;
}

// std::string overloads

void PruneCrlf(std::string &txt) {
	while (!txt.empty() && (txt.back() == '\n' || txt.back() == '\r')) {
		txt.pop_back();
	}
}

std::string first_letter(const std::string &txt) {
	const char *result = first_letter(txt.c_str());
	return result ? std::string(result) : std::string();
}

void delete_doubledollar(std::string &string) {
	std::string::size_type pos = 0;
	while ((pos = string.find("$$", pos)) != std::string::npos) {
		string.erase(pos, 1);
		++pos;
	}
}

int is_number(const std::string &str) {
	return is_number(str.c_str());
}

bool IsValidEmail(const std::string &address) {
	return IsValidEmail(address.c_str());
}

void skip_dots(std::string &string) {
	auto pos = string.find_first_not_of(" .");
	if (pos != std::string::npos) {
		string.erase(0, pos);
	} else {
		string.clear();
	}
}

std::string::size_type str_str(const std::string &cs, const std::string &ct) {
	if (cs.empty() || ct.empty()) {
		return std::string::npos;
	}
	const char *result = str_str(cs.c_str(), ct.c_str());
	if (!result) {
		return std::string::npos;
	}
	return result - cs.c_str();
}

void kill_ems(std::string &str) {
	str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
