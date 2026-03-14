#pragma once

// Pure-logic functions from dg_triggers.cpp exposed for unit testing.
// These functions have no global side-effects and depend only on their arguments.

char *one_phrase(char *arg, char *first_arg);
int   is_substring(const char *sub, const char *string);
int   word_check(const char *str, const char *wordlist);
int   compare_cmd(int mode, const char *source, const char *dest);

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
