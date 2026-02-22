// trigger_indenter.h
// Stateful indenter for DG script triggers

#ifndef TRIGGER_INDENTER_H
#define TRIGGER_INDENTER_H

#include <stack>
#include <string>

/**
 * TriggerIndenter - stateful indenter for DG script triggers.
 * Tracks nesting level of control structures (if/while/switch/case) and
 * applies proper indentation to script commands.
 *
 * Replaces global thread_local stack with explicit state management.
 */
class TriggerIndenter {
public:
	TriggerIndenter() = default;
	~TriggerIndenter() = default;

	// Indent a single command line. Updates level for next line.
	// Returns newly allocated string (caller must free).
	char *indent(char *cmd, int *level);

	// Reset indenter state (clears stack)
	void reset() {
		std::stack<std::string> empty_stack;
		indent_stack_.swap(empty_stack);
	}

private:
	std::stack<std::string> indent_stack_;
};

#endif // TRIGGER_INDENTER_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
