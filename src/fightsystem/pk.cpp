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

#include "pk.h"

#include "structs/global_objects.h"
#include "color.h"
#include "house.h"
#include "handler.h"
#include "fightsystem/fight.h"
#include "game_classes/classes.h"

void SetWait(CharData *ch, int waittime, int victim_in_room);

#define FirstPK  1
#define SecondPK 5
#define ThirdPK     10
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

int pk_count(CharData *ch) {
	struct PK_Memory_type *pk;
	int i;
	for (i = 0, pk = ch->pk_list; pk; pk = pk->next) {
		i += pk->kill_num;
	}
	return i;
}

bool check_agrobd(CharData *ch) {
	if (ch->agrobd)
		return true;
	return false;
}

PK_Memory_type *findPKEntry(CharData *agressor, CharData *victim) {
//	struct PK_Memory_type* pk;
	for (auto pk = agressor->pk_list; pk; pk = pk->next) {
		if (pk->unique == GET_UNIQUE(victim)) {
			return pk;
		}
	}
	return nullptr;
}

//Количество убитых игроков (уникальное мыло) int
int pk_player_count(CharData *ch) {
	struct PK_Memory_type *pk, *pkg;
	unsigned count = 0;
	for (pk = ch->pk_list; pk; pk = pk->next) {
		long i = get_ptable_by_unique(pk->unique);
		bool flag = true;
		for (pkg = pk->next; pkg && flag; pkg = pkg->next) {
			long j = get_ptable_by_unique(pkg->unique);
			flag = strcmp(player_table[i].mail, player_table[j].mail) != 0;
		}
		if (flag) ++count;
	}
	return count;
}

int pk_calc_spamm(CharData *ch) {
	struct PK_Memory_type *pk, *pkg;
	int count = 0;
	for (pk = ch->pk_list; pk; pk = pk->next) {
		if (time(nullptr) - pk->kill_at <= SPAM_PK_TIME * 60) {
			long i = get_ptable_by_unique(pk->unique);
			bool spamPK = true;
			for (pkg = pk->next; pkg && spamPK; pkg = pkg->next) {
				long j = get_ptable_by_unique(pkg->unique);
				// Cчитаем убийства со временем больше TIME_PK_GROUP (5 секунд) и чаров с разных мыл
				spamPK = !(MAX(pk->kill_at, pkg->kill_at) - MIN(pk->kill_at, pkg->kill_at) <= TIME_PK_GROUP
					|| strcmp(player_table[i].mail, player_table[j].mail) == 0);
			}
			if (spamPK) {
				++count;
			}
		}
	}
	return (count);
}

void pk_check_spamm(CharData *ch) {
	if (pk_calc_spamm(ch) > MAX_PKILL_FOR_PERIOD) {
		SET_GOD_FLAG(ch, GF_GODSCURSE);
		GCURSE_DURATION(ch) = time(0) + TIME_GODS_CURSE * 60 * 60;
		act("Боги прокляли тот день, когда ты появился на свет!", false, ch, 0, 0, kToChar);
	}
	if (pk_player_count(ch) >= KillerPK) {
		PLR_FLAGS(ch).set(PLR_KILLER);
	}
}

// функция переводит переменные *pkiller и *pvictim на хозяев, если это чармы
void pk_translate_pair(CharData **pkiller, CharData **pvictim) {
	if (pkiller != nullptr && pkiller[0] != nullptr && !pkiller[0]->purged()) {
		if (IS_NPC(pkiller[0])
			&& pkiller[0]->has_master()
			&& AFF_FLAGGED(pkiller[0], EAffectFlag::AFF_CHARM)
			&& IN_ROOM(pkiller[0]) == IN_ROOM(pkiller[0]->get_master())) {
			pkiller[0] = pkiller[0]->get_master();
		}
	}
	if (pvictim != nullptr && pvictim[0] != nullptr && !pvictim[0]->purged()) {
		if (IS_NPC(pvictim[0])
			&& pvictim[0]->has_master()
			&& (AFF_FLAGGED(pvictim[0], EAffectFlag::AFF_CHARM)
				|| IS_HORSE(pvictim[0]))) {
			if (IN_ROOM(pvictim[0]) == IN_ROOM(pvictim[0]->get_master())) {
				if (HERE(pvictim[0]->get_master()))
					pvictim[0] = pvictim[0]->get_master();
			}
		}

		if (!HERE(pvictim[0])) {
			pvictim[0] = nullptr;
		}
	}
}

// agressor совершил противоправные действия против victim
// выдать/обновить клан-флаг
void pk_update_clanflag(CharData *agressor, CharData *victim) {
	struct PK_Memory_type *pk = findPKEntry(agressor, victim);

	if (!pk && (!IS_GOD(victim))) {
		CREATE(pk, 1);
		pk->unique = GET_UNIQUE(victim);
		pk->next = agressor->pk_list;
		agressor->pk_list = pk;
	}

	if (victim->desc && (!IS_GOD(victim))) {
		if (pk->clan_exp > time(nullptr)) {
			act("Вы продлили право клановой мести $N2!", false, victim, 0, agressor, kToChar);
			act("$N продлил$G право еще раз отомстить вам!", false, agressor, 0, victim, kToChar);
		} else {
			act("Вы получили право клановой мести $N2!", false, victim, 0, agressor, kToChar);
			act("$N получил$G право на ваш отстрел!", false, agressor, 0, victim, kToChar);
		}
	}
	if (pk) {
		pk->clan_exp = time(nullptr) + CLAN_REVENGE * 60;
	}
	agressor->save_char();
	agressor->agrobd = true;
	return;
}

