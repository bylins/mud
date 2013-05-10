/* ************************************************************************
*   File: pk.cpp                                        Part of Bylins    *
*  Usage: ПК система                                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include <map>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "constants.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "screen.h"
#include "pk.h"
#include "char.hpp"
#include "room.hpp"
#include "house.h"

ACMD(do_revenge);

extern CHAR_DATA *character_list;


#define FirstPK  1
#define SecondPK 5
#define ThirdPK	 10
#define FourthPK 20
#define FifthPK  30
#define KillerPK 25 //уникальных игроков

// Временные константы в минутах реального времени
#define KILLER_UNRENTABLE  30
#define REVENGE_UNRENTABLE 10
#define CLAN_REVENGE       10
#define THIEF_UNRENTABLE   30
#define BATTLE_DURATION    3
#define SPAM_PK_TIME       30
#define PENTAGRAM_TIME     5
#define BLOODY_DURATION    30

// Временные константы в секундах реального времени
#define TIME_PK_GROUP      5

// Временные константы в часах реального времени
#define TIME_GODS_CURSE    192

#define MAX_PKILL_FOR_PERIOD 3

#define MAX_PKILLER_MEM  100
#define MAX_REVENGE      3


int pk_count(CHAR_DATA * ch)
{
	struct PK_Memory_type *pk;
	int i;
	for (i = 0, pk = ch->pk_list; pk; pk = pk->next)
		i += pk->kill_num;
	return i;
}

//Количество убитых игроков (уникальное мыло)
int pk_player_count(CHAR_DATA * ch)
{
	struct PK_Memory_type *pk, *pkg;
	unsigned count = 0;
	for (pk = ch->pk_list; pk; pk = pk->next)
	{
		long i = get_ptable_by_unique(pk->unique);
		bool flag=true;
		for (pkg = pk->next; pkg && flag; pkg = pkg->next)
		{
			long j = get_ptable_by_unique(pkg->unique);
			flag = strcmp(player_table[i].mail, player_table[j].mail)!=0;
		}
		if (flag) ++count;
	}
	return count;
}

int pk_calc_spamm(CHAR_DATA * ch)
{
	struct PK_Memory_type *pk, *pkg;
	int count = 0;
    	for (pk = ch->pk_list; pk; pk = pk->next)
	{
		if (time(NULL) - pk->kill_at <= SPAM_PK_TIME * 60)
		{
			long i = get_ptable_by_unique(pk->unique);
			bool flag = true; //false если запись не соответствует критериям спам пк
			for (pkg = pk->next; pkg && flag; pkg = pkg->next)
			{
				long j = get_ptable_by_unique(pkg->unique);
				//Щитаем убийства со временем больше TIME_PK_GROUP (5 секунд) и чаров с разных мыл
				flag = !(MAX(pk->kill_at, pkg->kill_at) - MIN(pk->kill_at, pkg->kill_at) <= TIME_PK_GROUP || strcmp(player_table[i].mail, player_table[j].mail)==0);
			}
			if (flag)
				++count;
		}
	}
	return (count);
}

void pk_check_spamm(CHAR_DATA * ch)
{
	if (pk_calc_spamm(ch) > MAX_PKILL_FOR_PERIOD)
	{
		SET_GOD_FLAG(ch, GF_GODSCURSE);
		GCURSE_DURATION(ch) = time(0) + TIME_GODS_CURSE * 60 * 60;
		act("Боги прокляли тот день, когда ты появился на свет!", FALSE, ch, 0, 0, TO_CHAR);
	}
	if (pk_player_count(ch) >= KillerPK)
		SET_BIT(PLR_FLAGS(ch, PLR_KILLER), PLR_KILLER);
}

// функция переводит переменные *pkiller и *pvictim на хозяев, если это чармы
void pk_translate_pair(CHAR_DATA * *pkiller, CHAR_DATA * *pvictim)
{
	if (pkiller != NULL && pkiller[0] != NULL)
		if (IS_NPC(pkiller[0]) && pkiller[0]->master &&
				AFF_FLAGGED(pkiller[0], AFF_CHARM) && IN_ROOM(pkiller[0]) == IN_ROOM(pkiller[0]->master))
			pkiller[0] = pkiller[0]->master;

	if (pvictim != NULL && pvictim[0] != NULL)
	{
		if (IS_NPC(pvictim[0]) && pvictim[0]->master &&
				(AFF_FLAGGED(pvictim[0], AFF_CHARM) || IS_HORSE(pvictim[0])))
			if (IN_ROOM(pvictim[0]) == IN_ROOM(pvictim[0]->master))
				pvictim[0] = pvictim[0]->master;
		if (!HERE(pvictim[0]))
			pvictim[0] = NULL;
	}
}

// agressor совершил противоправные действия против victim
// выдать/обновить клан-флаг
void pk_update_clanflag(CHAR_DATA * agressor, CHAR_DATA * victim)
{
	struct PK_Memory_type *pk;

	for (pk = agressor->pk_list; pk; pk = pk->next)
	{
		// эта жертва уже есть в списке игрока ?
		if (pk->unique == GET_UNIQUE(victim))
			break;
	}
	if (!pk)
	{
		CREATE(pk, struct PK_Memory_type, 1);
		pk->unique = GET_UNIQUE(victim);
		pk->next = agressor->pk_list;
		agressor->pk_list = pk;
	}
	if (victim->desc)
	{
		if (pk->clan_exp > time(NULL))
		{
			act("Вы продлили право клановой мести $N2!", FALSE, victim, 0, agressor, TO_CHAR);
			act("$N продлил$G право еще раз отомстить вам!", FALSE, agressor, 0, victim, TO_CHAR);
		}
		else
		{
			act("Вы получили право клановой мести $N2!", FALSE, victim, 0, agressor, TO_CHAR);
			act("$N получил$G право на ваш отстрел!", FALSE, agressor, 0, victim, TO_CHAR);
		}
	}
	pk->clan_exp = time(NULL) + CLAN_REVENGE * 60;

	agressor->save_char();

	return;
}

// victim убил agressor (оба в кланах)
// снять клан-флаг у agressor
void pk_clear_clanflag(CHAR_DATA * agressor, CHAR_DATA * victim)
{
	struct PK_Memory_type *pk;

	for (pk = agressor->pk_list; pk; pk = pk->next)
	{
		// эта жертва уже есть в списке игрока ?
		if (pk->unique == GET_UNIQUE(victim))
			break;
	}
	if (!pk)
		return;		// а мести-то и не было :(

	if (pk->clan_exp > time(NULL))
	{
		act("Вы использовали право клановой мести $N2.", FALSE, victim, 0, agressor, TO_CHAR);
	}
	pk->clan_exp = 0;

	return;
}

// Продлевается время поединка и БД
void pk_update_revenge(CHAR_DATA * agressor, CHAR_DATA * victim, int attime, int renttime)
{
	struct PK_Memory_type *pk;

	for (pk = agressor->pk_list; pk; pk = pk->next)
		if (pk->unique == GET_UNIQUE(victim))
			break;
	if (!pk && !attime && !renttime)
		return;
	if (!pk)
	{
		CREATE(pk, struct PK_Memory_type, 1);
		pk->unique = GET_UNIQUE(victim);
		pk->next = agressor->pk_list;
		agressor->pk_list = pk;
	}
	pk->battle_exp = time(NULL) + attime * 60;
	if (!same_group(agressor, victim))
		RENTABLE(agressor) = MAX(RENTABLE(agressor), time(NULL) + renttime * 60);
	return;
}

// agressor совершил противоправные действия против victim
// 1. выдать флаг
// 2. начать поединок
// 3. если нужно, начать БД
void pk_increment_kill(CHAR_DATA * agressor, CHAR_DATA * victim, int rent, bool flag_temp)
{
	struct PK_Memory_type *pk;

	if (ROOM_FLAGGED(agressor->in_room, ROOM_NOBATTLE) || ROOM_FLAGGED(victim->in_room, ROOM_NOBATTLE))
	{
		may_kill_here(agressor, victim);
		return;
	}

	if (CLAN(agressor) && (CLAN(victim) || flag_temp))
	{
		// межклановая разборка
		pk_update_clanflag(agressor, victim);
	}
	else
	{
		// не все участники приписаны
		for (pk = agressor->pk_list; pk; pk = pk->next)
		{
			// эта жертва уже есть в списке игрока ?
			if (pk->unique == GET_UNIQUE(victim))
				break;
		}
		if (!pk)
		{
			CREATE(pk, struct PK_Memory_type, 1);
			pk->unique = GET_UNIQUE(victim);
			pk->next = agressor->pk_list;
			agressor->pk_list = pk;
		}
		if (victim->desc)
		{
			if (pk->kill_num > 0)
			{
				act("Вы получили право еще раз отомстить $N2!", FALSE, victim, 0, agressor, TO_CHAR);
				act("$N получил$G право еще раз отомстить вам!", FALSE, agressor, 0, victim, TO_CHAR);
			}
			else
			{
				act("Вы получили право отомстить $N2!", FALSE, victim, 0, agressor, TO_CHAR);
				act("$N получил$G право на ваш отстрел!", FALSE, agressor, 0, victim, TO_CHAR);
			}
		}
		pk->kill_num++;
		pk->kill_at = time(NULL);
		// saving first agression room
		AGRESSOR(agressor) = GET_ROOM_VNUM(IN_ROOM(agressor));
		pk_check_spamm(agressor);
	}

// shapirus: прописываем время получения флага агрессора
	AGRO(agressor) = MAX(AGRO(agressor), time(NULL) + KILLER_UNRENTABLE * 60);

	agressor->save_char();

	pk_update_revenge(agressor, victim, BATTLE_DURATION, rent ? KILLER_UNRENTABLE : 0);
	pk_update_revenge(victim, agressor, BATTLE_DURATION, rent ? REVENGE_UNRENTABLE : 0);

	return;
}

// victim реализовал месть на agressor
void pk_decrement_kill(CHAR_DATA * agressor, CHAR_DATA * victim)
{
	struct PK_Memory_type *pk;

	// межклановые разборки
	if (CLAN(agressor) && CLAN(victim))
	{
		// Снимаю клан-флаг у трупа (agressor)
		pk_clear_clanflag(agressor, victim);
		return;
	}

	for (pk = agressor->pk_list; pk; pk = pk->next)
	{
		// эта жертва уже есть в списке игрока ?
		if (pk->unique == GET_UNIQUE(victim))
			break;
	}
	if (!pk)
		return;		// а мести-то и не было :(

	// check temporary flag
	if (CLAN(agressor) && pk->clan_exp > 0)
	{
		// Снимаю клан-флаг у трупа (agressor)
		pk_clear_clanflag(agressor, victim);
		return;
	}

	if (pk->thief_exp > time(NULL))
	{
		act("Вы отомстили $N2 за воровство.", FALSE, victim, 0, agressor, TO_CHAR);
		pk->thief_exp = 0;
		return;
	}

	if (pk->kill_num)
	{
		if (--(pk->kill_num) == 0)
			act("Вы больше не можете мстить $N2.", FALSE, victim, 0, agressor, TO_CHAR);
		pk->revenge_num = 0;
	}
	return;
}

// очередная попытка реализовать месть со стороны agressor
void pk_increment_revenge(CHAR_DATA * agressor, CHAR_DATA * victim)
{
	struct PK_Memory_type *pk;

	for (pk = victim->pk_list; pk; pk = pk->next)
		if (pk->unique == GET_UNIQUE(agressor))
			break;
	if (!pk)
	{
		mudlog("Инкремент реализации без флага мести!", CMP, LVL_GOD, SYSLOG, TRUE);
		return;
	}
	if (CLAN(agressor) && (CLAN(victim) || pk->clan_exp > time(NULL)))
	{
		// межклановая разборка (любая атака после боя - взводится клан-флаг)
		pk_update_clanflag(agressor, victim);
		return;
	}
	if (!pk->kill_num)
	{
		// Попытка мести. Может на самом деле мести и нет,
		// но это ничего страшного
		return;
	}
	act("Вы использовали право мести $N2.", FALSE, agressor, 0, victim, TO_CHAR);
	act("$N отомстил$G вам.", FALSE, victim, 0, agressor, TO_CHAR);
	if (pk->revenge_num == MAX_REVENGE)
	{
		mudlog("Переполнение revenge_num!", CMP, LVL_GOD, SYSLOG, TRUE);
	}
	++(pk->revenge_num);
	return;
}

void pk_increment_gkill(CHAR_DATA * agressor, CHAR_DATA * victim)
{
	if (!AFF_FLAGGED(victim, AFF_GROUP))
	{
		// нападение на одиночку
		pk_increment_kill(agressor, victim, TRUE, false);
	}
	else
	{
		// нападение на члена группы
		CHAR_DATA *leader;
		struct follow_type *f;
		bool has_clanmember = has_clan_members_in_group(victim);

		leader = victim->master ? victim->master : victim;

		if (AFF_FLAGGED(leader, AFF_GROUP) &&
				IN_ROOM(leader) == IN_ROOM(victim) && pk_action_type(agressor, leader) > PK_ACTION_FIGHT)
			pk_increment_kill(agressor, leader, leader == victim, has_clanmember);

		for (f = leader->followers; f; f = f->next)
			if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
					IN_ROOM(f->follower) == IN_ROOM(victim) &&
					pk_action_type(agressor, f->follower) > PK_ACTION_FIGHT)
				pk_increment_kill(agressor, f->follower, f->follower == victim, has_clanmember);
	}
}

void pk_agro_action(CHAR_DATA * agressor, CHAR_DATA * victim)
{

	pk_translate_pair(&agressor, &victim);
	switch (pk_action_type(agressor, victim))
	{
	case PK_ACTION_NO:	// без конфликтов просто выход
		break;

	case PK_ACTION_REVENGE:	// agressor пытается реализовать месть на victim
		pk_increment_revenge(agressor, victim);
		// тут break не нужен, т.к. нужно начать пединок и БД

	case PK_ACTION_FIGHT:	// обе стороны продолжают участвовать в поединке
		// обновить время поединка и БД
		pk_update_revenge(agressor, victim, BATTLE_DURATION, REVENGE_UNRENTABLE);
		pk_update_revenge(victim, agressor, BATTLE_DURATION, REVENGE_UNRENTABLE);
		break;

	case PK_ACTION_KILL:	// agressor начал боевые действия против victim
		// раздача флагов всей группе
		// поединок со всей группой
		// БД только между непосредственными участниками
		pk_increment_gkill(agressor, victim);
		break;
	}

	return;
}

// * Пришлось дублировать функцию для суммона, чтобы спасти душиков, т.е я удалил проверку на душиков
int pk_action_type_summon(CHAR_DATA * agressor, CHAR_DATA * victim)
{
	struct PK_Memory_type *pk;

	pk_translate_pair(&agressor, &victim);
	if (!agressor || !victim || agressor == victim || ROOM_FLAGGED(IN_ROOM(agressor), ROOM_ARENA) || ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA) ||	// предотвращаем баги с чармисами и ареной
			IS_NPC(agressor) || IS_NPC(victim))
		return PK_ACTION_NO;


	for (pk = agressor->pk_list; pk; pk = pk->next)
	{
		if (pk->unique != GET_UNIQUE(victim))
			continue;
		if (pk->battle_exp > time(NULL))
			return PK_ACTION_FIGHT;
		if (pk->revenge_num >= MAX_REVENGE)
			pk_decrement_kill(agressor, victim);
	}

	for (pk = victim->pk_list; pk; pk = pk->next)
	{
		if (pk->unique != GET_UNIQUE(agressor))
			continue;
		if (pk->battle_exp > time(NULL))
			return PK_ACTION_FIGHT;
		if (pk->revenge_num >= MAX_REVENGE)
			pk_decrement_kill(victim, agressor);
		if (CLAN(agressor) &&	// атакующий должен быть в клане
				// CLAN(victim)   && // атакуемый может быть и не в клане
				// это значит, что его исключили на время
				// действия клан-флага
				pk->clan_exp > time(NULL))
			return PK_ACTION_REVENGE;	// месть по клан-флагу
		if (pk->kill_num && !(CLAN(agressor) && CLAN(victim)))
			return PK_ACTION_REVENGE;	// обычная месть
		if (pk->thief_exp > time(NULL))
			return PK_ACTION_REVENGE;	// месть вору
	}

	return PK_ACTION_KILL;
}


void pk_thiefs_action(CHAR_DATA * thief, CHAR_DATA * victim)
{
	struct PK_Memory_type *pk;

	pk_translate_pair(&thief, &victim);
	switch (pk_action_type(thief, victim))
	{
	case PK_ACTION_NO:
		return;

	case PK_ACTION_FIGHT:
	case PK_ACTION_REVENGE:
	case PK_ACTION_KILL:
		// продлить/установить флаг воровства
		for (pk = thief->pk_list; pk; pk = pk->next)
			if (pk->unique == GET_UNIQUE(victim))
				break;
		if (!pk)
		{
			CREATE(pk, struct PK_Memory_type, 1);
			pk->unique = GET_UNIQUE(victim);
			pk->next = thief->pk_list;
			thief->pk_list = pk;
		}
		if (pk->thief_exp == 0)
			act("$N получил$G право на ваш отстрел!", FALSE, thief, 0, victim, TO_CHAR);
		else
			act("$N продлил$G право на ваш отстрел!", FALSE, thief, 0, victim, TO_CHAR);
		pk->thief_exp = time(NULL) + THIEF_UNRENTABLE * 60;
		RENTABLE(thief) = MAX(RENTABLE(thief), time(NULL) + THIEF_UNRENTABLE * 60);
		break;
	}
	return;
}

void pk_revenge_action(CHAR_DATA * killer, CHAR_DATA * victim)
{

	if (killer)
	{
		pk_translate_pair(&killer, NULL);

		if (!IS_NPC(killer) && !IS_NPC(victim))
		{
			// один флаг отработал
			pk_decrement_kill(victim, killer);
			// остановить разборку, убийце установить признак БД
			pk_update_revenge(killer, victim, 0, REVENGE_UNRENTABLE);
		}
	}
	// завершить все поединки, в которых участвовал victim
	for (killer = character_list; killer; killer = killer->next)
	{
		if (IS_NPC(killer))
			continue;
		pk_update_revenge(victim, killer, 0, 0);
		pk_update_revenge(killer, victim, 0, 0);
	}
}

int pk_action_type(CHAR_DATA * agressor, CHAR_DATA * victim)
{
	struct PK_Memory_type *pk;

	pk_translate_pair(&agressor, &victim);
	if (!agressor || !victim || agressor == victim || ROOM_FLAGGED(IN_ROOM(agressor), ROOM_ARENA) || ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA) ||	// предотвращаем баги с чармисами и ареной
			IS_NPC(agressor) || IS_NPC(victim) || (agressor != victim && (ROOM_FLAGGED(agressor->in_room, ROOM_NOBATTLE)
	 			|| ROOM_FLAGGED(victim->in_room, ROOM_NOBATTLE))))
		return PK_ACTION_NO;

	// Душегубов можно бить когда угодно и кому угодно
	// Клановая принадлежность тут ни при чем
	// shapirus: агрессоров тоже можно бить
	if (PLR_FLAGGED(victim, PLR_KILLER) || (AGRO(victim) && RENTABLE(victim)))
		return PK_ACTION_FIGHT;

	for (pk = agressor->pk_list; pk; pk = pk->next)
	{
		if (pk->unique != GET_UNIQUE(victim))
			continue;
		if (pk->battle_exp > time(NULL))
			return PK_ACTION_FIGHT;
		if (pk->revenge_num >= MAX_REVENGE)
			pk_decrement_kill(agressor, victim);
	}

	for (pk = victim->pk_list; pk; pk = pk->next)
	{
		if (pk->unique != GET_UNIQUE(agressor))
			continue;
		if (pk->battle_exp > time(NULL))
			return PK_ACTION_FIGHT;
		if (pk->revenge_num >= MAX_REVENGE)
			pk_decrement_kill(victim, agressor);
		if (CLAN(agressor) &&	// атакующий должен быть в клане
				// CLAN(victim)   && // атакуемый может быть и не в клане
				// это значит, что его исключили на время
				// действия клан-флага
				pk->clan_exp > time(NULL))
			return PK_ACTION_REVENGE;	// месть по клан-флагу
		if (pk->kill_num && !(CLAN(agressor) && CLAN(victim)))
			return PK_ACTION_REVENGE;	// обычная месть
		if (pk->thief_exp > time(NULL))
			return PK_ACTION_REVENGE;	// месть вору
	}

	return PK_ACTION_KILL;
}


const char *CCPK(CHAR_DATA * ch, int lvl, CHAR_DATA * victim)
{
	int i;

	i = pk_count(victim);
	if (i >= FifthPK)
		return CCIRED(ch, lvl);
	else if (i >= FourthPK)
		return CCIMAG(ch, lvl);
	else if (i >= ThirdPK)
		return CCIYEL(ch, lvl);
	else if (i >= SecondPK)
		return CCICYN(ch, lvl);
	else if (i >= FirstPK)
		return CCIGRN(ch, lvl);
	else
		return CCNRM(ch, lvl);
}

void aura(CHAR_DATA * ch, int lvl, CHAR_DATA * victim, char *s)
{
	int i;

	i = pk_count(victim);
	if (i >= FifthPK)
	{
		sprintf(s, "%s(кровавая аура)%s", CCRED(ch, lvl), CCIRED(ch, lvl));
		return;
	}
	else if (i >= FourthPK)
	{
		sprintf(s, "%s(пурпурная аура)%s", CCIMAG(ch, lvl), CCIRED(ch, lvl));
		return;
	}
	else if (i >= ThirdPK)
	{
		sprintf(s, "%s(желтая аура)%s", CCIYEL(ch, lvl), CCIRED(ch, lvl));
		return;
	}
	else if (i >= SecondPK)
	{
		sprintf(s, "%s(голубая аура)%s", CCICYN(ch, lvl), CCIRED(ch, lvl));
		return;
	}
	else if (i >= FirstPK)
	{
		sprintf(s, "%s(зеленая аура)%s", CCIGRN(ch, lvl), CCIRED(ch, lvl));
		return;
	}
	else
	{
		sprintf(s, "%s(чистая аура)%s", CCINRM(ch, lvl), CCIRED(ch, lvl));
		return;
	}
}

// Печать списка пк
void pk_list_sprintf(CHAR_DATA * ch, char *buff)
{
	struct PK_Memory_type *pk;
	char *temp;

	*buff = '\0';
	strcat(buff, "ПК список:\r\n");
	strcat(buff, "              Имя    Kill Rvng Clan Batl Thif\r\n");
	for (pk = ch->pk_list; pk; pk = pk->next)
	{
		temp = get_name_by_unique(pk->unique);
		sprintf(buff + strlen(buff), "%20s %4ld %4ld", temp ? temp : "<УДАЛЕН>", pk->kill_num, pk->revenge_num);
		if (pk->clan_exp > time(NULL))
			sprintf(buff + strlen(buff), " %4ld", pk->clan_exp - time(NULL));
		else
			strcat(buff, "    -");
		if (pk->battle_exp > time(NULL))
			sprintf(buff + strlen(buff), " %4ld", pk->battle_exp - time(NULL));
		else
			strcat(buff, "    -");
		if (pk->thief_exp > time(NULL))
			sprintf(buff + strlen(buff), " %4ld", pk->thief_exp - time(NULL));
		else
			strcat(buff, "    -");
		strcat(buff, "\r\n");
	}
}


ACMD(do_revenge)
{
	CHAR_DATA *tch;
	struct PK_Memory_type *pk;
	int found = FALSE;
	char *temp;
	char arg2[MAX_INPUT_LENGTH];
	bool bOnlineOnly;

	if (IS_NPC(ch))
		return;

	*buf = '\0';
	*arg2 = '\0';
	two_arguments(argument, arg, arg2);

	// отображать только находящихся онлайн
	bOnlineOnly = !(is_abbrev(arg2, "все") || is_abbrev(arg2, "all"));

	// "месть мне [все]"
	// кто может мне отомстить
	if (is_abbrev(arg, "мне") || is_abbrev(arg, "me"))
	{
		if (bOnlineOnly) strcat(buf, "Вам имеют право отомстить (находятся сейчас онлайн):\r\n");
		else strcat(buf, "Вам имеют право отомстить (полный список):\r\n");
		for (pk = ch->pk_list; pk; pk = pk->next)
		{
			// если местей нет, проверяем на БД
			if ((pk->kill_num == 0) && !(pk->battle_exp > time(NULL))) continue;
			temp = get_name_by_unique(pk->unique);
			if (!temp) continue;
			temp[0] = UPPER(temp[0]);
			// если нада исключаем тех, кто находится оффлайн
			if (bOnlineOnly)
			{
				for (tch = character_list; tch; tch = tch->next)
				{
					if (IS_NPC(tch)) continue;
					if (GET_UNIQUE(tch) == pk->unique)
					{
						found = TRUE;

						if (pk->battle_exp > time(NULL))
							sprintf(buf + strlen(buf), "  %-40s <БОЕВЫЕ ДЕЙСТВИЯ>\r\n", temp);
						else
							sprintf(buf + strlen(buf), "  %-40s %3ld %3ld\r\n", temp, pk->kill_num, pk->revenge_num);
						break;
					}
				}
			}
			else
			{
				found = TRUE;

				if (pk->battle_exp > time(NULL))
					sprintf(buf + strlen(buf), "  %-40s <БОЕВЫЕ ДЕЙСТВИЯ>\r\n", temp);
				else
					sprintf(buf + strlen(buf), "  %-40s %3ld %3ld\r\n", temp, pk->kill_num, pk->revenge_num);
			}
		}
		if (found) send_to_char(buf, ch);
		else send_to_char("Вам никто не имеет права отомстить.\r\n", ch);
		return;
	}

	found = FALSE;
	*buf = '\0';
	for (tch = character_list; tch; tch = tch->next)
	{
		if (IS_NPC(tch))
			continue;
		if (*arg && !isname(GET_NAME(tch), arg))
			continue;

		for (pk = tch->pk_list; pk; pk = pk->next)
			if (pk->unique == GET_UNIQUE(ch))
			{
				if (pk->revenge_num >= MAX_REVENGE && pk->battle_exp <= time(NULL))
					pk_decrement_kill(tch, ch);
				// Сначала проверка клан флага
				if (CLAN(ch) && pk->clan_exp > time(NULL))
				{
					sprintf(buf, "  %-40s <ВОЙНА>\r\n", GET_NAME(tch));
				}
				else if (pk->clan_exp > time(NULL))
				{
					sprintf(buf, "  %-40s <ВРЕМЕННЫЙ ФЛАГ>\r\n", GET_NAME(tch));
				}
				else if (pk->kill_num + pk->revenge_num > 0)
				{
					sprintf(buf, "  %-40s %3ld %3ld\r\n",
							GET_NAME(tch), pk->kill_num, pk->revenge_num);
				}
				else
					continue;
				if (!found)
					send_to_char("Вы имеете право отомстить :\r\n", ch);
				send_to_char(buf, ch);
				found = TRUE;
				break;
			}
	}

	if (!found)
		send_to_char("Вам некому мстить.\r\n", ch);
}


ACMD(do_forgive)
{
	CHAR_DATA *tch;
	struct PK_Memory_type *pk;
	int found = FALSE;
	bool bForgive = false;

	if (IS_NPC(ch))
		return;

	if (RENTABLE(ch))
	{
		send_to_char("Вам сначала стоит завершить боевые действия.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("Кого вы хотите простить?\r\n", ch);
		return;
	}

	*buf = '\0';

	for (tch = character_list; tch; tch = tch->next)
	{
		if (IS_NPC(tch))
			continue;
		if (!CAN_SEE_CHAR(ch, tch))
			continue;
		if (!isname(GET_NAME(tch), arg))
			continue;

		// попытка простить самого себя
		if (ch == tch)
		{
			found = false;
			bForgive = true;
			break;
		}

		found = TRUE;
		for (pk = tch->pk_list; pk; pk = pk->next)
			if (pk->unique == GET_UNIQUE(ch))
			{
				// может нам нечего прощать?
				if (pk->kill_num != 0) bForgive = true;
				pk->kill_num = 0;
				pk->kill_at = 0;
				pk->revenge_num = 0;
				pk->battle_exp = 0;
				pk->thief_exp = 0;
				pk->clan_exp = 0;
				break;
			}
		if (bForgive) pk_update_revenge(ch, tch, 0, 0);
		break;
	}

	if (found && bForgive)
	{
		act("Вы великодушно простили $N3.", FALSE, ch, 0, tch, TO_CHAR);
		act("$N великодушно простил$G вас!", FALSE, tch, 0, ch, TO_CHAR);
	}
	else if (found && !bForgive)
		act("$N не нуждается в вашем прощении.", FALSE, ch, 0, tch, TO_CHAR);
	else if (!found && bForgive)
		send_to_char("Для отпущения грехов Боги рекомендуют воспользоваться церковью.\r\n", ch);
	else
		send_to_char("Похоже, этого игрока нет в игре.\r\n", ch);
}

void pk_free_list(CHAR_DATA * ch)
{
	struct PK_Memory_type *pk, *pk_next;

	for (pk = ch->pk_list; pk; pk = pk_next)
	{
		pk_next = pk->next;
		free(pk);
	}
}

// сохранение списка пк-местей в файл персонажа
void save_pkills(CHAR_DATA * ch, FILE * saved)
{
	struct PK_Memory_type *pk, *temp, *tpk;
	CHAR_DATA *tch = NULL;

	fprintf(saved, "Pkil:\n");
	for (pk = ch->pk_list; pk && !PLR_FLAGGED(ch, PLR_DELETED);)
	{
		if (pk->kill_num > 0 && correct_unique(pk->unique))
		{
			if (pk->revenge_num >= MAX_REVENGE && pk->battle_exp <= time(NULL))
			{
				for (tch = character_list; tch; tch = tch->next)
					if (!IS_NPC(tch) && GET_UNIQUE(tch) == pk->unique)
						break;
				if (--(pk->kill_num) == 0 && tch)
					act("Вы больше не можете мстить $N2.", FALSE, tch, 0, ch, TO_CHAR);
			}
			if (pk->kill_num <= 0)
			{
				tpk = pk->next;
				REMOVE_FROM_LIST(pk, ch->pk_list, next);
				free(pk);
				pk = tpk;
				continue;
			}
			fprintf(saved, "%ld %ld\n", pk->unique, pk->kill_num);
		}
		pk = pk->next;
	}
	fprintf(saved, "~\n");
}

// Проверка может ли ch начать аргессивные действия против victim
int may_kill_here(CHAR_DATA * ch, CHAR_DATA * victim)
{
	if (!victim)
		return TRUE;

	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT))
		return (FALSE);

	if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOFIGHT))
	{
		act("Боги предотвратили ваше нападение на $N3.", FALSE, ch, 0, victim, TO_CHAR);
		return (FALSE);
	}
	//запрет на любые агры
	if (ch != victim && (ROOM_FLAGGED(ch->in_room, ROOM_NOBATTLE) || ROOM_FLAGGED(victim->in_room, ROOM_NOBATTLE)))
	{
		send_to_char("Высшие силы воспретили здесь сражаться!\r\n", ch);
		return (FALSE);
	}
	// не агрим чарами <=10 на чаров >=20
	if (!IS_NPC(victim) && !IS_NPC(ch) && GET_LEVEL(ch) <= 10
			&& GET_LEVEL(victim) >= 20 && !(pk_action_type(ch, victim) & (PK_ACTION_REVENGE | PK_ACTION_FIGHT)))
	{
		act("Вы еще слишком слабы, чтобы напасть на $N3.", FALSE, ch, 0, victim, TO_CHAR);
		return (FALSE);
	}

	if ((ch->get_fighting() && ch->get_fighting() == victim) || (victim->get_fighting() && victim->get_fighting() == ch))
		return (TRUE);

	if (ch != victim && !ROOM_FLAGGED(victim->in_room, ROOM_ARENA)
		&& (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) || ROOM_FLAGGED(victim->in_room, ROOM_PEACEFUL)))
	{
		// Один из участников в мирной комнате
		if (MOB_FLAGGED(victim, MOB_HORDE) || (MOB_FLAGGED(ch, MOB_IGNORPEACE) && !AFF_FLAGGED(ch, AFF_CHARM)))
		{
			return TRUE;
		}
		if (IS_GOD(ch) ||
				(IS_NPC(ch) && ch->nr == real_mobile(DG_CASTER_PROXY)) ||
				(pk_action_type(ch, victim) & (PK_ACTION_REVENGE | PK_ACTION_FIGHT)))
			return (TRUE);
		else
		{
			send_to_char("Здесь слишком мирно, чтобы начинать драку...\r\n", ch);
			return (FALSE);
		}
	}
	return (TRUE);
}

// Определяет необходимость вводить
// имя жертвы полностью для начала агродействий
bool need_full_alias(CHAR_DATA * ch, CHAR_DATA * opponent)
{
	// Потенциальная жертва приведет к ПК?
	if (IS_NPC(opponent) && (!opponent->master || IS_NPC(opponent->master)
							 || opponent->master == ch))
		return false;

	// Уже воюю?
	if (ch->get_fighting() == opponent || opponent->get_fighting() == ch)
		return false;

	return true;
}

//Проверка, является ли строка arg полным именем ch
int name_cmp(CHAR_DATA * ch, const char *arg)
{
    char opp_name_part[200] = "\0", opp_name[200] = "\0", *opp_name_remain;
	strcpy(opp_name, GET_NAME(ch));
	for (opp_name_remain = opp_name; *opp_name_remain;)
	{
		opp_name_remain = one_argument(opp_name_remain, opp_name_part);
		if (!str_cmp(arg, opp_name_part))
			return TRUE;
	}
	return FALSE;
}

// Проверка потенциальной возможности агродействий
// TRUE - агродействие разрешено, FALSE - агродействие запрещено
int check_pkill(CHAR_DATA * ch, CHAR_DATA * opponent, const char *arg)
{

	if (!need_full_alias(ch, opponent))
		return TRUE;

	// Имя не указано?
	if (!*arg)
		return TRUE;

	while (*arg && (*arg == '.' || (*arg >= '0' && *arg <= '9')))
		++arg;

    if (name_cmp(opponent, arg))
        return TRUE;
    //Svent: Вынес в отдельную функцию, ибо понадобилась такая же проверка.
	/*strcpy(opp_name, GET_NAME(opponent));
	for (opp_name_remain = opp_name; *opp_name_remain;)
	{
		opp_name_remain = one_argument(opp_name_remain, opp_name_part);
		if (!str_cmp(arg, opp_name_part))
			return TRUE;
	}*/
	// Совпадений не нашел
	send_to_char("Для исключения незапланированной агрессии введите имя жертвы полностью.\r\n", ch);
	return FALSE;
}
int check_pkill(CHAR_DATA * ch, CHAR_DATA * opponent, const std::string &arg)
{
	char opp_name_part[200] = "\0", opp_name[200] = "\0", *opp_name_remain;

	if (!need_full_alias(ch, opponent))
		return TRUE;

	// Имя не указано?
	if (!arg.length())
		return TRUE;

	std::string::const_iterator i;
	for (i = arg.begin(); i != arg.end() && (*i == '.' || (*i >= '0' && *i <= '9')); i++);

	std::string tmp(i, arg.end());

	strcpy(opp_name, GET_NAME(opponent));
	for (opp_name_remain = opp_name; *opp_name_remain;)
	{
		opp_name_remain = one_argument(opp_name_remain, opp_name_part);
		if (!str_cmp(tmp, opp_name_part))
			return TRUE;
	}

	// Совпадений не нашел
	send_to_char("Для исключения незапланированной агрессии введите имя жертвы полностью.\r\n", ch);
	return FALSE;
}

