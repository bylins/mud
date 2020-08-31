/* ************************************************************************
*   File: act.offensive.cpp                               Part of Bylins  *
*  Usage: player-level commands of an offensive nature                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                               *
*  $Date$                                                                 *
*  $Revision$                                                             *
************************************************************************ */

#include "act.offensive.h"
#include "ability.rollsystem.hpp"
#include "action.targeting.hpp"
#include "obj.hpp"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "constants.h"
#include "screen.h"
#include "spells.h"
#include "skills.h"
#include "pk.h"
#include "privilege.hpp"
#include "random.hpp"
#include "char.hpp"
#include "room.hpp"
#include "fight.h"
#include "fight_hit.hpp"
#include "features.hpp"
#include "db.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include "skills/do_flee.h"
#include "skills/do_strangle.h"
#include "skills/do_kick.h"
#include "skills/do_bash.h"
#include "skills/do_stun.h"
#include "skills/do_protect.h"

#include <cmath>
#include "logger.hpp"

using namespace FightSystem;
using namespace AbilitySystem;

// extern variables
extern DESCRIPTOR_DATA *descriptor_list;

// extern functions
int compute_armor_class(CHAR_DATA * ch);
int awake_others(CHAR_DATA * ch);
void appear(CHAR_DATA * ch);
int legal_dir(CHAR_DATA * ch, int dir, int need_specials_check, int show_msg);
void alt_equip(CHAR_DATA * ch, int pos, int dam, int chance);

// local functions
void do_assist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_hit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_kill(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_order(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_manadrain(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_expedient_cut(CHAR_DATA *ch, char *argument, int cmd, int subcmd);


inline bool dontCanAct(CHAR_DATA *ch) {
	return (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT));
}



void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room) {
	if (!WAITLESS(ch) && (!victim_in_room || (ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())))) {
		WAIT_STATE(ch, waittime * PULSE_VIOLENCE);
	}
}

void setSkillCooldown(CHAR_DATA* ch, ESkill skill, int cooldownInPulses) {
	if (ch->getSkillCooldownInPulses(skill) < cooldownInPulses) {
		ch->setSkillCooldown(skill, cooldownInPulses*PULSE_VIOLENCE);
	}
}

void setSkillCooldownInFight(CHAR_DATA* ch, ESkill skill, int cooldownInPulses) {
	if (ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())) {
		setSkillCooldown(ch, skill, cooldownInPulses);
	} else {
		WAIT_STATE(ch, PULSE_VIOLENCE/6);
	}
}

CHAR_DATA* findVictim(CHAR_DATA* ch, char* argument) {
	one_argument(argument, arg);
	CHAR_DATA* victim = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	if (!victim) {
		if (!*arg && ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())) {
			victim = ch->get_fighting();
		}
	}
	return victim;
}

int set_hit(CHAR_DATA * ch, CHAR_DATA * victim) {
	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return (FALSE);
	}

	if (ch->get_fighting() || GET_MOB_HOLD(ch)) {
		return (FALSE);
	}
	victim = try_protect(victim, ch);

	bool message = false;
	// если жертва пишет на доску - вываливаем его оттуда и чистим все это дело
	if (victim->desc && (STATE(victim->desc) == CON_WRITEBOARD || STATE(victim->desc) == CON_WRITE_MOD)) {
		victim->desc->message.reset();
		victim->desc->board.reset();
		if (victim->desc->writer->get_string()) {
			victim->desc->writer->clear();
		}
		STATE(victim->desc) = CON_PLAYING;
		if (!IS_NPC(victim)) {
			PLR_FLAGS(victim).unset(PLR_WRITING);
		}
		if (victim->desc->backstr) {
			free(victim->desc->backstr);
			victim->desc->backstr = nullptr;
		}
		victim->desc->writer.reset();
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_CLANEDIT)) {
		// аналогично, если жерва правит свою дружину в олц
		victim->desc->clan_olc.reset();
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_SPEND_GLORY)) {
		// или вливает-переливает славу
		victim->desc->glory.reset();
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_GLORY_CONST)) {
		// или вливает-переливает славу
		victim->desc->glory_const.reset();
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_MAP_MENU)) {
		// или ковыряет опции карты
		victim->desc->map_options.reset();
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	} else if (victim->desc && (STATE(victim->desc) == CON_TORC_EXCH)) {
		// или меняет гривны (чистить особо и нечего)
		STATE(victim->desc) = CON_PLAYING;
		message = true;
	}

	if (message) {
		send_to_char(victim, "На вас было совершено нападение, редактирование отменено!\r\n");
	}

	if (MOB_FLAGGED(ch, MOB_MEMORY) && GET_WAIT(ch) > 0) {
		if (!IS_NPC(victim)) {
			remember(ch, victim);
		} else if (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM)
					&& victim->has_master()
					&& !IS_NPC(victim->get_master())) {
						if (MOB_FLAGGED(victim, MOB_CLONE)) {
							remember(ch, victim->get_master());
						} else if (ch->isInSameRoom(victim->get_master()) && CAN_SEE(ch, victim->get_master())) {
									remember(ch, victim->get_master());
								}
		}
		return (FALSE);
	}
	hit(ch, victim, ESkill::SKILL_UNDEF, 
		AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) ? FightSystem::OFFHAND : FightSystem::MAIN_HAND);
	set_wait(ch, 2, TRUE);
	//ch->setSkillCooldown(SKILL_GLOBAL_COOLDOWN, 2);
	return (TRUE);
};