// victim убил agressor (оба в кланах)
// снять клан-флаг у agressor
void pk_clear_clanflag(CharData *agressor, CharData *victim) {
	struct PK_Memory_type *pk = findPKEntry(agressor, victim);
	if (!pk)
		return;

	if (pk->clan_exp > time(nullptr)) {
		act("Вы использовали право клановой мести $N2.", false, victim, 0, agressor, kToChar);
	}
	pk->clan_exp = 0;

	return;
}

// Продлевается время поединка и БД
void pk_update_revenge(CharData *agressor, CharData *victim, int attime, int renttime) {
	struct PK_Memory_type *pk = findPKEntry(agressor, victim);
	if (!pk && !attime && !renttime) {
		return;
	}
	if (!pk) {
		CREATE(pk, 1);
		pk->unique = GET_UNIQUE(victim);
		pk->next = agressor->pk_list;
		agressor->pk_list = pk;
	}
	pk->battle_exp = time(nullptr) + attime * 60;
	if (!same_group(agressor, victim)) {
		NORENTABLE(agressor) = MAX(NORENTABLE(agressor), time(nullptr) + renttime * 60);
	}
	return;
}

// agressor совершил противоправные действия против victim
// 1. выдать флаг
// 2. начать поединок
// 3. если нужно, начать БД
void pk_increment_kill(CharData *agressor, CharData *victim, int rent, bool flag_temp) {

	if (ROOM_FLAGGED(agressor->in_room, ROOM_NOBATTLE) || ROOM_FLAGGED(victim->in_room, ROOM_NOBATTLE)) {
		may_kill_here(agressor, victim, NoArgument);
		return;
	}

	if (CLAN(agressor) && (CLAN(victim) || flag_temp)) {
		pk_update_clanflag(agressor, victim);
	} else {
		struct PK_Memory_type *pk = findPKEntry(agressor, victim);
		if (!pk && (!IS_GOD(victim))) {
			CREATE(pk, 1);
			pk->unique = GET_UNIQUE(victim);
			pk->next = agressor->pk_list;
			agressor->pk_list = pk;
		}
		if (victim->desc) {
			if (pk->kill_num > 0) {
				act("Вы получили право еще раз отомстить $N2!", false, victim, 0, agressor, kToChar);
				act("$N получил$G право еще раз отомстить вам!", false, agressor, 0, victim, kToChar);
			} else {
				act("Вы получили право отомстить $N2!", false, victim, 0, agressor, kToChar);
				act("$N получил$G право на ваш отстрел!", false, agressor, 0, victim, kToChar);
			}
		}
		pk->kill_num++;
		pk->kill_at = time(nullptr);
		// saving first agression room
		AGRESSOR(agressor) = GET_ROOM_VNUM(IN_ROOM(agressor));
		pk_check_spamm(agressor);
	}

// shapirus: прописываем время получения флага агрессора
	AGRO(agressor) = MAX(AGRO(agressor), time(nullptr) + KILLER_UNRENTABLE * 60);
	//  вешаем агробд на агрессора
	agressor->agrobd = true;
	pk_update_revenge(agressor, victim, BATTLE_DURATION, rent ? KILLER_UNRENTABLE : 0);
	pk_update_revenge(victim, agressor, BATTLE_DURATION, rent ? REVENGE_UNRENTABLE : 0);
	//Костыль cнимаем цацки недоступные и кладем в чара.
	for (int i = 0; i < NUM_WEARS; i++) {
		ObjData *p_item;
		if (GET_EQ(agressor, i)) {
			p_item = GET_EQ(agressor, i);
			if (invalid_no_class(agressor, p_item)) {
				obj_to_char(unequip_char(agressor, i, CharEquipFlags()), agressor);
				remove_otrigger(p_item, agressor);
			}
		}
		if (GET_EQ(victim, i)) {
			p_item = GET_EQ(victim, i);
			if (invalid_no_class(victim, p_item)) {
				obj_to_char(unequip_char(victim, i, CharEquipFlags()), victim);
				remove_otrigger(p_item, victim);
			}
		}
	}
	agressor->save_char();
	victim->save_char();
	return;
}

void pk_decrement_kill(CharData *agressor, CharData *victim) {

	if (CLAN(agressor) && CLAN(victim)) {
		pk_clear_clanflag(agressor, victim);
		return;
	}

	struct PK_Memory_type *pk = findPKEntry(agressor, victim);
	if (!pk) {
		return;
	}

	if (CLAN(agressor) && pk->clan_exp > 0) {
		pk_clear_clanflag(agressor, victim);
		return;
	}

	if (pk->thief_exp > time(nullptr)) {
		act("Вы отомстили $N2 за воровство.", false, victim, 0, agressor, kToChar);
		pk->thief_exp = 0;
		return;
	}

	if (pk->kill_num) {
		if (--(pk->kill_num) == 0) {
			act("Вы больше не можете мстить $N2.", false, victim, 0, agressor, kToChar);
		}
		pk->revenge_num = 0;
	}
	return;
}

