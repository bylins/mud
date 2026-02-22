/**************************************************************************
*  File: dg_triggers.cpp                                   Part of Bylins *
*                                                                         *
*  Usage: contains all the trigger functions for scripts.                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#include "dg_scripts.h"
#include "dg_triggers.h"
#include "engine/core/handler.h"
#include "gameplay/magic/spells_info.h"
#include "engine/olc/olc.h"
#include "engine/db/global_objects.h"
#include "utils/backtrace.h"

extern const char *dirs[];

// external functions from scripts.cpp
extern int script_driver(void *go, Trigger *trig, int type, int mode);
char *matching_quote(char *p);

void ADD_UID_CHAR_VAR(char *buf, Trigger *trig, const ObjData *go, const char *name, const long context) {
	sprintf(buf, "%c%ld", UID_CHAR, go->get_id());
	add_var_cntx(trig->var_list, name, buf, context);
}

void ADD_UID_CHAR_VAR(char *buf, Trigger *trig, const CharData *go, const char *name, const long context) {
	sprintf(buf, "%c%ld", UID_CHAR, go->get_uid());
	add_var_cntx(trig->var_list, name, buf, context);
}

void ADD_UID_OBJ_VAR(char *buf, Trigger *trig, const ObjData *go, const char *name, const long context) {
	sprintf(buf, "%c%ld", UID_OBJ, go->get_id());
	add_var_cntx(trig->var_list, name, buf, context);
}

void ADD_UID_OBJ_VAR(char *buf, Trigger *trig, const CharData *go, const char *name, const long context) {
	sprintf(buf, "%c%ld", UID_OBJ, go->get_uid());
	add_var_cntx(trig->var_list, name, buf, context);
}

// mob trigger types
const char *trig_types[] = {"Random Global",
							"Random",
							"Command",
							"Speech",
							"Act",
							"Death",
							"Greet",
							"Greet-All",
							"Entry",
							"Receive",
							"Fight",
							"HitPrcnt",
							"Bribe",
							"Load",
							"Kill",
							"Damage",
							"Great PC",
							"Great-All PC",
							"Income",
							"Income PC",
							"Агродействие",
							"Раунд боя",
							"Каст в моба",
							"Смена времени",
							"UNUSED",
							"Auto",
							"\n"
};

// obj trigger types
const char *otrig_types[] = {"Random Global",
							 "Random",
							 "Command",
							 "Разрушился",
							 "Fighting round",
							 "Timer",
							 "Get",
							 "Drop",
							 "Give",
							 "Wear",
							 "UNUSED",
							 "Remove",
							 "UNUSED",
							 "Load",
							 "Unlock",
							 "Open",
							 "Lock",
							 "Close",
							 "Взломать",
							 "Вход PC",
							 "Смена времени",
							 "Положили в контейнер",
							 "UNUSED",
							 "UNUSED",
							 "UNUSED",
							 "Auto",
							 "\n"
};

// wld trigger types
const char *wtrig_types[] = {"Random Global",
							 "Random",
							 "Command",
							 "Speech",
							 "Enter PC",
							 "Zone Reset",
							 "Enter",
							 "Drop",
							 "Unlock",
							 "Open",
							 "Lock",
							 "Close",
							 "Взломать",
							 "Смена времени",
							 "Kill PC",
							 "UNUSED",
							 "UNUSED",
							 "UNUSED",
							 "UNUSED",
							 "UNUSED",
							 "UNUSED",
							 "UNUSED",
							 "UNUSED",
							 "UNUSED",
							 "UNUSED",
							 "Auto",
							 "\n"
};

// * Copy first phrase into first_arg, returns rest of string
char *one_phrase(char *arg, char *first_arg) {
	skip_spaces(&arg);

	if (!*arg)
		*first_arg = '\0';
	else if (*arg == '"') {
		char *p, c;
		p = matching_quote(arg);
		c = *p;
		*p = '\0';
		strcpy(first_arg, arg + 1);
		if (c == '\0')
			return p;
		else
			return p + 1;
	} else {
		char *s, *p;

		s = first_arg;
		p = arg;

		while (*p && !isspace(*p) && *p != '"')
			*s++ = *p++;

		*s = '\0';
		return p;
	}

	return arg;
}

int is_substring(const char *sub, const char *string) {
	const char *s = str_str(string, sub);

	if (s) {
		size_t len = strlen(string);
		size_t sublen = strlen(sub);

		// check front
		if ((s == string || isspace(*(s - 1)) || ispunct(static_cast<unsigned char>(*(s - 1))))
			// check end
			&& ((s + sublen == string + len) || isspace(s[sublen])
				|| ispunct(static_cast<unsigned char>(s[sublen])))) {
			return 1;
		}
	}

	return 0;
}

/*
 * return 1 if str contains a word or phrase from wordlist.
 * phrases are in double quotes (").
 * if wrdlist is NULL, then return 1, if str is NULL, return 0.
 */
