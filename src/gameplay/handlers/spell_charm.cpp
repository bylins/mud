/**
 \file spell_charm.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellCharm manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/minions.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "engine/core/char_equip_flags.h"
#include "engine/db/obj_save.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"
#include "gameplay/skills/animal_master.h"
#include "engine/ui/cmd/do_follow.h"
#include "gameplay/mechanics/stuff.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/core/remort.h"

namespace handlers {

EStageResult SpellCharm(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	int k_skills = 0;
	ESkill skill_id = ESkill::kUndefined;
		Affect<EApply> af;
	if (victim == nullptr || ch == nullptr)
		return EStageResult::kSuccess;

	// Rejection narration: six of the SpellCharm reject
	// paths share semantics with existing kSummon* / kResurrect* keys; the
	// per-spell kCharm sheaf carries the charm-specific wording while the
	// kDefault texts (phrased for resurrect/summon) stay intact for those
	// callers. Four messages without a clean key match stay inline.
	auto SendCharmMsg = [ch](ESpellMsg key) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kCharm, key) + "\r\n", ch);
	};
	if (victim == ch)
		SendCharmMsg(ESpellMsg::kCustomMsgOne);  // self-cast humor; see kCharm sheaf.
	else if (!victim->IsNpc()) {
		// 3 charm one-offs migrated to kCharm's kCustomMsg slots.
		SendCharmMsg(ESpellMsg::kCustomMsgTwo);
		if (!pk_agro_action(ch, victim))
			return EStageResult::kSuccess;
	} else if (!privilege::IsImmortal(ch)
		&& (AFF_FLAGGED(victim, EAffect::kSanctuary) || victim->IsFlagged(EMobFlag::kProtect)))
		SendCharmMsg(ESpellMsg::kResurrectConsecrated);
	else if (!privilege::IsImmortal(ch) && (AFF_FLAGGED(victim, EAffect::kGodsShield) || victim->IsFlagged(EMobFlag::kProtect)))
		SendCharmMsg(ESpellMsg::kResurrectProtected);
	else if (!privilege::IsImmortal(ch) && victim->IsFlagged(EMobFlag::kNoCharm))
		SendCharmMsg(ESpellMsg::kResurrectNoPower);
	else if (AFF_FLAGGED(ch, EAffect::kCharmed))
		SendCharmMsg(ESpellMsg::kSummonCharmed);
	else if (AFF_FLAGGED(victim, EAffect::kCharmed)
		|| victim->IsFlagged(EMobFlag::kAgressive)
		|| victim->IsFlagged(EMobFlag::kAgressiveMono)
		|| victim->IsFlagged(EMobFlag::kAgressivePoly)
		|| victim->IsFlagged(EMobFlag::kAgressiveDay)
		|| victim->IsFlagged(EMobFlag::kAggressiveNight)
		|| victim->IsFlagged(EMobFlag::kAgressiveFullmoon)
		|| victim->IsFlagged(EMobFlag::kAgressiveWinter)
		|| victim->IsFlagged(EMobFlag::kAgressiveSpring)
		|| victim->IsFlagged(EMobFlag::kAgressiveSummer)
		|| victim->IsFlagged(EMobFlag::kAgressiveAutumn))
		SendCharmMsg(ESpellMsg::kSummonFail);
	else if (mount::IsHorse(victim))
		SendCharmMsg(ESpellMsg::kSummonWarhorse);
	else if (victim->GetEnemy() || victim->GetPosition() < EPosition::kRest)
		act(MUD::SpellMessages().GetMessage(ESpell::kCharm, ESpellMsg::kCustomMsgThree).c_str(),
			false, ch, nullptr, victim, kToChar);
	else if (follow::CircleFollow(victim, ch))
		SendCharmMsg(ESpellMsg::kCustomMsgFour);
	else if (!privilege::IsImmortal(ch)
		&& CalcGeneralSaving(ch, victim, ESaving::kWill, (GetRealCha(ch) - 10) * 4 + remort::GetRealRemort(ch) * 3)) //предлагаю завязать на каст
		SendCharmMsg(ESpellMsg::kSummonFail);
	else {
		if (!CheckCharmices(ch, victim, ESpell::kCharm)) {
			return EStageResult::kSuccess;
		}

		// Левая проверка
		if (victim->has_master()) {
			if (follow::StopFollower(victim, follow::kSfMasterdie)) {
				return EStageResult::kSuccess;
			}
		}

		if (sight::CanSee(victim, ch)) {
			mob_ai::mobRemember(victim, ch);
		}

		if (victim->IsFlagged(EMobFlag::kNoGroup))
			victim->UnsetFlag(EMobFlag::kNoGroup);
		RemoveCharmBond(victim);
		if (GetRealInt(victim) > GetRealInt(ch)) {
			af.duration = CalcDuration(victim, victim, ESkill::kUndefined, GetRealCha(ch), 0, 0, 0);
		} else {
			af.duration = CalcDuration(victim, victim, ESkill::kUndefined, GetRealCha(ch) + number(1, 10) + remort::GetRealRemort(ch) * 2, 0, 0, 0);
		}
		af.modifier = 0;
		af.location = EApply::kNone;
		af.battleflag = kAfCharmBond;

		// резервируем место под фит ()
		// the ~390-line AnimalMaster body moved to
		// src/gameplay/skills/animal_master.{h,cpp}; the race check now uses the
		// named ENpcRace::kAnimal constant instead of the magic number 104.
		if (CanUseFeat(ch, EFeat::kAnimalMaster) && GET_RACE(victim) == ENpcRace::kAnimal) {
			ApplyAnimalMaster(ch, victim, af, k_skills, skill_id);
		}
		victim->summon_helpers.clear();
		if (victim->IsNpc()) {
			// a charmed NPC is now an ally -- mark it as a companion (never set on PCs)
			victim->SetFlag(EMobFlag::kCompanion);
			// strip equipment unless this is an AnimalMaster mag-zver (charmice keeps its magic gear)
			if (victim->get_type_charmice() == 0) {
				for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
					if (GET_EQ(victim, i)) {
						if (!remove_otrigger(GET_EQ(victim, i), victim)) {
							continue;
						}
						act("$n прекратил$g использовать $o3.",
							true, victim, GET_EQ(victim, i), nullptr, kToRoom);
						PlaceObjToInventory(UnequipChar(victim, i, CharEquipFlag::show_msg), victim);
					}
				}
			} else {
				// запрещаем умке одевать шмот
				victim->UnsetFlag(ENpcFlag::kWielding);
				victim->UnsetFlag(ENpcFlag::kArmoring);
			}
			victim->UnsetFlag(EMobFlag::kAgressive);
			victim->UnsetFlag(EMobFlag::kSpec);
			victim->UnsetFlag(EPrf::kPunctual);
			victim->SetFlag(EMobFlag::kNoSkillTrain);
			// по идее при речарме и последующем креше можно оказаться с сейвом без шмота на чармисе -- Krodo
			if (!NPC_FLAGGED(victim, ENpcFlag::kNoMercList)) {
				MobVnum mvn = GET_MOB_VNUM(victim);

				if (mvn / 100 >=  dungeons::kZoneStartDungeons) {
					mvn = zone_table[GetZoneRnum(mvn / 100)].copy_from_zone * 100 + mvn % 100;
				}
				ch->updateCharmee(mvn, 0);
			}
			Crash_crashsave(ch);
			ch->save_char();
		}
		af.modifier = 0;
		af.location = EApply::kNone;
		af.battleflag = kAfCharmBond;
		af.affect_type = EAffect::kCharmed;
		affect_to_char(victim, af);
		follow::AddFollower(ch, victim);
	}
	// тут обрабатываем, если виктим маг-зверь => передаем в фунцию создание маг шмоток (цель, базовый скил, процент владения)
	if (victim->get_type_charmice() != 0) {
		create_charmice_stuff(victim, skill_id, k_skills);
	}
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
