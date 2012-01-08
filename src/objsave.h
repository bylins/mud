// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJSAVE_HPP_INCLUDED
#define OBJSAVE_HPP_INCLUDED

/* these factors should be unique integers */
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

void write_one_object(std::stringstream &out, OBJ_DATA * object, int location);
int Crash_offer_rent(CHAR_DATA * ch, CHAR_DATA * receptionist, int display, int factor, int *totalcost);
void Crash_rentsave(CHAR_DATA * ch, int cost);
void Crash_crashsave(CHAR_DATA * ch);
int Crash_write_timer(int index);

#endif // OBJSAVE_HPP_INCLUDED
