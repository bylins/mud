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
void go_protect(CHAR_DATA * ch, CHAR_DATA * vict);
void go_stun(CHAR_DATA * ch, CHAR_DATA * vict);

// local functions
void do_assist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_hit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_kill(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_backstab(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_order(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_flee(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_bash(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_rescue(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_kick(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_manadrain(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_strangle(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_expedient_cut(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
CHAR_DATA *try_protect(CHAR_DATA * victim, CHAR_DATA * ch);

inline bool dontCanAct(CHAR_DATA *ch) {
	return (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT));
}

int have_mind(CHAR_DATA * ch) {
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && !IS_HORSE(ch))
		return (TRUE);
	return (FALSE);
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

	hit(ch, victim, TYPE_UNDEFINED, AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) ? LEFT_WEAPON : RIGHT_WEAPON);
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

CHAR_DATA* try_protect(CHAR_DATA* victim, CHAR_DATA* ch) {
	int percent = 0;
	int prob = 0;
	bool protect = false;

	if (ch->get_fighting() == victim)
		return victim;

	for (const auto vict : world[IN_ROOM(victim)]->people) {
		if (vict->get_protecting() == victim
			&& !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPFIGHT)
			&& !AFF_FLAGGED(vict, EAffectFlag::AFF_MAGICSTOPFIGHT)
			&& !AFF_FLAGGED(vict, EAffectFlag::AFF_BLIND)
			&& !GET_MOB_HOLD(vict)
			&& GET_POS(vict) >= POS_FIGHTING) {
			if (vict == ch) {
				act("Вы попытались напасть на того, кого прикрывали, и замерли в глубокой задумчивости.", FALSE, vict, 0, victim, TO_CHAR);
				act("$N пытается напасть на вас! Лучше бы вам отойти.", FALSE, victim, 0, vict, TO_CHAR);
				vict->set_protecting(0);
				vict->BattleAffects.unset(EAF_PROTECT);
				WAIT_STATE(vict, PULSE_VIOLENCE);
				AFFECT_DATA<EApplyLocation> af;
				af.type = SPELL_BATTLE;
				af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
				af.location = EApplyLocation::APPLY_NONE;
				af.modifier = 0;
				af.duration = pc_duration(vict, 1, 0, 0, 0, 0);
				af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
				affect_join(vict, af, TRUE, FALSE, TRUE, FALSE);
				return victim;
			}

			if (protect) {
				send_to_char(vict, "Чьи-то широкие плечи помешали вам прикрыть %s.\r\n", GET_PAD(vict->get_protecting(), 3));
				continue;
			}

			protect = true;
			percent = number(1, skill_info[SKILL_PROTECT].max_percent);
			prob = calculate_skill(vict, SKILL_PROTECT, victim);
			prob = prob * 8 / 10;
			if (vict->haveCooldown(SKILL_PROTECT)) {
				prob /= 2;
			};
			improve_skill(vict, SKILL_PROTECT, prob >= percent, ch);

			if (GET_GOD_FLAG(vict, GF_GODSCURSE))
				prob = 0;

			if ((vict->get_fighting() != ch) && (ch != victim)) {
				// агрим жертву после чего можно будет проверить возможно ли его здесь прикрыть(костыли конечно)
				if (!pk_agro_action(ch, victim))
					return victim;
				if (!may_kill_here(vict, ch, NoArgument))
					continue;
				// Вписываемся в противника прикрываемого ...
				stop_fighting(vict, FALSE);
				set_fighting(vict, ch);
			}

			if (prob < percent) {
				act("Вы не смогли прикрыть $N3.", FALSE, vict, 0, victim, TO_CHAR);
				act("$N не смог$Q прикрыть вас.", FALSE, victim, 0, vict, TO_CHAR);
				act("$n не смог$q прикрыть $N3.", TRUE, vict, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				//set_wait(vict, 3, TRUE);
				setSkillCooldownInFight(vict, SKILL_GLOBAL_COOLDOWN, 3);
			} else {
				if (!pk_agro_action(vict, ch))
					return victim; // по аналогии с реском прикрывая кого-то можно пофлагаться
				act("Вы героически прикрыли $N3, приняв удар на себя.", FALSE, vict, 0, victim, TO_CHAR);
				act("$N героически прикрыл$G вас, приняв удар на себя.", FALSE, victim, 0, vict, TO_CHAR);
				act("$n героически прикрыл$g $N3, приняв удар на себя.", TRUE, vict, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				//set_wait(vict, 1, TRUE);
				setSkillCooldownInFight(vict, SKILL_GLOBAL_COOLDOWN, 1);
				setSkillCooldownInFight(vict, SKILL_PROTECT, 1);
				return vict;
			}
		}
	}
	return victim;
}

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


// *********************** BACKSTAB VICTIM
// Проверка на стаб в бою происходит до вызова этой функции
void go_backstab(CHAR_DATA * ch, CHAR_DATA * vict) {
	int percent, prob;

	if (onhorse(ch))
		return;

	vict = try_protect(vict, ch);

	if (!pk_agro_action(ch, vict))
		return;

	if ((MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) && !IS_GOD(ch)) {
		act("Вы заметили, что $N попытал$u вас заколоть!", FALSE, vict, 0, ch, TO_CHAR);
		act("$n заметил$g вашу попытку заколоть $s!", FALSE, vict, 0, ch, TO_VICT);
		act("$n заметил$g попытку $N1 заколоть $s!", FALSE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		set_hit(vict, ch);
		return;
	}

	percent = number(1, skill_info[SKILL_BACKSTAB].max_percent - GET_REMORT(ch) * 2);
	prob = train_skill(ch, SKILL_BACKSTAB, skill_info[SKILL_BACKSTAB].max_percent, vict);

	if (can_use_feat(ch, SHADOW_STRIKE_FEAT)) {
		prob = prob + prob * 20 / 100;
	};

	if (vict->get_fighting())
		prob = prob * (GET_REAL_DEX(ch) + 50) / 100;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE))
		prob += 5;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_NOOB_REGEN))
		prob += 5;
	if (GET_MOB_HOLD(vict))
		prob = prob * 5 / 4;
	if (GET_GOD_FLAG(vict, GF_GODSCURSE))
		prob = percent;
	if (GET_GOD_FLAG(vict, GF_GODSLIKE) || GET_GOD_FLAG(ch, GF_GODSCURSE))
		prob = 0;
	if (percent > prob) {
		Damage dmg(SkillDmg(SKILL_BACKSTAB), ZERO_DMG, PHYS_DMG);
		dmg.process(ch, vict);
	} else {
		hit(ch, vict, SKILL_BACKSTAB, RIGHT_WEAPON);
		//if (!ch->get_fighting())
			//WAIT_STATE(ch, PULSE_VIOLENCE / 6);
	}
	//set_wait(ch, 2, TRUE);
	setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
	setSkillCooldownInFight(ch, SKILL_BACKSTAB, 2);
}

void do_backstab(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_skill(SKILL_BACKSTAB) < 1) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_BACKSTAB)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (onhorse(ch)) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	CHAR_DATA* vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	if (!vict) {
		send_to_char("Кого вы так сильно ненавидите, что хотите заколоть?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы, определенно, садомазохист!\r\n", ch);
		return;
	}

	if (!GET_EQ(ch, WEAR_WIELD) && !IS_NPC(ch)) {
		send_to_char("Требуется держать оружие в правой руке.\r\n", ch);
		return;
	}

	if (!IS_NPC(ch) && GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != type_pierce) {
		send_to_char("ЗаКОЛоть можно только КОЛющи оружием!\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (vict->get_fighting() && !can_use_feat(ch, THIEVES_STRIKE_FEAT)) {
		send_to_char("Ваша цель слишком быстро движется - вы можете пораниться!\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument) && !IS_NPC(ch))
		return;
	if (!check_pkill(ch, vict, arg) && !IS_NPC(ch))
		return;
	go_backstab(ch, vict);
}

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

void reduce_exp_after_flee(CHAR_DATA* ch, CHAR_DATA* victim, room_rnum room)
{
	if (can_use_feat(ch, RETREAT_FEAT) || ROOM_FLAGGED(room, ROOM_ARENA))
		return;

	const auto loss = MAX(1, GET_REAL_MAX_HIT(victim) - GET_HIT(victim)) * GET_LEVEL(victim);
	gain_exp(ch, -loss);
}

// ********************* FLEE PROCEDURE
void go_flee(CHAR_DATA* ch)
{
	if (GET_MOB_HOLD(ch) || GET_WAIT(ch) > 0) {
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_NOFLEE) || AFF_FLAGGED(ch, EAffectFlag::AFF_LACKY) || PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Невидимые оковы мешают вам сбежать.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("Вы не можете сбежать из этого положения.\r\n", ch);
		return;
	}

	if (!WAITLESS(ch))
		WAIT_STATE(ch, PULSE_VIOLENCE);

	if (on_horse(ch) && (GET_POS(get_horse(ch)) < POS_FIGHTING || GET_MOB_HOLD(get_horse(ch)))) {
		send_to_char("Ваш скакун не в состоянии вынести вас из боя!\r\n", ch);
		return;
	}

	int dirs[NUM_OF_DIRS];
	int correct_dirs = 0;

	for (auto i = 0; i < NUM_OF_DIRS; ++i) {
		if (legal_dir(ch, i, TRUE, FALSE) && !ROOM_FLAGGED(EXIT(ch, i)->to_room(), ROOM_DEATH)) {
			dirs[correct_dirs] = i;
			++correct_dirs;
		}
	}

	if (correct_dirs > 0 && !bernoulli_trial(std::pow((1.0 - static_cast<double>(correct_dirs) / NUM_OF_DIRS), NUM_OF_DIRS))) {
		const auto direction = dirs[number(0, correct_dirs - 1)];
		const auto was_fighting = ch->get_fighting();
		const auto was_in = ch->in_room;

		if (do_simple_move(ch, direction, TRUE, 0, true)) {
			act("$n запаниковал$g и пытал$u сбежать!", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			send_to_char("Вы быстро убежали с поля битвы.\r\n", ch);
			if (on_horse(ch)) {
				act("Верн$W $N вынес$Q вас из боя.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
			}

			if (was_fighting && !IS_NPC(ch)) {
				reduce_exp_after_flee(ch, was_fighting, was_in);
			}
		} else {
			act("$n запаниковал$g и попытал$u убежать, но не смог$q!", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			send_to_char("ПАНИКА ОВЛАДЕЛА ВАМИ. Вы не смогли сбежать!\r\n", ch);
		}
	} else {
		act("$n запаниковал$g и попытал$u убежать, но не смог$q!", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		send_to_char("ПАНИКА ОВЛАДЕЛА ВАМИ. Вы не смогли сбежать!\r\n", ch);
	}
}

void go_dir_flee(CHAR_DATA * ch, int direction) {
	if (GET_MOB_HOLD(ch) || GET_WAIT(ch) > 0) {
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_NOFLEE) ||AFF_FLAGGED(ch, EAffectFlag::AFF_LACKY)|| PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Невидимые оковы мешают вам сбежать.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("Вы не сможете сбежать из этого положения.\r\n", ch);
		return;
	}

	if (legal_dir(ch, direction, TRUE, FALSE)
		&& !ROOM_FLAGGED(EXIT(ch, direction)->to_room(), ROOM_DEATH)) {
		if (do_simple_move(ch, direction, TRUE, 0, true)) {
			const auto was_in = ch->in_room;
			const auto was_fighting = ch->get_fighting();

			act("$n запаниковал$g и попытал$u убежать.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			send_to_char("Вы быстро убежали с поля битвы.\r\n", ch);
			if (was_fighting && !IS_NPC(ch)) {
				reduce_exp_after_flee(ch, was_fighting, was_in);
			}

			if (!WAITLESS(ch)) {
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
			}
			return;
		}
	}
	go_flee(ch);
}


const char *FleeDirs[] = { "север",
						   "восток",
						   "юг",
						   "запад",
						   "вверх",
						   "вниз",
						   "\n" };

void do_flee(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int direction = -1;
	if (!ch->get_fighting()) {
		send_to_char("Но вы ведь ни с кем не сражаетесь!\r\n", ch);
		return;
	}
	if (can_use_feat(ch, CALMNESS_FEAT) || GET_GOD_FLAG(ch, GF_GODSLIKE)) {
		one_argument(argument, arg);
		if ((direction = search_block(arg, dirs, FALSE)) >= 0 ||
				(direction = search_block(arg, FleeDirs, FALSE)) >= 0) {
			go_dir_flee(ch, direction);
			return;
		}
	}
	go_flee(ch);
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

// ************************* BASH PROCEDURES
void go_bash(CHAR_DATA * ch, CHAR_DATA * vict) {
	if (dontCanAct(ch) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	if (onhorse(ch))
		return;

	if (ch == vict) {
		return;
	}

	if (!(IS_NPC(ch)
			|| GET_EQ(ch, WEAR_SHIELD)
			|| IS_IMMORTAL(ch)
			|| GET_MOB_HOLD(vict)
			|| GET_GOD_FLAG(vict, GF_GODSCURSE))) {
		send_to_char("Вы не можете сделать этого без щита.\r\n", ch);
		return;
	};

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	vict = try_protect(vict, ch);

	int percent = number(1, skill_info[SKILL_BASH].max_percent);
	int prob = train_skill(ch, SKILL_BASH, skill_info[SKILL_BASH].max_percent, vict);

	if (GET_MOB_HOLD(vict) || GET_GOD_FLAG(vict, GF_GODSCURSE)) {
		prob = percent;
	}
	if (MOB_FLAGGED(vict, MOB_NOBASH) || GET_GOD_FLAG(ch, GF_GODSCURSE)) {
		prob = 0;
	}

	if (percent > prob) {
		Damage dmg(SkillDmg(SKILL_BASH), ZERO_DMG, PHYS_DMG);
		dmg.process(ch, vict);
		GET_POS(ch) = POS_SITTING;
		prob = 3;
	} else {
		/*
		 * If we bash a player and they wimp out, they will move to the previous
		 * room before we set them sitting.  If we try to set the victim sitting
		 * first to make sure they don't flee, then we can't bash them!  So now
		 * we only set them sitting if they didn't flee. -gg 9/21/98
		 */

		//не дадим башить мобов в лаге которые спят, оглушены и прочее
		if (GET_POS(vict) <= POS_STUNNED && GET_WAIT(vict) > 0) {
			send_to_char("Ваша жертва и так слишком слаба, надо быть милосерднее.\r\n", ch);
			//set_wait(ch, 1, FALSE);
			ch->setSkillCooldown(SKILL_GLOBAL_COOLDOWN, PULSE_VIOLENCE);
			return;
		}

		int dam = str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GET_REAL_DR(ch) +
				  MAX(0, ch->get_skill(SKILL_BASH) / 10 - 5) + GET_LEVEL(ch) / 5;

//делаем блокирование баша
		if ((GET_AF_BATTLE(vict, EAF_BLOCK)
			|| (can_use_feat(vict, DEFENDER_FEAT)
			&& GET_EQ(vict, WEAR_SHIELD)
			&& PRF_FLAGGED(vict, PRF_AWAKE)
			&& vict->get_skill(SKILL_AWAKE)
			&& vict->get_skill(SKILL_BLOCK)
			&& GET_POS(vict) > POS_SITTING))
				&& !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPFIGHT)
				&& !AFF_FLAGGED(vict, EAffectFlag::AFF_MAGICSTOPFIGHT)
				&& !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPLEFT)
				&& GET_WAIT(vict) <= 0
				&& GET_MOB_HOLD(vict) == 0) {
			if (!(GET_EQ(vict, WEAR_SHIELD) ||
					IS_NPC(vict) || IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE)))
				send_to_char("У вас нечем отразить атаку противника.\r\n", vict);
			else
			{
				int range, prob2;
				range = number(1, skill_info[SKILL_BLOCK].max_percent);
				prob2 = train_skill(vict, SKILL_BLOCK, skill_info[SKILL_BLOCK].max_percent, ch);
				if (prob2 < range) {
					act("Вы не смогли блокировать попытку $N1 сбить вас.",
						FALSE, vict, 0, ch, TO_CHAR);
					act("$N не смог$Q блокировать вашу попытку сбить $S.",
						FALSE, ch, 0, vict, TO_CHAR);
					act("$n не смог$q блокировать попытку $N1 сбить $s.",
						TRUE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
				} else {
					act("Вы блокировали попытку $N1 сбить вас с ног.",
						FALSE, vict, 0, ch, TO_CHAR);
					act("Вы хотели сбить $N1, но он$G блокировал$G Вашу попытку.",
						FALSE, ch, 0, vict, TO_CHAR);
					act("$n блокировал$g попытку $N1 сбить $s.",
						TRUE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
					alt_equip(vict, WEAR_SHIELD, 30, 10);
					if (!ch->get_fighting()) {
						set_fighting(ch, vict);
						set_wait(ch, 1, TRUE);
						//setSkillCooldownInFight(ch, SKILL_BASH, 1);
					}
					return;
				}
			}
		}
//делаем блокирование баша

		prob = 0;
		Damage dmg(SkillDmg(SKILL_BASH), dam, PHYS_DMG);
		dmg.flags.set(NO_FLEE_DMG);
		dam = dmg.process(ch, vict);

		if (dam > 0 || (dam == 0 && AFF_FLAGGED(vict, EAffectFlag::AFF_SHIELD))) {
			prob = 3;
			if (ch->isInSameRoom(vict)) {
				GET_POS(vict) = POS_SITTING;
				drop_from_horse(vict);
			}
			set_wait(vict, prob, FALSE);
			prob = 2;
		}
	}
	set_wait(ch, prob, TRUE);
}

void do_bash(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if ((IS_NPC(ch) && (!AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER))) || !ch->get_skill(SKILL_BASH)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_BASH)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (onhorse(ch)) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого же вы так сильно желаете сбить?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Ваш сокрушающий удар поверг вас наземь... Вы почувствовали себя глупо.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !ch->get_fighting()) {
		go_bash(ch, vict);
	} else if (isHaveNoExtraAttack(ch)) {
				if (!IS_NPC(ch))
				act("Хорошо. Вы попытаетесь сбить $N3.", FALSE, ch, 0, vict, TO_CHAR);
				ch->set_extra_attack(EXTRA_ATTACK_BASH, vict);
			}
}

void do_stun(CHAR_DATA* ch, char* argument, int, int)
{
	if (ch->get_skill(SKILL_STUN) < 1) {
			send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_STUN)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!on_horse(ch)) {
			send_to_char("Вы привстали на стременах и поняли: 'лошадь украли!!!'\r\n", ch);
		return;
	}
	if ((GET_SKILL(ch, SKILL_HORSE) < 151) && (!IS_NPC(ch))) {
		send_to_char("Вы слишком неуверенно управляете лошадью, чтоб на ней пытаться ошеломить противника.\r\n", ch);
		return;
	}
	if (timed_by_skill(ch, SKILL_STUN) && (!IS_NPC(ch))) {
			send_to_char("Ваш грозный вид не испугает даже мышь, попробуйте ошеломить попозже.\r\n", ch);
		return;
	}
	if (!IS_NPC(ch) && !(GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_BOTHS))) {
		send_to_char("Вы должны держать оружие в основной руке.\r\n", ch);
		return;
	}
	if (GET_EQ(ch, WEAR_BOTHS) && GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS)) == SKILL_BOWS) 	{
		send_to_char("Луком ударить по голове? Оригинально....\r\n", ch);
		return;
	}
	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кто это так сильно путается у вас под руками?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы БОЛЬНО стукнули себя по голове! 'А еще я туда ем', - подумали вы...\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	go_stun(ch, vict);
}

void go_stun(CHAR_DATA * ch, CHAR_DATA * vict) {
	timed_type timed;
	if (GET_SKILL(ch, SKILL_STUN) < 150) {
		improve_skill(ch, SKILL_STUN, TRUE, vict);
		timed.skill = SKILL_STUN;
		timed.time = 7;
		timed_to_char(ch, &timed);
		act("У вас не получилось ошеломить $N3, надо больше тренироваться!", FALSE, ch, 0, vict, TO_CHAR);
		act("$N3 попытал$U ошеломить вас, но не получилось.", FALSE, vict, 0, ch, TO_CHAR);
		act("$n попытал$u ошеломить $N3, но плохому танцору и тапки мешают.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
		set_hit(ch, vict);
	    return;
	}

	timed.skill = SKILL_STUN;
	timed.time = std::clamp(7 - (GET_SKILL(ch, SKILL_STUN) - 150)/10, 2, 7);
	timed_to_char(ch, &timed);
	//weap_weight = GET_EQ(ch, WEAR_BOTHS)?  GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS)) : GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD));
	//float num = MIN(95, (pow(GET_SKILL(ch, SKILL_STUN), 2) + pow(weap_weight, 2) + pow(GET_REAL_STR(ch), 2)) /
		//(pow(GET_REAL_DEX(vict), 2) + (GET_REAL_CON(vict) - GET_SAVE(vict, SAVING_STABILITY)) * 30.0));

	int percent = number(1, skill_info[SKILL_STUN].max_percent);
	int prob = calculate_skill(ch, SKILL_STUN, vict);

	if (percent > prob)
	{
		improve_skill(ch, SKILL_STUN, FALSE, vict);
		act("У вас не получилось ошеломить $N3, надо больше тренироваться!", FALSE, ch, 0, vict, TO_CHAR);
		act("$N3 попытал$U ошеломить вас, но не получилось.", FALSE, vict, 0, ch, TO_CHAR);
		act("$n попытал$u ошеломить $N3, но плохому танцору и тапки мешают.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
//			Damage dmg(SkillDmg(SKILL_STUN), 1, PHYS_DMG);
//			dmg.process(ch, vict);
		set_hit(ch, vict);
	} else {
		improve_skill(ch, SKILL_STUN, TRUE, vict);
		act("Мощным ударом вы ошеломили $N3!", FALSE, ch, 0, vict, TO_CHAR);
		act("Ошеломляющий удар $N1 сбил вас с ног и лишил сознания.", FALSE, vict, 0, ch, TO_CHAR);
		act("$n мощным ударом ошеломил$g $N3!", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
		GET_POS(vict) = POS_INCAP;
		WAIT_STATE(vict, (2 + GET_REMORT(ch) / 5) * PULSE_VIOLENCE);
		//WAIT_STATE(ch, (3 * PULSE_VIOLENCE));
		ch->setSkillCooldown(SKILL_STUN, 3*PULSE_VIOLENCE);
		set_hit(ch, vict);
	}
}

// ******************* RESCUE PROCEDURES
void fighting_rescue(CHAR_DATA * ch,CHAR_DATA * vict, CHAR_DATA * tmp_ch)
{
	if (vict->get_fighting() == tmp_ch)
		stop_fighting(vict, FALSE);
	if (ch->get_fighting())
		ch->set_fighting(tmp_ch);
	else
		set_fighting(ch, tmp_ch);
	if (tmp_ch->get_fighting())
		tmp_ch->set_fighting(ch);
	else
		set_fighting(tmp_ch, ch);
}

void go_rescue(CHAR_DATA * ch, CHAR_DATA * vict, CHAR_DATA * tmp_ch) {
	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (on_horse(ch)) {
	    send_to_char(ch, "Ну раскорячили вы ноги по сторонам, но спасти %s как?\r\n", GET_PAD(vict,1));
	    return;
	}

	int percent = number(1, skill_info[SKILL_RESCUE].max_percent);
	int prob = calculate_skill(ch, SKILL_RESCUE, tmp_ch);
	improve_skill(ch, SKILL_RESCUE, prob >= percent, tmp_ch);

	if (GET_GOD_FLAG(ch, GF_GODSLIKE))
		prob = percent;
	if (GET_GOD_FLAG(ch, GF_GODSCURSE))
		prob = 0;

	if (percent != skill_info[SKILL_RESCUE].max_percent && percent > prob) {
		act("Вы безуспешно пытались спасти $N3.", FALSE, ch, 0, vict, TO_CHAR);
		//set_wait(ch, 1, FALSE);
		ch->setSkillCooldown(SKILL_GLOBAL_COOLDOWN, PULSE_VIOLENCE);
		return;
	}

	if (!pk_agro_action(ch, tmp_ch))
		return;

	act("Хвала Богам, вы героически спасли $N3!", FALSE, ch, 0, vict, TO_CHAR);
	act("Вы были спасены $N4. Вы чувствуете себя Иудой!", FALSE, vict, 0, ch, TO_CHAR);
	act("$n героически спас$q $N3!", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);

	int hostilesCounter = 0;
	if (can_use_feat(ch, LIVE_SHIELD_FEAT)) {
		for (const auto i : world[ch->in_room]->people) {
			if (i->get_fighting() == vict) {
				fighting_rescue(ch, vict, i);
				++hostilesCounter;
			}
		}
		hostilesCounter /= 2;
	} else {
		fighting_rescue(ch, vict, tmp_ch);
	}
	//set_wait(ch, 1, FALSE);
	setSkillCooldown(ch, SKILL_GLOBAL_COOLDOWN, 1);
	setSkillCooldown(ch, SKILL_RESCUE, 1 + hostilesCounter);
	set_wait(vict, 2, FALSE);
}

void do_rescue(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->get_skill(SKILL_RESCUE)) {
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_RESCUE)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кто это так сильно путается под вашими ногами?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Ваше спасение вы можете доверить только Богам.\r\n", ch);
		return;
	}
	if (ch->get_fighting() == vict) {
		send_to_char("Вы пытаетесь спасти атакующего вас?\r\n" "Это не о вас ли писали Марк и Лука?\r\n", ch);
		return;
	}

	CHAR_DATA* enemy = nullptr;
	for (const auto i : world[ch->in_room]->people) {
		if (i->get_fighting() == vict) {
			enemy = i;
			break;
		}
	}

	if (!enemy) {
		act("Но никто не сражается с $N4!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if (IS_NPC(vict)
		&& enemy
		&& (!IS_NPC(enemy)
			|| (AFF_FLAGGED(enemy, EAffectFlag::AFF_CHARM)
				&& enemy->has_master()
				&& !IS_NPC(enemy->get_master())))
		&& (!IS_NPC(ch)
			|| (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
				&& ch->has_master()
				&& !IS_NPC(ch->get_master())))) {
		send_to_char("Вы пытаетесь спасти чужого противника.\r\n", ch);
		return;
	}

	// Двойники и прочие очарки не в группе с тем, кого собираются спасать:
	// Если тот, кто собирается спасать - "чармис" и у него существует хозяин
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && ch->has_master()) {
		// Если спасаем "чармиса", то проверять надо на нахождение в одной
		// группе хозянина спасющего и спасаемого.
		if (AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
			&& vict->has_master()
			&& !same_group(vict->get_master(), ch->get_master())) {
			act("Спасали бы вы лучше другов своих.", FALSE, ch, 0, vict, TO_CHAR);
			act("Вы не можете спасти весь мир.", FALSE, ch->get_master(), 0, vict, TO_CHAR);
			return;
		}
	}

	if (!may_kill_here(ch, enemy, argument)) {
		return;
	}

	go_rescue(ch, vict, enemy);
}

// ******************  KICK PROCEDURES
void go_kick(CHAR_DATA * ch, CHAR_DATA * vict) {
	const char *to_char = NULL, *to_vict = NULL, *to_room = NULL;

	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_KICK)) {
		send_to_char("Вы уже все ноги себе отбили, отдохните слегка.\r\n", ch);
		return;
	};

	vict = try_protect(vict, ch);

	int percent = ((10 - (compute_armor_class(vict) / 10)) * 2) + number(1, skill_info[SKILL_KICK].max_percent);
	int prob = train_skill(ch, SKILL_KICK, skill_info[SKILL_KICK].max_percent, vict);
	if (GET_GOD_FLAG(vict, GF_GODSCURSE) || GET_MOB_HOLD(vict)) {
		prob = percent;
	}
	if (GET_GOD_FLAG(ch, GF_GODSCURSE) || (!on_horse(ch) && on_horse(vict))) {
		prob = 0;
	}

	if (check_spell_on_player(ch, SPELL_WEB)) {
		prob /= 3;
	}

	if (percent > prob) {
		Damage dmg(SkillDmg(SKILL_KICK), ZERO_DMG, PHYS_DMG);
		dmg.process(ch, vict);
		prob = 2;
	} else {
		int dam = str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GET_REAL_DR(ch) + GET_LEVEL(ch) / 6;
		if (!IS_NPC(ch) || (IS_NPC(ch) && GET_EQ(ch, WEAR_FEET))) {
			int modi = MAX(0, (ch->get_skill(SKILL_KICK) + 4) / 5);
			dam += number(0, modi * 2);
			modi = 5 * (10 + (GET_EQ(ch, WEAR_FEET) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_FEET)) : 0));
			dam = modi * dam / 100;
		}
		if (on_horse(ch) && (ch->get_skill(SKILL_HORSE) >= 150) && (ch->get_skill(SKILL_KICK) >= 150)) {
			AFFECT_DATA<EApplyLocation> af;
			af.location = APPLY_NONE;
			af.type = SPELL_BATTLE;
			af.modifier = 0;
			af.battleflag = 0;
			float modi = ((ch->get_skill(SKILL_KICK) + GET_REAL_STR(ch) * 5)
							+ (GET_EQ(ch, WEAR_FEET) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_FEET)) : 0) * 3) / float(GET_SIZE(vict));
			if (number(1, 1000) < modi * 10) {
				switch (number(0, (ch->get_skill(SKILL_KICK) - 150) / 10)) {
				case 0:
				case 1:
					if (!AFF_FLAGGED(vict, EAffectFlag::AFF_STOPRIGHT)) {
						to_char = "Каблук вашего сапога надолго запомнится $N2, если конечно он выживет.";
						to_vict = "Мощный удар ноги $n1 изуродовал вам правую руку.";
						to_room = "След сапога $n1 надолго запомнится $N2, если конечно он выживет.";
						af.type = SPELL_BATTLE;
						af.bitvector = to_underlying(EAffectFlag::AFF_STOPRIGHT);
						af.duration = pc_duration(vict, 3 + GET_REMORT(ch) / 4, 0, 0, 0, 0);
						af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
					} else if (!AFF_FLAGGED(vict, EAffectFlag::AFF_STOPLEFT)) {
							to_char = "Каблук вашего сапога надолго запомнится $N2, если конечно он выживет.";
							to_vict = "Мощный удар ноги $n1 изуродовал вам левую руку.";
							to_room = "След сапога $n1 надолго запомнится $N2, если конечно он выживет.";
							af.bitvector = to_underlying(EAffectFlag::AFF_STOPLEFT);
							af.duration = pc_duration(vict, 3 + GET_REMORT(ch) / 4, 0, 0, 0, 0);
							af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
							} else {
								to_char = "Каблук вашего сапога надолго запомнится $N2, $M теперь даже бить вас нечем.";
								to_vict = "Мощный удар ноги $n1 вывел вас из строя.";
								to_room = "Каблук сапога $n1 надолго запомнится $N2, $M теперь даже биться нечем.";
								af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
								af.duration = pc_duration(vict, 3 + GET_REMORT(ch) / 4, 0, 0, 0, 0);
								af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
							}
					dam *= 2;
					break;
				case 2:
				case 3:
					to_char = "Сильно пнув в челюсть, вы заставили $N3 замолчать.";
					to_vict = "Мощный удар ноги $n1 попал вам точно в челюсть, заставив вас замолчать.";
					to_room = "Сильно пнув ногой в челюсть $N3, $n заставил$q $S замолчать.";
					af.type = SPELL_BATTLE;
					af.bitvector = to_underlying(EAffectFlag::AFF_SILENCE);
					af.duration = pc_duration(vict, 3 + GET_REMORT(ch) / 5, 0, 0, 0, 0);
					af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
					dam *= 2;
					break;
				case 4:
				case 5:
					WAIT_STATE(vict, number(2, 5) * PULSE_VIOLENCE);
					if (GET_POS(vict) > POS_SITTING) {
						GET_POS(vict) = POS_SITTING;
					}
					to_char = "Ваш мощный пинок выбил пару зубов $N2, усадив $S на землю!";
					to_vict = "Мощный удар ноги $n1 попал точно в голову, свалив вас с ног.";
					to_room = "Мощный пинок $n1 выбил пару зубов $N2, усадив $S на землю!";
					dam *= 2;
					break;
				default:
					break;
				}
			} else if (number(1, 1000) < (ch->get_skill(SKILL_HORSE) / 2)) {
				dam *= 2;
				if (!IS_NPC(ch))
					send_to_char("Вы привстали на стременах.\r\n", ch);
			}

			if (to_char) {
				if (!IS_NPC(ch)) {
					sprintf(buf, "&G&q%s&Q&n", to_char);
					act(buf, FALSE, ch, 0, vict, TO_CHAR);
					sprintf(buf, "%s", to_room);
					act(buf, TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
				}
			}
			if (to_vict) {
				if (!IS_NPC(vict)) {
					sprintf(buf, "&R&q%s&Q&n", to_vict);
					act(buf, FALSE, ch, 0, vict, TO_VICT);
				}
			}
			affect_join(vict, af, TRUE, FALSE, TRUE, FALSE);
		}

		if (GET_AF_BATTLE(vict, EAF_AWAKE)) {
			dam >>= (2 - on_horse(ch));
		}
		Damage dmg(SkillDmg(SKILL_KICK), dam, PHYS_DMG);
		dmg.process(ch, vict);
		prob = 2;
	}
	//set_wait(ch, prob, TRUE);
	setSkillCooldownInFight(ch, SKILL_KICK, prob);
	setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
}

void do_kick(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(SKILL_KICK)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_KICK)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кто это так сильно путается под вашими ногами?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы БОЛЬНО пнули себя! Поздравляю, вы сошли с ума...\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !ch->get_fighting()) {
		go_kick(ch, vict);
	} else if (isHaveNoExtraAttack(ch)) {
			act("Хорошо. Вы попытаетесь пнуть $N3.", FALSE, ch, 0, vict, TO_CHAR);
			ch->set_extra_attack(EXTRA_ATTACK_KICK, vict);
		}
}

