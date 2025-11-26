#ifndef CLASS_HPP_
#define CLASS_HPP_

#include "engine/structs/structs.h"
#include "classes_constants.h"

#include <array>

void advance_level(CharData *ch);
int invalid_no_class(CharData *ch, const ObjData *obj);
int GetExtraDamroll(ECharClass class_id, int level);
long GetExpUntilNextLvl(CharData *ch, int level);
int GetExtraAc0(ECharClass class_id, int level);
ECharClass FindAvailableCharClassId(const std::string &class_name);

class GroupPenalties {
 public:
	using ClassPenalties = std::array<int, kMaxRemort + 1>;
	using Penalties = std::unordered_map<ECharClass, ClassPenalties>;

	int init();
	const auto &operator[](const ECharClass class_id) const { return grouping_.at(class_id); }

 private:
	Penalties grouping_;
};

extern GroupPenalties grouping;
void DoPcInit(CharData *ch, bool is_newbie);
int GetThac0(ECharClass class_id, int level);
bool IsMage(const CharData *ch);
bool IsCaster(const CharData *ch);
bool IsFighter(const CharData *ch);

#endif // CLASS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
