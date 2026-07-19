// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "poison.h"
#include "administration/privilege.h"

#include <algorithm>
#include "gameplay/affects/affect_handler.h"
#include "utils/grammar/gender.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/mount.h"

#include "engine/entities/obj_data.h"
#include "engine/entities/char_data.h"
#include "liquid.h"
#include "engine/ui/color.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/damage.h"

void PerformPoisonedWeapom(CharData *ch, CharData *vict, ESpell spell_id);
void PerformToxicate(CharData *ch, CharData *vict, int modifier);

namespace {

// * Наложение ядов с пушек, аффект стакается до трех раз.
	// issue.affect-migration: keyed on the kAfPoison flag (every poison-cluster slot carries it), not on
	// Affect::type. A poison occupies a fixed set of stat slots (EApply locations) that are DISJOINT across
	// the five poisons, so "an existing kAfPoison affect at the same location" means the same poison on the
	// same slot: same caster -> stack it, different caster -> "борода" (refuse). Non-poison affects lack
	// kAfPoison, so they are never matched.
	bool poison_affect_join(CharData *ch, CharData *vict, Affect<EApply> &af) {
		bool is_poisoned_by_me = false;

		for (auto affect_i = vict->affected.begin(); affect_i != vict->affected.end() && af.location; ++affect_i) {
			const auto affect = *affect_i;

			if (!IS_SET(affect->battleflag, kAfPoison) || affect->location != af.location) {
				continue;
			}

			if (affect->caster_id != ch->get_uid()) {
				// тот же слот яда от другого кастера - борода
				return false;
			}

			// свой яд на этом слоте - стакаем
			{
				if (abs(affect->modifier / 3) < abs(af.modifier)) {
					affect->modifier += af.modifier;
				}
				affect->duration = 7;
				if (!vict->IsNpc()) {
					affect->duration *= 30;
				}
				RemoveAffect(vict, affect_i);
				affect_to_char(vict, *affect);
				is_poisoned_by_me = true;
				break;
			}
		}

		if (!is_poisoned_by_me) {
			affect_to_char(vict, af);
		}
		return true;
	}

// * Отравление с пушек.
	bool PoisonVictWithWeapon(CharData *ch, CharData *vict, ESpell spell_id) {
		if (spell_id == ESpell::kAconitumPoison) {
			// урон 5 + левел/2, от 5 до 20 за стак
			Affect<EApply> af[3];
			// issue.damage-over-time: af[0] is the DoT carrier. Its kAconitumPoison-location apply folds
			// modifier/4 into GET_POISON (affect_modify), so aconite contributes to the SHARED poison DoT at
			// a reduced rate (4x weaker than a plain poison -- it is also a heavy debuff). It is a kPoisoned
			// affect (like the aconite SPELL), so the single kPoisoned <damage source="poison"> tick deals it
			// once per round together with any regular poison -- no separate mechanic, no double-count. The
			// "отравление аконитом" identity + magic-resist debuff live on af[2] (kAconitumPoison affect).
			af[0].location = EApply::kAconitumPoison;
			af[0].modifier = GetSkill(ch, ESkill::kPoisoning);
			af[0].affect_type = EAffect::kPoisoned;

			af[1].location = EApply::kPhysicResist;
			af[1].modifier = -4;
			af[1].affect_type = EAffect::kNoBattleSwitch;

			af[2].location = EApply::kMagicResist;
			af[2].modifier = -4;
			af[2].affect_type = EAffect::kAconitumPoison;   // issue.affect-migration: cluster identity (was kUndefined)

			bool was_poisoned = true;

			for (auto & i : af) {
				i.caster_id = ch->get_uid();
				i.duration = 7;
				if (!vict->IsNpc()) {
					i.duration *= 30;
				}
				i.battleflag = kAfSameTime | kAfCurable;

				if (!poison_affect_join(ch, vict, i)) {
					was_poisoned = false;
				}
			}
			if (was_poisoned) {
				return true;   // автор отравления уже записан в caster_id наложенных аффектов
			}

		} else if (spell_id == ESpell::kScopolaPoison) {
			//Снимает сависы с жертвы
			Affect<EApply> af[4];
			af[0].location = EApply::kSavingReflex;
			af[1].location = EApply::kSavingCritical;
			af[2].location = EApply::kSavingWill;
			af[3].location = EApply::kSavingStability;
			// One EAffect per affect now: keep the poison flag on af[3] and put
			// kNoBattleSwitch on a sibling. All entries share type/duration, so the
			// victim ends up with the same flags, applied and removed together.
			af[3].affect_type = EAffect::kScopolaPoison;
			af[0].affect_type = EAffect::kNoBattleSwitch;
			af[1].affect_type = EAffect::kScopolaPoison;   // issue.affect-migration: cluster identity (was kUndefined)
			af[2].affect_type = EAffect::kScopolaPoison;   // issue.affect-migration: cluster identity (was kUndefined)
			int affect_modifier = 10;

			if (vict->IsNpc()) {
				affect_modifier += std::min((GetSkill(ch, ESkill::kPoisoning) * 0.1), 40.0);
			} else {
				affect_modifier += std::min((GetSkill(ch, ESkill::kPoisoning) * 0.05), 10.0);
			}

			bool was_poisoned = true;

			for (auto & i : af) {
				i.caster_id = ch->get_uid();
				i.duration = 7;
				if (!vict->IsNpc()) {
					i.duration *= 30;
				}
				i.modifier = affect_modifier;
				i.battleflag = kAfSameTime | kAfCurable;

				if (!poison_affect_join(ch, vict, i)) {
					was_poisoned = false;
				}
			}
			if (was_poisoned) {
				return true;
			}

		} else if (spell_id == ESpell::kBelenaPoison) {
			// не переключается (моб)
			// -хитролы/хп-рег/дамаг-физ.атак/скилы

			Affect<EApply> af[4];
			// One EAffect per affect now: the poison flag plus its two secondary
			// flags are spread across sibling entries (same type/duration).
			af[1].affect_type = EAffect::kBelenaPoison;
			af[0].affect_type = EAffect::kSkillReduce;
			af[2].affect_type = EAffect::kNoBattleSwitch;

			// скилл * 0.05 + 5 на чаров и + 10 на мобов. 5.5-15% и 10.5-20% (10-200 скила)
			int percent = (std::min(GetSkill(ch, ESkill::kPoisoning), 200) * 5 / 100) + (vict->IsNpc() ? 10 : 5);
			// -скиллы
			af[0].location = EApply::kBelenaPoison;
			af[0].modifier = std::max(percent, 10);
			// -дамаг физ.атак
			af[1].location = EApply::kPhysicDamagePercent;
			af[1].modifier = -std::max(percent, 10);
			// -хитролы
			int remove_hit = (GET_REAL_HR(vict) * percent) * 0.01;
			af[2].location = EApply::kHitroll;
			af[2].modifier = -remove_hit;
			// -хп-рег
			int remove_hp = (vict->get_hitreg() * percent) * 0.01;
			af[3].location = EApply::kHpRegen;
			af[3].modifier = -remove_hp;
			af[3].affect_type = EAffect::kBelenaPoison;   // issue.affect-migration: cluster identity (was kUndefined)

			bool was_poisoned = true;
			for (auto & i : af) {
				i.caster_id = ch->get_uid();
				i.duration = 7;
				if (!vict->IsNpc()) {
					i.duration *= 30;
				}
				i.battleflag = kAfSameTime | kAfCurable;

				if (!poison_affect_join(ch, vict, i)) {
					was_poisoned = false;
				}
			}

			if (was_poisoned) {
				return true;
			}
		} else if (spell_id == ESpell::kDaturaPoison) {
			// не переключается (моб)
			// -каст/мем-рег/дамаг-заклов/скилы
			// AFF_DATURA_POISON - флаг на снижение дамага с заклов

			Affect<EApply> af[3];
			// One EAffect per affect now: the poison flag plus its two secondary
			// flags are spread across sibling entries (same type/duration).
			af[0].affect_type = EAffect::kDaturaPoison;
			af[1].affect_type = EAffect::kSkillReduce;
			af[2].affect_type = EAffect::kNoBattleSwitch;

			// скилл * 0.05 + 5 на чаров и + 10 на мобов. 5.5-15% и 10.5-20% (10-200 скила)
			int percent = (std::min(GetSkill(ch, ESkill::kPoisoning), 200) * 5 / 100) + (vict->IsNpc() ? 10 : 5);
			// -скиллы
			af[0].location = EApply::kDaturaPoison;
			af[0].modifier = std::max(percent, 10);;
			// -маг.дамаг
			af[0].location = EApply::kMagicDamagePercent;
			af[0].modifier = -std::max(percent, 10);;
			// -каст
			af[1].location = EApply::kCastSuccess;
			af[1].modifier = -(GET_CAST_SUCCESS(vict) * percent) * 0.01;
			// -мем
			int remove_mem = (GET_MANAREG(vict) * percent) * 0.01;
			af[2].location = EApply::kManaRegen;
			af[2].modifier = -remove_mem;

			bool was_poisoned = true;
			for (auto & i : af) {
				i.duration = 7;
				if (!ch->IsNpc()) {
					i.duration *= 30;
				}
				i.caster_id = ch->get_uid();
				i.battleflag = kAfSameTime | kAfCurable;

				if (!poison_affect_join(ch, vict, i)) {
					was_poisoned = false;
				}
			}
			if (was_poisoned) {
				return true;
			}
		}
		return false;
	}

// * Крит при отравлении с пушек.
	void ProcessCritWeaponPoison(CharData *ch, CharData *vict, ESpell/* spell_num*/) {
		Affect<EApply> af;
		// issue #3610: крит-яд вешает свои узлы kPoisoned поверх основного, а тик берет из списка
		// первый узел этого типа. Без автора весь урон яда переставал кому-либо засчитываться,
		// и моб, умерший от яда, не давал опыта отравителю.
		af.caster_id = ch->get_uid();
		if (number(1, 100) <= 15) {
			switch (number(1, 3)) {
				case 1:
					// аналог баша с лагом
					if (vict->GetPosition() >= EPosition::kFight) {
						if (mount::IsOnHorse(vict)) {
							SendMsgToChar(ch, "%sОт действия вашего яда у %s закружилась голова!%s\r\n",
										  kColorGrn, sight::PersonName(vict, ch, 1), kColorNrm);
							SendMsgToChar(vict, "Вы почувствовали сильное головокружение и не смогли усидеть на %s!\r\n",
										  GET_PAD(mount::GetHorse(vict), 5));
							act("$n0 зашатал$u и не смог$q усидеть на $N5.",
								true, vict, nullptr, mount::GetHorse(vict), kToNotVict);
							mount::DropFromHorse(vict);
						} else {
							SendMsgToChar(ch, "%sОт действия вашего яда у %s подкосились ноги!%s\r\n",
										  kColorGrn, sight::PersonName(vict, ch, 1), kColorNrm);
							SendMsgToChar(vict, "Вы почувствовали сильное головокружение и не смогли устоять на ногах!\r\n");
							act("$N0 зашатал$U и не смог$Q устоять на ногах.",
								true, ch, nullptr, vict, kToNotVict);
							vict->SetPosition(EPosition::kSit);
							mount::DropFromHorse(vict);
							SetBattleLag(vict, 3);
						}
						break;
					}
					break;
				case 2: {
					// минус статы (1..10)
					af.duration = 30;
					if (!vict->IsNpc()) {
						af.duration *= 30;
					}
					af.modifier = -GetRealLevel(ch) / 6 * 2;
					af.affect_type = EAffect::kPoisoned;
					af.battleflag = kAfSameTime | kAfCurable;

					for (int i = EApply::kStr; i <= EApply::kCha; i++) {
						af.location = static_cast<EApply>(i);
						ImposeAffect(vict, af, false, false, false, false);
					}

					SendMsgToChar(ch, "%sОт действия вашего яда %s побледнел%s!%s\r\n",
								  kColorGrn, sight::PersonName(vict, ch, 0), grammar::VisSexEnding(sight::CanSee((ch), (vict)), (vict)->get_sex(), 1), kColorNrm);
					SendMsgToChar(vict, "Вы почувствовали слабость во всем теле!\r\n");
					act("$N0 побледнел$G на ваших глазах.", true, ch, nullptr, vict, kToNotVict);
					break;
				} // case 2
				case 3: {
					// минус инициатива (1..5)
					af.duration = 30;
					if (!vict->IsNpc()) {
						af.duration *= 30;
					}
					af.location = EApply::kInitiative;
					af.modifier = -GetRealLevel(ch) / 6;
					af.affect_type = EAffect::kPoisoned;
					af.battleflag = kAfSameTime | kAfCurable;
					ImposeAffect(vict, af, false, false, false, false);
					SendMsgToChar(ch, "%sОт действия вашего яда %s стал%s заметно медленнее двигаться!%s\r\n",
								  kColorGrn, sight::PersonName(vict, ch, 0), grammar::VisSexEnding(sight::CanSee((ch), (vict)), (vict)->get_sex(), 1), kColorNrm);
					SendMsgToChar(vict, "Вы стали заметно медленнее двигаться!\r\n");
					act("$N0 стал$G заметно медленнее двигаться!",
						true, ch, nullptr, vict, kToNotVict);
					break;
				} // case 3
			} // switch

		}
	}

} // namespace