// ******************* BLOCK PROCEDURES
void go_block(CHAR_DATA * ch) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT)) {
		send_to_char("Ваша рука парализована.\r\n", ch);
		return;
	}
	SET_AF_BATTLE(ch, EAF_BLOCK);
	send_to_char("Хорошо, вы попробуете отразить щитом следующую атаку.\r\n", ch);
}

void do_block(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(SKILL_BLOCK)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_BLOCK)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (!ch->get_fighting()) {
		send_to_char("Но вы ни с кем не сражаетесь!\r\n", ch);
		return;
	};
	if (!(IS_NPC(ch)
			|| GET_EQ(ch, WEAR_SHIELD)
			|| IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, GF_GODSLIKE))) {
		send_to_char("Вы не можете сделать это без щита.\r\n", ch);
		return;
	}
	if (GET_AF_BATTLE(ch, EAF_BLOCK)) {
		send_to_char("Вы уже прикрываетесь щитом!\r\n", ch);
		return;
	}
	go_block(ch);
}

// **************** MULTYPARRY PROCEDURES
void go_multyparry(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT) || dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	SET_AF_BATTLE(ch, EAF_MULTYPARRY);
	send_to_char("Вы попробуете использовать веерную защиту.\r\n", ch);
}

void do_multyparry(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(SKILL_MULTYPARRY)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_MULTYPARRY)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (!ch->get_fighting()) {
		send_to_char("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	OBJ_DATA *primary = GET_EQ(ch, WEAR_WIELD), *offhand = GET_EQ(ch, WEAR_HOLD);
	if (!(IS_NPC(ch)
		|| (primary
			&& GET_OBJ_TYPE(primary) == OBJ_DATA::ITEM_WEAPON
			&& offhand
			&& GET_OBJ_TYPE(offhand) == OBJ_DATA::ITEM_WEAPON)
		|| IS_IMMORTAL(ch)
		|| GET_GOD_FLAG(ch, GF_GODSLIKE))) {
		send_to_char("Вы не можете отражать атаки безоружным.\r\n", ch);
		return;
	}
	if (GET_AF_BATTLE(ch, EAF_STUPOR)) {
		send_to_char("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
		return;
	}
	go_multyparry(ch);
}

// **************** PARRY PROCEDURES
void go_parry(CHAR_DATA * ch) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT) || dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	SET_AF_BATTLE(ch, EAF_PARRY);
	send_to_char("Вы попробуете отклонить следующую атаку.\r\n", ch);
}