int word_check(const char *str, const char *wordlist) {
	char words[kMaxInputLength], phrase[kMaxInputLength], *s;

	if (*wordlist == '*')
		return 1;

	strcpy(words, wordlist);

	for (s = one_phrase(words, phrase); *phrase; s = one_phrase(s, phrase))
		if (is_substring(phrase, str))
			return 1;

	return 0;
}

// *  mob triggers
void random_mtrigger(CharData *ch) {
	if (!ch || ch->purged()) {
		return;
	}

	if (!(CheckScript(ch, MTRIG_RANDOM) || CheckScript(ch, MTRIG_RANDOM_GLOBAL))
		|| AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return;
	}

	for (auto t : SCRIPT(ch)->script_trig_list) {

		if ((TRIGGER_CHECK(t, MTRIG_RANDOM_GLOBAL) || (TRIGGER_CHECK(t, MTRIG_RANDOM) && zone_table[world[ch->in_room]->zone_rn].used))
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

void bribe_mtrigger(CharData *ch, CharData *actor, int amount) {
	char buf[kMaxInputLength];

	if (!ch || ch->purged()
		|| !actor || actor->purged()) {
		return;
	}

	if (!CheckScript(ch, MTRIG_BRIBE)
		|| !CAN_START_MTRIG(ch)
		|| GET_INVIS_LEV(actor)) {
		return;
	}

	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_BRIBE)
			&& (amount >= GET_TRIG_NARG(t))) {
			snprintf(buf, kMaxInputLength, "%d", amount);
			add_var_cntx(t->var_list, "amount", buf, 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);

			break;
		}
	}
}

void greet_mtrigger(CharData *actor, int dir) {
	char buf[kMaxInputLength];
	int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth, EDirection::kEast, EDirection::kDown, EDirection::kUp};

	if (!actor || actor->purged()) {
		return;
	}

	const auto people_copy = world[actor->in_room]->people;
	for (const auto ch : people_copy) {
		if (ch->purged()) {
			continue;
		}

		const auto mask = MTRIG_GREET | MTRIG_GREET_ALL | MTRIG_GREET_PC | MTRIG_GREET_PC_ALL;
		if (!CheckScript(ch, mask)
			|| !AWAKE(ch)
			|| ch->GetEnemy()
			|| ch == actor
			|| !CAN_START_MTRIG(ch)
			|| GET_INVIS_LEV(actor)) {
			continue;
		}

		for (auto t : SCRIPT(ch)->script_trig_list) {
			if (((IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET)
				&& CAN_SEE(ch, actor))
				|| IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_ALL)
				|| (IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_PC)
					&& !actor->IsNpc() && CAN_SEE(ch, actor))
				|| (IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_PC_ALL)
					&& !actor->IsNpc())) && !GET_TRIG_DEPTH(t)
				&& (number(1, 100) <= GET_TRIG_NARG(t))) {
				if (dir >= 0) {
					add_var_cntx(t->var_list, "direction", dirs[rev_dir[dir]], 0);
				}

				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);

				if (ch->purged()) {
					break;
				}
			}
		}
	}
}

void income_mtrigger(CharData *ch, int dir) {
	int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth, EDirection::kEast, EDirection::kDown, EDirection::kUp};
	int ispcinroom = 0;
	CharData *actor = nullptr;

	if (!ch || ch->purged())
		return;

	if ((!CheckScript(ch, MTRIG_INCOME)
		&& !CheckScript(ch, MTRIG_INCOME_PC))
		|| !CAN_START_MTRIG(ch)) {
		return;
	}

	for (const auto i : world[ch->in_room]->people) {
		if ((!i->IsNpc()
			&& CAN_SEE(ch, i)) && !GET_INVIS_LEV(i)) {
			ispcinroom = 1;
			actor = i;
		}
	}

	for (auto t : SCRIPT(ch)->script_trig_list) {
		if ((TRIGGER_CHECK(t, MTRIG_INCOME) || (TRIGGER_CHECK(t, MTRIG_INCOME_PC) && ispcinroom))
			&& number(1, 100) <= GET_TRIG_NARG(t)) {
			if (dir >= 0) {
				add_var_cntx(t->var_list, "direction", dirs[rev_dir[dir]], 0);
			}

			if (actor) {
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			}

			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);

			break;
		}
	}

	return;
}

int entry_mtrigger(CharData *ch) {
	if (!ch || ch->purged())
		return 1;

	if (!CheckScript(ch, MTRIG_ENTRY)
		|| !CAN_START_MTRIG(ch)) {
		return 1;
	}

	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_ENTRY)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int compare_cmd(int mode, const char *source, const char *dest) {
	int result = false;
	if (!source || !*source || !dest || !*dest)
		return (false);
	if (*source == '*')
		return (true);

	switch (mode) {
		case 0: result = word_check(dest, source);

			break;
		case 1: result = is_substring(dest, source);

			break;
		default:
			if (!str_cmp(source, dest))
				return (true);
	}
	return (result);
}

