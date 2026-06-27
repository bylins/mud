/**
\file gender.h - a part of the Bylins engine.
\authors Created by Sventovit.
\brief Gender-dependent Russian word endings and pronouns.
\detail issue.utils-cleaning: the GET_CH_SUF_n / GET_OBJ_SUF_n / HSHR etc. macros, reborn as
        pure-grammar functions keyed by EGender (no CharData/ObjData dependency).
*/

#ifndef BYLINS_SRC_UTILS_GRAMMAR_GENDER_H_
#define BYLINS_SRC_UTILS_GRAMMAR_GENDER_H_

// Grammatical gender of a character or object. Lives here (the grammar module) because gender is
// a language concept the rest of the engine reads from -- entities_constants.h re-exports it by
// including this header. Underlying type `char` matches the engine's `byte` alias (byte = char).
enum class EGender : char {
	kNeutral = 0,
	kMale = 1,
	kFemale = 2,
	kPoly = 3,
	kLast
};

namespace grammar {

// Past-tense / adjectival endings selected by the actor's gender, indexed by the historical
// act() $-code slot (variant 1..8). Character and object actors use different tables for some
// variants, so they are kept separate. The Vis* forms take the viewer's visibility: when the
// viewer cannot see the actor a neutral fallback ending is returned instead of the gendered one.
const char *SexEnding(EGender sex, int variant);
const char *VisSexEnding(bool visible, EGender sex, int variant);
const char *ObjSexEnding(EGender sex, int variant);
const char *ObjVisSexEnding(bool visible, EGender sex, int variant);

// Soft-stem instrumental ending ($h / {intensity}); same table for char and object actors.
const char *InstrEnding(EGender sex);
const char *VisInstrEnding(bool visible, EGender sex);

// Verb plural agreement. The character form depends on polymorph state (a bool), the object form
// on grammatical gender (kPoly -> plural).
const char *PluralVerbEnding(bool is_poly);
const char *ObjPluralVerbEnding(EGender sex);
// Pick the singular or plural (poly) form of a NUMBER-only word/ending. Russian present/
// future verbs and many nouns vary by number, not gender, so a gender $-code cannot select
// them; e.g. NumberForm(IsPoly(ch), "<sing>", "<plur>"). Complements the gender selectors.
const char *NumberForm(bool is_poly, const char *one, const char *many);

// Pronouns. SHR/SSH/MHR are identical for characters and objects; possessive "your" differs only
// in the neuter form, so it has a separate object variant.
const char *PossessivePronoun(EGender sex);
const char *PersonalPronoun(EGender sex);
const char *DativePronoun(EGender sex);
const char *PossessiveYour(EGender sex);
const char *PossessiveYourObj(EGender sex);

}  // namespace grammar

#endif  // BYLINS_SRC_UTILS_GRAMMAR_GENDER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