void ProcessToxicMob(CharData *ch, CharData *victim, HitData &hit_data) {
	if (hit_data.dam
		&& ch->IsNpc()
		&& NPC_FLAGGED(ch, ENpcFlag::kToxic)
		&& !AFF_FLAGGED(ch, EAffect::kCharmed)
		&& ch->get_wait() <= 0
		&& !AFF_FLAGGED(victim, EAffect::kPoisoned)
		&& number(0, 100) < GET_LIKES(ch) + GetRealLevel(ch) - GetRealLevel(victim)
		&& !CalcGeneralSaving(ch, victim, ESaving::kCritical, -GetRealCon(victim))) {
		PerformToxicate(ch, victim, std::max(1, GetRealLevel(ch) - GetRealLevel(victim)) * 10);
	}
}

void PerformToxicate(CharData *ch, CharData *vict, int modifier) {
	Affect<EApply> af[4];

	// change strength
	af[0].location = EApply::kStr;
	af[0].duration = CalcDuration(ch, vict, ESkill::kUndefined, 1, 0, 0, 0);
	af[0].modifier = -std::min(2, (modifier + 29) / 40);
	af[0].affect_type = EAffect::kPoisoned;
	af[0].battleflag = kAfSameTime | kAfCurable;
	// change damroll
	af[1].location = EApply::kDamroll;
	af[1].duration = af[0].duration;
	af[1].modifier = -std::min(2, (modifier + 29) / 30);
	af[1].affect_type = EAffect::kPoisoned;
	af[1].battleflag = kAfSameTime | kAfCurable;
	// change hitroll
	af[2].location = EApply::kHitroll;
	af[2].duration = af[0].duration;
	af[2].modifier = -std::min(2, (modifier + 19) / 20);
	af[2].affect_type = EAffect::kPoisoned;
	af[2].battleflag = kAfSameTime | kAfCurable;
	// change poison level
	af[3].location = EApply::kPoison;
	af[3].duration = af[0].duration;
	af[3].modifier = GetRealLevel(ch);
	af[3].affect_type = EAffect::kPoisoned;
	af[3].battleflag = kAfSameTime | kAfCurable;

	for (auto & i : af) {
		i.caster_id = ch->get_uid();   // автор отравления -- хранится в аффекте (для засчёта убийства)
		ImposeAffect(vict, i, false, false, false, false);
	}

	snprintf(buf, sizeof(buf), "%sВы отравили $N3.%s", kColorBoldGrn, kColorCyn);
	act(buf, false, ch, nullptr, vict, kToChar);
	snprintf(buf, sizeof(buf), "%s$n отравил$g вас.%s", kColorBoldRed, kColorCyn);
	act(buf, false, ch, nullptr, vict, kToVict);
}