int onhorse(CHAR_DATA* ch) {
	if (on_horse(ch)) {
		act("Вам мешает $N.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return (TRUE);
	}
	return (FALSE);
};

void parry_override(CHAR_DATA * ch) {
	std::string message = "";
	if (GET_AF_BATTLE(ch, EAF_BLOCK)) {
		message = "Вы прекратили прятаться за щит и бросились в бой.";
		CLR_AF_BATTLE(ch, EAF_BLOCK);
	}
	if (GET_AF_BATTLE(ch, EAF_PARRY)) {
		message = "Вы прекратили парировать атаки и бросились в бой.";
		CLR_AF_BATTLE(ch, EAF_PARRY);
	}
	if (GET_AF_BATTLE(ch, EAF_MULTYPARRY)) {
		message = "Вы забыли о защите и бросились в бой.";
		CLR_AF_BATTLE(ch, EAF_MULTYPARRY);
	}
	act(message.c_str(), FALSE, ch, 0, 0, TO_CHAR);
}

int isHaveNoExtraAttack(CHAR_DATA * ch) {
	std::string message = "";
	parry_override(ch);
	if (ch->get_extra_victim()) {
		switch (ch->get_extra_attack_mode()) {
		case EXTRA_ATTACK_BASH:
			message = "Невозможно. Вы пытаетесь сбить $N3.";
			break;
		case EXTRA_ATTACK_KICK:
			message = "Невозможно. Вы пытаетесь пнуть $N3.";
			break;
		case EXTRA_ATTACK_CHOPOFF:
			message = "Невозможно. Вы пытаетесь подсечь $N3.";
			break;
		case EXTRA_ATTACK_DISARM:
			message = "Невозможно. Вы пытаетесь обезоружить $N3.";
			break;
		case EXTRA_ATTACK_THROW:
			message = "Невозможно. Вы пытаетесь метнуть оружие в $N3.";
			break;
        case EXTRA_ATTACK_CUT_PICK:
        case EXTRA_ATTACK_CUT_SHORTS:
            message = "Невозможно. Вы пытаетесь провести боевой прием против $N1.";
            break;
		default:
			return false;
		}
	} else {
		return true;
	};

	act(message.c_str(), FALSE, ch, 0, ch->get_extra_victim(), TO_CHAR);
	return false;
}

void do_assist(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_fighting()) {
		send_to_char("Невозможно. Вы сражаетесь сами.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	CHAR_DATA *helpee, *opponent;
	if (!*arg) {
		helpee = nullptr;
		for (const auto i : world[ch->in_room]->people) {
			if (i->get_fighting() && i->get_fighting() != ch
				&& ((ch->has_master() && ch->get_master() == i->get_master())
					|| ch->get_master() == i || i->get_master() == ch
					|| (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && ch->get_master() && ch->get_master()->get_master()
					&& (ch->get_master()->get_master() == i || ch->get_master()->get_master() == i->get_master())))) {
				helpee = i;
				break;
			}
		}

		if (!helpee) {
			send_to_char("Кому вы хотите помочь?\r\n", ch);
			return;
		}
	} else {
		if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
			send_to_char(NOPERSON, ch);
			return;
		}
	}
	if (helpee == ch) {
		send_to_char("Вам могут помочь только Боги!\r\n", ch);
		return;
	}

	if (helpee->get_fighting()) {
		opponent = helpee->get_fighting();
	} else {
		opponent = nullptr;
		for (const auto i : world[ch->in_room]->people) {
			if (i->get_fighting() == helpee) {
				opponent = i;
				break;
			}
		}
	}

	if (!opponent)
		act("Но никто не сражается с $N4!", FALSE, ch, 0, helpee, TO_CHAR);
	else if (!CAN_SEE(ch, opponent))
		act("Вы не видите противника $N1!", FALSE, ch, 0, helpee, TO_CHAR);
	else if (opponent == ch)
		act("Дык $E сражается с ВАМИ!", FALSE, ch, 0, helpee, TO_CHAR);
	else if (!may_kill_here(ch, opponent, NoArgument))
		return;
	else if (need_full_alias(ch, opponent))
		act("Используйте команду 'атаковать' для нападения на $N1.", FALSE, ch, 0, opponent, TO_CHAR);
	else if (set_hit(ch, opponent)) {
		act("Вы присоединились к битве, помогая $N2!", FALSE, ch, 0, helpee, TO_CHAR);
		act("$N решил$G помочь вам в битве!", 0, helpee, 0, ch, TO_CHAR);
		act("$n вступил$g в бой на стороне $N1.", FALSE, ch, 0, helpee, TO_NOTVICT | TO_ARENA_LISTEN);
	}
}

void do_hit(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd) {
	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Вы не видите цели.\r\n", ch);
		return;
	}
	if (vict == ch) {
		send_to_char("Вы ударили себя... Ведь больно же!\r\n", ch);
		act("$n ударил$g себя, и громко завопил$g 'Мамочка, больно ведь...'", FALSE, ch, 0, vict, TO_ROOM | CHECK_DEAF | TO_ARENA_LISTEN);
		act("$n ударил$g себя", FALSE, ch, 0, vict, TO_ROOM | CHECK_NODEAF | TO_ARENA_LISTEN);
		return;
	}
	if (!may_kill_here(ch, vict, argument)) {
		return;
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && (ch->get_master() == vict)) {
		act("$N слишком дорог для вас, чтобы бить $S.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if (subcmd != SCMD_MURDER && !check_pkill(ch, vict, arg)) {
		return;
	}

	if (ch->get_fighting()) {
		if (vict == ch->get_fighting()) {
			act("Вы уже сражаетесь с $N4.", FALSE, ch, 0, vict, TO_CHAR);
			return;
		}
		if (ch != vict->get_fighting()) {
			act("$N не сражается с вами, не трогайте $S.", FALSE, ch, 0, vict, TO_CHAR);
			return;
		}
		vict = try_protect(vict, ch);
		stop_fighting(ch, 2);
		set_fighting(ch, vict);
		set_wait(ch, 2, TRUE);
		//ch->setSkillCooldown(SKILL_GLOBAL_COOLDOWN, 2);
		return;
	}
	if ((GET_POS(ch) == POS_STANDING) && (vict != ch->get_fighting())) {
		set_hit(ch, vict);
	} else {
		send_to_char("Вам явно не до боя!\r\n", ch);
	}
}

void do_kill(CHAR_DATA *ch, char *argument, int cmd, int subcmd) {
	if (!IS_GRGOD(ch)) {
		do_hit(ch, argument, cmd, subcmd);
		return;
	};
	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы жизни лишить хотите-то?\r\n", ch);
		return;
	}
	if (ch == vict) {
		send_to_char("Вы мазохист... :(\r\n", ch);
		return;
	};
	if (IS_IMPL(vict) || PRF_FLAGGED(vict, PRF_CODERINFO)) {
		send_to_char("А если он вас чайником долбанет? Думай, Господи, думай!\r\n", ch);
	} else {
		act("Вы обратили $N3 в прах! Взглядом! Одним!", FALSE, ch, 0, vict, TO_CHAR);
		act("$N обратил$g вас в прах своим ненавидящим взором!", FALSE, vict, 0, ch, TO_CHAR);
		act("$n просто испепелил$g взглядом $N3!", FALSE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
		raw_kill(vict, ch);
	};
};

// ****************** CHARM ORDERS PROCEDURES
void do_order(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
	bool found = FALSE;
	room_rnum org_room;
	CHAR_DATA *vict;

	if (!ch)
		return;

	half_chop(argument, name, message);
	if (GET_GOD_FLAG(ch, GF_GODSCURSE)) {
		send_to_char("Вы прокляты Богами и никто не слушается вас!\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)) {
		send_to_char("Вы не в состоянии приказывать сейчас.\r\n", ch);
		return;
	}
	if (!*name || !*message)
		send_to_char("Приказать что и кому?\r\n", ch);
	else if (!(vict = get_char_vis(ch, name, FIND_CHAR_ROOM)) &&
			 !is_abbrev(name, "followers") && !is_abbrev(name, "все") && !is_abbrev(name, "всем"))
		send_to_char("Вы не видите такого персонажа.\r\n", ch);
	else if (ch == vict && !is_abbrev(name, "все") && !is_abbrev(name, "всем"))
		send_to_char("Вы начали слышать императивные голоса - срочно к психиатру!\r\n", ch);
	else
	{
		if (vict && !IS_NPC(vict) && !IS_GOD(ch)) {
			send_to_char(ch, "Игрокам приказывать могут только Боги!\r\n");
			return;
		}

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
			send_to_char("В таком состоянии вы не можете сами отдавать приказы.\r\n", ch);
			return;
		}

		if (vict
			&& !is_abbrev(name, "все")
			&& !is_abbrev(name, "всем")
			&& !is_abbrev(name, "followers")) {
			sprintf(buf, "$N приказал$g вам '%s'", message);
			act(buf, FALSE, vict, 0, ch, TO_CHAR | CHECK_DEAF);
			act("$n отдал$g приказ $N2.", FALSE, ch, 0, vict, TO_ROOM | CHECK_DEAF);

			if (vict->get_master() != ch
				|| !AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_DEAFNESS)) {
				if (!IS_POLY(vict)) {
					act("$n безразлично смотрит по сторонам.", FALSE, vict, 0, 0, TO_ROOM);
				} else {
					act("$n безразлично смотрят по сторонам.", FALSE, vict, 0, 0, TO_ROOM);
				}
			} else {
				send_to_char(OK, ch);
				if (vict->get_wait() <= 0) {
					command_interpreter(vict, message);
				} else if (vict->get_fighting()) {
					if (vict->last_comm != NULL) {
						free(vict->last_comm);
					}
					vict->last_comm = str_dup(message);
				}
			}
		} else {
			org_room = ch->in_room;
			act("$n отдал$g приказ.", FALSE, ch, 0, 0, TO_ROOM | CHECK_DEAF);

			CHAR_DATA::followers_list_t followers = ch->get_followers_list();

			for (const auto follower : followers) {
				if (org_room != follower->in_room) {
					continue;
				}

				if (AFF_FLAGGED(follower, EAffectFlag::AFF_CHARM)
					&& !AFF_FLAGGED(follower, EAffectFlag::AFF_DEAFNESS)) {
					found = TRUE;
					if (follower->get_wait() <= 0) {
						command_interpreter(follower, message);
					} else if (follower->get_fighting()) {
						if (follower->last_comm != NULL) {
							free(follower->last_comm);
						}
						follower->last_comm = str_dup(message);
					}
				}
			}

			if (found) {
				send_to_char(OK, ch);
			} else {
				send_to_char("Вы страдаете манией величия!\r\n", ch);
			}
		}
	}
}