void do_parry(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(SKILL_PARRY)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_PARRY)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!ch->get_fighting()) {
		send_to_char("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch) && !GET_GOD_FLAG(ch, GF_GODSLIKE)) {
		if (GET_EQ(ch, WEAR_BOTHS)) {
			send_to_char("Вы не можете отклонить атаку двуручным оружием.\r\n", ch);
			return;
		}

		bool prim = 0, offh = 0;
		if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == OBJ_DATA::ITEM_WEAPON) {
			prim = 1;
		}
		if (GET_EQ(ch, WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == OBJ_DATA::ITEM_WEAPON) {
			offh = 1;
		}

		if (!prim && !offh) {
			send_to_char("Вы не можете отклонить атаку безоружным.\r\n", ch);
			return;
		} else if (!prim || !offh) {
					send_to_char("Вы можете отклонить атаку только с двумя оружиями в руках.\r\n", ch);
					return;
				}
	}

	if (GET_AF_BATTLE(ch, EAF_STUPOR)) {
		send_to_char("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
		return;
	}
	go_parry(ch);
}

// ************** PROTECT PROCEDURES
void go_protect(CHAR_DATA * ch, CHAR_DATA * vict) {
	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	ch->set_protecting(vict);
	act("Вы попытаетесь прикрыть $N3 от нападения.", FALSE, ch, 0, vict, TO_CHAR);
	SET_AF_BATTLE(ch, EAF_PROTECT);
}

void do_protect(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, arg);
	if (!*arg) {
		if (ch->get_protecting()) {
			CLR_AF_BATTLE(ch, EAF_PROTECT);
			ch->set_protecting(0);
			send_to_char("Вы перестали прикрывать своего товарища.\r\n", ch);
		} else {
			send_to_char("Вы никого не прикрываете.\r\n", ch);
		}
		return;
	}

	if (IS_NPC(ch) || !ch->get_skill(SKILL_PROTECT)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_PROTECT)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CHAR_DATA *vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	if (!vict) {
		send_to_char("И кто так сильно мил вашему сердцу?\r\n", ch);
		return;
	};

	if (vict == ch) {
		send_to_char("Попробуйте парировать удары или защищаться щитом.\r\n", ch);
		return;
	}

	if (ch->get_fighting() == vict) {
		send_to_char("Вы явно пацифист, или мазохист.\r\n", ch);
		return;
	}

	CHAR_DATA *tch = nullptr;
	for (const auto i : world[ch->in_room]->people) {
		if (i->get_fighting() == vict) 	{
			tch = i;
			break;
		}
	}

	if (IS_NPC(vict) && tch
		&& (!IS_NPC(tch)
			|| (AFF_FLAGGED(tch, EAffectFlag::AFF_CHARM)
				&& tch->has_master()
				&& !IS_NPC(tch->get_master())))
		&& (!IS_NPC(ch)
			|| (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
				&& ch->has_master()
				&& !IS_NPC(ch->get_master())))) {
		send_to_char("Вы пытаетесь прикрыть чужого противника.\r\n", ch);
		return;
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (tch->get_fighting() == vict && !may_kill_here(ch, tch, argument)) {
			return;
		}
	}

	go_protect(ch, vict);
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

// ************* DISARM PROCEDURES
void go_disarm(CHAR_DATA * ch, CHAR_DATA * vict) {
	OBJ_DATA *wielded = GET_EQ(vict, WEAR_WIELD) ? GET_EQ(vict, WEAR_WIELD) :
						GET_EQ(vict, WEAR_BOTHS), *helded = GET_EQ(vict, WEAR_HOLD);

	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (!((wielded && GET_OBJ_TYPE(wielded) != OBJ_DATA::ITEM_LIGHT)
		|| (helded && GET_OBJ_TYPE(helded) != OBJ_DATA::ITEM_LIGHT))) {
		return;
	}
	int pos = 0;
	if (number(1, 100) > 30) {
		pos = wielded ? (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD) : WEAR_HOLD;
	} else {
		pos = helded ? WEAR_HOLD : (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD);
	}

	if (!pos || !GET_EQ(vict, pos))
		return;
	if (!pk_agro_action(ch, vict))
		return;
	int percent = number(1, skill_info[SKILL_DISARM].max_percent);
	int prob = train_skill(ch, SKILL_DISARM, skill_info[SKILL_DISARM].max_percent, vict);
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(vict, GF_GODSCURSE) || GET_GOD_FLAG(ch, GF_GODSLIKE))
		prob = percent;
	if (IS_IMMORTAL(vict) || GET_GOD_FLAG(ch, GF_GODSCURSE) || GET_GOD_FLAG(vict, GF_GODSLIKE) || can_use_feat(vict, STRONGCLUTCH_FEAT))
		prob = 0;


	if (percent > prob || GET_EQ(vict, pos)->get_extra_flag(EExtraFlag::ITEM_NODISARM)) {
		send_to_char(ch, "%sВы не сумели обезоружить %s...%s\r\n", CCWHT(ch, C_NRM), GET_PAD(vict, 3), CCNRM(ch, C_NRM));
		prob = 3;
	} else {
		wielded = GET_EQ(vict, pos);
		send_to_char(ch, "%sВы ловко выбили %s из рук %s!%s\r\n",
						CCIBLU(ch, C_NRM), wielded->get_PName(3).c_str(), GET_PAD(vict, 1), CCNRM(ch, C_NRM));
		send_to_char(vict, "Ловкий удар %s выбил %s%s из ваших рук.\r\n",
						GET_PAD(ch, 1), wielded->get_PName(3).c_str(), char_get_custom_label(wielded, vict).c_str());
		act("$n ловко выбил$g $o3 из рук $N1.", TRUE, ch, wielded, vict, TO_NOTVICT | TO_ARENA_LISTEN);
		unequip_char(vict, pos);
		//if (GET_WAIT(vict) <= 0) {
			//set_wait(vict, IS_NPC(vict) ? 1 : 2, FALSE);
			setSkillCooldown(ch, SKILL_GLOBAL_COOLDOWN, IS_NPC(vict) ? 1 : 2);
		//}
		prob = 2;

		if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA) || (!IS_MOB(vict)) || vict->has_master()) {
            obj_to_char(wielded, vict);
        } else {
            obj_to_room(wielded, IN_ROOM(vict));
            obj_decay(wielded);
		};
	}

	appear(ch);
	if (IS_NPC(vict) && CAN_SEE(vict, ch) && have_mind(vict) && GET_WAIT(ch) <= 0) {
		set_hit(vict, ch);
	}
	//set_wait(ch, prob, FALSE);
	setSkillCooldown(ch, SKILL_DISARM, prob);
	setSkillCooldown(ch, SKILL_GLOBAL_COOLDOWN, 1);
}

