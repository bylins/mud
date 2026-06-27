#ifndef CLASS_HPP_
#define CLASS_HPP_

#include "engine/structs/structs.h"
#include "engine/boot/cfg_manager.h"
#include "classes_constants.h"

#include <array>

class CObjectPrototype;

int invalid_no_class(CharData *ch, const ObjData *obj);
void check_max_hp(CharData *ch);
int invalid_anti_class_proto(CharData *ch, const CObjectPrototype *obj);
int invalid_no_class_proto(CharData *ch, const CObjectPrototype *obj);
int GetExtraDamroll(ECharClass class_id, int level);
int GetExtraAc0(ECharClass class_id, int level);
ECharClass FindAvailableCharClassId(const std::string &class_name);

class GroupPenalties {
 public:
	using ClassPenalties = std::array<int, kMaxRemort + 1>;
	using Penalties = std::unordered_map<ECharClass, ClassPenalties>;

	// Loads cfg/mechanics/group_exp_handicap.xml directly (used by tests); returns 0 on success.
	int init();
	// Fills the table from an already-parsed <group_exp_handicap> node (used by cfg_manager).
	void Load(parser_wrapper::DataNode data);
	[[nodiscard]] bool loaded() const { return loaded_; }
	const auto &operator[](const ECharClass class_id) const { return grouping_.at(class_id); }

 private:
	Penalties grouping_;
	bool loaded_{false};
};

// cfg_manager adapter: loads group_exp_handicap.xml into the global `grouping` (boot + hot reload).
class GroupPenaltiesLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

extern GroupPenalties grouping;
void DoPcInit(CharData *ch, bool is_newbie);
int GetThac0(ECharClass class_id, int level);
bool IsMage(const CharData *ch);
bool IsCaster(const CharData *ch);
bool IsFighter(const CharData *ch);

#endif // CLASS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