void drop_from_horse(CHAR_DATA *victim) {
	if (on_horse(victim)) {
		act("Вы упали с $N1.", FALSE, victim, 0, get_horse(victim), TO_CHAR);
		AFF_FLAGS(victim).unset(EAffectFlag::AFF_HORSE);
	}

	if (IS_HORSE(victim) && on_horse(victim->get_master())) {
		horse_drop(victim);
	}
}



// ************* TOUCH PROCEDURES
void go_touch(CHAR_DATA * ch, CHAR_DATA * vict) {
	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	act("Вы попытаетесь перехватить следующую атаку $N1.", FALSE, ch, 0, vict, TO_CHAR);
	SET_AF_BATTLE(ch, EAF_TOUCH);
	ch->set_touching(vict);
}

void do_touch(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(SKILL_TOUCH)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_TOUCH)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	OBJ_DATA *primary = GET_EQ(ch, WEAR_WIELD) ? GET_EQ(ch, WEAR_WIELD) : GET_EQ(ch,
						WEAR_BOTHS);
	if (!(IS_IMMORTAL(ch) || IS_NPC(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE) ||	!primary)) {
		send_to_char("У вас заняты руки.\r\n", ch);
		return;
	}

	CHAR_DATA *vict = nullptr;
	one_argument(argument, arg);
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		for (const auto i : world[ch->in_room]->people) {
			if (i->get_fighting() == ch) {
				vict = i;
				break;
			}
		}

		if (!vict) {
			if (!ch->get_fighting()) {
				send_to_char("Но вы ни с кем не сражаетесь.\r\n", ch);
				return;
			} else {
				vict = ch->get_fighting();
			}
		}
	}

	if (ch == vict) {
		send_to_char(GET_NAME(ch), ch);
		send_to_char(", вы похожи на котенка, ловящего собственный хвост.\r\n", ch);
		return;
	}
	if (vict->get_fighting() != ch && ch->get_fighting() != vict) {
		act("Но вы не сражаетесь с $N4.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (GET_AF_BATTLE(ch, EAF_MIGHTHIT)) {
		send_to_char("Невозможно. Вы приготовились к богатырскому удару.\r\n", ch);
		return;
	}

	if (!check_pkill(ch, vict, arg))
		return;

	parry_override(ch);

	go_touch(ch, vict);
}

