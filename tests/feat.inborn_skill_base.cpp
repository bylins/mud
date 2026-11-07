// issue.inborn-magic: способность может задавать умению врожденный базовый уровень --
// блок <skill_base> в cfg/feats.xml. Это не прибавка к выученному (skill_mods), а пол:
// умение считается не ниже указанного значения, даже если персонаж его не учил.
// Нужно это витязю: заклинания считают свои роллы от школы магии, а школ ему классом
// не положено, и на нулевом умении все школьные слагаемые обнуляются.
//
// Тесты пришпиливают разбор блока и то, что skill_base и skill_mods не путаются между
// собой (оба -- Mod<ESkill>, ошибиться копипастой проще всего). Инлайновый XML, без
// загрузки мира -- по образцу affects.loader.cpp.

#include <gtest/gtest.h>

#include "gameplay/abilities/talents_effects.h"
#include "gameplay/skills/skills.h"
#include "utils/parser_wrapper.h"

#include <fstream>
#include <string>

namespace {

// Effects непереносим (pimpl), поэтому заполняем переданный объект, а не возвращаем свой.
void BuildEffects(talents_effects::Effects &effects, const char *file_name, const std::string &body) {
	{
		std::ofstream f(file_name);
		f << "<feat id=\"kInbornMagic\" mode=\"kEnabled\"><talent_effects>" << body
		  << "</talent_effects></feat>";
	}
	parser_wrapper::DataNode doc(file_name);
	ASSERT_TRUE(doc.GoToChild("talent_effects"));
	effects.Build(doc);
}

} // namespace

TEST(FeatInbornSkillBase, ParsesListedSchools) {
	talents_effects::Effects effects;
	BuildEffects(effects, "feat_skill_base_src.xml",
				 R"(<skill_base>)"
				 R"(<skill id="kLightMagic" val="200"/>)"
				 R"(<skill id="kMindMagic" val="150"/>)"
				 R"(</skill_base>)");

	EXPECT_EQ(effects.GetSkillBase(ESkill::kLightMagic), 200);
	EXPECT_EQ(effects.GetSkillBase(ESkill::kMindMagic), 150);
	// Не перечисленная школа врожденной не становится.
	EXPECT_EQ(effects.GetSkillBase(ESkill::kDarkMagic), 0);
	// И умение не из блока тоже.
	EXPECT_EQ(effects.GetSkillBase(ESkill::kPunch), 0);
}

TEST(FeatInbornSkillBase, SkillModsAreNotSkillBase) {
	talents_effects::Effects effects;
	BuildEffects(effects, "feat_skill_mods_only_src.xml",
				 R"(<skill_mods><skill id="kMindMagic" val="20"/></skill_mods>)");

	// skill_mods -- прибавка к выученному, базовым уровнем она не считается.
	EXPECT_EQ(effects.GetSkillBase(ESkill::kMindMagic), 0);
}

TEST(FeatInbornSkillBase, NoBlockMeansNoBase) {
	talents_effects::Effects effects;
	BuildEffects(effects, "feat_no_effects_src.xml", "");

	EXPECT_EQ(effects.GetSkillBase(ESkill::kLightMagic), 0);
}
