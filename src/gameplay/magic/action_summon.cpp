/**
 \file action_summon.cpp - a part of the Bylins engine.
 \brief Summon stage of the cast pipeline: data-driven summons and corpse-based summons (animate dead,
        resurrection, skeletons).
*/

#include "magic_internal.h"

#include "administration/privilege.h"
#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/mechanics/summon.h"
#include "gameplay/handlers/bone_commander.h"
#include "engine/core/char_handler.h"
#include "engine/core/obj_handler.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/remort.h"

// The caster's own (level + remort + 4) caps the tier: low-level necromancers can never spawn
// higher-tier undead.
static MobVnum PickNecroMobForCorpse(CharData *ch, int corpse_mob_level) {
	return static_cast<MobVnum>(animate_dead::PickTier(ch, corpse_mob_level));
}

// Rename a resurrection mob as "умертвие <его_имя>" in all six cases; kFuryOfDarkness adds +20%
// dam/hit/HR.
static void RenameAsUndead(CharData *ch, CharData *mob) {
	ClearMinionTalents(mob);
	if (mob->IsFlagged(EMobFlag::kNoGroup)) {
		mob->UnsetFlag(EMobFlag::kNoGroup);
	}

	mob->SetCharAliases(fmt::format("умертвие {} {}", GET_PAD(mob, 1), GET_NAME(mob)));
	mob->set_npc_name(fmt::format("умертвие {}", GET_PAD(mob, 1)));
	mob->player_data.long_descr = "";
	mob->player_data.PNames[grammar::ECase::kNom] = fmt::format("умертвие {}", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kDat] = fmt::format("умертвию {}", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kAcc] = fmt::format("умертвие {}", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kIns] = fmt::format("умертвием {}", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kPre] = fmt::format("умертвии {}", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kGen] = fmt::format("умертвия {}", GET_PAD(mob, 1));
	mob->set_sex(EGender::kNeutral);
	mob->SetFlag(EMobFlag::kResurrected);
	mob->SetFlag(EMobFlag::kUndead);
	if (CanUseFeat(ch, EFeat::kFuryOfDarkness)) {
		GET_DR(mob) = GET_DR(mob) + GET_DR(mob) * 0.20;
		mob->set_max_hit(mob->get_max_hit() + mob->get_max_hit() * 0.20);
		mob->set_hit(mob->get_max_hit());
		GET_HR(mob) = GET_HR(mob) + GET_HR(mob) * 0.20;
	}
}

// kAnimateDead post-spawn: undead flag, per-tier rescue for kLoyalAssist / kHauntingSpirit,
// ice shield for 75+ wisdom.
static void EnhanceAnimateDead(CharData *ch, CharData *mob, MobVnum mob_num,
							   ESpell spell_id, int charm_duration) {
	// Not kResurrected: that flag marks a corpse revived by kResurrection (original mob's vnum).
	mob->SetFlag(EMobFlag::kUndead);
	if (mob_num == kMobSkeleton && CanUseFeat(ch, EFeat::kLoyalAssist)) {
		SetSkill(mob, ESkill::kRescue, 100);
	}
	if (mob_num == kMobBonespirit && CanUseFeat(ch, EFeat::kHauntingSpirit)) {
		SetSkill(mob, ESkill::kRescue, 120);
	}

	// даем всем поднятым, ну наверное не будет чернок 75+ мудры вызывать зомби в щите.
	const float eff_wis = CalcEffectiveWis(ch, spell_id);
	if (eff_wis >= 65) {
		// пока не даем, если надо включите
		//af.affect_type = to_underlying(EAffectFlag::AFF_MAGICGLASS);
		//affect_to_char(mob, af);
	}
	if (eff_wis >= 75) {
		Affect<EApply> af;
		af.duration = charm_duration * (1 + remort::GetRealRemort(ch));
		af.modifier = 0;
		af.location = EApply::kNone;
		af.affect_type = EAffect::kIceShield;
		af.battleflag = {kAfCharmBond};   // щит нежити -- часть её пакета, снятию не подлежит
		affect_to_char(mob, af);
	}
}

// Spill the corpse's contents into the caster's room (decay-checking each item) and extract it.
static void SpillCorpseContents(CharData *ch, ObjData *obj) {
	for (ObjData *tobj = obj->get_contains(); tobj; ) {
		ObjData *next_obj = tobj->get_next_content();
		RemoveObjFromObj(tobj);
		PlaceObjToRoom(tobj, ch->in_room);
		if (!CheckObjDecay(tobj) && tobj->get_in_room() != kNowhere) {
			act("На земле остал$U лежать $o.", false, ch, tobj, nullptr, kToRoom | kToArenaListen);
		}
		tobj = next_obj;
	}
	ExtractObjFromWorld(obj);
}