// ************* DEVIATE PROCEDURES
void go_deviate(CHAR_DATA * ch) {
	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (onhorse(ch)) {
		return;
	};
	SET_AF_BATTLE(ch, EAF_DEVIATE);
	send_to_char("Хорошо, вы попытаетесь уклониться от следующей атаки!\r\n", ch);
}

void do_deviate(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(SKILL_DEVIATE)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_DEVIATE)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!(ch->get_fighting())) {
		send_to_char("Но вы ведь ни с кем не сражаетесь!\r\n", ch);
		return;
	}

	if (onhorse(ch)) {
		return;
	}

	if (GET_AF_BATTLE(ch, EAF_DEVIATE)) {
		send_to_char("Вы и так вертитесь, как волчок.\r\n", ch);
		return;
	};
	go_deviate(ch);
}

const char *cstyles[] = { "normal",
						  "обычный",
						  "punctual",
						  "точный",
						  "awake",
						  "осторожный",
						  "\n"
						};

void do_style(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->haveCooldown(SKILL_GLOBAL_COOLDOWN)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	int tp;
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Вы сражаетесь %s стилем.\r\n",
			PRF_FLAGS(ch).get(PRF_PUNCTUAL) ? "точным" : PRF_FLAGS(ch).get(PRF_AWAKE) ? "осторожным" : "обычным");
		return;
	}
	if (tryFlipActivatedFeature(ch, argument)) {
		return;
	}
	if ((tp = search_block(arg, cstyles, FALSE)) == -1) {
		send_to_char("Формат: стиль { название стиля }\r\n", ch);
		return;
	}
	tp >>= 1;
	if ((tp == 1 && !ch->get_skill(SKILL_PUNCTUAL)) || (tp == 2 && !ch->get_skill(SKILL_AWAKE))) {
		send_to_char("Вам неизвестен такой стиль боя.\r\n", ch);
		return;
	}

	switch (tp) {
	case 0:
	case 1:
	case 2:
		PRF_FLAGS(ch).unset(PRF_PUNCTUAL);
		PRF_FLAGS(ch).unset(PRF_AWAKE);

		if (tp == 1) {
			PRF_FLAGS(ch).set(PRF_PUNCTUAL);
		}
		if (tp == 2) {
			PRF_FLAGS(ch).set(PRF_AWAKE);
		}

		if (ch->get_fighting() && !(AFF_FLAGGED(ch, EAffectFlag::AFF_COURAGE) ||
							  AFF_FLAGGED(ch, EAffectFlag::AFF_DRUNKED) || AFF_FLAGGED(ch, EAffectFlag::AFF_ABSTINENT))) {
			CLR_AF_BATTLE(ch, EAF_PUNCTUAL);
			CLR_AF_BATTLE(ch, EAF_AWAKE);
			if (tp == 1)
				SET_AF_BATTLE(ch, EAF_PUNCTUAL);
			else if (tp == 2)
				SET_AF_BATTLE(ch, EAF_AWAKE);
		}
		send_to_char(ch, "Вы выбрали %s%s%s стиль боя.\r\n",
				CCRED(ch, C_SPR), tp == 0 ? "обычный" : tp == 1 ? "точный" : "осторожный", CCNRM(ch, C_OFF));
		break;
	}

	ch->setSkillCooldown(SKILL_GLOBAL_COOLDOWN, 2);
}

