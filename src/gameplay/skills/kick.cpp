#include "kick.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"

#include "skill_messages.h"
#include "engine/db/global_objects.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/common.h"
#include "protect.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/mechanics/damage.h"
#include "gameplay/core/remort.h"

// ******************  KICK PROCEDURES
void go_kick(CharData *ch, CharData *vict) {
	const char *to_char = nullptr, *to_vict = nullptr, *to_room = nullptr;

	if (IsUnableToAct(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kKick, ESkillMsg::kCantFightNow) + "\r\n", ch);
		return;
	}
	if (ch->Skills().HasActiveCooldown(ESkill::kKick)) {
		SendMsgToChar("Вы уже все ноги себе отбили, отдохните слегка.\r\n", ch);
		return;
	};
//	if (vict->purged()) {
//		return;
//	}

	vict = TryToFindProtector(vict, ch);

	bool success = false;
//	if (ch->IsFlagged(EPrf::kTester)) {
	SkillRollResult result = MakeSkillTest(ch, ESkill::kKick, vict);
	success = result.success;
/*	} else {
		int percent = ((10 - (compute_armor_class(vict) / 10)) * 2) + number(1, MUD::Skill(ESkill::kKick).difficulty);
		int prob = CalcCurrentSkill(ch, ESkill::kKick, vict);
		if (GET_GOD_FLAG(vict, EGf::kGodscurse) || AFF_FLAGGED(vict, EAffect::kHold)) {
			prob = percent;
		}
		if (GET_GOD_FLAG(ch, EGf::kGodscurse) || (!mount::IsOnHorse(ch) && mount::IsOnHorse(vict))) {
			prob = 0;
		}
		if (IsAffectedWithFlag(ch, kAfEntanglement)) {
			prob /= 3;
		}
		success = percent <= prob;
		SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kKick).name, percent, prob, success);
	}
*/
	TrainSkill(ch, ESkill::kKick, success, vict);
	int cooldown = 2;
	if (!success) {
		Damage dmg(SkillDmg(ESkill::kKick), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.Process(ch, vict);
		cooldown = 2;
	} else {
	int dam =str_bonus(GetRealStr(ch), STR_TO_DAM) + GetRealDamroll(ch) + GetRealLevel(ch);
	int skill_modi =std::max(0, GetSkill(ch, ESkill::kKick) * 2 / 5);
	int nice = number(1, 200) <= number(0, number(0, ch->calc_morale()));
	int bottom = (nice? skill_modi / 4 : 0);
	skill_modi = number(bottom, skill_modi);
	dam += dam * skill_modi / 100;
	int weight_modi = 5 * (20 + (GET_EQ(ch, EEquipPos::kFeet) ? GET_EQ(ch, EEquipPos::kFeet)->get_weight() : 0));
	dam = dam * weight_modi / 100;
	dam = number(dam * 2  /5, dam);
		if (mount::IsOnHorse(ch) && (GetSkill(ch, ESkill::kRiding) >= 150) && (GetSkill(ch, ESkill::kKick) >= 150)) {
			Affect<EApply> af;
			af.location = EApply::kNone;
			af.modifier = 0;
			af.battleflag = 0;
			float modi = ((GetSkill(ch, ESkill::kKick) + GetRealStr(ch) * 5)
				+ (GET_EQ(ch, EEquipPos::kFeet) ? GET_EQ(ch, EEquipPos::kFeet)->get_weight() : 0) * 3) / float(GET_SIZE(vict));
			if (number(1, 1000) < modi * 10) {
				switch (number(0, (GetSkill(ch, ESkill::kKick) - 150) / 10)) {
					case 0:
					case 1:
						if (!AFF_FLAGGED(vict, EAffect::kStopRight)) {
							to_char = "Каблук вашего сапога надолго запомнится $N2, если конечно он выживет.";
							to_vict = "Мощный удар ноги $n1 изуродовал вам правую руку.";
							to_room = "След сапога $n1 надолго запомнится $N2, если конечно он$Q выживет.";
							af.affect_type = EAffect::kStopRight;
							af.duration = CalcDuration(vict, vict, ESkill::kUndefined, 3 + remort::GetRealRemort(ch) / 4, 0, 0, 0);
							af.battleflag = kAfBattledec | kAfPulsedec;
						} else if (!AFF_FLAGGED(vict, EAffect::kStopLeft)) {
							to_char = "Каблук вашего сапога надолго запомнится $N2, если конечно он выживет.";
							to_vict = "Мощный удар ноги $n1 изуродовал вам левую руку.";
							to_room = "След сапога $n1 надолго запомнится $N2, если конечно он выживет.";
							af.affect_type = EAffect::kStopLeft;
							af.duration = CalcDuration(vict, vict, ESkill::kUndefined, 3 + remort::GetRealRemort(ch) / 4, 0, 0, 0);
							af.battleflag = kAfBattledec | kAfPulsedec;
						} else {
							to_char = "Каблук вашего сапога надолго запомнится $N2, $M теперь даже бить вас нечем.";
							to_vict = "Мощный удар ноги $n1 вывел вас из строя.";
							to_room = "Каблук сапога $n1 надолго запомнится $N2, $M теперь даже биться нечем.";
							af.affect_type = EAffect::kStopFight;
							af.duration = CalcDuration(vict, vict, ESkill::kUndefined, 3 + remort::GetRealRemort(ch) / 4, 0, 0, 0);
							af.battleflag = kAfBattledec | kAfPulsedec;
						}
						break;
					case 2:
					case 3:
						to_char = "Сильно пнув в челюсть, вы заставили $N3 замолчать.";
						to_vict = "Мощный удар ноги $n1 попал вам точно в челюсть, заставив вас замолчать.";
						to_room = "Сильно пнув ногой в челюсть $N3, $n заставил$q $S замолчать.";
						af.affect_type = EAffect::kSilence;
						af.duration = CalcDuration(vict, vict, ESkill::kUndefined, 3 + remort::GetRealRemort(ch) / 5, 0, 0, 0);
						af.battleflag = kAfBattledec | kAfPulsedec;
						dam *= 2;
						break;
					default:
						if (!vict->IsFlagged(EMobFlag::kNoBash)) {
							SetBattleLag(vict, number(2, 5));
							if (vict->GetPosition() > EPosition::kSit) {
								vict->SetPosition(EPosition::kSit);
								mount::DropFromHorse(vict);
							}
							to_char = "Ваш мощный пинок выбил пару зубов $N2, усадив $S на землю!";
							to_vict = "Мощный удар ноги $n1 попал точно в голову, свалив вас с ног.";
							to_room = "Мощный пинок $n1 выбил пару зубов $N2, усадив $S на землю!";
						}
						dam *= 3;
						break;
				}
			} else if (number(1, 1000) < (GetSkill(ch, ESkill::kRiding) / 2)) {
				dam *= 2;
				if (!ch->IsNpc())
					SendMsgToChar("Вы привстали на стременах.\r\n", ch);
			}

			if (to_char) {
				if (!ch->IsNpc()) {
					sprintf(buf, "&G&q%s&Q&n", to_char);
					act(buf, false, ch, nullptr, vict, kToChar);
					sprintf(buf, "%s", to_room);
					act(buf, true, ch, nullptr, vict, kToNotVict | kToArenaListen);
				}
			}
			if (to_vict) {
				if (!vict->IsNpc()) {
					sprintf(buf, "&R&q%s&Q&n", to_vict);
					act(buf, false, ch, nullptr, vict, kToVict);
				}
			}
			ImposeAffect(vict, af, true, false, true, false);
		}

		if (vict->battle_affects.get(kEafAwake)) {
			dam >>= (2 - (mount::IsOnHorse(ch) ? 1 : 0));
		}
		if (result.CritLuck && !mount::IsOnHorse(ch)) {
			dam *= 2;
			if (!vict->IsFlagged(EMobFlag::kNoBash)) {
				if (vict->GetPosition() > EPosition::kSit) {
					vict->SetPosition(EPosition::kSit);
					mount::DropFromHorse(vict);
					SetBattleLag(vict, 2);
					to_char = "$N упал$A на землю!";
					to_vict = "Мощный удар $n1 свалил вас с ног.";
					to_room = "Мощный пинок $n1 усадил $N1 на землю!";
					if (!ch->IsNpc()) {
						sprintf(buf, "&G&q%s&Q&n", to_char);
						act(buf, false, ch, nullptr, vict, kToChar);
						sprintf(buf, "%s", to_room);
						act(buf, true, ch, nullptr, vict, kToNotVict | kToArenaListen);
					}
					if (!vict->IsNpc()) {
						sprintf(buf, "&R&q%s&Q&n", to_vict);
						act(buf, false, ch, nullptr, vict, kToVict);
					}
				}
			}
		}
		Damage dmg(SkillDmg(ESkill::kKick), dam, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kIgnoreBlink);
		dmg.Process(ch, vict);
		cooldown = 2;
	}
	SetSkillCooldownInFight(ch, ESkill::kKick, cooldown);
	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
}

void do_kick(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (GetSkill(ch, ESkill::kKick) < 1) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kKick, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kKick, ESkillMsg::kNoTarget) + "\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	do_kick(ch, vict);
}

void do_kick(CharData *ch, CharData *vict) {
	if (GetSkill(ch, ESkill::kKick) < 1) {
		log("ERROR: вызов пинка для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->Skills().HasActiveCooldown(ESkill::kKick)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kKick, ESkillMsg::kOnCooldown) + "\r\n", ch);
		return;
	};

	if (vict == ch) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kKick, ESkillMsg::kCantTargetSelf) + "\r\n", ch);
		return;
	}

	if (privilege::IsImpl(ch) || !ch->GetEnemy()) {
		go_kick(ch, vict);
	} else if (IsHaveNoExtraAttack(ch)) {
		act("Хорошо. Вы попытаетесь пнуть $N3.", false, ch, nullptr, vict, kToChar);
		ch->SetExtraAttack(kExtraAttackKick, vict);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