int command_mtrigger(CharData *actor, char *cmd, const char *argument) {
	char buf[kMaxInputLength];

	const auto people_copy = world[actor->in_room]->people;
	for (const auto ch : people_copy) {
		if ((CheckScript(ch, MTRIG_COMMAND)
			&& CAN_START_MTRIG(ch)
			&& (actor != ch))
			&& !GET_INVIS_LEV(actor)) {
			for (auto t : SCRIPT(ch)->script_trig_list) {
				if (t->get_attach_type() != MOB_TRIGGER)//детачим триги не для мобов
				{
					snprintf(buf,
							 kMaxInputLength,
							 "SYSERR: M-Trigger #%d has wrong attach_type %s expected %s char:%s[%d]!",
							 GET_TRIG_VNUM(t),
							 attach_name[(int) t->get_attach_type()],
							 attach_name[MOB_TRIGGER],
							 ch->get_name().c_str(),
							 GET_MOB_VNUM(ch));
					mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);
					SCRIPT(ch)->remove_trigger(trig_index[(t)->get_rnum()]->vnum);

					break;
				}

				if (!TRIGGER_CHECK(t, MTRIG_COMMAND)) {
					continue;
				}

				if (t->arglist.empty()) {
					snprintf(buf,
							 kMaxInputLength,
							 "SYSERR: Command Trigger #%d has no text argument!",
							 GET_TRIG_VNUM(t));
					mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);
					continue;
				}

				if (compare_cmd(GET_TRIG_NARG(t), t->arglist.c_str(), cmd)) {
					if (!actor->IsNpc()
						&& (actor->GetPosition() == EPosition::kSleep))   // command триггер не будет срабатывать если игрок спит
					{
						SendMsgToChar("Сделать это в ваших снах?\r\n", actor);
						return 1;
					}

					ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
					skip_spaces(&argument);
					add_var_cntx(t->var_list, "arg", argument, 0);
					skip_spaces(&cmd);
					add_var_cntx(t->var_list, "cmd", cmd, 0);

					if (script_driver(ch, t, MOB_TRIGGER, TRIG_NEW)) {
						return 1;
					}
				}
			}
		}
	}

	return 0;
}

void speech_mtrigger(CharData *actor, char *str) {
	char buf[kMaxInputLength];

	if (!actor || actor->purged())
		return;

	const auto people_copy = world[actor->in_room]->people;
	for (const auto ch : people_copy) {
		if ((CheckScript(ch, MTRIG_SPEECH)
			&& AWAKE(ch)
			&& !AFF_FLAGGED(ch, EAffect::kDeafness)
			&& CAN_START_MTRIG(ch)
			&& (actor != ch))
			&& !GET_INVIS_LEV(actor)) {
			for (auto t : SCRIPT(ch)->script_trig_list) {
				if (!TRIGGER_CHECK(t, MTRIG_SPEECH)) {
					continue;
				}

				if (t->arglist.empty()) {
					snprintf(buf,
							 kMaxInputLength,
							 "SYSERR: Speech Trigger #%d has no text argument!",
							 GET_TRIG_VNUM(t));
					mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);
					continue;
				}

				if (compare_cmd(GET_TRIG_NARG(t), t->arglist.c_str(), str)) {
					ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
					add_var_cntx(t->var_list, "speech", str, 0);
					script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);

					break;
				}
			}
		}
	}
}

void act_mtrigger(CharData *ch, char *str, CharData *actor, CharData *victim,
				  const ObjData *object, const ObjData *target, char *arg) {
	char buf[kMaxInputLength];

	if (!ch || ch->purged())
		return;

	if ((CheckScript(ch, MTRIG_ACT) && CAN_START_MTRIG(ch) && (actor != ch)) && !GET_INVIS_LEV(actor))
		for (auto t : SCRIPT(ch)->script_trig_list) {
			if (!TRIGGER_CHECK(t, MTRIG_ACT))
				continue;

			if (t->arglist.empty()) {
				snprintf(buf, kMaxInputLength, "SYSERR: Act Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
				mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);
				continue;
			}

			if (compare_cmd(GET_TRIG_NARG(t), t->arglist.c_str(), str)) {
				if (actor) {
					ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				}

				if (victim) {
					ADD_UID_CHAR_VAR(buf, t, victim, "victim", 0);
				}

				if (object) {
					ADD_UID_OBJ_VAR(buf, t, object, "object", 0);
				}

				if (target) {
					ADD_UID_CHAR_VAR(buf, t, target, "target", 0);
				}

				if (arg) {
					skip_spaces(&arg);
					add_var_cntx(t->var_list, "arg", arg, 0);
				}

				script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);

				break;
			}
		}
}

int fight_mtrigger(CharData *ch) {
	char buf[kMaxInputLength];

	if (!ch || ch->purged()) {
		return 1;
	}
	if (!CheckScript(ch, MTRIG_FIGHT) || !ch->GetEnemy() || !CAN_START_MTRIG(ch))
		return 1;
	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_FIGHT) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			snprintf(buf, kMaxInputLength, "%d", ch->round_counter);
			add_var_cntx(t->var_list, "round", buf, 0);
			ADD_UID_CHAR_VAR(buf, t, ch->GetEnemy(), "actor", 0);
			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
	return 1;
}

