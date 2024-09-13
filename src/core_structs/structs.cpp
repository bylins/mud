#include "structs.h"

#include "utils/logger.h"
#include "utils/parse.h"
#include "utils/utils.h"
#include "utils/parser_wrapper.h"

void tascii(const uint32_t *pointer, int num_planes, char *ascii) {
	bool found = false;

	for (int i = 0; i < num_planes; i++) {
		for (int c = 0; c < 30; c++) {
			if (pointer[i] & (1 << c)) {
				found = true;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strcat(ascii, found ? " " : "0 ");
}

const char* nothing_string = "ничего";

bool sprintbitwd(Bitvector bitvector, const char *names[], char *result, const char *div, const int print_flag) {

	long nr = 0;
	Bitvector fail;
	int plane = 0;
	char c = 'a';

	*result = '\0';

	fail = bitvector >> 30;
	bitvector &= 0x3FFFFFFF;
	while (fail) {
		if (*names[nr] == '\n') {
			fail--;
			plane++;
		}
		nr++;
	}

	bool can_show;
	for (; bitvector; bitvector >>= 1) {
		if (IS_SET(bitvector, 1)) {
			can_show = ((*names[nr] != '*') || (print_flag & 4));

			if (*result != '\0' && can_show)
				strcat(result, div);

			if (*names[nr] != '\n') {
				if (print_flag & 1) {
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				if ((print_flag & 2) && (!strcmp(names[nr], "UNUSED"))) {
					sprintf(result + strlen(result), "%ld:", nr + 1);
				}
				if (can_show)
					strcat(result, (*names[nr] != '*' ? names[nr] : names[nr] + 1));
			} else {
				if (print_flag & 2) {
					sprintf(result + strlen(result), "%ld:", nr + 1);
				} else if (print_flag & 1) {
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				strcat(result, "UNDEF");
			}
		}

		if (print_flag & 1) {
			c++;
			if (c > 'z') {
				c = 'A';
			}
		}
		if (*names[nr] != '\n') {
			nr++;
		}
	}

	if ('\0' == *result) {
		strcat(result, nothing_string);
		return false;
	}

	return true;
}

namespace base_structs {

void ParseValueToNameCase(const char *value, std::string &name_case) {
	try {
		name_case = parse::ReadAsStr(value);
	} catch (std::exception &e) {
		err_log("invalid name case (value: %s).", e.what());
	}
}

void ParseNodeToNameCases(parser_wrapper::DataNode &node, ItemName::NameCases &name_cases) {
	ParseValueToNameCase(node.GetValue("nom"), name_cases[ECase::kNom]);
	ParseValueToNameCase(node.GetValue("gen"), name_cases[ECase::kGen]);
	ParseValueToNameCase(node.GetValue("dat"), name_cases[ECase::kDat]);
	ParseValueToNameCase(node.GetValue("acc"), name_cases[ECase::kAcc]);
	ParseValueToNameCase(node.GetValue("ins"), name_cases[ECase::kIns]);
	ParseValueToNameCase(node.GetValue("pre"), name_cases[ECase::kPre]);
}

ItemName::ItemName() {
	std::fill(plural_names_.begin(), plural_names_.end(), "UNDEF");
	std::fill(singular_names_.begin(), singular_names_.end(), "UNDEF");
}

ItemName::Ptr ItemName::Build(parser_wrapper::DataNode &node) {
	Ptr ptr(new ItemName());
	node.GoToChild("singular");
	ParseNodeToNameCases(node, ptr->singular_names_);
	node.GoToSibling("plural");
	ParseNodeToNameCases(node, ptr->plural_names_);
	node.GoToParent();
	return ptr;
}

ItemName &ItemName::operator=(ItemName &&i) noexcept {
	if (&i == this) {
		return *this;
	}
	singular_names_ = std::move(i.singular_names_);
	plural_names_ = std::move(i.plural_names_);
	return *this;
}

ItemName::ItemName(ItemName &&i) noexcept {
	singular_names_ = std::move(i.singular_names_);
	plural_names_ = std::move(i.plural_names_);
};

const std::string &ItemName::GetSingular(ECase name_case) const {
	return singular_names_.at(name_case);
}

const std::string &ItemName::GetPlural(ECase name_case) const {
	return plural_names_.at(name_case);
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

