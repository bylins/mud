#include "mud_string.h"

#include "utils.h"

int search_block(const char *target_string, const char **list, int exact);

static const char *dir_fill[] = {"in",
	"from",
	"with",
	"the",
	"on",
	"at",
	"to",
	"\n"
};

static const char *reserved[] = {"a",
	"an",
	"self",
	"me",
	"all",
	"room",
	"someone",
	"something",
	"\n"
};

int fill_word(const char *argument) {
	return (search_block(argument, dir_fill, true) >= 0);
}

int reserved_word(const char *argument) {
	return (search_block(argument, reserved, true) >= 0);
}

template<typename T>
T one_argument_template(T argument, char *first_arg) {
	char *begin = first_arg;

	if (!argument) {
		log("SYSERR: one_argument received a NULL pointer!");
		*first_arg = '\0';
		return (nullptr);
	}
	do {
		skip_spaces(&argument);
		first_arg = begin;
		while (*argument && !a_isspace(*argument)) {
			*(first_arg++) = a_lcc(*argument);
			argument++;
		}
		*first_arg = '\0';
	} while (fill_word(begin));
	skip_spaces(&argument);
	return (argument);
}

template<typename T>
T any_one_arg_template(T argument, char *first_arg) {
	if (!argument) {
		log("SYSERR: any_one_arg() passed a NULL pointer.");
		return 0;
	}
	skip_spaces(&argument);

	int num = 0;
	while (*argument && !a_isspace(*argument) && num < kMaxStringLength - 1) {
		*first_arg = a_lcc(*argument);
		++first_arg;
		++argument;
		++num;
	}
	*first_arg = '\0';
	skip_spaces(&argument);
	return argument;
}

char *one_argument(char *argument, char *first_arg) { return one_argument_template(argument, first_arg); }
const char *one_argument(const char *argument, char *first_arg) { return one_argument_template(argument, first_arg); }
char *any_one_arg(char *argument, char *first_arg) { return any_one_arg_template(argument, first_arg); }
const char *any_one_arg(const char *argument, char *first_arg) { return any_one_arg_template(argument, first_arg); }

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
	const char *temp = any_one_arg_template(string, arg1);
	skip_spaces(&temp);
	strl_cpy(arg2, temp, kMaxStringLength);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
