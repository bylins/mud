#ifndef BYLINS_SRC_UTILS_MUD_STRING_H_
#define BYLINS_SRC_UTILS_MUD_STRING_H_

#include <string>
#include <vector>

/// Проверить является ли слово служебным (in, from, with, the, on, at, to).
int fill_word(const char *argument);

/// Проверить является ли слово зарезервированным (a, an, self, me, all, room, ...).
int reserved_word(const char *argument);

/// Извлечь первый аргумент из строки, пропуская служебные слова.
/// Приводит к нижнему регистру.
char *one_argument(char *argument, char *first_arg);
const char *one_argument(const char *argument, char *first_arg);

/// Извлечь первый аргумент из строки БЕЗ пропуска служебных слов.
/// Приводит к нижнему регистру.
char *any_one_arg(char *argument, char *first_arg);
const char *any_one_arg(const char *argument, char *first_arg);

/// Разбор строки на два аргумента через one_argument.
template<typename T>
T two_arguments(T argument, char *first_arg, char *second_arg) {
	return (one_argument(one_argument(argument, first_arg), second_arg));
}

/// Разбор строки на три аргумента через one_argument.
template<typename T>
T three_arguments(T argument, char *first_arg, char *second_arg, char *third_arg) {
	return (one_argument(one_argument(one_argument(argument, first_arg), second_arg), third_arg));
}

/// Разделить строку на первое слово (arg1) и остаток (arg2).
void half_chop(const char *string, char *arg1, char *arg2);

/// Разбить строку на отдельные аргументы.
void SplitArgument(const char *arguments, std::vector<std::string> &out);
void SplitArgument(const char *arguments, std::vector<short> &out);
void SplitArgument(const char *arguments, std::vector<int> &out);

#endif //BYLINS_SRC_UTILS_MUD_STRING_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
