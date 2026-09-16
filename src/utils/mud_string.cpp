#include "mud_string.h"

#include "utils.h"
#include "utils/native_text.h"

template<typename T>
T one_argument_template(T argument, char *first_arg) {
	if (!argument) {
		log("SYSERR: one_argument received a NULL pointer!");
		*first_arg = '\0';
		return (nullptr);
	}
	skip_spaces(&argument);

	int num = 0;
	// Lowercase one whole character at a time (issue #3681). The a_isspace() test stays
	// byte-based on purpose: it only ever runs at a character boundary, and every
	// whitespace character is ASCII, so no multibyte lead byte can be mistaken for one.
	// `num` counts bytes, so it stays a buffer guard.
	while (*argument && !a_isspace(*argument) && num < kMaxStringLength - 1) {
		const size_t n = native_text::copy_lower_char(argument, first_arg);
		first_arg += n;
		argument += n;
		num += static_cast<int>(n);
	}
	*first_arg = '\0';
	skip_spaces(&argument);
	return argument;
}

char *one_argument(char *argument, char *first_arg) { return one_argument_template(argument, first_arg); }
const char *one_argument(const char *argument, char *first_arg) { return one_argument_template(argument, first_arg); }

void SplitArgument(const char *arguments, std::vector<std::string> &out) {
	char local_buf[kMaxTrglineLength];
	const char *current_arg = arguments;
	out.clear();
	do {
		current_arg = one_argument(current_arg, local_buf);
		if (!*local_buf) {
			break;
		}
		out.emplace_back(local_buf);
	} while (*current_arg);
}

void SplitArgument(const char *arguments, std::vector<short> &out) {
	std::vector<std::string> tmp;
	SplitArgument(arguments, tmp);
	for (const auto &value : tmp) {
		out.push_back(atoi(value.c_str()));
	}
}

void SplitArgument(const char *arguments, std::vector<int> &out) {
	std::vector<std::string> tmp;
	SplitArgument(arguments, tmp);
	for (const auto &value : tmp) {
		out.push_back(atoi(value.c_str()));
	}
}

void half_chop(const char *string, char *arg1, char *arg2) {
	const char *temp = one_argument_template(string, arg1);
	skip_spaces(&temp);
	strl_cpy(arg2, temp, kMaxStringLength);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
