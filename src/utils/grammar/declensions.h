/**
\file declensions.h - a part of the Bylins engine.
\authors Created by Sventovit.
\brief Numeral-agreement (declension) of Russian nouns by count.
\detail issue.utils-cleaning: moved out of utils.h -- the "N штук / N штука / N штуки"
        agreement is language grammar, not a generic utility.
*/

#ifndef BYLINS_SRC_UTILS_GRAMMAR_DECLENSIONS_H_
#define BYLINS_SRC_UTILS_GRAMMAR_DECLENSIONS_H_

#include "cases.h"
#include "gender.h"

// What kind of thing a count is being agreed with (selects the noun + its three
// count-forms). Kept at global scope for now, like the sibling ECase; namespacing
// the grammar tokens is a separate follow-up.
namespace grammar {

enum class EWhat : int  {
	kDay,
	kHour,
	kYear,
	kPoint,
	kMinA,
	kMinU,
	kThingA,
	kThingU,
	kLvl,
	kMoveA,
	kMoveU,
	kOneA,
	kOneU,
	kSec,
	kDegree,
	kRow,
	kObject,
	kObjU,
	kRemort,
	kWeek,
	kMonth,
	kWeekU,
	kPeople,
	kStr,
	kGulp,
};

// Return the correctly-declined noun for `amount` of `of_what`
// (e.g. 1 -> "штука", 2 -> "штуки", 5 -> "штук").
const char *GetDeclensionInNumber(long amount, EWhat of_what);

// issue.currencies: endings for a counted "<size-adjective> <pile-noun>" phrase
// (e.g. the "...ая горстк-а" in "малюсенькая горстка"), indexed by grammatical case.
//   slot 0 = feminine adjective ending, slot 1 = velar-stem feminine noun ending,
//   slot 2 = hard-stem feminine noun ending.
const char *CountedFormEnding(ECase gram_case, int slot);

// issue.currencies: ending of the numeral "один" (after the "од" stem), by gender and case.
const char *OneNumeralEnding(EGender gender, ECase gram_case);

}  // namespace grammar

#endif //BYLINS_SRC_UTILS_GRAMMAR_DECLENSIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