// очередная попытка реализовать месть со стороны agressor
int pk_increment_revenge(CharData *agressor, CharData *victim) {
	struct PK_Memory_type *pk;

	for (pk = victim->pk_list; pk; pk = pk->next) {
		if (pk->unique == GET_UNIQUE(agressor)) {
			break;
		}
	}
	if (!pk) {
		mudlog("Инкремент реализации без флага мести!", CMP, kLvlGod, SYSLOG, true);
		return 0;
	}
	if (CLAN(agressor) && (CLAN(victim) || pk->clan_exp > time(nullptr))) {
		pk_update_clanflag(agressor, victim);
		return 0;
	}
	if (!pk->kill_num) {
		return 0;
	}
	act("Вы использовали право мести $N2.", false, agressor, 0, victim, kToChar);
	act("$N отомстил$G вам.", false, victim, 0, agressor, kToChar);
	++(pk->revenge_num);
	return pk->revenge_num;
}

void pk_increment_gkill(CharData *agressor, CharData *victim) {
	if (!AFF_FLAGGED(victim, EAffectFlag::AFF_GROUP)) {
		pk_increment_kill(agressor, victim, true, false);
		return;
	}

	CharData *leader;
	struct Follower *f;
	bool has_clanmember = false;
	if (!IS_GOD(victim)) {
		has_clanmember = has_clan_members_in_group(victim);
	}

	leader = victim->has_master() ? victim->get_master() : victim;

	if (AFF_FLAGGED(leader, EAffectFlag::AFF_GROUP)
		&& IN_ROOM(leader) == IN_ROOM(victim)
		&& pk_action_type(agressor, leader) > PK_ACTION_FIGHT) {
		pk_increment_kill(agressor, leader, leader == victim, has_clanmember);
	}
	for (f = leader->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->ch, EAffectFlag::AFF_GROUP)
			&& IN_ROOM(f->ch) == IN_ROOM(victim)
			&& pk_action_type(agressor, f->ch) > PK_ACTION_FIGHT) {
			pk_increment_kill(agressor, f->ch, f->ch == victim, has_clanmember);
		}
	}
}

bool pk_agro_action(CharData *agressor, CharData *victim) {
	int pkType = 0;
	pk_translate_pair(&agressor, &victim);
	if (victim == nullptr) {
		return false;
	}
	// если клан-замок - выдворяем за пределы
	if (ROOM_FLAGGED(agressor->in_room, ROOM_HOUSE) && !ROOM_FLAGGED(agressor->in_room, ROOM_ARENA) && CLAN(agressor)) {
		if (victim->get_fighting() != nullptr)
			stop_fighting(victim, false);
		if (victim->get_fighting() != nullptr)
			stop_fighting(agressor, false);
		act("$n был$g выдворен$a за пределы замка!", true, agressor, 0, 0, kToRoom);
		char_from_room(agressor);
		if (IS_FEMALE(agressor))
			send_to_char("Охолонись малая, на своих бросаться не дело!\r\n", agressor);
		else
			send_to_char("Охолонись малец, на своих бросаться не дело!\r\n", agressor);
		send_to_char("Защитная магия взяла вас за шиворот и выкинула вон из замка!\r\n", agressor);
		char_to_room(agressor, real_room(CLAN(agressor)->out_rent));
		look_at_room(agressor, real_room(CLAN(agressor)->out_rent));
		act("$n свалил$u с небес, выкрикивая какие-то ругательства!", true, agressor, 0, 0, kToRoom);
		SetWait(agressor, 1, true);
		return false;
	}

	pkType = pk_action_type(agressor, victim);
	log("pk_agro_action: %s vs %s, pkType = %d", GET_NAME(agressor), GET_NAME(victim), pkType);

	switch (pkType) {
		case PK_ACTION_NO: break;

		case PK_ACTION_REVENGE:
			if (pk_increment_revenge(agressor, victim) >= MAX_REVENGE) {
				pk_decrement_kill(agressor, victim);
			}
			pk_update_revenge(agressor, victim, BATTLE_DURATION, REVENGE_UNRENTABLE);
			pk_update_revenge(victim, agressor, BATTLE_DURATION, REVENGE_UNRENTABLE);
			break;
		case PK_ACTION_FIGHT: pk_update_revenge(agressor, victim, BATTLE_DURATION, REVENGE_UNRENTABLE);
			pk_update_revenge(victim, agressor, BATTLE_DURATION, REVENGE_UNRENTABLE);
			break;

		case PK_ACTION_KILL:
			if (IS_GOD(agressor) || IS_GOD(victim)) {
				break;
			}
			pk_increment_gkill(agressor, victim);
			break;
	}

	return true;
}

// * Пришлось дублировать функцию для суммона, чтобы спасти душиков, т.е я удалил проверку на душиков
int pk_action_type_summon(CharData *agressor, CharData *victim) {
	struct PK_Memory_type *pk;

	pk_translate_pair(&agressor, &victim);
	if (victim == nullptr) {
		return false;
	}

	if (!agressor || !victim || agressor == victim
		|| ROOM_FLAGGED(IN_ROOM(agressor), ROOM_ARENA)
		|| ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA)
		|| IS_NPC(agressor) || IS_NPC(victim)) {
		return PK_ACTION_NO;
	}

	for (pk = agressor->pk_list; pk; pk = pk->next) {
		if (pk->unique != GET_UNIQUE(victim))
			continue;
		if (pk->battle_exp > time(nullptr))
			return PK_ACTION_FIGHT;
	}

	for (pk = victim->pk_list; pk; pk = pk->next) {
		if (pk->unique != GET_UNIQUE(agressor))
			continue;
		if (pk->battle_exp > time(nullptr))
			return PK_ACTION_FIGHT;
		if (CLAN(agressor) &&    // атакующий должен быть в клане
			// CLAN(victim)   && // атакуемый может быть и не в клане
			// это значит, что его исключили на время
			// действия клан-флага
			pk->clan_exp > time(nullptr))
			return PK_ACTION_REVENGE;    // месть по клан-флагу
		if (pk->kill_num && !(CLAN(agressor) && CLAN(victim)) && !IS_GOD(agressor))
			return PK_ACTION_REVENGE;    // обычная месть
		if (pk->thief_exp > time(nullptr) && (!IS_GOD(agressor)))
			return PK_ACTION_REVENGE;    // месть вору
	}

	return PK_ACTION_KILL;
}

