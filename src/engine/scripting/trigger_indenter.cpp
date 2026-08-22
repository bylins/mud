// trigger_indenter.cpp
// Implementation of TriggerIndenter class

#include "trigger_indenter.h"

#include "utils/utils.h"
#include "engine/core/sysdep.h"
#include <cstring>
#include <algorithm>

char *TriggerIndenter::indent(char *cmd, int *level) {
	*level = std::max(0, *level);
	if (*level == 0) {
		reset();
	}

	int currlev, nextlev;
	currlev = nextlev = *level;

	if (!cmd) {
		return cmd;
	}

	char *ptr = cmd;
	skip_spaces(&ptr);

	if (utils::IsAbbr("case ", ptr) || utils::IsAbbr("default", ptr)) {
		if (!indent_stack_.empty()
			&& utils::IsAbbr("case ", indent_stack_.top().c_str())) {
			--currlev;
		} else {
			indent_stack_.push(ptr);
		}
		nextlev = currlev + 1;
	} else if (utils::IsAbbr("if ", ptr) || utils::IsAbbr("while ", ptr)
		|| utils::IsAbbr("foreach ", ptr) || utils::IsAbbr("switch ", ptr)) {
		++nextlev;
		indent_stack_.push(ptr);
	} else if (utils::IsAbbr("elseif ", ptr) || utils::IsAbbr("else", ptr)) {
		--currlev;
	} else if (utils::IsAbbr("break", ptr) || utils::IsAbbr("end", ptr)
		|| utils::IsAbbr("done", ptr)) {
		if ((utils::IsAbbr("done", ptr) || utils::IsAbbr("end", ptr))
			&& !indent_stack_.empty()
			&& (utils::IsAbbr("case ", indent_stack_.top().c_str())
				|| utils::IsAbbr("default", indent_stack_.top().c_str()))) {
			--currlev;
			--nextlev;
			indent_stack_.pop();
		}
		if (!indent_stack_.empty()) {
			indent_stack_.pop();
		}
		--nextlev;
		--currlev;
	}

	if (nextlev < 0) nextlev = 0;
	if (currlev < 0) currlev = 0;

	char *tmp = (char *) malloc(currlev * 2 + 1);
	memset(tmp, 0x20, currlev * 2);
	tmp[currlev * 2] = '\0';

	tmp = str_add(tmp, ptr);

	cmd = (char *) realloc(cmd, strlen(tmp) + 1);
	cmd = strcpy(cmd, tmp);

	free(tmp);

	*level = nextlev;
	return cmd;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