// ***************** STOPFIGHT
void do_stopfight(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (!ch->get_fighting() || IS_NPC(ch)) {
		send_to_char("Но вы же ни с кем не сражаетесь.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("Из этого положения отступать невозможно.\r\n", ch);
		return;
	}

	if (PRF_FLAGS(ch).get(PRF_IRON_WIND) || AFF_FLAGGED(ch, EAffectFlag::AFF_LACKY)) {
		send_to_char("Вы не желаете отступать, не расправившись со всеми врагами!\r\n", ch);
		return;
	}

	CHAR_DATA* tmp_ch = nullptr;
	for (const auto i : world[ch->in_room]->people) {
		if (i->get_fighting() == ch) {
			tmp_ch = i;
			break;
		}
	}

	if (tmp_ch) {
		send_to_char("Невозможно, вы сражаетесь за свою жизнь.\r\n", ch);
		return;
	} else {
		stop_fighting(ch, TRUE);
		if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
			WAIT_STATE(ch, PULSE_VIOLENCE);
		send_to_char("Вы отступили из битвы.\r\n", ch);
		act("$n выбыл$g из битвы.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	}
}



void do_manadrain(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {

	struct timed_type timed;
	int drained_mana, prob, percent, skill;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !ch->get_skill(SKILL_MANADRAIN)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}

	if (timed_by_skill(ch, SKILL_MANADRAIN) || ch->haveCooldown(SKILL_MANADRAIN)) {
		send_to_char("Так часто не получится.\r\n", ch);
		return;
	}

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы столь сильно ненавидите?\r\n", ch);
		return;
	}

	if (ch == vict) {
		send_to_char("Вы укусили себя за левое ухо.\r\n", ch);
		return;
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) || ROOM_FLAGGED(ch->in_room, ROOM_NOBATTLE)) {
		send_to_char("Поищите другое место для выражения своих кровожадных наклонностей.\r\n", ch);
		return;
	}

	if (!IS_NPC(vict)) {
		send_to_char("На живом человеке? Креста не вас нет!\r\n", ch);
		return;
	}

	if (affected_by_spell(vict, SPELL_SHIELD) || MOB_FLAGGED(vict, MOB_PROTECT)) {
		send_to_char("Боги хранят вашу жертву.\r\n", ch);
		return;
	}

	skill = ch->get_skill(SKILL_MANADRAIN);

	percent = number(1, skill_info[SKILL_MANADRAIN].max_percent);
	prob = MAX(20, 90 - 5 * MAX(0, GET_LEVEL(vict) - GET_LEVEL(ch)));
	improve_skill(ch, SKILL_MANADRAIN, percent > prob, vict);

	Damage manadrainDamage(SkillDmg(SKILL_MANADRAIN), ZERO_DMG, MAGE_DMG);
	manadrainDamage.magic_type = STYPE_DARK;
	if (percent <= prob) {
		skill = MAX(10, skill - 10 * MAX(0, GET_LEVEL(ch) - GET_LEVEL(vict)));
		drained_mana = (GET_MAX_MANA(ch) - GET_MANA_STORED(ch)) * skill / 100;
		GET_MANA_STORED(ch) = MIN(GET_MAX_MANA(ch), GET_MANA_STORED(ch) + drained_mana);
		manadrainDamage.dam = 10;
	}
	manadrainDamage.process(ch, vict);

	if (!IS_IMMORTAL(ch)) {
		timed.skill = SKILL_MANADRAIN;
		timed.time = 6 - MIN(4, (ch->get_skill(SKILL_MANADRAIN) + 30) / 50);
		timed_to_char(ch, &timed);
	}

}

extern char cast_argument[MAX_INPUT_LENGTH];
void do_townportal(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {

	struct char_portal_type *tmp, *dlt = NULL;
	char arg2[MAX_INPUT_LENGTH];
	int vnum = 0;

	if (IS_NPC(ch) || !ch->get_skill(SKILL_TOWNPORTAL)) {
		send_to_char("Прежде изучите секрет постановки врат.\r\n", ch);
		return;
	}

	two_arguments(argument, arg, arg2);
	if (!str_cmp(arg, "забыть")) {
		vnum = find_portal_by_word(arg2);
		for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next) {
			if (tmp->vnum == vnum) {
				if (dlt) {
					dlt->next = tmp->next;
				} else {
					GET_PORTALS(ch) = tmp->next;
				}
				free(tmp);
				sprintf(buf, "Вы полностью забыли, как выглядит камень '&R%s&n'.\r\n", arg2);
				send_to_char(buf, ch);
				break;
			}
			dlt = tmp;
		}
		return;
	}

	*cast_argument = '\0';
	if (argument)
		strcat(cast_argument, arg);
	spell_townportal(GET_LEVEL(ch), ch, NULL, NULL);
}

void do_turn_undead(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/) {

	if (!ch->get_skill(SKILL_TURN_UNDEAD)) {
		send_to_char("Вам это не по силам.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_TURN_UNDEAD)) {
		send_to_char("Вам нужно набраться сил для применения этого навыка.\r\n", ch);
		return;
	};

	int skillTurnUndead = ch->get_skill(SKILL_TURN_UNDEAD);
	timed_type timed;
	timed.skill = SKILL_TURN_UNDEAD;
	if (can_use_feat(ch, EXORCIST_FEAT)) {
		timed.time = timed_by_skill(ch, SKILL_TURN_UNDEAD) + HOURS_PER_TURN_UNDEAD - 2;
		skillTurnUndead += 10;
	} else {
		timed.time = timed_by_skill(ch, SKILL_TURN_UNDEAD) + HOURS_PER_TURN_UNDEAD;
	}
	if (timed.time > HOURS_PER_DAY) {
		send_to_char("Вам пока не по силам изгонять нежить, нужно отдохнуть.\r\n", ch);
		return;
	}
	timed_to_char(ch, &timed);

	send_to_char(ch, "Вы свели руки в магическом жесте и отовсюду хлынули яркие лучи света.\r\n");
	act("$n свел$g руки в магическом жесте и отовсюду хлынули яркие лучи света.\r\n", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

// костылиии... и магик намберы
	int victimsAmount = 20;
	int victimssHPAmount = skillTurnUndead*25 + MAX(0, skillTurnUndead - 80)*50;
	Damage turnUndeadDamage(SkillDmg(SKILL_TURN_UNDEAD), ZERO_DMG, MAGE_DMG);
	turnUndeadDamage.magic_type = STYPE_LIGHT;
	turnUndeadDamage.flags.set(IGNORE_FSHIELD);
	TechniqueRollType turnUndeadRoll;
	ActionTargeting::FoesRosterType roster{ch, [](CHAR_DATA*, CHAR_DATA* target) {return IS_UNDEAD(target);}};
	for (const auto target : roster) {
		turnUndeadDamage.dam = ZERO_DMG;
		turnUndeadRoll.initialize(ch, TURN_UNDEAD_FEAT, target);
		if (turnUndeadRoll.isSuccess()) {
			if (turnUndeadRoll.isCriticalSuccess() && ch->get_level() > target->get_level() + dice(1, 5)) {
				send_to_char(ch, "&GВы окончательно изгнали %s из мира!&n\r\n", GET_PAD(target, 3));
				turnUndeadDamage.dam = MAX(1, GET_HIT(target) + 11);
			} else {
				turnUndeadDamage.dam = turnUndeadRoll.calculateDamage();
				victimssHPAmount -= turnUndeadDamage.dam;
			};
		} else if (turnUndeadRoll.isCriticalFail() && !IS_CHARMICE(target)) {
			act("&BВаши жалкие лучи света лишь привели $n3 в ярость!\r\n&n", FALSE, target, 0, ch, TO_VICT);
			act("&BЧахлый луч света $N1 лишь привел $n3 в ярость!\r\n&n", FALSE, target, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			AFFECT_DATA<EApplyLocation> af[2];
			af[0].type = SPELL_COURAGE;
			af[0].duration = pc_duration(target, 3, 0, 0, 0, 0);
			af[0].modifier = MAX(1, turnUndeadRoll.getDegreeOfSuccess()*2);
			af[0].location = APPLY_DAMROLL;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			af[0].battleflag = 0;
			af[1].type = SPELL_COURAGE;
			af[1].duration = pc_duration(target, 3, 0, 0, 0, 0);
			af[1].modifier = MAX(1, 25 + turnUndeadRoll.getDegreeOfSuccess()*5);
			af[1].location = APPLY_HITREG;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			af[1].battleflag = 0;
			affect_join(target, af[0], TRUE, FALSE, TRUE, FALSE);
			affect_join(target, af[1], TRUE, FALSE, TRUE, FALSE);
		};
		turnUndeadDamage.process(ch, target);
		if (!target->purged() && turnUndeadRoll.isSuccess() && !MOB_FLAGGED(target, MOB_NOFEAR)
			&& !general_savingthrow(ch, target, SAVING_WILL, GET_REAL_WIS(ch) + GET_REAL_INT(ch))) {
			go_flee(target);
		};
		--victimsAmount;
		if (victimsAmount == 0 || victimssHPAmount <= 0) {
			break;
		};
	};
	//set_wait(ch, 1, TRUE);
	setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
	setSkillCooldownInFight(ch, SKILL_TURN_UNDEAD, 2);
}

void ApplyNoFleeAffect(CHAR_DATA *ch, int duration) {
    //Это жутко криво, но почемук-то при простановке сразу 2 флагов битвектора начинаются глюки, хотя все должно быть нормально
    //По-видимому, где-то просто не учтено, что ненулевых битов может быть более 1
	AFFECT_DATA<EApplyLocation> Noflee;
	Noflee.type = SPELL_BATTLE;
	Noflee.bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
	Noflee.location = EApplyLocation::APPLY_NONE;
	Noflee.modifier = 0;
	Noflee.duration = pc_duration(ch, duration, 0, 0, 0, 0);;
	Noflee.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
	affect_join(ch, Noflee, TRUE, FALSE, TRUE, FALSE);

    AFFECT_DATA<EApplyLocation> Battle;
    Battle.type = SPELL_BATTLE;
    Battle.bitvector = to_underlying(EAffectFlag::AFF_EXPEDIENT);
    Battle.location = EApplyLocation::APPLY_NONE;
    Battle.modifier = 0;
    Battle.duration = pc_duration(ch, duration, 0, 0, 0, 0);;
    Battle.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
    affect_join(ch, Battle, TRUE, FALSE, TRUE, FALSE);

    send_to_char("Вы выпали из ритма боя.\r\n", ch);
}

ESkill ExpedientWeaponSkill(CHAR_DATA *ch) {
	ESkill skill = SKILL_PUNCH;

	if (GET_EQ(ch, WEAR_WIELD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == CObjectPrototype::ITEM_WEAPON)) {
        skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_WIELD));
	} else if (GET_EQ(ch, WEAR_BOTHS) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS)) == CObjectPrototype::ITEM_WEAPON)) {
        skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS));
	} else if (GET_EQ(ch, WEAR_HOLD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == CObjectPrototype::ITEM_WEAPON)) {
        skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_HOLD));
	};

	return skill;
}