void pk_thiefs_action(CharData *thief, CharData *victim) {
	struct PK_Memory_type *pk;

	pk_translate_pair(&thief, &victim);
	if (victim == nullptr) {
//		mudlog("Противник исчез при ПК куда-то! функция 3", CMP, kLevelGod, SYSLOG, true);
		return;
	}

	switch (pk_action_type(thief, victim)) {
		case PK_ACTION_NO: return;

		case PK_ACTION_FIGHT:
		case PK_ACTION_REVENGE:
		case PK_ACTION_KILL:
			// продлить/установить флаг воровства
			for (pk = thief->pk_list; pk; pk = pk->next)
				if (pk->unique == GET_UNIQUE(victim))
					break;
			if (!pk && (!IS_GOD(victim)) && (!IS_GOD(thief))) {
				CREATE(pk, 1);
				pk->unique = GET_UNIQUE(victim);
				pk->next = thief->pk_list;
				thief->pk_list = pk;
			} else
				break;
			if (pk->thief_exp == 0)
				act("$N получил$G право на ваш отстрел!", false, thief, 0, victim, kToChar);
			else
				act("$N продлил$G право на ваш отстрел!", false, thief, 0, victim, kToChar);
			pk->thief_exp = time(nullptr) + THIEF_UNRENTABLE * 60;
			NORENTABLE(thief) = MAX(NORENTABLE(thief), time(nullptr) + THIEF_UNRENTABLE * 60);
			break;
	}
	return;
}

void pk_revenge_action(CharData *killer, CharData *victim) {

	if (killer) {
		pk_translate_pair(&killer, nullptr);
		if (victim == nullptr) {
			return;
		}

		if (!IS_NPC(killer) && !IS_NPC(victim)) {
			// один флаг отработал
			pk_decrement_kill(victim, killer);
			// остановить разборку, убийце установить признак БД
			pk_update_revenge(killer, victim, 0, REVENGE_UNRENTABLE);
		}
	}

	// завершить все поединки, в которых участвовал victim
	for (const auto &killer : character_list) {
		if (IS_NPC(killer)) {
			continue;
		}

		pk_update_revenge(victim, killer.get(), 0, 0);
		pk_update_revenge(killer.get(), victim, 0, 0);
	}
}

int pk_action_type(CharData *agressor, CharData *victim) {
	struct PK_Memory_type *pk;

	pk_translate_pair(&agressor, &victim);

	if (!agressor || !victim || agressor == victim
		|| ROOM_FLAGGED(IN_ROOM(agressor), ROOM_ARENA)
		|| ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA)
		|| IS_NPC(agressor) || IS_NPC(victim)
		|| (agressor != victim
			&& (ROOM_FLAGGED(agressor->in_room, ROOM_NOBATTLE) || ROOM_FLAGGED(victim->in_room, ROOM_NOBATTLE))))
		return PK_ACTION_NO;

	if (PLR_FLAGGED(victim, PLR_KILLER) || (AGRO(victim) && NORENTABLE(victim)))
		return PK_ACTION_FIGHT;

	for (pk = agressor->pk_list; pk; pk = pk->next) {
		if (pk->unique != GET_UNIQUE(victim))
			continue;
		if (pk->battle_exp > time(nullptr))
			return PK_ACTION_FIGHT;
	}

	for (pk = victim->pk_list; pk; pk = pk->next) {
		if (pk->unique != GET_UNIQUE(agressor))
			continue;
		if (pk->battle_exp > time(nullptr))
			return PK_ACTION_FIGHT;
		if (CLAN(agressor) && pk->clan_exp > time(nullptr))
			return PK_ACTION_REVENGE;    // месть по клан-флагу
		if (pk->kill_num && !(CLAN(agressor) && CLAN(victim)))
			return PK_ACTION_REVENGE;    // обычная месть
		if (pk->thief_exp > time(nullptr))
			return PK_ACTION_REVENGE;    // месть вору
	}

	return PK_ACTION_KILL;
}