int damage_mtrigger(CharData *damager, CharData *victim, int amount, const char* name_skillorspell, int is_skill, ObjData *obj) {
	if (!damager || damager->purged() || !victim  || victim->purged()) {
		log("SYSERROR: damager = %s, victim = %s (%s:%d)",
			damager ? (damager->purged() ? "purged" : "true") : "false",
			victim ? (victim->purged() ? "purged" : "true") : "false",
			__FILE__, __LINE__);
		return 1;
	}

	char buf[kMaxInputLength];

	if (!CheckScript(victim, MTRIG_DAMAGE)
		|| !CAN_START_MTRIG(victim)) {
		return 1;
	}

	for (auto t : SCRIPT(victim)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_DAMAGE) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(buf, t, damager, "damager", 0);
			add_var_cntx(t->var_list, "amount", std::to_string(amount).c_str(), 0);
			add_var_cntx(t->var_list, "name", name_skillorspell, 0);
			add_var_cntx(t->var_list, "is_skill", std::to_string(is_skill).c_str(), 0);
			if(obj) {
				ADD_UID_OBJ_VAR(buf, t, obj, "weapon", 0);
			}
			return script_driver(victim, t, MOB_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

void hitprcnt_mtrigger(CharData *ch) {
	if (!ch || ch->purged()) {
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return;
	}

	char buf[kMaxInputLength];

	if (!CheckScript(ch, MTRIG_HITPRCNT) || !ch->GetEnemy() || !CAN_START_MTRIG(ch))
		return;

	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_HITPRCNT) && ch->get_max_hit() &&
			(((ch->get_hit() * 100) / ch->get_max_hit()) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(buf, t, ch->GetEnemy(), "actor", 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

int receive_mtrigger(CharData *ch, CharData *actor, ObjData *obj) {
	if (!ch || ch->purged()) {
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return 1;
	}

	char buf[kMaxInputLength];

	if (!CheckScript(ch, MTRIG_RECEIVE) || !CAN_START_MTRIG(ch) || GET_INVIS_LEV(actor))
		return 1;

	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_RECEIVE)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			ADD_UID_OBJ_VAR(buf, t, obj, "object", 0);

			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int death_mtrigger(CharData *ch, CharData *actor) {
	if (!ch || ch->purged()) {
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return 1;
	}

	char buf[kMaxInputLength];
	if (!CheckScript(ch, MTRIG_DEATH)
		|| AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return 1;
	}
	
	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_DEATH)) {
			if (actor) {
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			}

			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int kill_mtrigger(CharData *ch, CharData *actor) {
	if (!ch || ch->purged()) {
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return 0;
	}

	if (!CheckScript(ch, MTRIG_KILL)) {
		return 0;
	}

	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_KILL)) {
			if (actor) {
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			}

			std::ostringstream out_list;
			if (!ch->kill_list.empty()) {
				auto it = ch->kill_list.begin();
				out_list << UID_CHAR << *it;
				++it;

				for (; it != ch->kill_list.end(); ++it) {
					out_list << " " << UID_CHAR << *it;
				}
			}
			add_var_cntx(t->var_list, "list", out_list.str().c_str(), 0);

			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
		}
	}

	return 0;
}

void load_mtrigger(CharData *ch) {
	if (!ch || ch->purged()) {
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return;
	}
	if (!CheckScript(ch, MTRIG_LOAD))
		return;

	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_LOAD)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);

			break;
		}
	}
}

int start_fight_mtrigger(CharData *ch, CharData *actor) {
	if (!ch || ch->purged() || !actor || actor->purged()) {
		log("SYSERROR: start_fight_mtrigger: ch = %s, actor = %s (%s:%d)", ch ? (ch->purged() ? "purged" : "true") : "false",
				actor ? (actor->purged() ? "purged" : "true") : "false",__FILE__, __LINE__);
		return 1;
	}
	if (!CheckScript(ch, MTRIG_START_FIGHT)) {
		return 1;
	}
	char buf[kMaxInputLength];
	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_START_FIGHT)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
		}
	}
	return 1;
}

void round_num_mtrigger(CharData *ch, CharData *actor) {
	if (!ch || ch->purged() || !actor || actor->purged()) {
		log("SYSERROR: ch_purged: ch = %s, actor = %s (%s:%d)", ch ? (ch->purged() ? "purged" : "true") : "false",
			actor ? (actor->purged() ? "purged" : "true") : "false",
			__FILE__, __LINE__);
		return;
	}

	if (!CheckScript(ch, MTRIG_ROUND_NUM)
		|| GET_INVIS_LEV(actor)) {
		return;
	}

	char buf[kMaxInputLength];
	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_ROUND_NUM)
			&& ch->round_counter == GET_TRIG_NARG(t)) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			return;
		}
	}
}