int GetExpedientKeyParameter(CHAR_DATA *ch, ESkill skill) {
    switch (skill) {
	case SKILL_PUNCH:
	case SKILL_CLUBS:
	case SKILL_AXES:
	case SKILL_BOTHHANDS:
	case SKILL_SPADES:
        return ch->get_str();
        break;
	case SKILL_LONGS:
	case SKILL_SHORTS:
	case SKILL_NONSTANDART:
	case SKILL_BOWS:
	case SKILL_PICK:
        return ch->get_dex();
        break;
	default:
        return ch->get_str();
    }
}

int ParameterBonus(int parameter) {
    return ((parameter-20)/4);
}

int ExpedientRating(CHAR_DATA *ch, ESkill skill) {
	return (ch->get_skill(skill)/2.00+ParameterBonus(GetExpedientKeyParameter(ch, skill)));
}

int ExpedientCap(CHAR_DATA *ch, ESkill skill) {
	if (!IS_NPC(ch)) {
        return floor(1.33*(MAX_EXP_RMRT_PERCENT(ch)/2.00+ParameterBonus(GetExpedientKeyParameter(ch, skill))));
    } else {
        return floor(1.33*((MAX_EXP_PERCENT+5*MAX(0,GET_LEVEL(ch)-30)/2.00+ParameterBonus(GetExpedientKeyParameter(ch, skill)))));
    }
}