const char *CCPK(CharData *ch, int lvl, CharData *victim) {
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

void aura(CharData *ch, int lvl, CharData *victim, char *s) {
	int i;

	i = pk_count(victim);
	if (i >= FifthPK) {
		sprintf(s, "%s(кровавая аура)%s", CCRED(ch, lvl), CCIRED(ch, lvl));
		return;
	} else if (i >= FourthPK) {
		sprintf(s, "%s(пурпурная аура)%s", CCIMAG(ch, lvl), CCIRED(ch, lvl));
		return;
	} else if (i >= ThirdPK) {
		sprintf(s, "%s(желтая аура)%s", CCIYEL(ch, lvl), CCIRED(ch, lvl));
		return;
	} else if (i >= SecondPK) {
		sprintf(s, "%s(голубая аура)%s", CCICYN(ch, lvl), CCIRED(ch, lvl));
		return;
	} else if (i >= FirstPK) {
		sprintf(s, "%s(зеленая аура)%s", CCIGRN(ch, lvl), CCIRED(ch, lvl));
		return;
	} else {
		sprintf(s, "%s(чистая аура)%s", CCINRM(ch, lvl), CCIRED(ch, lvl));
		return;
	}
}

// Печать списка пк
void pk_list_sprintf(CharData *ch, char *buff) {
	struct PK_Memory_type *pk;

	*buff = '\0';
	strcat(buff, "ПК список:\r\n");
	strcat(buff, "              Имя    Kill Rvng Clan Batl Thif\r\n");
	for (pk = ch->pk_list; pk; pk = pk->next) {
		const char *temp = get_name_by_unique(pk->unique);
		sprintf(buff + strlen(buff), "%20s %4ld %4ld", temp ? temp : "<УДАЛЕН>", pk->kill_num, pk->revenge_num);

		if (pk->clan_exp > time(nullptr)) {
			sprintf(buff + strlen(buff), " %4ld", static_cast<long>(pk->clan_exp - time(nullptr)));
		} else {
			strcat(buff, "    -");
		}

		if (pk->battle_exp > time(nullptr)) {
			sprintf(buff + strlen(buff), " %4ld", static_cast<long>(pk->battle_exp - time(nullptr)));
		} else {
			strcat(buff, "    -");
		}

		if (pk->thief_exp > time(nullptr)) {
			sprintf(buff + strlen(buff), " %4ld", static_cast<long>(pk->thief_exp - time(nullptr)));
		} else {
			strcat(buff, "    -");
		}

		strcat(buff, "\r\n");
	}
}

void do_revenge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	struct PK_Memory_type *pk;
	int found = false;
	char arg2[kMaxInputLength];
	bool bOnlineOnly;

	if (IS_NPC(ch))
		return;

	*buf = '\0';
	*arg2 = '\0';
	two_arguments(argument, arg, arg2);

	// отображать только находящихся онлайн
	bOnlineOnly = !(utils::IsAbbrev(arg2, "все") || utils::IsAbbrev(arg2, "all"));

	// "месть мне [все]"
	// кто может мне отомстить
	if (utils::IsAbbrev(arg, "мне") || utils::IsAbbrev(arg, "me")) {
		if (bOnlineOnly) {
			strcat(buf, "Вам имеют право отомстить (находятся сейчас онлайн):\r\n");
		} else {
			strcat(buf, "Вам имеют право отомстить (полный список):\r\n");
		}
		for (pk = ch->pk_list; pk; pk = pk->next) {
			// если местей нет, проверяем на БД
			if ((pk->kill_num == 0) && !(pk->battle_exp > time(nullptr))) {
				continue;
			}

			const char *temp = get_name_by_unique(pk->unique);
			if (!temp) {
				continue;
			}

			const_cast<char *>(temp)[0] = UPPER(temp[0]);
			// если нада исключаем тех, кто находится оффлайн
			if (bOnlineOnly) {
				for (const auto &tch : character_list) {
					if (IS_NPC(tch)) {
						continue;
					}

					if (GET_UNIQUE(tch) == pk->unique) {
						found = true;
						if (pk->battle_exp > time(nullptr)) {
							sprintf(buf + strlen(buf), "  %-40s <БОЕВЫЕ ДЕЙСТВИЯ>\r\n", temp);
						} else {
							sprintf(buf + strlen(buf), "  %-40s %3ld %3ld\r\n", temp, pk->kill_num, pk->revenge_num);
						}
						break;
					}
				}
			} else {
				found = true;
				if (pk->battle_exp > time(nullptr)) {
					sprintf(buf + strlen(buf), "  %-40s <БОЕВЫЕ ДЕЙСТВИЯ>\r\n", temp);
				} else {
					sprintf(buf + strlen(buf), "  %-40s %3ld %3ld\r\n", temp, pk->kill_num, pk->revenge_num);
				}
			}
		}

		if (found) {
			send_to_char(buf, ch);
		} else {
			send_to_char("Ни у кого нет права мести вам.\r\n", ch);
		}

		return;
	}

	found = false;
	*buf = '\0';
	for (const auto &tch : character_list) {
		if (IS_NPC(tch)) {
			continue;
		}
		if (*arg && !isname(GET_NAME(tch), arg)) {
			continue;
		}

		for (pk = tch->pk_list; pk; pk = pk->next) {
			if (pk->unique == GET_UNIQUE(ch)) {
				if (pk->revenge_num >= MAX_REVENGE && pk->battle_exp <= time(nullptr)) {
					pk_decrement_kill(tch.get(), ch);
				}

				// Сначала проверка клан флага
				if (CLAN(ch) && pk->clan_exp > time(nullptr)) {
					sprintf(buf + strlen(buf), "  %-40s <ВОЙНА>\r\n", GET_NAME(tch));
				} else if (pk->clan_exp > time(nullptr)) {
					sprintf(buf + strlen(buf), "  %-40s <ВРЕМЕННЫЙ ФЛАГ>\r\n", GET_NAME(tch));
				} else if (pk->kill_num + pk->revenge_num > 0) {
					sprintf(buf + strlen(buf), "  %-40s %3ld %3ld\r\n",
							GET_NAME(tch), pk->kill_num, pk->revenge_num);
				} else {
					continue;
				}

				if (!found)
					send_to_char("Вы имеете право отомстить :\r\n", ch);
				send_to_char(buf, ch);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		send_to_char("Вам некому мстить.\r\n", ch);
	}
}

void do_forgive(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	struct PK_Memory_type *pk;
	bool bForgive = false;

	if (IS_NPC(ch))
		return;

	if (NORENTABLE(ch)) {
		send_to_char("Вам сначала стоит завершить боевые действия.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg) {
		send_to_char("Кого вы хотите простить?\r\n", ch);
		return;
	}

	*buf = '\0';

	CharData *found = nullptr;
	for (const auto &tch : character_list) {
		if (IS_NPC(tch)
			|| !CAN_SEE_CHAR(ch, tch)
			|| !isname(GET_NAME(tch), arg)) {
			continue;
		}

		// попытка простить самого себя
		if (ch == tch.get()) {
			bForgive = true;

			break;
		}

		found = tch.get();
		for (pk = tch->pk_list; pk; pk = pk->next) {
			if (pk->unique == GET_UNIQUE(ch)) {
				// может нам нечего прощать?
				if (pk->kill_num != 0) {
					bForgive = true;
				}
				pk->kill_num = 0;
				pk->kill_at = 0;
				pk->revenge_num = 0;
				pk->battle_exp = 0;
				pk->thief_exp = 0;
				pk->clan_exp = 0;
				break;
			}
		}

		if (bForgive) {
			pk_update_revenge(ch, tch.get(), 0, 0);
		}

		break;
	}

	if (found) {
		if (bForgive) {
			act("Вы великодушно простили $N3.", false, ch, 0, found, kToChar);
			act("$N великодушно простил$G вас!", false, found, 0, ch, kToChar);
		} else {
			act("$N не нуждается в вашем прощении.", false, ch, 0, found, kToChar);
		}
	} else {
		if (bForgive) {
			send_to_char("Для отпущения грехов Боги рекомендуют воспользоваться церковью.\r\n", ch);
		} else {
			send_to_char("Похоже, этого игрока нет в игре.\r\n", ch);
		}
	}
}

void pk_free_list(CharData *ch) {
	struct PK_Memory_type *pk, *pk_next;

	for (pk = ch->pk_list; pk; pk = pk_next) {
		pk_next = pk->next;
		free(pk);
	}
}

// сохранение списка пк-местей в файл персонажа
void save_pkills(CharData *ch, FILE *saved) {
	struct PK_Memory_type *pk, *tpk;

	fprintf(saved, "Pkil:\n");
	for (pk = ch->pk_list; pk && !PLR_FLAGGED(ch, PLR_DELETED);) {
		if (pk->kill_num > 0 && correct_unique(pk->unique)) {
			if (pk->revenge_num >= MAX_REVENGE && pk->battle_exp <= time(nullptr)) {
				CharData *result = nullptr;
				for (const auto &tch : character_list) {
					if (!IS_NPC(tch) && GET_UNIQUE(tch) == pk->unique) {
						result = tch.get();
						break;
					}
				}

				if (--(pk->kill_num) == 0 && nullptr != result) {
					act("Вы больше не можете мстить $N2.", false, result, 0, ch, kToChar);
				}
				pk->revenge_num = 0;
			}

			if (pk->kill_num <= 0) {
				tpk = pk->next;
				REMOVE_FROM_LIST(pk, ch->pk_list);
				free(pk);
				pk = tpk;
				continue;
			}
			fprintf(saved, "%ld %ld %ld\n", pk->unique, pk->kill_num, pk->revenge_num);
		}
		pk = pk->next;
	}
	fprintf(saved, "~\n");
}

// Проверка может ли ch начать аргессивные действия против victim
int may_kill_here(CharData *ch, CharData *victim, char *argument) {
	if (!victim)
		return true;

	if (MOB_FLAGGED(ch, MOB_NOFIGHT))
		return (false);

	if (MOB_FLAGGED(victim, MOB_NOFIGHT)) {
		act("Боги предотвратили ваше нападение на $N3.", false, ch, 0, victim, kToChar);
		return (false);
	}
	//запрет на любые агры
	if (ch != victim && (ROOM_FLAGGED(ch->in_room, ROOM_NOBATTLE) || ROOM_FLAGGED(victim->in_room, ROOM_NOBATTLE))) {
		send_to_char("Высшие силы воспретили здесь сражаться!\r\n", ch);
		return (false);
	}
	// не агрим чарами <=10 на чаров >=20
	if (!IS_NPC(victim) && !IS_NPC(ch) && GetRealLevel(ch) <= 10
		&& GetRealLevel(victim) >= 20 && !(pk_action_type(ch, victim) & (PK_ACTION_REVENGE | PK_ACTION_FIGHT))) {
		act("Вы еще слишком слабы, чтобы напасть на $N3.", false, ch, 0, victim, kToChar);
		return (false);
	}
	// уже идёт сражение
	if ((ch->get_fighting() && ch->get_fighting() == victim)
		|| (victim->get_fighting() && victim->get_fighting() == ch)) {
		return true;
	}
	// Один из участников в мирной комнате
	if (ch != victim && !ROOM_FLAGGED(victim->in_room, ROOM_ARENA)
		&& (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)
			|| ROOM_FLAGGED(victim->in_room, ROOM_PEACEFUL))) {
		// но это специальные мобы
		if (MOB_FLAGGED(victim, MOB_HORDE))
			return true;
		if (MOB_FLAGGED(ch, MOB_IGNORPEACE) && !IS_CHARMICE(ch))
			return true;
		// моб по триггеру имеет право
		if (IS_NPC(ch) && ch->get_rnum() == real_mobile(DG_CASTER_PROXY))
			return true;
		// богам, мстящим и продолжающим агро-бд можно
		if (IS_GOD(ch) || pk_action_type(ch, victim) & (PK_ACTION_REVENGE | PK_ACTION_FIGHT))
			return true;
		send_to_char("Здесь слишком мирно, чтобы начинать драку...\r\n", ch);
		return false;
	}
	//Проверка на чармиса(своего или группы)
	if (argument) {
		skip_spaces(&argument);
		if (victim && IS_CHARMICE(victim) && victim->get_master() && !IS_NPC(victim->get_master())) {
			if (!name_cmp(victim, argument)) {
				send_to_char(ch, "Для исключения незапланированной агрессии введите имя жертвы полностью.\r\n");
				return false;
			}
		}
		return true;
	}
	return true;
}

// Определяет необходимость вводить
// имя жертвы полностью для начала агродействий
bool need_full_alias(CharData *ch, CharData *opponent) {
	// Цель - НПЦ, не являющаяся чармисом игрока
	if (IS_NPC(opponent) && (!opponent->has_master() || IS_NPC(opponent->get_master()))) {
		return false;
	}
	// Уже воюю?
	if (ch->get_fighting() == opponent || opponent->get_fighting() == ch) {
		return false;
	}
	return true;
}

//Проверка, является ли строка arg полным именем ch
int name_cmp(CharData *ch, const char *arg) {
	char opp_name_part[200] = "\0", opp_name[200] = "\0", *opp_name_remain;
	strcpy(opp_name, GET_NAME(ch));
	for (opp_name_remain = opp_name; *opp_name_remain;) {
		opp_name_remain = one_argument(opp_name_remain, opp_name_part);
		if (!str_cmp(arg, opp_name_part)) {
			return true;
		}
	}
	return false;
}

// Проверка потенциальной возможности агродействий
// true - агродействие разрешено, false - агродействие запрещено
int check_pkill(CharData *ch, CharData *opponent, const char *arg) {
	if (!need_full_alias(ch, opponent))
		return true;

	// Имя не указано?
	if (!*arg)
		return true;

	while (*arg && (*arg == '.' || (*arg >= '0' && *arg <= '9')))
		++arg;

	log("check_pkill, ch: %s, opp: %s, arg: %s",
		ch ? GET_NAME(ch) : "NULL",
		opponent ? GET_NAME(opponent) : "NULL",
		arg ? arg : "NULL");

	if (name_cmp(opponent, arg))
		return true;

	// Совпадений не нашел
	send_to_char(ch, "Для исключения незапланированной агрессии введите имя жертвы полностью.\r\n");
	return false;
}
int check_pkill(CharData *ch, CharData *opponent, const std::string &arg) {
	char opp_name_part[200] = "\0", opp_name[200] = "\0", *opp_name_remain;

	if (!need_full_alias(ch, opponent))
		return true;

	// Имя не указано?
	if (!arg.length())
		return true;

	std::string::const_iterator i;
	for (i = arg.begin(); i != arg.end() && (*i == '.' || (*i >= '0' && *i <= '9')); i++);

	std::string tmp(i, arg.end());

	strcpy(opp_name, GET_NAME(opponent));
	for (opp_name_remain = opp_name; *opp_name_remain;) {
		opp_name_remain = one_argument(opp_name_remain, opp_name_part);
		if (!str_cmp(tmp, opp_name_part))
			return true;
	}

	// Совпадений не нашел
	send_to_char("Для исключения незапланированной агрессии введите имя жертвы полностью.\r\n", ch);
	return false;
}

// Проверяет, есть ли члены любого клан в группе чара и находятся ли они
// в одной с ним комнате
bool has_clan_members_in_group(CharData *ch) {
	CharData *leader;
	struct Follower *f;
	leader = ch->has_master() ? ch->get_master() : ch;

	// проверяем, был ли в группе клановый чар
	if (CLAN(leader)) {
		return true;
	} else {
		for (f = leader->followers; f; f = f->next) {
			if (AFF_FLAGGED(f->ch, EAffectFlag::AFF_GROUP) && IN_ROOM(f->ch) == ch->in_room
				&& CLAN(f->ch)) {
				return true;
			}
		}
	}
	return false;
}

//Polud
void pkPortal(CharData *ch) {
	AGRO(ch) = MAX(AGRO(ch), time(nullptr) + PENTAGRAM_TIME * 60);
	NORENTABLE(ch) = MAX(NORENTABLE(ch), time(nullptr) + PENTAGRAM_TIME * 60);
}

BloodyInfoMap &bloody_map = GlobalObjects::bloody_map();

//Устанавливает экстрабит кровавому стафу
void set_bloody_flag(ObjData *list, const CharData *ch) {
	if (!list) {
		return;
	}
	set_bloody_flag(list->get_contains(), ch);
	set_bloody_flag(list->get_next_content(), ch);
	const int t = GET_OBJ_TYPE(list);
	if (!list->get_extra_flag(EExtraFlag::ITEM_BLOODY)
		&& (t == ObjData::ITEM_LIGHT
			|| t == ObjData::ITEM_WAND
			|| t == ObjData::ITEM_STAFF
			|| t == ObjData::ITEM_WEAPON
			|| t == ObjData::ITEM_ARMOR
			|| (t == ObjData::ITEM_CONTAINER
				&& GET_OBJ_VAL(list, 0))
			|| t == ObjData::ITEM_ARMOR_LIGHT
			|| t == ObjData::ITEM_ARMOR_MEDIAN
			|| t == ObjData::ITEM_ARMOR_HEAVY
			|| t == ObjData::ITEM_INGREDIENT
			|| t == ObjData::ITEM_WORN)) {
		list->set_extra_flag(EExtraFlag::ITEM_BLOODY);
		bloody_map[list].owner_unique = GET_UNIQUE(ch);
		bloody_map[list].kill_at = time(nullptr);
		bloody_map[list].object = list;
	}
}

void bloody::update() {
	const long t = time(nullptr);
	BloodyInfoMap::iterator it = bloody_map.begin();
	while (it != bloody_map.end()) {
		BloodyInfoMap::iterator cur = it++;
		if (t - cur->second.kill_at >= BLOODY_DURATION * 60) {
			cur->second.object->unset_extraflag(EExtraFlag::ITEM_BLOODY);
			bloody_map.erase(cur);
		}
	}
}

void bloody::remove_obj(const ObjData *obj) {
	BloodyInfoMap::iterator it = bloody_map.find(obj);
	if (it != bloody_map.end()) {
		it->second.object->unset_extraflag(EExtraFlag::ITEM_BLOODY);
		bloody_map.erase(it);
	}
}

bool bloody::handle_transfer(CharData *ch, CharData *victim, ObjData *obj, ObjData *container) {
	CharData *initial_ch = ch;
	CharData *initial_victim = victim;
	if (!obj || (ch && IS_GOD(ch))) return true;
	pk_translate_pair(&ch, &victim);
	bool result = false;
	BloodyInfoMap::iterator it = bloody_map.find(obj);
	if (!obj->get_extra_flag(EExtraFlag::ITEM_BLOODY)
		|| it == bloody_map.end()) {
		result = true;
	} else {
		//Если отдаем владельцу или берет владелец
		if (victim
			&& (GET_UNIQUE(victim) == it->second.owner_unique
				|| (CLAN(victim)
					&& (CLAN(victim)->is_clan_member(it->second.owner_unique)
						|| CLAN(victim)->is_alli_member(it->second.owner_unique)))
				|| strcmp(player_table[get_ptable_by_unique(it->second.owner_unique)].mail, GET_EMAIL(victim)) == 0)) {
			remove_obj(obj); //снимаем флаг
			result = true;
		} else if (!ch && victim && (!IS_GOD(victim))) //лут не владельцем
		{
			if (IS_NPC(initial_victim)) //чармисам брать нельзя
			{
				return false;
			}
			AGRO(victim) = MAX(AGRO(victim), KILLER_UNRENTABLE * 60 + it->second.kill_at);
			NORENTABLE(victim) = MAX(NORENTABLE(victim), KILLER_UNRENTABLE * 60 + it->second.kill_at);
			result = true;
		} else if (ch
			&& container
			&& (container->get_carried_by() == ch
				|| container->get_worn_by() == ch)) //чар пытается положить в контейнер в инвентаре или экипировке
		{
			result = true;
		} else //нельзя передавать кровавый шмот
		{
			if (ch) {
				act("Кровь, покрывающая $o3, намертво въелась вам в руки, не давая избавиться от н$S.",
					false,
					ch,
					obj,
					0,
					kToChar);
			}
			return false;
		}
	}
	//обработка контейнеров
	for (ObjData *nobj = obj->get_contains(); nobj != nullptr && result; nobj = nobj->get_next_content()) {
		result = handle_transfer(initial_ch, initial_victim, nobj);
	}
	return result;
}

void bloody::handle_corpse(ObjData *corpse, CharData *ch, CharData *killer) {
	pk_translate_pair(&ch, &killer);
	//Если игрок убил игрока, который не был в агро бд и убитый не душегуб,
	// то с него выпадает окровавленный стаф
	if (ch && killer
		&& ch != killer
		&& !IS_NPC(ch)
		&& !IS_NPC(killer)
		&& !PLR_FLAGGED(ch, PLR_KILLER)
		&& !AGRO(ch)
		&& !IS_GOD(killer)) {
		//Проверим, может у killer есть месть на ch
		struct PK_Memory_type *pk = 0;
		for (pk = ch->pk_list; pk; pk = pk->next)
			if (pk->unique == GET_UNIQUE(killer) && (pk->thief_exp > time(nullptr) || pk->kill_num))
				break;
		if (!pk && corpse) //не нашли мести
			set_bloody_flag(corpse->get_contains(), ch);
	}
}

bool bloody::is_bloody(const ObjData *obj) {
	if (obj->get_extra_flag(EExtraFlag::ITEM_BLOODY)) {
		return true;
	}
	bool result = false;
	for (ObjData *nobj = obj->get_contains(); nobj != nullptr && !result; nobj = nobj->get_next_content()) {
		result = is_bloody(nobj);
	}
	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
