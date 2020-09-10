#include "start.fight.h"

#include "fight.h"
#include "pk.h"
#include "fight_hit.hpp"
#include "skills/protect.h"
#include "remember.hpp"
#include "mobact.hpp"
#include "common.h"

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
            mobRemember(ch, victim);
        } else if (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM)
                   && victim->has_master()
                   && !IS_NPC(victim->get_master())) {
            if (MOB_FLAGGED(victim, MOB_CLONE)) {
                mobRemember(ch, victim->get_master());
            } else if (ch->isInSameRoom(victim->get_master()) && CAN_SEE(ch, victim->get_master())) {
                mobRemember(ch, victim->get_master());
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