void do_disarm(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(SKILL_DISARM)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_DISARM)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого обезоруживаем?\r\n", ch);
		return;
	}

	if (ch == vict) {
		send_to_char("Попробуйте набрать \"снять <название.оружия>\".\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (!((GET_EQ(vict, WEAR_WIELD)
			&& GET_OBJ_TYPE(GET_EQ(vict, WEAR_WIELD)) != OBJ_DATA::ITEM_LIGHT)
			|| (GET_EQ(vict, WEAR_HOLD)
				&& GET_OBJ_TYPE(GET_EQ(vict, WEAR_HOLD)) != OBJ_DATA::ITEM_LIGHT)
			|| (GET_EQ(vict, WEAR_BOTHS)
				&& GET_OBJ_TYPE(GET_EQ(vict, WEAR_BOTHS)) != OBJ_DATA::ITEM_LIGHT))) {
		send_to_char("Вы не можете обезоружить безоружное создание.\r\n", ch);
		return;
	}

	if (IS_IMPL(ch) || !ch->get_fighting()) {
		go_disarm(ch, vict);
	} else if (isHaveNoExtraAttack(ch)) {
		act("Хорошо. Вы попытаетесь разоружить $N3.", FALSE, ch, 0, vict, TO_CHAR);
		ch->set_extra_attack(EXTRA_ATTACK_DISARM, vict);
	}
}