void ProcessPoisonedWeapom(CharData *ch, CharData *victim, HitData &hit_data) {
	if (!victim->IsFlagged(EMobFlag::kProtect)
		&& hit_data.dam
		&& hit_data.wielded
		&& hit_data.wielded->has_timed_spell()
		&& GetSkill(ch, ESkill::kPoisoning)) {
		PerformPoisonedWeapom(ch, victim, hit_data.wielded->timed_spell().IsSpellPoisoned());
	}
}

// * Попытка травануть с пушки при ударе.
void PerformPoisonedWeapom(CharData *ch, CharData *vict, ESpell spell_id) {
	if (spell_id < ESpell::kFirst) {
		return;
	}
	SkillRollResult result = MakeSkillTest(ch, ESkill::kPoisoning, vict);
	bool success = result.success;

	if (((success) && (number(1, 100) <= 25)) ||
		(!vict->battle_affects.get(kEafFirstPoison) && !AFF_FLAGGED(vict, EAffect::kPoisoned))) {
		ImproveSkill(ch, ESkill::kPoisoning, true, vict);
		if (PoisonVictWithWeapon(ch, vict, spell_id)) {
			if (spell_id == ESpell::kAconitumPoison) {
				SendMsgToChar(ch, "Кровоточащие язвы покрыли тело %s.\r\n",
							  sight::PersonName(vict, ch, 1));
			} else if (spell_id == ESpell::kScopolaPoison) {
				strcpy(buf1, sight::PersonName(vict, ch, 0));
				utils::CAP(buf1);
				SendMsgToChar(ch, "%s скрючил%s от нестерпимой боли.\r\n",
							  buf1, grammar::VisSexEnding(sight::CanSee((ch), (vict)), (vict)->get_sex(), 2));
				vict->battle_affects.set(kEafFirstPoison);
			} else if (spell_id == ESpell::kBelenaPoison) {
				strcpy(buf1, sight::PersonName(vict, ch, 3));
				utils::CAP(buf1);
				SendMsgToChar(ch, "%s перестали слушаться руки.\r\n", buf1);
				vict->battle_affects.set(kEafFirstPoison);
			} else if (spell_id == ESpell::kDaturaPoison) {
				strcpy(buf1, sight::PersonName(vict, ch, 2));
				utils::CAP(buf1);
				SendMsgToChar(ch, "%s стало труднее плести заклинания.\r\n", buf1);
				vict->battle_affects.set(kEafFirstPoison);
			} else {
				SendMsgToChar(ch, "Вы отравили %s.\r\n", sight::PersonName(ch, vict, 3));
			}
			SendMsgToChar(vict, "%s%s отравил%s вас.%s\r\n",
						  kColorBoldRed, sight::PersonName(ch, vict, 0),
						  grammar::VisSexEnding(sight::CanSee((vict), (ch)), (ch)->get_sex(), 1), kColorNrm);
			ProcessCritWeaponPoison(ch, vict, spell_id);
		}
	}
}

