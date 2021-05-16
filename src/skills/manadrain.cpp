#include "manadrain.h"

#include "handler.h"
#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/start.fight.h"
#include "skills_info.h"

using  namespace FightSystem;

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

    percent = number(1, skill_info[SKILL_MANADRAIN].difficulty);
    prob = MAX(20, 90 - 5 * MAX(0, GET_LEVEL(vict) - GET_LEVEL(ch)));
  ImproveSkill(ch, SKILL_MANADRAIN, percent > prob, vict);

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
