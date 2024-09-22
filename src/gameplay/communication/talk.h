/**
\file talk.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Обмен репликами, разговор и все такое.
\detail Сюда нужно поместить весь код, связанный с теллми. реплиами в комнату и т.п.
*/

#ifndef BYLINS_SRC_GAMEPLAY_COMMUNICATION_TALK_H_
#define BYLINS_SRC_GAMEPLAY_COMMUNICATION_TALK_H_

class CharData;
void tell_to_char(CharData *keeper, CharData *ch, const char *argument);
bool tell_can_see(CharData *ch, CharData *vict);

#endif //BYLINS_SRC_GAMEPLAY_COMMUNICATION_TALK_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