int DegreeOfSuccess(int roll, int rating) {
    return ((rating-roll)/5);
}

bool CheckExpedientSuccess(CHAR_DATA *ch, CHAR_DATA *victim) {
    ESkill DoerSkill = ExpedientWeaponSkill(ch);
    int DoerRating = ExpedientRating(ch, DoerSkill);
    int DoerCap = ExpedientCap(ch, DoerSkill);
    int DoerRoll = dice(1, DoerCap);
    int DoerSuccess = DegreeOfSuccess(DoerRoll, DoerRating);

    ESkill VictimSkill = ExpedientWeaponSkill(victim);
    int VictimRating = ExpedientRating(victim, VictimSkill);
    int VictimCap = ExpedientCap(victim, VictimSkill);
    int VictimRoll = dice(1, VictimCap);
    int VictimSuccess = DegreeOfSuccess(VictimRoll, VictimRating);

    if ((DoerRoll <= DoerRating) && (VictimRoll > VictimRating))
        return true;
    if ((DoerRoll > DoerRating) && (VictimRoll <= VictimRating))
        return false;
    if ((DoerRoll > DoerRating) && (VictimRoll > VictimRating))
        return CheckExpedientSuccess(ch, victim);

    if (DoerSuccess > VictimSuccess)
        return true;
    if (DoerSuccess < VictimSuccess)
        return false;

    if (ParameterBonus(GetExpedientKeyParameter(ch, DoerSkill)) > ParameterBonus(GetExpedientKeyParameter(victim, VictimSkill)))
        return true;
    if (ParameterBonus(GetExpedientKeyParameter(ch, DoerSkill)) < ParameterBonus(GetExpedientKeyParameter(victim, VictimSkill)))
        return false;

    if (DoerRoll < VictimRoll)
        return true;
    if (DoerRoll > VictimRoll)
        return true;

    return CheckExpedientSuccess(ch, victim);
}