// * Проверка типа жидкости на яд для нанесения на пушку.
bool poison_in_vessel(int liquid_num) {
	if (liquid_num == LIQ_POISON_ACONITUM
		|| liquid_num == LIQ_POISON_SCOPOLIA
		|| liquid_num == LIQ_POISON_BELENA
		|| liquid_num == LIQ_POISON_DATURA) {
		return true;
	}
	return false;
}

// * Сет яда на пушку в зависимости от типа жидкости.
void set_weap_poison(ObjData *weapon, int liquid_num) {
	const int poison_timer = 30;
	if (liquid_num == LIQ_POISON_ACONITUM)
		weapon->add_timed_spell(ESpell::kAconitumPoison, poison_timer);
	else if (liquid_num == LIQ_POISON_SCOPOLIA)
		weapon->add_timed_spell(ESpell::kScopolaPoison, poison_timer);
	else if (liquid_num == LIQ_POISON_BELENA)
		weapon->add_timed_spell(ESpell::kBelenaPoison, poison_timer);
	else if (liquid_num == LIQ_POISON_DATURA)
		weapon->add_timed_spell(ESpell::kDaturaPoison, poison_timer);
	else
		log("SYSERROR: liquid_num == %d (%s %s %d)", liquid_num, __FILE__, __func__, __LINE__);
}

