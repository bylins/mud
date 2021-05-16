#include "turnundead.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/common.h"
#include "action_targeting.h"
#include "abilities/abilities_rollsystem.h"
#include "handler.h"
#include "spells.h"
#include "cmd/flee.h"

using  namespace FightSystem;
using  namespace abilities;

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
