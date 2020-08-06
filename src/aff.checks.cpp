#include "spells.h"
#include "aff.checks.hpp"
#include "structs.h"
#include "comm.h"
#include "sysdep.h"
#include "skills.h"

#include <string>

#define SpINFO spell_info[spellnum]
#define SkINFO skill_info[skillnum]


bool incap_check(CHAR_DATA *ch, int ix)
{
	const int MAX_INCAP_MSG = 5;
	const std::string char_msg[MAX_INCAP_MSG] = {
		"Белый Ангел возник перед вами, маняще помахивая крыльями.\r\n",
		"Рогатый Демон, призывно улыбаясь, правит острие своих вил.\r\n",
		"Вам начали слышаться голоса предков, зовущие вас к себе.\r\n",
		"Безносая аккуратно потыкала косой ваше тело...\r\n",
		"Врата Вальгаллы замаячили пред вами...\r\n"
	};
	if (GET_POS(ch) < POS_SLEEPING)
	{
		int i;
		if (ix < 0 || ix > MAX_INCAP_MSG)
			i = number(0, MAX_INCAP_MSG-1);
		else 
			i = ix;
		send_to_char(char_msg[i].c_str(), ch);
		return true;
	}
	return false;
}

bool sleep_check(CHAR_DATA *ch, int ix)
{
	const int MAX_SLEEP_MSG = 6;
	const std::string char_msg[MAX_SLEEP_MSG] = {
		"Виделся часто сон беспокойный...\r\n",
		"Морфей медленно задумчиво провел рукой по струнам и заиграл колыбельную.\r\n",
		"Вы спите и не могете думать больше ни о чем.\r\n",
		"И снится вам трава, трава у дома..\r\n",
		"И снится вам сияющий огнями вечерний пир в родимой стороне...\r\n",
		"Сделать это в ваших снах?\r\n"
	};
	if (GET_POS(ch) == POS_SLEEPING)
	{
		int i;
		if (ix < 0 || ix > MAX_SLEEP_MSG)
			i = number(0, MAX_SLEEP_MSG-1);
		else 
			i = ix;
		send_to_char(char_msg[i].c_str(), ch);
		return true;
	}
	return false;
}

bool blind_check(CHAR_DATA *ch, int ix)
{
	const int MAX_BLIND_MSG = 5;
	const std::string char_msg[MAX_BLIND_MSG] = {
		"Вы слепы, как котенок!\r\n",
		"Вы все еще слепы...\r\n",
		"Вы ослеплены!\r\n",
		"Вы ничего не видите!\r\n",
		"Вы слепы как крот.\r\n"
	};
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		int i;
		if (ix < 0 || ix > MAX_BLIND_MSG)
			i = number(0, MAX_BLIND_MSG-1);
		else 
			i = ix;
		send_to_char(char_msg[i].c_str(), ch);
		return true;
	}
	return false;
}

bool morph_check(CHAR_DATA *ch, int ix)
{
	return true;
}

bool silence_check(CHAR_DATA *ch, int ix, bool doAct)
{
	const int MAX_SILENCE_MSG = 4;
	const std::string char_msg[MAX_SILENCE_MSG] = {
		"Вы не смогли вымолвить и слова.\r\n",
		"Вы немы, как рыба об лед.\r\n",
		"Вы попытались издать хотя бы звук, но тщетно.\r\n",
		"Вы безуспешно попытались жестами выразить свою мысль.\r\n"
	};
	const std::string act_msg[MAX_SILENCE_MSG] = {
		"$n набрал$g в себя воздух и застыл$g.\r\n",
		"$n начал$g открывать и закрывать рот, как рыба.\r\n",
		"$n напряг$u и покраснел$g, выкатив глаза.\r\n",
		"$n начал$g жестикулировать. Имя что ли хочет своё сказать...\r\n"
	};

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE))
	{
		int i;
		if (ix < 0 || ix > MAX_SILENCE_MSG)
			i = number(0, MAX_SILENCE_MSG-1);
		else 
			i = ix;
		send_to_char(char_msg[i].c_str(), ch);
		if (doAct)
			act(act_msg[i].c_str(), TRUE, ch, NULL, NULL, TO_ROOM | TO_ARENA_LISTEN);
		return true;
	}
	return false;
}
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
