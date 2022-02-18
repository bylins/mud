#ifndef __CLASS_HPP__
#define __CLASS_HPP__

#include "structs/structs.h"
#include "classes_constants.h"

#include <array>

int invalid_no_class(CharData *ch, const ObjData *obj);
int extra_damroll(int class_num, int level);
void LoadClassSkills();

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

#endif // __CLASS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
