#ifndef SKILLS_INFO_
#define SKILLS_INFO_

#include "skills.h"

#include <unordered_map>
#include <utility>

#include "entities/entity_constants.h"
#include "classes/classes_constants.h"

/*template<class T>
bool IsInRange(const T &t) {
	return (t >= T::kFirst && t <= T::kLast);
}*/

struct SkillInfo {
	SkillInfo() = default;
	SkillInfo(const std::string &name, const std::string &short_name, const ESaving saving, int difficulty, int cap) :
		name(name),
		short_name(short_name),
		save_type(saving),
		difficulty(difficulty),
		cap(cap),
		autosuccess(false) {};

	std::string name{"!undefined!"};
	std::string short_name{"!error"};
	ESaving save_type{ESaving::kFirst};
	int difficulty{1};
	int cap{1};
	bool autosuccess{false};

	/**
	 * Чтобы не писать все время c_str()
	 * Если нужен именно std::string - можно получить напрямую.
	 * По мере избавления от сишных строк вызов функции следует заменять на .name
	 */
	[[nodiscard]] const char* GetName() const { return name.c_str(); };
	[[nodiscard]] const char* GetAbbr() const { return short_name.c_str(); };
};

class SkillsInfo {
 public:
	SkillsInfo() {
		if (!items_) {
			items_ = std::make_unique<Register>();
			items_->emplace(ESkill::kUndefined, std::make_unique<ItemPair>(std::make_pair(false, SkillInfo())));
		}
	};
	SkillsInfo(SkillsInfo &s) = delete;
	void operator=(const SkillsInfo &s) = delete;

	/*
	 *  Доступ к элементу с указанным id или kUndefined элементу.
	 */
	const SkillInfo &operator[](ESkill id) const;

	/*
	 *  Инициализация. Для реинициализации используйте Reload();
	 */
	void Init();

	/*
	 *  Такой id известен. Не гарантируется, что он означает корректный элемент.
	 */
	bool IsKnown(ESkill id);

	/*
	 *  Такой id известен и он корректен, т.е. определен, лежит между первым и последним элементом
	 *  и не откллючен.
	 */
	bool IsValid(ESkill id);

	/*
	 *  Такой id некорректен, т.е. не определен, отключен или лежит вне корректного диапазона.
	 */
	bool IsInvalid(ESkill id) { return !IsValid(id); };

 private:
	using ItemPair = std::pair<bool, SkillInfo>;
	using ItemPairPtr = std::unique_ptr<ItemPair>;
	using Register = std::unordered_map<ESkill, ItemPairPtr>;
	using RegisterPtr = std::unique_ptr<Register>;

	RegisterPtr items_;

	bool IsInitizalized();
	bool IsEnabled(ESkill id);

	void InitSkill(ESkill id, const std::string &name, const std::string &short_name,
				   ESaving saving, int difficulty, int cap, bool enabled = true);
};

// Этому место в структуре скилл_инфо (а еще точнее - абилок), но во-первых, в messages запихали и сообщения спеллов,
// и еще черта лысого в ступе, во-вторых, это надо переделывать структуру и ее парсинг. Поэтому пока так.
struct AttackMsg {
	char *attacker_msg{nullptr};	// message to attacker //
	char *victim_msg{nullptr};		// message to victim   //
	char *room_msg{nullptr};		// message to room     //
};

struct AttackMsgSet {
	AttackMsg die_msg;        // messages when death        //
	AttackMsg miss_msg;        // messages when miss         //
	AttackMsg hit_msg;        // messages when hit       //
	AttackMsg god_msg;        // messages when hit on god      //
	AttackMsgSet *next{nullptr};    // to next messages of this kind.   //
};

struct AttackMessages {
	int attack_type{0};				// Attack type          //
	int number_of_attacks{0};			// How many attack messages to chose from. //
	AttackMsgSet *msg_set{nullptr};	// List of messages.       //
};

const int kMaxMessages = 600; // Эту похабень надо переделать на вектор или хотя бы std::array.
extern AttackMessages fight_messages[kMaxMessages];

#endif // SKILLS_INFO_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
