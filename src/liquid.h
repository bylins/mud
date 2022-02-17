/*************************************************************************
*   File: liquid.hpp                                   Part of Bylins    *
*   Все по жидкостям                                                     *
*                                                                        *
*  $Author$                                                      *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"

// виды жидскостей, наливаемых в контейнеры
enum {
	LIQ_WATER = 0,
	LIQ_BEER = 1,
	LIQ_WINE = 2,
	LIQ_ALE = 3,
	LIQ_QUAS = 4,
	LIQ_BRANDY = 5,
	LIQ_MORSE = 6,
	LIQ_VODKA = 7,
	LIQ_BRAGA = 8,
	LIQ_MED = 9,
	LIQ_MILK = 10,
	LIQ_TEA = 11,
	LIQ_COFFE = 12,
	LIQ_BLOOD = 13,
	LIQ_SALTWATER = 14,
	LIQ_CLEARWATER = 15,
	LIQ_POTION = 16,
	LIQ_POTION_RED = 17,
	LIQ_POTION_BLUE = 18,
	LIQ_POTION_WHITE = 19,
	LIQ_POTION_GOLD = 20,
	LIQ_POTION_BLACK = 21,
	LIQ_POTION_GREY = 22,
	LIQ_POTION_FUCHSIA = 23,
	LIQ_POTION_PINK = 24,
	LIQ_POISON_ACONITUM = 25,
	LIQ_POISON_SCOPOLIA = 26,
	LIQ_POISON_BELENA = 27,
	LIQ_POISON_DATURA = 28,
	NUM_LIQ_TYPES = 29
};

extern const char *drinks[];
extern const char *drinknames[];
extern const int drink_aff[][3];
extern const char *color_liquid[];

void do_drink(CharData *ch, char *argument, int cmd, int subcmd);
void do_drunkoff(CharData *ch, char *argument, int cmd, int subcmd);
void do_pour(CharData *ch, char *argument, int cmd, int subcmd);

void name_from_drinkcon(ObjData *obj);
void name_to_drinkcon(ObjData *obj, int type);
bool is_potion(const ObjData *obj);

class CObjectPrototype;    // to avoit inclusion of "obj.hpp"

namespace drinkcon {

void identify(CharData *ch, const ObjData *obj);
std::string print_spells(CharData *ch, const ObjData *obj);
void copy_potion_values(const CObjectPrototype *from_obj, CObjectPrototype *to_obj);

} // namespace drinkcon

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
