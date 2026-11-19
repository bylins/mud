// Поиск аффекта по короткому имени (FindByShortDesc) -- то, чем dgaffect, %actor.affect(...)%
// и "лечить 'имя'" превращают написанное имя в EAffect.
//
// Короткие имена многословные ("мантия теней"), а аргумент читается до пробела, поэтому имя
// пишут через точку или подчёркивание. Побайтное сравнение на таком разделителе не совпадало
// никогда, и аффект молча не вешался.

#include "gameplay/affects/affect_messages.h"
#include "gameplay/affects/affect_contants.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <fstream>

namespace {

const char *kMsgSrc = "affect_short_desc_src.xml";

// Две связки с многословными именами -- ровно те, на которых спотыкались триггеры.
void LoadShortDescs() {
	{
		std::ofstream f(kMsgSrc);
		f << R"(<affect_msg>)"
		     R"(<msg_sheaf id="kShadowCloak"><message type="kShortDesc" val="мантия теней" /></msg_sheaf>)"
		     R"(<msg_sheaf id="kDrunked"><message type="kShortDesc" val="под мухой" /></msg_sheaf>)"
		     R"(</affect_msg>)";
	}
	parser_wrapper::DataNode doc(kMsgSrc);
	affects::AffectMessagesLoader loader;
	loader.Load(doc);
}

EAffect Resolve(const char *name) {
	EAffect out = EAffect::kUndefined;
	return affects::FindByShortDesc(name, out) ? out : EAffect::kUndefined;
}

}  // namespace

TEST(Affect_ShortDescLookup, SeparatorDoesNotMatter) {
	LoadShortDescs();
	EXPECT_EQ(EAffect::kShadowCloak, Resolve("мантия теней"));
	EXPECT_EQ(EAffect::kShadowCloak, Resolve("мантия.теней"));
	EXPECT_EQ(EAffect::kShadowCloak, Resolve("мантия_теней"));
	EXPECT_EQ(EAffect::kDrunked, Resolve("под_мухой"));
}

TEST(Affect_ShortDescLookup, AbbreviatedWordsStillResolve) {
	LoadShortDescs();
	// Префиксный поиск остался -- только теперь по словам.
	EXPECT_EQ(EAffect::kShadowCloak, Resolve("мантия"));
	EXPECT_EQ(EAffect::kShadowCloak, Resolve("мантия.тен"));
}

TEST(Affect_ShortDescLookup, UnknownNameDoesNotResolve) {
	LoadShortDescs();
	EXPECT_EQ(EAffect::kUndefined, Resolve("теней"));            // пропускать слова нельзя
	EXPECT_EQ(EAffect::kUndefined, Resolve("мантия.теней.мрака"));  // лишнее слово -- не совпадение
	EXPECT_EQ(EAffect::kUndefined, Resolve(""));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