// * Вывод имени яда по номеру его заклинания (для осмотра пушек).
std::string GetPoisonName(ESpell spell_id) {
	switch (spell_id) {
		case ESpell::kAconitumPoison: return drinknames[LIQ_POISON_ACONITUM];
		case ESpell::kScopolaPoison: return drinknames[LIQ_POISON_SCOPOLIA];
		case ESpell::kBelenaPoison: return drinknames[LIQ_POISON_BELENA];
		case ESpell::kDaturaPoison: return drinknames[LIQ_POISON_DATURA];
		default: return "";
	}
}

// * Проверка, является ли заклинание ядом.
bool IsSpellPoison(ESpell spell_id) {
	switch (spell_id) {
		case ESpell::kAconitumPoison:
		case ESpell::kScopolaPoison:
		case ESpell::kBelenaPoison:
		case ESpell::kDaturaPoison: return true;
		default: return false;
	}
	//return false;
}

/**
* Пока с AF_SAME_TIME получилось следующее (в бою/вне боя):
* APPLY_POISON - у плеера раз в 2 секунды везде, у моба раз в минуту везде.
* Остальные аффекты - у плеера раз в 2 секунды везде, у моба в бою раз в 2 секунды, вне боя - раз в минуту.
*/
// issue.damage-over-time: the regular-poison per-tick damage amount (GET_POISON based), extracted from
// ProcessPoisonDmg UNCHANGED so the future data-driven <damage source="poison"> tick can reuse the exact
// formula. Mobs take GET_POISON*4 per tick; players GET_POISON*5/30 (a player's affect tick is far more
// frequent than a mob's, hence the /30). GET_POISON is the bearer's accumulated poison level (sum of the
// active kPoison applies), so stacked poisons add up here.
int CalcPoisonDamage(CharData *ch) {
	int poison_dmg = GET_POISON(ch) * (ch->IsNpc() ? 4 : 5);
	if (!ch->IsNpc()) {
		poison_dmg = poison_dmg / 30;
	}
	return poison_dmg;
}