// ************************* CHOPOFF PROCEDURES
void go_chopoff(CHAR_DATA * ch, CHAR_DATA * vict) {
	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	if (onhorse(ch))
		return;

	if ((GET_POS(vict) < POS_FIGHTING)) {
		if (number(1, 100) < ch->get_skill(SKILL_CHOPOFF)) {
			send_to_char("Вы приготовились провести подсечку, но вовремя остановились.\r\n", ch);
			//set_wait(ch, 1, FALSE);
			//setSkillCooldown(ch, SKILL_CHOPOFF, 1);
			ch->setSkillCooldown(SKILL_CHOPOFF, PULSE_VIOLENCE/6);
			return;
		}
	}

	if (!pk_agro_action(ch, vict))
		return;

	int percent = number(1, skill_info[SKILL_CHOPOFF].max_percent);
	int prob = train_skill(ch, SKILL_CHOPOFF, skill_info[SKILL_CHOPOFF].max_percent, vict);

	if (check_spell_on_player(ch, SPELL_WEB)) {
		prob /= 3;
	}
	if (GET_GOD_FLAG(ch, GF_GODSLIKE)
		|| GET_MOB_HOLD(vict)
		|| GET_GOD_FLAG(vict, GF_GODSCURSE)) {
		prob = percent;
	}

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
			GET_GOD_FLAG(vict, GF_GODSLIKE) ||
			on_horse(vict) || GET_POS(vict) < POS_FIGHTING || MOB_FLAGGED(vict, MOB_NOTRIP) || IS_IMMORTAL(vict))
		prob = 0;

	bool skillFail = percent > prob;

	if (skillFail) {
		sprintf(buf, "%sВы попытались подсечь $N3, но упали сами...%s", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		act(buf,FALSE,ch,0,vict,TO_CHAR);
		act("$n попытал$u подсечь вас, но упал$g сам$g.", FALSE, ch, 0, vict, TO_VICT);
		act("$n попытал$u подсечь $N3, но упал$g сам$g.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SITTING;
		prob = 3;
		if (can_use_feat(ch, EVASION_FEAT)) {
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_EXPEDIENT;
			af.location = EApplyLocation::APPLY_PR;
			af.modifier = 50;
			af.duration = pc_duration(ch, 3, 0, 0, 0, 0);
			af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
			af.location = EApplyLocation::APPLY_AR;
			affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
			af.location = EApplyLocation::APPLY_MR;
			affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
			send_to_char(ch, "%sВы покатились по земле, пытаясь избежать атак %s.%s\r\n", CCIGRN(ch, C_NRM), GET_PAD(vict, 1), CCNRM(ch, C_NRM));
			act("$n покатил$u по земле, пытаясь избежать ваших атак.", FALSE, ch, 0, vict, TO_VICT);
			act("$n покатил$u по земле, пытаясь избежать атак $N1.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
		}
	} else {
		send_to_char(ch, "%sВы провели подсечку, ловко усадив %s на землю.%s\r\n", CCIBLU(ch, C_NRM), GET_PAD(vict, 3), CCNRM(ch, C_NRM));
		act("$n ловко подсек$q вас, усадив на попу.", FALSE, ch, 0, vict, TO_VICT);
		act("$n ловко подсек$q $N3, уронив $S на землю.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
		set_wait(vict, 3, FALSE);

		if (ch->isInSameRoom(vict)) {
			GET_POS(vict) = POS_SITTING;
		}

		if (IS_HORSE(vict) && on_horse(vict->get_master())) {
			horse_drop(vict);
		}
		prob = 1;
	}

	appear(ch);
	if (IS_NPC(vict)
		&& CAN_SEE(vict, ch)
		&& have_mind(vict)
		&& vict->get_wait() <= 0)
	{
		set_hit(vict, ch);
	}

	if (skillFail) {
		set_wait(ch, prob, FALSE);
	} else  {
		setSkillCooldownInFight(ch, SKILL_CHOPOFF, prob);
		setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
	}
}

void do_chopoff(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_skill(SKILL_CHOPOFF) < 1) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_CHOPOFF)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (onhorse(ch)) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы собираетесь подсечь?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы можете воспользоваться командой <сесть>.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !ch->get_fighting())
		go_chopoff(ch, vict);
	else if (isHaveNoExtraAttack(ch)) {
		if (!IS_NPC(ch))
			act("Хорошо. Вы попытаетесь подсечь $N3.", FALSE, ch, 0, vict, TO_CHAR);
		ch->set_extra_attack(EXTRA_ATTACK_CHOPOFF, vict);
	}
}