// Spell-specific post-spawn handlers, named by <summon handler="...">.
static const std::map<std::string, std::function<void(CharData *, CharData *, const ActionContext &, int)>>
		kSummonHandlers = {
	{"SetupKeeperStats",
		[](CharData *ch, CharData *mob, const ActionContext &ctx, int) { handlers::SetupKeeperStats(ch, mob, ctx); }},
	{"SetupFirekeeperStats",
		[](CharData *ch, CharData *mob, const ActionContext &ctx, int dur) { handlers::SetupFirekeeperStats(ch, mob, ctx, dur); }},
	{"CloneCascade", handlers::CloneCascade},
};

// Data-driven summon: fail roll, charmed guard, charmice cap, read mob, charm (+keeper) affect,
// place/follow, then the named handler. Fail = base_fail - C*competence_weight, floored at
// min_fail; C = ctx.CompetenceBase().
EStageResult CastSummonAction(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	const auto &s = ctx.action_or_default().GetSummon();
	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonCharmed) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	int pfail = s.base_fail - static_cast<int>(ctx.CompetenceBase() * s.competence_weight);
	if (pfail < s.min_fail) { pfail = s.min_fail; }
	if (pfail > 100) { pfail = 100; }
	if (!privilege::IsImmortal(ch) && number(0, 101) < pfail) {
		const bool fail_sticks = !CanUseFeat(ch, EFeat::kFavorOfDarkness) || number(0, 3) == 0;
		if (fail_sticks) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonFail) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
	}
	// A negative vnum tells ReadMobile to load a *summoned* instance: no trigger assignment, not
	// counted in mob-index total_online, EMobFlag::kCompanion set. <summon vnum> is positive.
	CharData *mob = ReadMobile(-s.mob_vnum, kVirtual);
	if (!mob) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoProto) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (IsSummonTargetProtected(ch, mob, spell_id)) {
		return EStageResult::kSuccess;
	}
	if (!CheckCharmices(ch, mob, spell_id)) {
		ExtractCharFromWorld(mob, false);
		return EStageResult::kSuccess;
	}
	const int duration = FinalizeSummonedMob(ch, mob, spell_id, s.keeper);
	if (!s.handler.empty()) {
		const auto it = kSummonHandlers.find(s.handler);
		if (it != kSummonHandlers.end()) {
			it->second(ch, mob, ctx, duration);
		} else {
			err_log("CastSummonAction: unknown summon handler '%s' for %s.",
					s.handler.c_str(), NAME_BY_ITEM<ESpell>(spell_id).c_str());
		}
	}
	mob->SetFlag(EMobFlag::kNoSkillTrain);
	mob->char_specials.saved.alignment = ch->char_specials.saved.alignment;
	return EStageResult::kSuccess;
}

