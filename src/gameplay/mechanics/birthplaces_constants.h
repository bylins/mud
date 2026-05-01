//  Part of Bylins http://www.mud.ru
// Лёгкий хедер с константами для точек входа в игру.
// Вынесен из birthplaces.h, чтобы config.h не тащил pugixml в каждый TU.

#ifndef BIRTH_PLACES_CONSTANTS_HPP_INCLUDED
#define BIRTH_PLACES_CONSTANTS_HPP_INCLUDED

// Для тех, у кого нет нормального файла рас, и нет зон.
const int kDefaultLoadroom = 4056;
const int kBirthplaceUndefined = -1;

#endif // BIRTH_PLACES_CONSTANTS_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
