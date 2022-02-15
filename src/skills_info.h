#ifndef SKILLS_INFO_
#define SKILLS_INFO_

#include "skills.h"

#include "boot/cfg_manager.h"
#include "entities/entity_constants.h"
#include "classes/classes_constants.h"
#include "structs/info_container.h"

/*
 * Загрузчик конфига умений.
 */
class SkillsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

/*
 * Класс-описание конкретного умения.
 */
struct SkillInfo : public info_container::IItem<ESkill> {
	ESkill id{ESkill::kFirst};
	std::string name{"!undefined!"};
	std::string short_name{"!error"};
	ESaving save_type{ESaving::kFirst};
	int difficulty{200};
	int cap{1};
	bool autosuccess{false};
	EItemMode mode{EItemMode::kDisabled};

	ESkill GetId() final { return id; };
	EItemMode GetMode() final { return mode; };

	/*
	 * Имя скилла в виде C-строки. По возможности используйте std::string
	 */
	[[nodiscard]] const char *GetName() const { return name.c_str(); };
	[[nodiscard]] const char *GetAbbr() const { return short_name.c_str(); };
	void Print(std::stringstream &buffer) const;
};

/*
 * Класс-билдер описания отдельного умения.
 */
class SkillInfoBuilder : public info_container::IItemBuilder<SkillInfo> {
 public:
	ItemOptional Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemOptional &ParseObligatoryValues(ItemOptional &optional, parser_wrapper::DataNode &node);
	static ItemOptional &ParseDispensableValues(ItemOptional &optional, parser_wrapper::DataNode &node);
};

using SkillsInfo = info_container::InfoContainer<ESkill, SkillInfo, SkillInfoBuilder>;

// Этому место в структуре скилл_инфо (а еще точнее - абилок), но во-первых, в messages запихали и сообщения спеллов,
// и еще черта лысого в ступе, во-вторых, это надо переделывать структуру и ее парсинг. Поэтому пока так.
struct AttackMsg {
	char *attacker_msg{nullptr};    // message to attacker //
	char *victim_msg{nullptr};        // message to victim   //
	char *room_msg{nullptr};        // message to room     //
};

struct AttackMsgSet {
	AttackMsg die_msg;        // messages when death        //
	AttackMsg miss_msg;        // messages when miss         //
	AttackMsg hit_msg;        // messages when hit       //
	AttackMsg god_msg;        // messages when hit on god      //
	AttackMsgSet *next{nullptr};    // to next messages of this kind.   //
};

struct AttackMessages {
	int attack_type{0};                // Attack type          //
	int number_of_attacks{0};            // How many attack messages to chose from. //
	AttackMsgSet *msg_set{nullptr};    // List of messages.       //
};

const int kMaxMessages = 600; // Эту похабень надо переделать на вектор или хотя бы std::array.
extern AttackMessages fight_messages[kMaxMessages];

#endif // SKILLS_INFO_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