/**
* Реакиця на каст в моба.
* \param ch - моб
* \param actor - кастер, идет в скрипт как %actor%
* \param spell_id - номер закла, идет в скрипт как %cast_num% и %castname%
*/
int cast_mtrigger(CharData *ch, CharData *actor, ESpell spell_id) {
	if (!ch || ch->purged() || !actor || actor->purged()) {
//		log("SYSERROR: ch_purged: ch = %s, actor = %s (%s:%d)", ch ? (ch->purged() ? "purged" : "true") : "false",
//			actor ? (actor->purged() ? "purged" : "true") : "false",
//			__FILE__, __LINE__);
		return 1;
	}

	if (!CheckScript(ch, MTRIG_CAST) || !CAN_START_MTRIG(ch) || GET_INVIS_LEV(actor)) {
		return 1;
	}
	char local_buf[kMaxInputLength];
	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_CAST)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(local_buf, t, actor, "actor", 0);
			sprintf(buf, "%d", to_underlying(spell_id));
			add_var_cntx(t->var_list, "castnum", buf, 0);
			add_var_cntx(t->var_list, "castname", MUD::Spell(spell_id).GetCName(), 0);
			if (MUD::Spell(spell_id).IsViolent()) {
				add_var_cntx(t->var_list, "violent", "1", 0);
			} else {
				add_var_cntx(t->var_list, "violent", "0", 0);
			}
			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
		}
	}
	return 1;
}

void timechange_mtrigger(CharData *ch, const int time, const int time_day) {
	if (!ch || ch->purged()) {
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return;
	}

	char buf[kMaxInputLength];

	if (!CheckScript(ch, MTRIG_TIMECHANGE))
		return;
	for (auto t : SCRIPT(ch)->script_trig_list) {
		if (TRIGGER_CHECK(t, MTRIG_TIMECHANGE)) {
			snprintf(buf, kMaxInputLength, "%d", time);
			add_var_cntx(t->var_list, "time", buf, 0);
			snprintf(buf, kMaxInputLength, "%d", time_day);
			add_var_cntx(t->var_list, "timeday", buf, 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

// *  object triggers
void random_otrigger(ObjData *obj) {
	if (!(CheckSript(obj, OTRIG_RANDOM) || CheckSript(obj, OTRIG_RANDOM_GLOBAL))) {
		return;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_RANDOM_GLOBAL) 
				|| (TRIGGER_CHECK(t, OTRIG_RANDOM) && obj->get_in_room() == kNowhere) //в инве или одет
				|| (TRIGGER_CHECK(t, OTRIG_RANDOM) && obj->get_in_room() != kNowhere && zone_table[world[obj->get_in_room()]->zone_rn].used)) {
			if (number(1, 100) <= GET_TRIG_NARG(t)) {
				script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
				break;
			}
		}
	}
}

Bitvector try_run_fight_otriggers(CharData *actor, ObjData *obj, int mode) {
	char buf[kMaxInputLength];
	Bitvector result = kNormalRound;
	if (!CheckSript(obj, OTRIG_FIGHT) || GET_INVIS_LEV(actor)) {
		return result;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_FIGHT) && IS_SET(GET_TRIG_NARG(t), mode)) {
			snprintf(buf, kMaxInputLength, "%d", actor->round_counter);
			add_var_cntx(t->var_list, "round", buf, 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			SET_BIT(result, script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW));
		}
	}
	return result;
}

Bitvector fight_otrigger(CharData *actor) {
	Bitvector result = kNormalRound;
	for (auto item: actor->equipment) {
		if (item) {
			SET_BIT(result, try_run_fight_otriggers(actor, item, OCMD_EQUIP));
		}
	}
	for (auto item = actor->carrying; item; item = item->get_next_content()) {
		SET_BIT(result, try_run_fight_otriggers(actor, item, OCMD_INVEN));
	}
	return result;
}

void timer_otrigger(ObjData *obj) {
	if (!CheckSript(obj, OTRIG_TIMER)) {
		return;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_TIMER)) {
			script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}
}

