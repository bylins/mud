#ifndef __ID_HPP__
#define __ID_HPP__

#include <unordered_set>

using object_id_t = long;
using object_id_set_t = std::unordered_set<object_id_t>;

class MaxID {
 public:
  MaxID();

  auto current() const { return m_value; }
  auto allocate() { return ++m_value; }

 private:
  // mob/object id's: MOBOBJ_ID_BASE and higher    //
  static constexpr int MOBOBJ_ID_BASE = 2000000;

  object_id_t m_value;    // for unique mob/obj id's
};

extern MaxID max_id;

#endif // __ID_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