// Corpse-based summon (kAnimateDead / kResurrection): the mob comes from the corpse and the fail
// chance is the con/level/cast-success/remort roll, not the <summon> formula, so it doesn't fit
// CastSummonAction.
static EStageResult CorpseSummon(ActionContext &ctx, bool resurrection) {
	CharData *const ch = ctx.caster();
	ObjData *const obj = ctx.ovict;
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonCharmed) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (obj == nullptr || !IS_CORPSE(obj)) {
		act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoCorpse).c_str(),
			false, ch, nullptr, nullptr, kToChar);
		return EStageResult::kSuccess;
	}
	MobVnum mob_num = GET_OBJ_VAL(obj, 2);
	int pfail = 0;
	if (resurrection) {
		if (mob_num <= 0) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectBadCorpse) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
		CharData *tmp_mob = mob_proto + GetMobRnum(mob_num);
		pfail = 10 + tmp_mob->get_con() * 2
			- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - remort::GetRealRemort(ch) * 5;
	} else {
		if (mob_num <= 0) {
			mob_num = kMobSkeleton;
		} else {
			CharData *tmp_mob = mob_proto + GetMobRnum(mob_num);
			pfail = 10 + tmp_mob->get_con() * 2
				- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - remort::GetRealRemort(ch) * 5;
			mob_num = PickNecroMobForCorpse(ch, GetRealLevel(tmp_mob));
		}
	}
	if (!privilege::IsImmortal(ch) && number(0, 101) < pfail) {
		const bool fail_sticks = !CanUseFeat(ch, EFeat::kFavorOfDarkness) || number(0, 3) == 0;
		if (fail_sticks) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonFail) + "\r\n", ch);
			ExtractObjFromWorld(obj);
			return EStageResult::kSuccess;
		}
	}
	if (!resurrection) {
		// Demote the tier to the strongest that fits the caster's skill-weight budget
		// (min(Dark-Magic,75) minus held-undead weights). 0 = minion limit reached.
		mob_num = animate_dead::FitUndeadTier(ch, mob_num);
		if (mob_num == 0) {
			SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
			ExtractObjFromWorld(obj);
			return EStageResult::kSuccess;
		}
	}
	CharData *mob = ReadMobile(-mob_num, kVirtual);
	if (!mob) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoProto) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (resurrection) {
		RenameAsUndead(ch, mob);
	}
	if (IsSummonTargetProtected(ch, mob, spell_id)) {
		return EStageResult::kSuccess;
	}
	if (!resurrection) {
		// HP/damage scale off cast potency C for all tiers.
		animate_dead::SetupUndeadStats(ch, mob, ctx.CompetenceBase());
	}
	if (!CheckCharmices(ch, mob, spell_id)) {
		ExtractCharFromWorld(mob, false);
		ExtractObjFromWorld(obj);
		return EStageResult::kSuccess;
	}
	const int duration = FinalizeSummonedMob(ch, mob, spell_id, false);
	if (!resurrection) {
		EnhanceAnimateDead(ch, mob, mob_num, spell_id, duration);
		// Броня, спасы, удача и инициатива выдаются аффектами: прямую запись в бонусы
		// affect_total стирает при каждом пересчёте, а аффекты возвращает сам.
		animate_dead::ApplyVolatileUndeadStats(mob, ctx.CompetenceBase(), duration);
		// Даровые аффекты своего заклинания -- по порогу умения контроля.
		animate_dead::ApplyGrantedAffects(ch, mob, spell_id, duration);
		// Нежить, поднятая при живой костяной свите, приходит уже ослабленной: прибавилась нежить,
		// между которой делится подать, -- и новая должна встать уже со своей долей.
		bone_commander::RefreshLevy(ch);
	}
	mob->SetFlag(EMobFlag::kNoSkillTrain);
	SpillCorpseContents(ch, obj);
	mob->char_specials.saved.alignment = ch->char_specials.saved.alignment;
	return EStageResult::kSuccess;
}