// Проверяет, есть ли члены любого клан в группе чара и находятся ли они
// в одной с ним комнате
bool has_clan_members_in_group(CHAR_DATA * ch)
{
	CHAR_DATA *leader;
	struct follow_type *f;
	leader = ch->master ? ch->master : ch;

	// проверяем, был ли в группе клановый чар
	if (CLAN(leader))
		return true;
	else
		for (f = leader->followers; f; f = f->next)
			if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
					IN_ROOM(f->follower) == IN_ROOM(ch) && CLAN(f->follower))
				return true;
	return false;
}

//Polud
void pkPortal(CHAR_DATA* ch)
{
	AGRO(ch) = MAX(AGRO(ch), time(NULL) + PENTAGRAM_TIME * 60);
	RENTABLE(ch) = MAX(RENTABLE(ch), time(NULL) + PENTAGRAM_TIME * 60);
}


//Структура для хранения информации о кровавом стафе
//Чтобы не добавлять новых полей в OBJ_DATA, объект просто помечается экстрафлагом ITEM_BLOODY и добавляется запись в bloody_map
struct BloodyInfo
{
	long owner_unique; //С кого сняли шмотку
	long kill_at; //Когда произошло убийство
	OBJ_DATA * object; //сама шмотка
	//какой-либо ID вместо указателя хранить не получается, потому что тогда при апдейте таймера кровавости для каждой записи придется искать шмотку по id по списку шмоток
	//А это O(n)
	//По-этому в деструкторе OBJ_DATA нужно удалять запись о шмотке из bloody_map. Такой вот костыль в отсутствие shared_ptr'ов и иже с ними
	BloodyInfo(const long _owner_unique=0, const long _kill_at=0, OBJ_DATA* _object=0):
		owner_unique(_owner_unique), kill_at(_kill_at), object(_object) { }
};

