/**
\file cases.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_GRAMMAR_CASES_H_
#define BYLINS_SRC_ENGINE_GRAMMAR_CASES_H_

#include <memory>
#include <array>
#include <string>
#include <string>

// \TODO Перенести список падажей в неймспейс grammar
enum ECase {
  kNom = 0,
  kGen,
  kDat,
  kAcc,
  kIns,
  kPre,
  kFirstCase = kNom,
  kLastCase = kPre,
};

namespace parser_wrapper {
// forward declaration
class DataNode;
}

namespace grammar {

//! A class for describing individual object, containing declension for singular and plural cases.
class ItemName {
 public:
  ItemName();

  using Ptr = std::unique_ptr<ItemName>;
  using NameCases = std::array<std::string, ECase::kLastCase + 1>;
  ItemName(ItemName &&i) noexcept;
  ItemName &operator=(ItemName &&i) noexcept;

  [[nodiscard]] const std::string &GetSingular(ECase name_case = ECase::kNom) const;
  [[nodiscard]] const std::string &GetPlural(ECase name_case = ECase::kNom) const;
  static Ptr Build(parser_wrapper::DataNode &node);

 private:
  NameCases singular_names_;
  NameCases plural_names_;
};

}

#endif //BYLINS_SRC_ENGINE_GRAMMAR_CASES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
