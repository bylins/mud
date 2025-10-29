/**
\file alias.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_ALIAS_H_
#define BYLINS_SRC_ENGINE_UI_ALIAS_H_

struct alias_data {
  char *alias;
  char *replacement;
  int type;
  struct alias_data *next;
};

inline constexpr int kAliasSimple{0};
inline constexpr int kAliasComplex{1};
inline constexpr char kAliasSepChar{'+'};
inline constexpr char kAliasVarChar{'='};
inline constexpr char kAliasGlobChar{'*'};

class CharData;
void WriteAliases(CharData *ch);
void ReadAliases(CharData *ch);
void FreeAlias(struct alias_data *a);
int PerformAlias(DescriptorData *d, char *orig);
struct alias_data *FindAlias(struct alias_data *alias_list, char *str);

#endif //BYLINS_SRC_ENGINE_UI_ALIAS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
