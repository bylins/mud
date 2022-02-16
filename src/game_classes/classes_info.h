/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных классам скиллах и спеллах должна лежать здесь.
*/

#ifndef BYLINS_SRC_CLASSES_CLASSES_INFO_H_
#define BYLINS_SRC_CLASSES_CLASSES_INFO_H_

#include "skills_info.h"
#include "structs/structs.h"

#include <array>
#include <optional>

namespace pugi {
class xml_node;
}

namespace classes {

struct ClassSkillInfo {
	using Ptr = std::unique_ptr<ClassSkillInfo>;

	int min_level{kLevelImplementator};
	int min_remort{kMaxRemort + 1};
	long improve{kMinImprove};
};

struct CharClassInfo {
	static const std::string cfg_file_name;
	static const std::string xml_main_tag;
	static const std::string xml_entity_tag;
	static const std::string load_fail_msg;

	using Ptr = std::unique_ptr<CharClassInfo>;
	using Pair = std::pair<ECharClass, Ptr>;
	using Optional = std::optional<Pair>;

	CharClassInfo() {
		skillls_ = std::make_unique<Skills>();
	}
	CharClassInfo(CharClassInfo &s) = delete;
	void operator=(const CharClassInfo &s) = delete;

	/* Class skills */
	using Skills = std::unordered_map<ESkill, ClassSkillInfo::Ptr>;
	using SkillsPtr = std::unique_ptr<Skills>;

	SkillsPtr skillls_;
	int skills_level_decrement_{1};

	[[nodiscard]] bool HasSkill(ESkill id) const;
	[[nodiscard]] bool HasntSkill(const ESkill id) const { return !HasSkill(id); };
	[[nodiscard]] int GetMinRemort(ESkill id) const;
	[[nodiscard]] int GetMinLevel(ESkill id) const;
	[[nodiscard]] int GetSkillLevelDecrement() const { return skills_level_decrement_; };
	[[nodiscard]] long GetImprove(ESkill id) const;

};

class CharClassInfoBuilder {
 public:
	[[nodiscard]] static CharClassInfo::Optional Build(const pugi::xml_node &node);
 private:
	static pugi::xml_node SelectXmlNode(const pugi::xml_node &node);
	static std::optional<std::string> GetCfgFileName(const pugi::xml_node &node);
	static void ParseXml(CharClassInfo::Optional &info, const pugi::xml_node &node);
	static void ParseSkills(CharClassInfo::Ptr &info, const pugi::xml_node &nodes);
	static void ParseSingleSkill(ClassSkillInfo::Ptr &info, const pugi::xml_node &node);
};

class ClassesInfo {
 public:
	ClassesInfo() = default;
	ClassesInfo(ClassesInfo &c) = delete;
	void operator=(const ClassesInfo &c) = delete;

	void Init();
	void Reload(const std::string &arg);
	const CharClassInfo &operator[](ECharClass id) const;

 private:
	using Ptr = std::unique_ptr<CharClassInfo>;
	using Register = std::unordered_map<ECharClass, Ptr>;
	using RegisterPtr = std::unique_ptr<Register>;
	using Optional = std::optional<RegisterPtr>;

	class RegisterBuilder {
	 public:
		static Optional Build(bool strict_parsing = true);
		static ClassesInfo::Optional Parse(const pugi::xml_node &nodes, const std::string &tag);
		static void EmplaceItem(ClassesInfo::Optional &items, CharClassInfo::Optional &item);
		static void EmplaceDefaultItems(ClassesInfo::Optional &items);
	 private:
		using ItemBuilder = CharClassInfoBuilder;

		static bool strict_pasring_;
	};

	RegisterPtr items_;
};


} // namespace classes

#endif //BYLINS_SRC_CLASSES_CLASSES_INFO_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
