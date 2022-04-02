#ifndef CLASS_HPP_
#define CLASS_HPP_

#include "structs/structs.h"
#include "classes_constants.h"

#include <array>

int invalid_no_class(CharData *ch, const ObjData *obj);
int extra_damroll(int class_num, int level);
int level_exp(CharData *ch, int level);
ECharClass FindAvailableCharClassId(const std::string &class_name);

class GroupPenalties {
 public:
	using class_penalties_t = std::array<int, kMaxRemort + 1>;
	using penalties_t = std::array<class_penalties_t, kNumPlayerClasses>;

	int init();
	const auto &operator[](const size_t character_class) const { return m_grouping[character_class]; }

 private:
	penalties_t m_grouping;
};

extern GroupPenalties grouping;
int thaco(int class_num, int level);

#endif // CLASS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
