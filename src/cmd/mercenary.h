#ifndef BYLINS_MERCENARY_H
#define BYLINS_MERCENARY_H

#include <map>

class CHAR_DATA;

int mercenary(CHAR_DATA *ch, void * /*me*/, int cmd, char *argument);

struct MERCDATA {
  int CharmedCount; // кол-во раз почармлено
  int spentGold; // если купец - сколько потрачено кун
  int deathCount; // кол-во раз, когда чармис сдох
  int currRemortAvail; // доступен на текущем реморте
  int isFavorite; // моб показывается в сокращенном списке
};

#endif //BYLINS_MERCENARY_H