// ************************* STUPOR PROCEDURES
void go_stupor(CHAR_DATA * ch, CHAR_DATA * victim) {
	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	victim = try_protect(victim, ch);

	if (!ch->get_fighting()) {
		SET_AF_BATTLE(ch, EAF_STUPOR);
		hit(ch, victim, SKILL_STUPOR, RIGHT_WEAPON);
		//set_wait(ch, 2, TRUE);
		if (ch->getSkillCooldown(SKILL_STUPOR) > 0) {
			setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
		}
	} else {
		act("Вы попытаетесь оглушить $N3.", FALSE, ch, 0, victim, TO_CHAR);
		if (ch->get_fighting() != victim) {
			stop_fighting(ch, FALSE);
			set_fighting(ch, victim);
			//set_wait(ch, 2, TRUE);
			setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 2);
		}
		SET_AF_BATTLE(ch, EAF_STUPOR);
	}
}

void do_stupor(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_skill(SKILL_STUPOR) < 1) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_STUPOR)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы хотите оглушить?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы громко заорали, заглушая свой собственный голос.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	parry_override(ch);

	go_stupor(ch, vict);
}

// ************************* MIGHTHIT PROCEDURES
void go_mighthit(CHAR_DATA * ch, CHAR_DATA * victim) {
	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	victim = try_protect(victim, ch);

	if (!ch->get_fighting()) {
		SET_AF_BATTLE(ch, EAF_MIGHTHIT);
		hit(ch, victim, SKILL_MIGHTHIT, RIGHT_WEAPON);
		if (ch->getSkillCooldown(SKILL_MIGHTHIT) > 0) {
			setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
		}
		//set_wait(ch, 2, TRUE);
		return;
	}

	if ((victim->get_fighting() != ch) && (ch->get_fighting() != victim)) {
		act("$N не сражается с вами, не трогайте $S.", FALSE, ch, 0, victim, TO_CHAR);
	} else {
		act("Вы попытаетесь нанести богатырский удар по $N2.", FALSE, ch, 0, victim, TO_CHAR);
		if (ch->get_fighting() != victim) {
			stop_fighting(ch, 2);
			set_fighting(ch, victim);
			setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 2);
			//set_wait(ch, 2, TRUE);
		}
		SET_AF_BATTLE(ch, EAF_MIGHTHIT);
	}
}