// issue.damage-over-time: ProcessPoisonDmg retired. The poison and aconite DoTs are now data-driven
// <damage source="poison"> tick actions (see CastDamage + CalcPoisonDamage; aconite folds into GET_POISON);
// the affect-update loops no longer call any hardcoded poison-tick function.

void TryDrinkPoison(CharData *ch, ObjData *jar, int amount) {
	// issue.potion-hotfix: a drink poisons when its stored poison LEVEL (kLiquidPoison) is positive --
	// set either by a builder or when the contents spoil (drinkcon::spoil_potion). val[3] no longer
	// carries this. The applied poison scales with both that level and the dose actually drunk.
	const int poison_level = jar->GetPotionValueKey(ObjVal::EValueKey::kLiquidPoison);
	if (poison_level > 0 && !privilege::IsGod(ch)) {
		SendMsgToChar("Что-то вкус какой-то странный!\r\n", ch);
		act("$n поперхнул$u и закашлял$g.", true, ch, 0, 0, kToRoom);
		const int dose = std::max(1, amount);   // объем 0 трактуем как один глоток
		Affect<EApply> af;
		af.duration = CalcDuration(ch, ch, ESkill::kUndefined, dose * 3, 0, 0, 0);
		af.modifier = -2;
		af.location = EApply::kStr;
		af.affect_type = EAffect::kPoisoned;
		af.battleflag = kAfSameTime | kAfCurable;
		ImposeAffect(ch, af, false, false, false, false);
		af.modifier = poison_level * dose;   // poison intensity = potion poison level x dose drunk
		af.location = EApply::kPoison;
		af.affect_type = EAffect::kPoisoned;
		af.battleflag = kAfSameTime | kAfCurable;
		ImposeAffect(ch, af, false, false, false, false);
		// самоотравление (выпил яд): автора нет -- caster_id аффектов остаётся 0
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