void go_cut_shorts(CHAR_DATA * ch, CHAR_DATA * vict) {

	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_EXPEDIENT)) {
		send_to_char("Вы еще не восстановил равновесие после предыдущего приема.\r\n", ch);
		return;
	}

	vict = try_protect(vict, ch);

    if (!CheckExpedientSuccess(ch, vict)) {
        act("Ваши свистящие удары пропали втуне, не задев $N3.", FALSE, ch, 0, vict, TO_CHAR);
		Damage dmg(SkillDmg(SKILL_SHORTS), ZERO_DMG, PHYS_DMG);
		dmg.process(ch, vict);
		ApplyNoFleeAffect(ch, 2);
		return;
    }

    act("$n сделал$g неуловимое движение и на мгновение исчез$q из вида.", FALSE, ch, 0, vict, TO_VICT);
    act("$n сделал$g неуловимое движение, сместившись за спину $N1.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    hit(ch, vict, ESkill::SKILL_UNDEF, FightSystem::MAIN_HAND);
    hit(ch, vict, ESkill::SKILL_UNDEF, FightSystem::OFFHAND);

    AFFECT_DATA<EApplyLocation> AffectImmunPhysic;
    AffectImmunPhysic.type = SPELL_EXPEDIENT;
    AffectImmunPhysic.location = EApplyLocation::APPLY_PR;
    AffectImmunPhysic.modifier = 100;
    AffectImmunPhysic.duration = pc_duration(ch, 3, 0, 0, 0, 0);
    AffectImmunPhysic.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
    affect_join(ch, AffectImmunPhysic, FALSE, FALSE, FALSE, FALSE);
    AFFECT_DATA<EApplyLocation> AffectImmunMagic;
    AffectImmunMagic.type = SPELL_EXPEDIENT;
    AffectImmunMagic.location = EApplyLocation::APPLY_MR;
    AffectImmunMagic.modifier = 100;
    AffectImmunMagic.duration = pc_duration(ch, 3, 0, 0, 0, 0);
    AffectImmunMagic.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
    affect_join(ch, AffectImmunMagic, FALSE, FALSE, FALSE, FALSE);

    ApplyNoFleeAffect(ch, 3);
}

void SetExtraAttackCutShorts(CHAR_DATA *ch, CHAR_DATA *victim) {
    if (!isHaveNoExtraAttack(ch)) {
        return;
	}

	if (!pk_agro_action(ch, victim)) {
		return;
	}


    if (!ch->get_fighting()) {
        act("Ваше оружие свистнуло, когда вы бросились на $N3, применив \"порез\".", FALSE, ch, 0, victim, TO_CHAR);
        set_fighting(ch, victim);
        ch->set_extra_attack(EXTRA_ATTACK_CUT_SHORTS, victim);
    } else {
        act("Хорошо. Вы попытаетесь порезать $N3.", FALSE, ch, 0, victim, TO_CHAR);
        ch->set_extra_attack(EXTRA_ATTACK_CUT_SHORTS, victim);
	}
}

void SetExtraAttackCutPick(CHAR_DATA *ch, CHAR_DATA *victim) {
    if (!isHaveNoExtraAttack(ch)) {
        return;
	}
	if (!pk_agro_action(ch, victim)) {
		return;
	}

    if (!ch->get_fighting()) {
        act("Вы перехватили оружие обратным хватом и проскользнули за спину $N1.", FALSE, ch, 0, victim, TO_CHAR);
        set_fighting(ch, victim);
        ch->set_extra_attack(EXTRA_ATTACK_CUT_PICK, victim);
    } else {
        act("Хорошо. Вы попытаетесь порезать $N3.", FALSE, ch, 0, victim, TO_CHAR);
        ch->set_extra_attack(EXTRA_ATTACK_CUT_PICK, victim);
	}
}

ESkill GetExpedientCutSkill(CHAR_DATA *ch) {
    ESkill skill = SKILL_INVALID;

	if (GET_EQ(ch, WEAR_WIELD) && GET_EQ(ch, WEAR_HOLD)) {
        skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_WIELD));
        if (skill != GET_OBJ_SKILL(GET_EQ(ch, WEAR_HOLD))) {
            send_to_char("Для этого приема в обеих руках нужно держать оружие одого типа!\r\n", ch);
            return SKILL_INVALID;
        }
	} else if (GET_EQ(ch, WEAR_BOTHS)) {
        skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS));
	} else {
		send_to_char("Для этого приема вам надо использовать одинаковое оружие в обеих руках либо двуручное.\r\n", ch);
		return SKILL_INVALID;
	}

	if (!can_use_feat(ch, find_weapon_master_by_skill(skill)) && !IS_IMPL(ch)) {
        send_to_char("Вы недостаточно искусны в обращении с этим видом оружия.\r\n", ch);
        return SKILL_INVALID;
    }

    return skill;
}

void do_expedient_cut(CHAR_DATA *ch, char *argument, int/* cmd*/, int /*subcmd*/) {

    ESkill skill;

	if (IS_NPC(ch) || (!can_use_feat(ch, EXPEDIENT_CUT_FEAT) && !IS_IMPL(ch))) {
		send_to_char("Вы не владеете таким приемом.\r\n", ch);
		return;
	}

	if (onhorse(ch)) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

    if (!isHaveNoExtraAttack(ch)) {
        return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы хотите порезать?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы таки да? Ой-вей, но тут таки Древняя Русь, а не Палестина!\r\n", ch);
		return;
	}
	if (ch->get_fighting() && vict->get_fighting() != ch) {
        act("$N не сражается с вами, не трогайте $S.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

    skill = GetExpedientCutSkill(ch);
    if (skill == SKILL_INVALID)
        return;

    switch (skill) {
    case SKILL_SHORTS:
        SetExtraAttackCutShorts(ch, vict);
    break;
    case SKILL_SPADES:
        SetExtraAttackCutShorts(ch, vict);
    break;
    case SKILL_LONGS:
    case SKILL_BOTHHANDS:
        send_to_char("Порез мечом (а тем более двуручником или копьем) - это сурьезно. Но пока невозможно.\r\n", ch);
    break;
    default:
        send_to_char("Ваше оружие не позволяет провести такой прием.\r\n", ch);
    }

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