void do_mighthit(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/){
	if (ch->get_skill(SKILL_MIGHTHIT) < 1) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_MIGHTHIT)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы хотите СИЛЬНО ударить?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы СИЛЬНО ударили себя. Но вы и не спали.\r\n", ch);
		return;
	}

	if (GET_AF_BATTLE(ch, EAF_TOUCH)) {
		if (!IS_NPC(ch))
			send_to_char("Невозможно. Вы сосредоточены на захвате противника.\r\n", ch);
		return;
	}
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
		&& (GET_EQ(ch, WEAR_BOTHS)
			|| GET_EQ(ch, WEAR_WIELD)
			|| GET_EQ(ch, WEAR_HOLD)
			|| GET_EQ(ch, WEAR_SHIELD)
			|| GET_EQ(ch, WEAR_LIGHT))) {
		send_to_char("Ваша экипировка мешает вам нанести удар.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	parry_override(ch);

	go_mighthit(ch, vict);
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

// ************* THROW PROCEDURES

// Временная костыльная функция до реализации встроенного механизма работы сайдскиллов
// TODO Не забыть реализовать постоянный механизм
void performShadowThrowSideAbilities(TechniqueRollType &technique) {
	OBJ_DATA *weapon = GET_EQ(technique.actor(), technique.getWeaponEquipPosition());
	if (!weapon) {
		return;
	}
	int featureID = INCORRECT_FEAT;
	std::string to_char, to_vict, to_room;
	void (*doSideAction)(TechniqueRollType &technique);
	uint32_t mobNoFlag = MOB_DELETE;

	switch (GET_OBJ_SKILL(weapon)) {
	case SKILL_SPADES:
		mobNoFlag = MOB_NOBASH;
		featureID = SHADOW_SPEAR_FEAT;
		to_char = "Попадание копья повалило $n3 наземь.";
		to_vict = "Копье $N1 попало вам в колено. Вы рухнули наземь! Кажется, ваши приключения сейчас закончатся...";
		to_room = "Копье $N1 сбило $n3 наземь!";
		doSideAction = ([](TechniqueRollType &technique) {
							drop_from_horse(technique.rival());
							GET_POS(technique.rival()) = MIN(GET_POS(technique.rival()), POS_SITTING);
							set_wait(technique.rival(), 2, TRUE);
						});
		break;
	case SKILL_SHORTS:
	case SKILL_PICK:
		mobNoFlag = MOB_NOSIELENCE;
		featureID = SHADOW_DAGGER_FEAT;
		to_char = "Меткое попадание вашего кинжала заставило $n3 умолкнуть.";
		to_vict = "Бросок $N1 угодил вам в горло. Вы прикусили язык!";
		to_room = "Меткое попадание $N1 заставило $n3 умолкнуть!";
		doSideAction = ([](TechniqueRollType &technique) {
							AFFECT_DATA<EApplyLocation> af;
							af.type = SPELL_BATTLE;
							af.bitvector = to_underlying(EAffectFlag::AFF_SILENCE);
							af.duration = pc_duration(technique.rival(), 2, GET_LEVEL(technique.actor()), 9, 6, 2);
							af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
							affect_join(technique.rival(), af, FALSE, FALSE, FALSE, FALSE);
						});
		break;
	case SKILL_CLUBS:
		mobNoFlag = MOB_NOSTUPOR;
		featureID = SHADOW_CLUB_FEAT;
		to_char = "Попадание булавы ошеломило $n3.";
		to_vict = "Брошенная $N4 булава врезалась вам в лоб! Какие красивые звёздочки вокруг...";
		to_room = "Попадание булавы $N1 ошеломило $n3!";
		doSideAction = ([](TechniqueRollType &technique) {
							AFFECT_DATA<EApplyLocation> af;
							af.type = SPELL_BATTLE;
							af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
							af.duration = pc_duration(technique.rival(), 3, 0, 0, 0, 0);
							af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
							affect_join(technique.rival(), af, FALSE, FALSE, FALSE, FALSE);
							set_wait(technique.rival(), 3, TRUE);
						});
		break;
	};

	if (!can_use_feat(technique.actor(), featureID)) {
		return;
	};
	TechniqueRollType sideAbility;
	sideAbility.initialize(technique.actor(), featureID, technique.rival());
	if (sideAbility.isSuccess() && !MOB_FLAGGED(technique.rival(), mobNoFlag)) {
		act(to_char.c_str(), FALSE, technique.rival(), 0, technique.actor(), TO_VICT);
		act(to_vict.c_str(), FALSE, technique.rival(), 0, technique.actor(), TO_CHAR);
		act(to_room.c_str(), FALSE, technique.rival(), 0, technique.actor(), TO_NOTVICT | TO_ARENA_LISTEN);
		doSideAction(technique);
	}
};

// TODO: Перенести подобную логику в модуль абилок, разделить уровни
void performWeaponThrow(TechniqueRollType &technique, Damage &techniqueDamage) {
	techniqueDamage.dam = ZERO_DMG;
	if (technique.isSuccess()) {
		techniqueDamage.dam = technique.calculateDamage();
		if (technique.isCriticalSuccess()) {
			send_to_char("&GВ яблочко!&n\r\n", technique.actor());
			techniqueDamage.flags.set(IGNORE_ARMOR);
			techniqueDamage.flags.set(CRIT_HIT);
		};
		if (timed_by_feat(technique.actor(), SHADOW_THROW_FEAT)) {
			decreaseFeatTimer(technique.actor(), SHADOW_THROW_FEAT);
		};
		if (technique.ID() == SHADOW_THROW_FEAT) {
			performShadowThrowSideAbilities(technique);
		};
	} else {
		if (technique.isCriticalFail()) {
			OBJ_DATA *weapon = unequip_char(technique.actor(), technique.getWeaponEquipPosition());
			if (weapon) {
				obj_to_char(weapon, technique.actor());
				send_to_char(technique.actor(), "&BВы выронили %s!&n\r\n", GET_OBJ_PNAME(weapon, 3).c_str());
			};
		};
	};
	techniqueDamage.process(technique.actor(), technique.rival());
};

void go_throw(CHAR_DATA * ch, CHAR_DATA * victim) {

	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	// TODO: Возможно, стоит добавить простой тест на добавление целей.
	short victimsAmount = 1 + PRF_FLAGGED(ch, PRF_DOUBLE_THROW) + 2*PRF_FLAGGED(ch, PRF_TRIPLE_THROW);

	int techniqueID = THROW_WEAPON_FEAT;
	DmgType throwDamageKind = PHYS_DMG;
	if (PRF_FLAGGED(ch, PRF_SHADOW_THROW)) {
		send_to_char("Рукоять оружия в вашей руке налилась неестественным холодом.\r\n", ch);
		act("Оружие в руках $n1 окружила призрачная дымка.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		techniqueID = SHADOW_THROW_FEAT;
		throwDamageKind = MAGE_DMG;
		struct timed_type shadowThrowTimed;
		shadowThrowTimed.skill = SHADOW_THROW_FEAT;
		shadowThrowTimed.time = 6;
		timed_feat_to_char(ch, &shadowThrowTimed);
		PRF_FLAGS(ch).unset(PRF_SHADOW_THROW);
	}
	TechniqueRollType weaponThrowRoll;
	Damage throwDamage(SkillDmg(SKILL_THROW), ZERO_DMG, throwDamageKind);
	throwDamage.magic_type = STYPE_DARK;

	ActionTargeting::FoesRosterType roster{ch, victim, [](CHAR_DATA* ch, CHAR_DATA* victim){return CAN_SEE(ch, victim);}};
	for (auto target : roster) {
		target = try_protect(target, ch);
		weaponThrowRoll.initialize(ch, techniqueID, target);
		if (weaponThrowRoll.isWrongConditions()) {
			weaponThrowRoll.sendDenyMessageToActor();
			break;
		};
		performWeaponThrow(weaponThrowRoll, throwDamage);
		--victimsAmount;
		if (ch->purged() || victimsAmount == 0) {
			break;
		};
	};

	setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
	if (techniqueID == THROW_WEAPON_FEAT) {
		setSkillCooldownInFight(ch, SKILL_THROW, 3);
	}
}

void do_throw(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd) {
	//Svent TODO: Не забыть убрать заглушку после дописывания навыков
	if (!ch->get_skill(SKILL_THROW)) {
		send_to_char("Вы принялись метать икру. Это единственное, что вы умеете метать.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(SKILL_THROW)) {
		send_to_char("Так и рука отвалится, нужно передохнуть.\r\n", ch);
		return;
	};
/*
	if (!IS_IMPL(ch) && !can_use_feat(ch, THROW_WEAPON_FEAT)) {
			send_to_char("Вы не умеете этого.\r\n", ch);
			return;
	}
*/
	CHAR_DATA* victim = findVictim(ch, argument);
	if (!victim) {
		send_to_char("В кого мечем?\r\n", ch);
		return;
	}

	if (ch == victim) {
		send_to_char("Вы начали метаться как белка в колесе.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, victim, argument)) {
		return;
	}
	if (!check_pkill(ch, victim, arg)) {
		return;
	}

	if (subcmd == SCMD_SHADOW_THROW) {
		if (timed_by_feat(ch, SHADOW_THROW_FEAT)) {
			send_to_char("Не стоит так часто беспокоить тёмные силы.\r\n", ch);
			return;
		}
		PRF_FLAGS(ch).set(PRF_SHADOW_THROW);
	};

	if (IS_IMPL(ch) || !ch->get_fighting()) {
		go_throw(ch, victim);
	} else {
		if (isHaveNoExtraAttack(ch)) {
			act("Хорошо. Вы попытаетесь метнуть оружие в $N3.", FALSE, ch, 0, victim, TO_CHAR);
			ch->set_extra_attack(EXTRA_ATTACK_THROW, victim);
		}
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

void go_iron_wind(CHAR_DATA * ch, CHAR_DATA * victim) {
	if (dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}
	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Вы уже впали в неистовство.\r\n", ch);
		return;
	}
	if (ch->get_fighting() && (ch->get_fighting() != victim)) {
		act("$N не сражается с вами, не трогайте $S.", FALSE, ch, 0, victim, TO_CHAR);
		return;
	}

	parry_override(ch);

	act("Вас обуяло безумие боя, и вы бросились на $N3!\r\n", FALSE, ch, 0, victim, TO_CHAR);
	OBJ_DATA *weapon;
	if ((weapon = GET_EQ(ch, WEAR_WIELD)) || (weapon = GET_EQ(ch, WEAR_BOTHS))) {
		strcpy(buf, "$n взревел$g и ринул$u на $N3, бешено размахивая $o4!");
		strcpy(buf2, "$N взревел$G и ринул$U на вас, бешено размахивая $o4!");
	} else {
		strcpy(buf, "$n бешено взревел$g и ринул$u на $N3!");
		strcpy(buf2, "$N бешено взревел$G и ринул$U на вас!");
	};
	act(buf, FALSE, ch, weapon, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	act(buf2, FALSE, victim, weapon, ch, TO_CHAR);

	if (!ch->get_fighting()) {
		PRF_FLAGS(ch).set(PRF_IRON_WIND);
		SET_AF_BATTLE(ch, EAF_IRON_WIND);
		hit(ch, victim, TYPE_UNDEFINED, RIGHT_WEAPON);
		set_wait(ch, 2, TRUE);
		//ch->setSkillCooldown(SKILL_GLOBAL_COOLDOWN, 2);
		//ch->setSkillCooldown(SKILL_IRON_WIND, 2);
	} else {
		PRF_FLAGS(ch).set(PRF_IRON_WIND);
		SET_AF_BATTLE(ch, EAF_IRON_WIND);
	}
}

void do_iron_wind(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(SKILL_IRON_WIND)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	};
	if (ch->haveCooldown(SKILL_IRON_WIND)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (GET_AF_BATTLE(ch, EAF_STUPOR) || GET_AF_BATTLE(ch, EAF_MIGHTHIT)) {
		send_to_char("Невозможно! Вы слишкм заняты боем!\r\n", ch);
		return;
	};
	int moves = GET_MAX_MOVE(ch) / (2 + MAX(15, ch->get_skill(SKILL_IRON_WIND)) / 15);
	if (GET_MAX_MOVE(ch) < moves * 2) {
		send_to_char("Вы слишком устали...\r\n", ch);
		return;
	}
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_DRUNKED) && !IS_IMMORTAL(ch) && !GET_GOD_FLAG(ch, GF_GODSLIKE)) {
		send_to_char("Вы слишком здравомыслящи для этого...\r\n", ch);
		return;
	};
	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вам угодно изрубить в капусту?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы с чувством собственного достоинства мощно пустили ветры... Железные.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument)) {
		return;
	}
	if (!check_pkill(ch, vict, arg)) {
		return;
	}

	go_iron_wind(ch, vict);
}

void go_strangle(CHAR_DATA * ch, CHAR_DATA * vict) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || dontCanAct(ch)) {
		send_to_char("Сейчас у вас не получится выполнить этот прием.\r\n", ch);
		return;
	}

	if (ch->get_fighting()) {
		send_to_char("Вы не можете делать это в бою!\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	vict = try_protect(vict, ch);
	if (!pk_agro_action(ch, vict))
		return;

	act("Вы попытались накинуть удавку на шею $N2.\r\n", FALSE, ch, 0, vict, TO_CHAR);

	int prob = train_skill(ch, SKILL_STRANGLE, skill_info[SKILL_STRANGLE].max_percent, vict);
	int delay = 6 - MIN(4, (ch->get_skill(SKILL_STRANGLE) + 30) / 50);
	int percent = number(1, skill_info[SKILL_STRANGLE].max_percent);

	if (percent > prob) {
		Damage dmg(SkillDmg(SKILL_STRANGLE), ZERO_DMG, PHYS_DMG);
		dmg.flags.set(IGNORE_ARMOR);
		dmg.process(ch, vict);
		//set_wait(ch, 3, TRUE);
		setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 3);
	} else {
		AFFECT_DATA<EApplyLocation> af;
		af.type = SPELL_STRANGLE;
		af.duration = IS_NPC(vict) ? 8 : 15;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.battleflag = AF_SAME_TIME;
		af.bitvector = to_underlying(EAffectFlag::AFF_STRANGLED);
		affect_to_char(vict, af);

		int dam = (GET_MAX_HIT(vict)*GaussIntNumber((300 + 5 * ch->get_skill(SKILL_STRANGLE)) / 70, 7.0, 1, 30)) / 100;
		dam = (IS_NPC(vict) ? MIN(dam, 6*GET_MAX_HIT(ch)) : MIN(dam, 2*GET_MAX_HIT(ch)));
		Damage dmg(SkillDmg(SKILL_STRANGLE), dam, PHYS_DMG);
		dmg.flags.set(IGNORE_ARMOR);
		dmg.process(ch, vict);
		if (GET_POS(vict) > POS_DEAD) {
			set_wait(vict, 2, TRUE);
			//vict->setSkillCooldown(SKILL_GLOBAL_COOLDOWN, 2);
			if (on_horse(vict)) {
				act("Рванув на себя, $N стащил$G Вас на землю.", FALSE, vict, 0, ch, TO_CHAR);
				act("Рванув на себя, Вы стащили $n3 на землю.", FALSE, vict, 0, ch, TO_VICT);
				act("Рванув на себя, $N стащил$G $n3 на землю.", FALSE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
				drop_from_horse(vict);
			}
			if (ch->get_skill(SKILL_CHOPOFF) && ch->isInSameRoom(vict)) {
				go_chopoff(ch, vict);
			}
			//set_wait(ch, 2, TRUE);
			setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 2);
		}
	}

	timed_type timed;
	timed.skill = SKILL_STRANGLE;
	timed.time = delay;
	timed_to_char(ch, &timed);
}

void do_strangle(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->get_skill(SKILL_STRANGLE)) {
		send_to_char("Вы не умеете этого.\r\n", ch);
		return;
	}

	if (timed_by_skill(ch, SKILL_STRANGLE) || ch->haveCooldown(SKILL_STRANGLE)) {
		send_to_char("Так часто душить нельзя - человеки кончатся.\r\n", ch);
		return;
	}

	CHAR_DATA* vict = findVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы жаждете удавить?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(vict, EAffectFlag::AFF_STRANGLED)) {
		send_to_char("Ваша жертва хватается руками за горло - не подобраться!\r\n", ch);
		return;
	}

	if (IS_UNDEAD(vict)
		|| GET_RACE(vict) == NPC_RACE_FISH
		|| GET_RACE(vict) == NPC_RACE_PLANT
		|| GET_RACE(vict) == NPC_RACE_THING) {
		send_to_char("Вы бы еще верстовой столб удавить попробовали...\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Воспользуйтесь услугами княжеского палача. Постоянным клиентам - скидки!\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument)) {
		return;
	}
	if (!check_pkill(ch, vict, arg)) {
		return;
	}
	go_strangle(ch, vict);
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
    hit(ch, vict, TYPE_UNDEFINED, RIGHT_WEAPON);
    hit(ch, vict, TYPE_UNDEFINED, LEFT_WEAPON);

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