typedef std::map<const OBJ_DATA*, BloodyInfo> BloodyInfoMap;
BloodyInfoMap bloody_map;

//Устанавливает экстрабит кровавому стафу
void set_bloody_flag(OBJ_DATA* list, const CHAR_DATA * ch)
{
	if (!list) return;
	set_bloody_flag(list->contains, ch);
	set_bloody_flag(list->next_content, ch);
	const int t = GET_OBJ_TYPE(list);
	if ((t == ITEM_LIGHT || t == ITEM_WAND || t == ITEM_STAFF || t == ITEM_WEAPON
		|| t == ITEM_ARMOR || (t == ITEM_CONTAINER && GET_OBJ_VAL(list, 0)) || t == ITEM_ARMOR_LIGHT
		|| t == ITEM_ARMOR_MEDIAN || t == ITEM_ARMOR_HEAVY || t == ITEM_INGRADIENT
		|| t == ITEM_WORN) && !IS_OBJ_STAT(list, ITEM_BLOODY))
	{
		SET_BIT(GET_OBJ_EXTRA(list, ITEM_BLOODY), ITEM_BLOODY);
		bloody_map[list].owner_unique = GET_UNIQUE(ch);
		bloody_map[list].kill_at = time(NULL);
		bloody_map[list].object = list;
	}
}

