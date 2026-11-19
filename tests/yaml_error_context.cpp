#include <gtest/gtest.h>

#include "engine/db/yaml_error_context.h"

namespace {

// Тот самый случай: в теле триггера строку потеряли отступ, блок оборвался, а парсер
// споткнулся уже на следующей записи.
constexpr const char *kBrokenTriggers =
	"# Triggers for zone 587\n"          // 1
	"\n"                                 // 2
	"# Trigger #58713\n"                 // 3
	"13:\n"                              // 4
	"  name: запиcку деду\n"             // 5
	"  script: |-2\n"                    // 6
	"    wait 3\n"                       // 7
	"    end\n"                          // 8
	"дать все %actor.iname%\n"           // 9  <- потерян отступ
	"\n"                                 // 10
	"# Trigger #58714\n"                 // 11
	"14:\n"                              // 12
	"  name: упали корабль 1\n"          // 13
	"  script: |-2\n"                    // 14
	"    wait 3\n";                      // 15

}  // namespace

TEST(YamlErrorContext, PointsAtTheLineThatLostItsIndent) {
	const auto hint = world_format::DescribeYamlError(kBrokenTriggers, 15);
	EXPECT_NE(hint.find("строка 15"), std::string::npos) << hint;
	EXPECT_NE(hint.find("похоже на причину -- строка 9"), std::string::npos) << hint;
	EXPECT_NE(hint.find("дать все %actor.iname%"), std::string::npos) << hint;
}

TEST(YamlErrorContext, NamesTheEntryTheBreakBelongsTo) {
	const auto hint = world_format::DescribeYamlError(kBrokenTriggers, 15);
	EXPECT_NE(hint.find("\"13:\""), std::string::npos) << hint;
	EXPECT_NE(hint.find("со строки 4"), std::string::npos) << hint;
}

TEST(YamlErrorContext, DoesNotRepeatTheLineWhenTheParserStoppedOnIt) {
	// Так вышло на настоящей зоне 587: парсер упёрся ровно в строку, потерявшую отступ.
	const auto hint = world_format::DescribeYamlError(kBrokenTriggers, 9);
	EXPECT_NE(hint.find("строка 9 без отступа"), std::string::npos) << hint;
	EXPECT_EQ(hint.find("похоже на причину"), std::string::npos) << hint;
	EXPECT_NE(hint.find("\"13:\""), std::string::npos) << hint;
}

TEST(YamlErrorContext, SaysTheBreakIsAboveWhenNoSuspectFound) {
	const std::string clean =
		"14:\n"
		"  name: упали корабль 1\n"
		"  script: |-2\n"
		"    wait 3\n";
	const auto hint = world_format::DescribeYamlError(clean, 4);
	EXPECT_NE(hint.find("разрыв блока обычно выше"), std::string::npos) << hint;
	EXPECT_EQ(hint.find("похоже на причину"), std::string::npos) << hint;
}

TEST(YamlErrorContext, EmptyForNonsenseInput) {
	EXPECT_TRUE(world_format::DescribeYamlError("", 1).empty());
	EXPECT_TRUE(world_format::DescribeYamlError("14:\n", 0).empty());
	EXPECT_TRUE(world_format::DescribeYamlError("14:\n", 99).empty());
}

TEST(YamlErrorContext, TopLevelKeyIsNotBlamed) {
	// Ключи верхнего уровня (zone.yaml) стоят без отступа законно -- в причины не годятся.
	const std::string zone =
		"vnum: 587\n"
		"name: Затопленные шхеры\n"
		"top_room: 58799\n"
		"commands:\n"
		"  - MOB 0 58708 1 58701\n";
	const auto hint = world_format::DescribeYamlError(zone, 5);
	EXPECT_EQ(hint.find("похоже на причину"), std::string::npos) << hint;
}
