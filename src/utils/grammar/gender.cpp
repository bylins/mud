/**
\file gender.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\brief Gender-dependent Russian word endings and pronouns.
*/

#include "gender.h"

namespace grammar {

namespace {

// Index order in every table is EGender: [kNeutral, kMale, kFemale, kPoly].
inline int gi(EGender sex) { const int i = static_cast<int>(sex); return (i >= 0 && i < 4) ? i : 0; }
inline int vi(int variant) { return (variant >= 1 && variant <= 8) ? variant - 1 : 0; }

const char *kChSuf[8][4] = {
	{"о", "", "а", "и"},
	{"ось", "ся", "ась", "ись"},
	{"ое", "ый", "ая", "ые"},
	{"ло", "", "ла", "ли"},
	{"ло", "ел", "ла", "ли"},
	{"о", "", "а", "ы"},
	{"ым", "ым", "ой", "ыми"},
	{"ое", "ой", "ая", "ие"}
};

// issue.utils-cleaning: variant 7 is the instrumental adjective ending (m "ym", f "oj", n "ym",
// pl "ymi"). The old GET_CH_VIS_SUF_7 macro had male/female swapped vs GET_CH_SUF_7 -- a latent
// bug; corrected here so the visible table matches the plain one (grammatically correct).
const char *kChVisFallback[8] = {"", "ся", "ый", "", "ел", "", "ым", "ой"};
const char *kChVisSuf[8][4] = {
	{"о", "", "а", "и"},
	{"ось", "ся", "ась", "ись"},
	{"ое", "ый", "ая", "ые"},
	{"ло", "", "ла", "ли"},
	{"ло", "ел", "ла", "ли"},
	{"о", "", "а", "ы"},
	{"ым", "ым", "ой", "ыми"},
	{"ое", "ой", "ая", "ие"}
};

const char *kObjSuf[8][4] = {
	{"о", "", "а", "и"},
	{"ось", "ся", "ась", "ись"},
	{"ое", "ый", "ая", "ые"},
	{"ло", "", "ла", "ли"},
	{"ло", "", "ла", "ли"},
	{"о", "", "а", "ы"},
	{"е", "", "а", "и"},
	{"ое", "ой", "ая", "ие"}
};
const char *kObjVisFallback[8] = {"о", "ось", "ый", "ло", "ло", "о", "е", "ой"};
const char *kObjVisSuf[8][4] = {
	{"о", "", "а", "и"},
	{"ось", "ся", "ась", "ись"},
	{"ое", "ый", "ая", "ые"},
	{"ло", "", "ла", "ли"},
	{"ло", "", "ла", "ли"},
	{"о", "", "а", "ы"},
	{"е", "", "а", "и"},
	{"ое", "ой", "ая", "ие"}
};

const char *kChInstr[4] = {"им", "им", "ей", "ими"};
const char *kChInstrFallback = "им";

const char *kPossessive[4] = {"его", "его", "ее", "их"};
const char *kPersonal[4]   = {"оно", "он", "она", "они"};
const char *kDative[4]     = {"ему", "ему", "ей", "им"};
const char *kYour[4]       = {"ваш", "ваш", "ваша", "ваши"};
const char *kYourObj[4]    = {"ваше", "ваш", "ваша", "ваши"};

}  // namespace

const char *SexEnding(EGender sex, int variant) { return kChSuf[vi(variant)][gi(sex)]; }
const char *ObjSexEnding(EGender sex, int variant) { return kObjSuf[vi(variant)][gi(sex)]; }

const char *VisSexEnding(bool visible, EGender sex, int variant) {
	return visible ? kChVisSuf[vi(variant)][gi(sex)] : kChVisFallback[vi(variant)];
}
const char *ObjVisSexEnding(bool visible, EGender sex, int variant) {
	return visible ? kObjVisSuf[vi(variant)][gi(sex)] : kObjVisFallback[vi(variant)];
}

const char *InstrEnding(EGender sex) { return kChInstr[gi(sex)]; }
const char *VisInstrEnding(bool visible, EGender sex) {
	return visible ? kChInstr[gi(sex)] : kChInstrFallback;
}

const char *PluralVerbEnding(bool is_poly) { return is_poly ? "те" : ""; }
const char *NumberForm(bool is_poly, const char *one, const char *many) { return is_poly ? many : one; }
const char *ObjPluralVerbEnding(EGender sex) { return sex == EGender::kPoly ? "ят" : "ит"; }

const char *PossessivePronoun(EGender sex) { return kPossessive[gi(sex)]; }
const char *PersonalPronoun(EGender sex)   { return kPersonal[gi(sex)]; }
const char *DativePronoun(EGender sex)     { return kDative[gi(sex)]; }
const char *PossessiveYour(EGender sex)    { return kYour[gi(sex)]; }
const char *PossessiveYourObj(EGender sex) { return kYourObj[gi(sex)]; }

}  // namespace grammar

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