int get_otrigger(ObjData *obj, CharData *actor) {
	char buf[kMaxInputLength];

	if (!CheckSript(obj, OTRIG_GET) || GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_GET)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

// checks for command trigger on specific object. assumes obj has cmd trig
int cmd_otrig(ObjData *obj, CharData *actor, char *cmd, const char *argument, int type) {
	char buf[kMaxInputLength];

	if ((obj && CheckSript(obj, OTRIG_COMMAND)) && !GET_INVIS_LEV(actor)) {
		for (auto t : obj->get_script()->script_trig_list) {
			if (t->get_attach_type() != OBJ_TRIGGER)//детачим триги не для объектов
			{
				snprintf(buf,
						 kMaxInputLength,
						 "SYSERR: O-Trigger #%d has wrong attach_type %s expected %s Object:%s[%d]!",
						 GET_TRIG_VNUM(t),
						 attach_name[(int) t->get_attach_type()],
						 attach_name[OBJ_TRIGGER],
						 obj->get_PName(ECase::kNom).empty() ? obj->get_PName(ECase::kNom).c_str() : "undefined",
						 GET_OBJ_VNUM(obj));
				mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);
				obj->get_script()->remove_trigger(trig_index[(t)->get_rnum()]->vnum);
				break;
			}

			if (!TRIGGER_CHECK(t, OTRIG_COMMAND)) {
				continue;
			}

			if (IS_SET(GET_TRIG_NARG(t), type) && t->arglist.empty()) {
				snprintf(buf,
						 kMaxInputLength,
						 "SYSERR: O-Command Trigger #%d has no text argument!",
						 GET_TRIG_VNUM(t));
				mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);
				continue;
			}

			if (IS_SET(GET_TRIG_NARG(t), type)
				&& (t->arglist[0] == '*'
				|| 0 == strn_cmp(t->arglist.c_str(), cmd, t->arglist.size()))) {
				if (!actor->IsNpc()
					&& (actor->GetPosition() == EPosition::kSleep))   // command триггер не будет срабатывать если игрок спит
				{
					SendMsgToChar("Сделать это в ваших снах?\r\n", actor);
					return 1;
				}
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				skip_spaces(&argument);
				add_var_cntx(t->var_list, "arg", argument, 0);
				skip_spaces(&cmd);
				add_var_cntx(t->var_list, "cmd", cmd, 0);

				if (script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW)) {
					return 1;
				}
			}
		}
	}

	return 0;
}

int command_otrigger(CharData *actor, char *cmd, const char *argument) {
	if (GET_INVIS_LEV(actor))
		return 0;

	for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (cmd_otrig(GET_EQ(actor, i), actor, cmd, argument, OCMD_EQUIP)) {
			return 1;
		}
	}

	for (ObjData *obj = actor->carrying; obj; obj = obj->get_next_content()) {
		if (cmd_otrig(obj, actor, cmd, argument, OCMD_INVEN)) {
			return 1;
		}
	}

	for (ObjData *obj = world[actor->in_room]->contents; obj; obj = obj->get_next_content()) {
		if (cmd_otrig(obj, actor, cmd, argument, OCMD_ROOM)) {
			return 1;
		}
	}

	return 0;
}

int wear_otrigger(ObjData *obj, CharData *actor, int where) {
	char buf[kMaxInputLength];

	if (!CheckSript(obj, OTRIG_WEAR) || GET_INVIS_LEV(actor)) {
		return 1;
	}
	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_WEAR)) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			snprintf(buf, kMaxInputLength, "%d", where);
			add_var_cntx(t->var_list, "where", buf, 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int put_otrigger(ObjData *obj, CharData *actor, ObjData *cont) {
	char buf[kMaxInputLength];

	if (!CheckSript(cont, OTRIG_PUT) || GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t :cont->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_PUT)) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			ADD_UID_OBJ_VAR(buf, t, obj, "object", 0);
			return script_driver(cont, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int remove_otrigger(ObjData *obj, CharData *actor) {
	char buf[kMaxInputLength];

	if (!CheckSript(obj, OTRIG_REMOVE) || GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_REMOVE)) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int drop_otrigger(ObjData *obj, CharData *actor, const Bitvector  argument) {
	char buf[kMaxInputLength];

	if (!CheckSript(obj, OTRIG_DROP)) {
		return 1;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_DROP)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			int tmpvar = atoi(t->arglist.c_str());
			if (tmpvar == 0 || IS_SET(tmpvar, argument)) {
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
			}
		}
	}

	return 1;
}