// "Оживить скелет". Особые виды скелетов описаны в animate_dead.xml ярусами spell="kAnimateSkeleton"
// и открываются способностью "костяной полководец". Без способности, с трупа ниже порога или с
// нечеловекоподобного встаёт обычный скелет. Бюджета свиты у скелетов нет (weight="0"):
// ограничение другое -- по одному каждого вида.
EStageResult SpellAnimateSkeleton(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	ObjData *const obj = ctx.ovict;
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonCharmed) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (obj == nullptr || !IS_CORPSE(obj)) {
		act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoCorpse).c_str(),
			false, ch, nullptr, nullptr, kToChar);
		return EStageResult::kSuccess;
	}

	// Труп помнит, чей он: по прототипу берём уровень (порог вида), расу (человекоподобие) и
	// телосложение (шанс провала) -- так же, как это делает "поднять труп".
	const MobVnum corpse_mob = GET_OBJ_VAL(obj, 2);
	int corpse_level = 0;
	bool humanoid = false;
	int pfail = 0;
	if (corpse_mob > 0 && GetMobRnum(corpse_mob) >= 0) {
		CharData *proto = mob_proto + GetMobRnum(corpse_mob);
		corpse_level = GetRealLevel(proto);
		humanoid = (GET_RACE(proto) == ENpcRace::kHuman);
		pfail = 10 + proto->get_con() * 2
			- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - remort::GetRealRemort(ch) * 5;
	}

	MobVnum mob_num = kMobSkeleton;
	const auto &tiers = MUD::AnimateDead().Skeletons();
	// Ярус без прототипа в мире не предлагаем: движок и мир обновляются порознь, и пока прототипов
	// нет, заклинание должно поднимать обычного скелета, а не отказывать игроку пустым списком видов.
	std::vector<const animate_dead::CreatureInfo *> usable;
	for (const auto &tier : tiers) {
		if (GetMobRnum(tier.proto_vnum) >= 0) {
			usable.push_back(&tier);
		}
	}
	if (!tiers.empty() && usable.size() < tiers.size()) {
		// Раз за перезагрузку: это не ошибка игрока, а недоложенный мир, и знать о нём должны боги.
		static bool reported = false;
		if (!reported) {
			reported = true;
			log("SYSERR: 'оживить скелет': в мире нет прототипов %zu из %zu видов скелетов "
				"(первый ярус -- моб #%d), поднимается обычный скелет",
				tiers.size() - usable.size(), tiers.size(), tiers.front().proto_vnum);
		}
	}
	if (CanUseFeat(ch, EFeat::kBoneCommander) && !usable.empty()) {
		// Вид называет игрок: это слово, дописанное после трупа. Сверяем со всеми падежными
		// формами прототипа -- игрок пишет "свирепого", а в именительном стоит "свирепый".
		const std::string &arg = ctx.cast_extra;
		std::string names;
		const animate_dead::CreatureInfo *chosen = nullptr;
		for (const auto *tier : usable) {
			CharData *tier_proto = mob_proto + GetMobRnum(tier->proto_vnum);
			if (!names.empty()) {
				names += ", ";
			}
			names += tier_proto->player_data.PNames[grammar::ECase::kAcc];
			if (arg.empty()) {
				continue;
			}
			for (const auto &form : tier_proto->player_data.PNames) {
				if (isname(arg, form)) {
					chosen = tier;
					break;
				}
			}
			if (chosen) {
				break;
			}
		}
		if (arg.empty()) {
			SendMsgToChar(fmt::format("Какого скелета вы желаете оживить? Вам подвластны: {}.\r\n", names), ch);
			return EStageResult::kSuccess;
		}
		if (!chosen) {
			SendMsgToChar(fmt::format("Такого скелета вы оживить не умеете. Вам подвластны: {}.\r\n", names), ch);
			return EStageResult::kSuccess;
		}
		if (corpse_level < chosen->corpse_min_level || !humanoid) {
			SendMsgToChar("Эти кости не годятся для такого скелета, из них встанет лишь обычный.\r\n", ch);
		} else {
			// По одному каждого вида: потолок суммарной силы свиты держится этим, а не отдельным
			// счётом урона -- виды разной силы, и каждый бывает только один.
			for (const auto *follower : ch->followers) {
				if (follower->IsNpc() && GET_MOB_VNUM(follower) == chosen->proto_vnum) {
					SendMsgToChar(fmt::format("У вас уже служит {}.\r\n",
											  follower->player_data.PNames[grammar::ECase::kNom]), ch);
					return EStageResult::kSuccess;
				}
			}
			mob_num = chosen->proto_vnum;
		}
	}

	if (!privilege::IsImmortal(ch) && number(0, 101) < pfail) {
		const bool fail_sticks = !CanUseFeat(ch, EFeat::kFavorOfDarkness) || number(0, 3) == 0;
		if (fail_sticks) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonFail) + "\r\n", ch);
			ExtractObjFromWorld(obj);
			return EStageResult::kSuccess;
		}
	}

	CharData *mob = ReadMobile(-mob_num, kVirtual);
	if (!mob) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoProto) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (IsSummonTargetProtected(ch, mob, spell_id)) {
		return EStageResult::kSuccess;
	}
	animate_dead::SetupUndeadStats(ch, mob, ctx.CompetenceBase());
	if (!CheckCharmices(ch, mob, spell_id)) {
		ExtractCharFromWorld(mob, false);
		ExtractObjFromWorld(obj);
		return EStageResult::kSuccess;
	}
	const int duration = FinalizeSummonedMob(ch, mob, spell_id, false);
	mob->SetFlag(EMobFlag::kUndead);
	// Броня, спасы и инициатива держатся аффектами: affect_total стирает прямую запись.
	animate_dead::ApplyVolatileUndeadStats(mob, ctx.CompetenceBase(), duration);
	// Даровые аффекты по порогу магии тьмы (видеть невидимых, полёт и прочее).
	animate_dead::ApplyGrantedAffects(ch, mob, spell_id, duration);
	if (mob_num != kMobSkeleton) {
		// Метка особого вида -- именно аффектом, а не флагом в файле моба: действия на удар (kPostHit)
		// движок ищет в списке аффектов, а флаг из affect_flags ничего не запускает. Часть пакета
		// нежити (kAfCharmBond), поэтому снятию не подлежит.
		Affect<EApply> mark;
		mark.affect_type = EAffect::kBoneServant;
		mark.location = EApply::kNone;
		mark.modifier = 0;
		mark.duration = duration;
		mark.battleflag = {kAfCharmBond};
		affect_to_char(mob, mark);
		// Свита выросла -- нежить хозяина отдаёт ей часть урона.
		bone_commander::RefreshLevy(ch);
	}
	mob->SetFlag(EMobFlag::kNoSkillTrain);
	SpillCorpseContents(ch, obj);
	mob->char_specials.saved.alignment = ch->char_specials.saved.alignment;
	return EStageResult::kSuccess;
}

EStageResult SpellAnimateDead(ActionContext &ctx) { return CorpseSummon(ctx, false); }
EStageResult SpellResurrection(ActionContext &ctx) { return CorpseSummon(ctx, true); }

// Registered summon handler names (sorted), for the Vedun editor pick-list.
std::vector<std::string> SummonHandlerNames() {
	std::vector<std::string> v; for (const auto &p : kSummonHandlers) v.push_back(p.first); return v;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