void bloody::update()
{
	const long t = time(NULL);
	BloodyInfoMap::iterator it = bloody_map.begin();
	while (it != bloody_map.end())
	{
		BloodyInfoMap::iterator cur = it++;
		if (t - cur->second.kill_at >= BLOODY_DURATION * 60) //Действие флага заканчивается
		{
			REMOVE_BIT(GET_OBJ_EXTRA(cur->second.object, ITEM_BLOODY), ITEM_BLOODY);
			bloody_map.erase(cur);
		}
	}
}

void bloody::remove_obj(const OBJ_DATA* obj)
{
	BloodyInfoMap::iterator it = bloody_map.find(obj);
	if (it != bloody_map.end())
	{
		REMOVE_BIT(GET_OBJ_EXTRA(it->second.object, ITEM_BLOODY), ITEM_BLOODY);
		bloody_map.erase(it);
	}
}

bool bloody::handle_transfer(CHAR_DATA* ch, CHAR_DATA* victim, OBJ_DATA* obj, OBJ_DATA* container)
{
	CHAR_DATA* initial_ch = ch;
	CHAR_DATA* initial_victim = victim;
	if (!obj || (ch && IS_GOD(ch))) return true;
	pk_translate_pair(&ch, &victim);
	bool result = false;
	BloodyInfoMap::iterator it = bloody_map.find(obj);
	if (!IS_OBJ_STAT(obj, ITEM_BLOODY) || it == bloody_map.end())
		result = true;
	else
	//Если отдаем владельцу или берет владелец
	if (victim && (GET_UNIQUE(victim) == it->second.owner_unique || (CLAN(victim) &&
		(CLAN(victim)->is_clan_member(it->second.owner_unique) || CLAN(victim)->is_alli_member(it->second.owner_unique)))
		|| strcmp(player_table[get_ptable_by_unique(it->second.owner_unique)].mail, GET_EMAIL(victim))==0))
	{
	remove_obj(obj); //снимаем флаг
		result = true;
	}
	else if (!ch && victim) //лут не владельцем
	{
		if (IS_NPC(initial_victim)) //чармисам брать нельзя
			return false;
		AGRO(victim) = MAX(AGRO(victim), KILLER_UNRENTABLE * 60 +it->second.kill_at);
		RENTABLE(victim) = MAX(RENTABLE(victim), KILLER_UNRENTABLE * 60 + it->second.kill_at);
		result = true;
	}
	else if (ch && container && (container->carried_by == ch || container->worn_by == ch)) //чар пытается положить в контейнер в инвентаре или экипировке
		result = true;
	else //нельзя передавать кровавый шмот
	{
		if (ch)
		act("Кровь, покрывающая $o3, намертво въелась вам в руки, не давая избавиться от н$S.", FALSE, ch, obj, 0, TO_CHAR);
		return false;
	}
	//обработка контейнеров
	for (OBJ_DATA* nobj = obj->contains; nobj!=NULL && result; nobj = nobj->next_content)
		result = handle_transfer(initial_ch, initial_victim, nobj);
	return result;
}