int give_otrigger(ObjData *obj, CharData *actor, CharData *victim) {
	char buf[kMaxInputLength];

	if (!CheckSript(obj, OTRIG_GIVE) || GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_GIVE)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			ADD_UID_CHAR_VAR(buf, t, victim, "victim", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

void load_otrigger(ObjData *obj) {
	if (!CheckSript(obj, OTRIG_LOAD)) {
		return;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_LOAD)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

void purge_otrigger(ObjData *obj) {
	if (!CheckSript(obj, OTRIG_PURGE)) {
		return;
	}
	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_PURGE) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

int pick_otrigger(ObjData *obj, CharData *actor) {
	char buf[kMaxInputLength];

	if (!CheckSript(obj, OTRIG_PICK) || GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_PICK)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}
	return 1;
}

int open_otrigger(ObjData *obj, CharData *actor, int unlock) {
	char buf[kMaxInputLength];
	int open_mode = unlock ? OTRIG_UNLOCK : OTRIG_OPEN;

	if (!CheckSript(obj, open_mode) || GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, open_mode)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			add_var_cntx(t->var_list, "mode", unlock ? "1" : "0", 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);

			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int close_otrigger(ObjData *obj, CharData *actor, int lock) {
	char buf[kMaxInputLength];
	int close_mode = lock ? OTRIG_LOCK : OTRIG_CLOSE;

	if (!CheckSript(obj, close_mode) || GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, close_mode)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			add_var_cntx(t->var_list, "mode", lock ? "1" : "0", 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);

			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

void greet_otrigger(CharData *actor, int dir) {
	char buf[kMaxInputLength];
	ObjData *obj;
	int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth, EDirection::kEast, EDirection::kDown, EDirection::kUp};

	if (actor->IsNpc() || GET_INVIS_LEV(actor)) {
		return;
	}

	for (obj = world[actor->in_room]->contents; obj; obj = obj->get_next_content()) {
		if (!CheckSript(obj, OTRIG_GREET_ALL_PC)) {
			continue;
		}

		for (auto t : obj->get_script()->script_trig_list) {
			if (TRIGGER_CHECK(t, OTRIG_GREET_ALL_PC)) {
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);

				if (dir >= 0) {
					add_var_cntx(t->var_list, "direction", dirs[rev_dir[dir]], 0);
				}

				script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
			}
		}
	}
}

int timechange_otrigger(ObjData *obj, const int time, const int time_day) {
	char buf[kMaxInputLength];

	if (!CheckSript(obj, OTRIG_TIMECHANGE)) {
		return 1;
	}
	for (Trigger *t : obj->get_script()->script_trig_list) {
		if (TRIGGER_CHECK(t, OTRIG_TIMECHANGE)) {
			snprintf(buf, kMaxInputLength, "%d", time);
			add_var_cntx(t->var_list, "time", buf, 0);
			snprintf(buf, kMaxInputLength, "%d", time_day);
			add_var_cntx(t->var_list, "timeday", buf, 0);
			script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
			break;
		}
	}
	return 1;
}

// *  world triggers

void reset_wtrigger(RoomData *room) {
	if (!CheckSript(room, WTRIG_RESET)) {
		return;
	}

	for (auto t : SCRIPT(room)->script_trig_list) {
		if (TRIGGER_CHECK(t, WTRIG_RESET)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

void random_wtrigger(RoomData *room, const TriggersList &) {
	if (!(CheckSript(room, WTRIG_RANDOM) || CheckSript(room, WTRIG_RANDOM_GLOBAL)))
		return;

	for (auto t : SCRIPT(room)->script_trig_list) {
		if ((TRIGGER_CHECK(t, WTRIG_RANDOM_GLOBAL) || (TRIGGER_CHECK(t, WTRIG_RANDOM) && zone_table[room->zone_rn].used))
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

int enter_wtrigger(RoomData *room, CharData *actor, int dir) {
	char buf[kMaxInputLength];
	int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth, EDirection::kEast, EDirection::kDown, EDirection::kUp};

	if (!actor || actor->purged())
		return 1;

	if ((!CheckSript(room, WTRIG_ENTER | WTRIG_ENTER_PC)) || GET_INVIS_LEV(actor))
		return 1;

	for (auto t : SCRIPT(room)->script_trig_list) {
		if ((TRIGGER_CHECK(t, WTRIG_ENTER)
			|| (TRIGGER_CHECK(t, WTRIG_ENTER_PC)
				&& !actor->IsNpc()))
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			if (dir >= 0) {
				add_var_cntx(t->var_list, "direction", dirs[rev_dir[dir]], 0);
			// триггер может удалить выход, но не вернуть 0 (есть такие билдеры)
				return (script_driver(room, t, WLD_TRIGGER, TRIG_NEW) && CAN_GO(actor, dir));
			} else {
				return (script_driver(room, t, WLD_TRIGGER, TRIG_NEW));
			}
		}
	}

	return 1;
}

int command_wtrigger(CharData *actor, char *cmd, const char *argument) {
	RoomData *room;
	char buf[kMaxInputLength];

	if (!actor || actor->in_room == kNowhere || !CheckSript(world[actor->in_room], WTRIG_COMMAND)
		|| GET_INVIS_LEV(actor))
		return 0;

	room = world[actor->in_room];
	for (auto t : SCRIPT(room)->script_trig_list) {
		if (t->get_attach_type() != WLD_TRIGGER)//детачим триги не для комнат
		{
			snprintf(buf,
					 kMaxInputLength,
					 "SYSERR: W-Trigger #%d has wrong attach_type %s expected %s room:%s[%d]!",
					 GET_TRIG_VNUM(t),
					 attach_name[(int) t->get_attach_type()],
					 attach_name[WLD_TRIGGER],
					 room->name,
					 room->vnum);
			mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);
			SCRIPT(room)->remove_trigger(trig_index[(t)->get_rnum()]->vnum);
			break;
		}

		if (!TRIGGER_CHECK(t, WTRIG_COMMAND)) {
			continue;
		}

		if (t->arglist.empty()) {
			snprintf(buf, kMaxInputLength, "SYSERR: W-Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
			mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);
			continue;
		}

		if (compare_cmd(GET_TRIG_NARG(t), t->arglist.c_str(), cmd)) {
			if (!actor->IsNpc()
				&& (actor->GetPosition() == EPosition::kSleep))   // command триггер не будет срабатывать если игрок спит
			{
				SendMsgToChar("Сделать это в ваших снах?\r\n", actor);
				return 1;
			}
// в идеале бы в триггере бой проверять а не хардкодом, ленивые скотины....
			if (actor->GetPosition() == EPosition::kFight && t->arglist[0] != '*') {
				SendMsgToChar("Вы не можете это сделать в бою.\r\n", actor); //command триггер не будет работать в бою
				return 1;
			}

			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			skip_spaces(&argument);
			add_var_cntx(t->var_list, "arg", argument, 0);
			skip_spaces(&cmd);
			add_var_cntx(t->var_list, "cmd", cmd, 0);

			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		}
	}
	return 0;
}

void kill_pc_wtrigger(CharData *killer, CharData *victim) {
	if (!killer || !victim || !CheckSript(world[killer->in_room], WTRIG_KILL_PC) || GET_INVIS_LEV(killer))
		return;
	auto room = world[victim->in_room];
	for (auto t : SCRIPT(room)->script_trig_list) {
		if (!TRIGGER_CHECK(t, WTRIG_KILL_PC)) {
			continue;
		}
		ADD_UID_CHAR_VAR(buf, t, killer, "killer", 0);
		ADD_UID_CHAR_VAR(buf, t, victim, "victim", 0);
		script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		break;
	}

}
void speech_wtrigger(CharData *actor, char *str) {
	char buf[kMaxInputLength];

	if (!actor || !CheckSript(world[actor->in_room], WTRIG_SPEECH) || GET_INVIS_LEV(actor))
		return;

	auto room = world[actor->in_room];
	for (auto t : SCRIPT(room)->script_trig_list) {
		if (!TRIGGER_CHECK(t, WTRIG_SPEECH)) {
			continue;
		}

		if (t->arglist.empty()) {
			snprintf(buf, kMaxInputLength, "SYSERR: W-Speech Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
			mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);

			continue;
		}

		if (compare_cmd(GET_TRIG_NARG(t), t->arglist.c_str(), str)) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			add_var_cntx(t->var_list, "speech", str, 0);
			script_driver(room, t, WLD_TRIGGER, TRIG_NEW);

			break;
		}
	}
}

int drop_wtrigger(ObjData *obj, CharData *actor) {
	char buf[kMaxInputLength];

	if (!actor
		|| !CheckSript(world[actor->in_room], WTRIG_DROP)
		|| GET_INVIS_LEV(actor)) {
		return 1;
	}

	auto room = world[actor->in_room];
	for (auto t : SCRIPT(room)->script_trig_list) {
		if (TRIGGER_CHECK(t, WTRIG_DROP)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			ADD_UID_OBJ_VAR(buf, t, obj, "object", 0);

			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int pick_wtrigger(RoomData *room, CharData *actor, int dir) {
	char buf[kMaxInputLength];

	if (!CheckSript(room, WTRIG_PICK)
		|| GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t : SCRIPT(room)->script_trig_list) {
		if (TRIGGER_CHECK(t, WTRIG_PICK)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			add_var_cntx(t->var_list, "direction", dirs[dir], 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);

			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int open_wtrigger(RoomData *room, CharData *actor, int dir, int unlock) {
	char buf[kMaxInputLength];
	int open_mode = unlock ? WTRIG_UNLOCK : WTRIG_OPEN;

	if (!CheckSript(room, open_mode)
		|| GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t : SCRIPT(room)->script_trig_list) {
		if (TRIGGER_CHECK(t, open_mode) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			add_var_cntx(t->var_list, "mode", unlock ? "1" : "0", 0);
			add_var_cntx(t->var_list, "direction", dirs[dir], 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);

			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int close_wtrigger(RoomData *room, CharData *actor, int dir, int lock) {
	char buf[kMaxInputLength];
	int close_mode = lock ? WTRIG_LOCK : WTRIG_CLOSE;

	if (!CheckSript(room, close_mode)
		|| GET_INVIS_LEV(actor)) {
		return 1;
	}

	for (auto t : SCRIPT(room)->script_trig_list) {
		if (TRIGGER_CHECK(t, close_mode)
			&& (number(1, 100) <= GET_TRIG_NARG(t))) {
			add_var_cntx(t->var_list, "mode", lock ? "1" : "0", 0);
			add_var_cntx(t->var_list, "direction", dirs[dir], 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);

			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int timechange_wtrigger(RoomData *room, const int time, const int time_day) {
	char buf[kMaxInputLength];

	if (!CheckSript(room, WTRIG_TIMECHANGE)) {
		return 1;
	}

	for (auto t : SCRIPT(room)->script_trig_list) {
		if (TRIGGER_CHECK(t, WTRIG_TIMECHANGE)) {
			snprintf(buf, kMaxInputLength, "%d", time);
			add_var_cntx(t->var_list, "time", buf, 0);
			snprintf(buf, kMaxInputLength, "%d", time_day);
			add_var_cntx(t->var_list, "timeday", buf, 0);
			script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
			break;;
		}
	}
	return 1;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
