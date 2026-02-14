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

	if (!strn_cmp("case ", ptr, 5) || !strn_cmp("default", ptr, 7)) {
		if (!indent_stack_.empty()
			&& !strn_cmp("case ", indent_stack_.top().c_str(), 5)) {
			--currlev;
		} else {
			indent_stack_.push(ptr);
		}
		nextlev = currlev + 1;
	} else if (!strn_cmp("if ", ptr, 3) || !strn_cmp("while ", ptr, 6)
		|| !strn_cmp("foreach ", ptr, 8) || !strn_cmp("switch ", ptr, 7)) {
		++nextlev;
		indent_stack_.push(ptr);
	} else if (!strn_cmp("elseif ", ptr, 7) || !strn_cmp("else", ptr, 4)) {
		--currlev;
	} else if (!strn_cmp("break", ptr, 5) || !strn_cmp("end", ptr, 3)
		|| !strn_cmp("done", ptr, 4)) {
		if ((!strn_cmp("done", ptr, 4) || !strn_cmp("end", ptr, 3))
			&& !indent_stack_.empty()
			&& (!strn_cmp("case ", indent_stack_.top().c_str(), 5)
				|| !strn_cmp("default", indent_stack_.top().c_str(), 7))) {
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
