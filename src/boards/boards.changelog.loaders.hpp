#ifndef __BOARDS_CHANGELOG_LOADERS_HPP__
#define __BOARDS_CHANGELOG_LOADERS_HPP__

#include "boards.h"

#include <iostream>
#include <memory>

namespace Boards {
class ChangeLogLoader {
 public:
  using shared_ptr = std::shared_ptr<ChangeLogLoader>;

  virtual ~ChangeLogLoader() {}

  virtual bool load(std::istream &is) = 0;

  static shared_ptr create(const std::string &kind, const Board::shared_ptr board);
};
}

#endif // __BOARDS_CHANGELOG_LOADERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
