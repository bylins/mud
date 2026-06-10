/**
\file declensions.h - a part of the Bylins engine.
\authors Created by Sventovit.
\brief Numeral-agreement (declension) of Russian nouns by count.
\detail issue.utils-cleaning: moved out of utils.h -- the "N я┬я┌я┐п╨ / N я┬я┌я┐п╨п╟ / N я┬я┌я┐п╨п╦"
        agreement is language grammar, not a generic utility.
*/

#ifndef BYLINS_SRC_UTILS_GRAMMAR_DECLENSIONS_H_
#define BYLINS_SRC_UTILS_GRAMMAR_DECLENSIONS_H_

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
	kMoneyA,
	kMoneyU,
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
	kGlory,
	kGloryU,
	kPeople,
	kStr,
	kGulp,
	kTorc,
	kGoldTorc,
	kSilverTorc,
	kBronzeTorc,
	kTorcU,
	kGoldTorcU,
	kSilverTorcU,
	kBronzeTorcU,
	kIceU,
	kNogataU
};

// Return the correctly-declined noun for `amount` of `of_what`
// (e.g. 1 -> "я┬я┌я┐п╨п╟", 2 -> "я┬я┌я┐п╨п╦", 5 -> "я┬я┌я┐п╨").
const char *GetDeclensionInNumber(long amount, EWhat of_what);

}  // namespace grammar

#endif //BYLINS_SRC_UTILS_GRAMMAR_DECLENSIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