void bloody::handle_corpse(OBJ_DATA* corpse, CHAR_DATA* ch, CHAR_DATA* killer)
{
	pk_translate_pair(&ch, &killer);
	//Если игрок убил игрока, который не был в агро бд и убитый не душегуб,
	// то с него выпадает окровавленный стаф
	if (ch && killer
		&& ch != killer
		&& !IS_NPC(ch)
		&& !IS_NPC(killer)
		&& !PLR_FLAGGED(ch, PLR_KILLER)
		&& !AGRO(ch))
	{
		//Проверим, может у killer есть месть на ch
		struct PK_Memory_type *pk = 0;
		for (pk = ch->pk_list; pk; pk = pk->next)
			if (pk->unique == GET_UNIQUE(killer) && (pk->thief_exp > time(NULL) || pk->kill_num))
				break;
		if (!pk && corpse) //не нашли мести
			set_bloody_flag(corpse->contains, ch);
	}
}

bool bloody::is_bloody(const OBJ_DATA* obj)
{
	if (IS_OBJ_STAT(obj, ITEM_BLOODY)) return true;
	bool result = false;
	for (OBJ_DATA* nobj = obj->contains; nobj!=NULL && !result; nobj = nobj->next_content)
		result = is_bloody(nobj);
	return result;
}
