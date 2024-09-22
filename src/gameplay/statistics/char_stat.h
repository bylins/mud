/**
\file char_stat.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_STATISTICS_CHAR_STAT_H_
#define BYLINS_SRC_GAMEPLAY_STATISTICS_CHAR_STAT_H_

#include <array>
#include "engine/structs/structs.h"

class CharData;

class CharStat {
 public:
  enum ECategory {
	PkRip = 0,
	MobRip,
	OtherRip,
	DtRip,
	ArenaRip,
	ArenaWin,
	ArenaDomRip,
	ArenaDomWin,
	ArenaExpLost,
	PkExpLost,
	MobExpLost,
	OtherExpLost,
	DtExpLost,
	PkRemortRip, // Дальше - статистика текущего реморта
	MobRemortRip,
	OtherRemortRip,
	DtRemortRip,
	ArenaRemortRip,
	ArenaRemortWin,
	ArenaDomRemortRip,
	ArenaDomRemortWin,
	ArenaRemortExpLost,
	PkRemortExpLost,
	MobRemortExpLost,
	OtherRemortExpLost,
	DtRemortExpLost,
	CategoryRemortFirst = PkRemortRip, // ВНИМАНИЕ, не забываем менять
	CategoryLast = DtRemortExpLost // Не забываем менять
  };

  CharStat() { Clear(); };
// \todo Добавить проверку на переполнение
  void Increase(ECategory category, ullong increment);
  [[nodiscard]] ullong GetValue(ECategory category) const;
  void Clear();
  void ClearThisRemort();
  static void UpdateOnKill(CharData *ch, CharData *killer, ullong dec_exp);
  static void Print(CharData *ch);

 private:
  std::array<ullong, ECategory::CategoryLast + 1> statistics_{};
};

#endif //BYLINS_SRC_GAMEPLAY_STATISTICS_CHAR_STAT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
