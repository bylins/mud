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
#include "structs.h"
#include "interpreter.h"

extern const int LIQ_WATER;
extern const int LIQ_BEER;
extern const int LIQ_WINE;
extern const int LIQ_ALE;
extern const int LIQ_QUAS;
extern const int LIQ_BRANDY;
extern const int LIQ_MORSE;
extern const int LIQ_VODKA;
extern const int LIQ_BRAGA;
extern const int LIQ_MED;
extern const int LIQ_MILK;
extern const int LIQ_TEA;
extern const int LIQ_COFFE;
extern const int LIQ_BLOOD;
extern const int LIQ_SALTWATER;
extern const int LIQ_CLEARWATER;
extern const int LIQ_POTION;
extern const int LIQ_POTION_RED;
extern const int LIQ_POTION_BLUE;
extern const int LIQ_POTION_WHITE;
extern const int LIQ_POTION_GOLD;
extern const int LIQ_POTION_BLACK;
extern const int LIQ_POTION_GREY;
extern const int LIQ_POTION_FUCHSIA;
extern const int LIQ_POTION_PINK;
extern const int LIQ_POISON_ACONITUM;
extern const int LIQ_POISON_SCOPOLIA;
extern const int LIQ_POISON_BELENA;
extern const int LIQ_POISON_DATURA;
extern const int NUM_LIQ_TYPES;

extern const char *drinks[];
extern const char *drinknames[];
extern const int drink_aff[][3];
extern const char *color_liquid[];

void do_drink(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_drunkoff(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_pour(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

void name_from_drinkcon(OBJ_DATA * obj);
void name_to_drinkcon(OBJ_DATA * obj, int type);
bool is_potion(const OBJ_DATA *obj);

class CObjectPrototype;	// to avoit inclusion of "obj.hpp"

namespace drinkcon
{

void identify(CHAR_DATA *ch, const OBJ_DATA *obj);
std::string print_spells(CHAR_DATA *ch, const OBJ_DATA *obj);
void copy_potion_values(const CObjectPrototype *from_obj, CObjectPrototype *to_obj);

} // namespace drinkcon

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
