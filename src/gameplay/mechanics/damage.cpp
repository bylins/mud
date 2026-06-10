/**
\file damage.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 05.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "damage.h"
#include "utils/grammar/gender.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/resist.h"

#include <fmt/format.h>

#include "gameplay/mechanics/bonus.h"
#include "engine/entities/char_data.h"
#include "utils/utils_time.h"
#include "engine/db/global_objects.h"
// \TODO Используемые функции из fight.h нужно вынести в механики. Помереть, скажем, можно не только в бою.
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/clans/house_exp.h"
#include "gameplay/statistics/dps.h"
#include "engine/observability/event_sink.h"

#include <chrono>
#include "engine/ui/color.h"
#include "gameplay/core/game_limits.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/affects/affect_handler.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/mechanics/tutelar.h"
#include "gameplay/mechanics/sight.h"
#include "utils/backtrace.h"


void TryRemoveExtrahits(CharData *ch, CharData *victim);

// Estern - нужно разобраться, почему функции работы с опытом распиханы по всем углам
int max_exp_gain_pc(CharData *ch);
//int max_exp_loss_pc(CharData *ch);

bool Damage::CalcMagisShieldsDmgAbsoption(CharData *ch, CharData *victim) {
	if (dam <= 0) {
		return false;
	}

	// отражение части маг дамага от зеркала
	if (AFF_FLAGGED(victim, EAffect::kMagicGlass)
		&& dmg_type == fight::kMagicDmg) {
		int pct = 6;
		if (victim->IsNpc() && !IsCharmice(victim)) {
			pct += 2;
			if (victim->get_role(static_cast<unsigned>(EMobClass::kBoss))) {
				pct += 2;
			}
		}
		// дамаг обратки
		const int mg_damage = dam * pct / 100;
		if (mg_damage > 0
			&& victim->GetEnemy()
			&& victim->GetPosition() > EPosition::kStun
			&& victim->in_room != kNowhere) {
			flags.set(fight::kDrawBriefMagMirror);
			Damage dmg(SpellDmg(ESpell::kMagicGlass), mg_damage, fight::kUndefDmg);
			dmg.flags.set(fight::kNoFleeDmg);
			dmg.flags.set(fight::kMagicReflect);
			dmg.Process(victim, ch);
		}
	}

	// обработка щитов, см Damage::post_init_shields()
	if (flags[fight::kVictimFireShield]
		&& !flags[fight::kCritHit]) {
		if (dmg_type == fight::kPhysDmg
			&& !flags[fight::kIgnoreFireShield]) {
			int pct = 15;
			if (victim->IsNpc() && !IsCharmice(victim)) {
				pct += 5;
				if (victim->get_role(static_cast<unsigned>(EMobClass::kBoss))) {
					pct += 5;
				}
			}
			fs_damage = dam * pct / 100;
		} else {
			act("Огненный щит вокруг $N1 ослабил вашу атаку.",
				false, ch, nullptr, victim, kToChar | kToNoBriefShields);
			act("Огненный щит принял часть повреждений на себя.",
				false, ch, nullptr, victim, kToVict | kToNoBriefShields);
			act("Огненный щит вокруг $N1 ослабил атаку $n1.",
				true, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
		}
		flags.set(fight::kDrawBriefFireShield);
		dam -= (dam * number(30, 50) / 100);
	}

	// если критический удар (не точка и стаб) и есть щит - 95% шанс в молоко
	// критическим считается любой удар который вложиля в определенные границы
	if (dam
		&& flags[fight::kCritHit] && flags[fight::kVictimIceShield]
		&& !dam_critic
		&& spell_id != ESpell::kPoison
		&& number(0, 100) < 94) {
		act("Ваше меткое попадания частично утонуло в ледяной пелене вокруг $N1.",
			false, ch, nullptr, victim, kToChar | kToNoBriefShields);
		act("Меткое попадание частично утонуло в ледяной пелене щита.",
			false, ch, nullptr, victim, kToVict | kToNoBriefShields);
		act("Ледяной щит вокруг $N1 частично поглотил меткое попадание $n1.",
			true, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);

		flags.reset(fight::kCritHit);
		if (dam > 0) dam -= (dam * number(30, 50) / 100);
	}
		//шоб небуло спама модернизировал условие
	else if (dam > 0
		&& flags[fight::kVictimIceShield]
		&& !flags[fight::kCritHit]) {
		flags.set(fight::kDrawBriefIceShield);
		act("Ледяной щит вокруг $N1 смягчил ваш удар.",
			false, ch, nullptr, victim, kToChar | kToNoBriefShields);
		act("Ледяной щит принял часть удара на себя.",
			false, ch, nullptr, victim, kToVict | kToNoBriefShields);
		act("Ледяной щит вокруг $N1 смягчил удар $n1.",
			true, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
		dam -= (dam * number(30, 50) / 100);
	}

	if (dam > 0
		&& flags[fight::kVictimAirShield]
		&& !flags[fight::kCritHit]) {
		flags.set(fight::kDrawBriefAirShield);
		act("Воздушный щит вокруг $N1 ослабил ваш удар.",
			false, ch, nullptr, victim, kToChar | kToNoBriefShields);
		act("Воздушный щит смягчил удар $n1.",
			false, ch, nullptr, victim, kToVict | kToNoBriefShields);
		act("Воздушный щит вокруг $N1 ослабил удар $n1.",
			true, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
		dam -= (dam * number(30, 50) / 100);
	}

	return false;
}

void Damage::CalcArmorDmgAbsorption(CharData *victim) {
	// броня на физ дамаг
	if (dam > 0 && dmg_type == fight::kPhysDmg) {
		DamageEquipment(victim, kNowhere, dam, 50);
		if (!flags[fight::kCritHit] && !flags[fight::kIgnoreArmor]) {
			// 50 брони = 50% снижение дамага
			int max_armour = 50;
			if (CanUseFeat(victim, EFeat::kImpregnable) && victim->IsFlagged(EPrf::kAwake)) {
				// непробиваемый в осторожке - до 75 брони
				max_armour = 75;
			}
			if (CanUseFeat(victim, EFeat::kShadowStrike) && victim->IsFlagged(EPrf::kAwake)) {
				// танцующая тень в осторожке - до 60 брони
				max_armour = 60;
			}
			int tmp_dam = dam * std::max(0, std::min(max_armour, GET_ARMOUR(victim))) / 100;
			// ополовинивание брони по флагу скила
			if (tmp_dam >= 2 && flags[fight::kHalfIgnoreArmor]) {
				tmp_dam /= 2;
			}
			dam -= tmp_dam;
			// крит удар умножает дамаг, если жертва без призмы и без лед.щита
		}
	}
}

/**
 * Обработка поглощения физ урона.
 * \return true - полное поглощение
 */
bool Damage::CalcDmgAbsorption(CharData *ch, CharData *victim) {
	if (dmg_type == fight::kPhysDmg
		&& skill_id < ESkill::kFirst
		&& spell_id < ESpell::kUndefined
		&& dam > 0
		&& GET_ABSORBE(victim) > 0) {
		// шансы поглощения: непробиваемый в осторожке 15%, остальные 10%
		int chance = 10 + GetRealRemort(victim) / 3;
		if (CanUseFeat(victim, EFeat::kImpregnable)
			&& victim->IsFlagged(EPrf::kAwake)) {
			chance += 5;
		}
		// физ урон - прямое вычитание из дамага
		if (number(1, 100) <= chance) {
			dam -= GET_ABSORBE(victim) / 2;
			if (dam <= 0) {
				act("Ваши доспехи полностью поглотили удар $n1.",
					false, ch, nullptr, victim, kToVict);
				act("Доспехи $N1 полностью поглотили ваш удар.",
					false, ch, nullptr, victim, kToChar);
				act("Доспехи $N1 полностью поглотили удар $n1.",
					true, ch, nullptr, victim, kToNotVict | kToArenaListen);
				return true;
			}
		}
	}
	if (dmg_type == fight::kMagicDmg
		&& dam > 0
		&& GET_ABSORBE(victim) > 0
		&& !flags[fight::kIgnoreAbsorbe]) {
// маг урон - по 1% за каждые 2 абсорба, максимум 25% (цифры из mag_damage)
		int absorb = std::min(GET_ABSORBE(victim) / 2, 25);
		dam -= dam * absorb / 100;
	}
	return false;
}

void Damage::SendCritHitMsg(CharData *ch, CharData *victim) {
	// Блочить мессагу крита при ледяном щите вроде нелогично,
	// так что добавил отдельные сообщения для ледяного щита (Купала)
	if (!flags[fight::kVictimIceShield]) {
		sprintf(buf, "&G&qВаше меткое попадание тяжело ранило %s.&Q&n\r\n",
				PersonName(victim, ch, 3));
	} else {
		sprintf(buf, "&B&qВаше меткое попадание утонуло в ледяной пелене щита %s.&Q&n\r\n",
				PersonName(victim, ch, 1));
	}

	SendMsgToChar(buf, ch);

	if (!flags[fight::kVictimIceShield]) {
		sprintf(buf, "&r&qМеткое попадание %s тяжело ранило вас.&Q&n\r\n",
				PersonName(ch, victim, 1));
	} else {
		sprintf(buf, "&r&qМеткое попадание %s утонуло в ледяной пелене вашего щита.&Q&n\r\n",
				PersonName(ch, victim, 1));
	}

	SendMsgToChar(buf, victim);
	// Закомментил чтобы не спамило, сделать потом в виде режима
	//act("Меткое попадание $N1 заставило $n3 пошатнуться.", true, victim, nullptr, ch, TO_NOTVICT);
}

void Damage::ProcessBlink(CharData *ch, CharData *victim) {
	if (flags[fight::kIgnoreBlink] || flags[fight::kCritLuck])
		return;
	ubyte blink = 0;
	// даже в случае попадания можно уклониться мигалкой
	if (dmg_type == fight::kMagicDmg) {
		if (AFF_FLAGGED(victim, EAffect::kCloudly) || victim->add_abils.percent_spell_blink_mag > 0) {
			if (victim->IsNpc()) {
				blink = GetRealLevel(victim) + GetRealRemort(victim);
			} else if(victim->add_abils.percent_spell_blink_mag > 0) {
				blink = victim->add_abils.percent_spell_blink_mag;
			} else {
				blink = 10;
			}
		}
	} else if(dmg_type == fight::kPhysDmg) {
		if (AFF_FLAGGED(victim, EAffect::kBlink) || victim->add_abils.percent_spell_blink_phys > 0) {
			if (victim->IsNpc()) {
				blink = GetRealLevel(victim) + GetRealRemort(victim);
			} else if (victim->add_abils.percent_spell_blink_phys > 0) {
				blink = victim->add_abils.percent_spell_blink_phys;
			} else {
				blink = 10;
			}
		}
	}
	if(blink < 1)
		return;
//	ch->send_to_TC(false, true, false, "Шанс мигалки равен == %d процентов.\r\n", blink);
//	victim->send_to_TC(false, true, false, "Шанс мигалки равен == %d процентов.\r\n", blink);
	int bottom = 1;
	if (ch->calc_morale() > number(1, 100)) // удача
		bottom = 10;
	if (number(bottom, blink) >= number(1, 100)) {
		sprintf(buf, "%sНа мгновение вы исчезли из поля зрения противника.%s\r\n",
				kColorBoldBlk, kColorNrm);
		SendMsgToChar(buf, victim);
		act("$n исчез$q из вашего поля зрения.", true, victim, nullptr, ch, kToVict);
		act("$n исчез$q из поля зрения $N1.", true, victim, nullptr, ch, kToNotVict);
		dam = 0;
		fs_damage = 0;
		return;
	}
}

void Damage::ProcessDeath(CharData *ch, CharData *victim) const {
	CharData *killer = nullptr;

	if (victim->IsNpc() || victim->desc) {
		if (victim == ch && victim->in_room != kNowhere) {
			// Урон сам себе (тик повреждающего аффекта и т.п.): убийство засчитывается "автору"
			// урона (author_uid -- обычно caster_id аффекта), если он рядом. Нет автора (0) или
			// автор сам victim -- не засчитывается. Любой будущий повреждающий аффект просто
			// выставляет author_uid, отдельная ветка тут не нужна.
			if (author_uid != 0 && author_uid != victim->get_uid()) {
				for (const auto author : world[victim->in_room]->people) {
					if (author != victim && author->get_uid() == author_uid) {
						killer = author;
						break;
					}
				}
			} else if (damage_source == fight::EDamageSource::kSuffering) {
				for (const auto attacker : world[victim->in_room]->people) {
					if (attacker->GetEnemy() == victim) {
						killer = attacker;
					}
				}
			}
		}

		if (ch != victim) {
			killer = ch;
		}
	}
	if (killer) {
		if (AFF_FLAGGED(killer, EAffect::kGroup)) {
			// т.к. помечен флагом AFF_GROUP - точно PC
			group_gain(killer, victim);
		} else if ((AFF_FLAGGED(killer, EAffect::kCharmed)
			|| killer->IsFlagged(EMobFlag::kTutelar)
			|| killer->IsFlagged(EMobFlag::kMentalShadow))
			&& killer->has_master())
			// killer - зачармленный NPC с хозяином
		{
			// по логике надо бы сделать, что если хозяина нет в клетке, но
			// кто-то из группы хозяина в клетке, то опыт накинуть согруппам,
			// которые рядом с убившим моба чармисом.
			if (AFF_FLAGGED(killer->get_master(), EAffect::kGroup)
				&& killer->in_room == killer->get_master()->in_room) {
				// Хозяин - PC в группе => опыт группе
				group_gain(killer->get_master(), victim);
			} else if (killer->in_room == killer->get_master()->in_room) {
				perform_group_gain(killer->get_master(), victim, 1, 100);
			}
			// else
			// А хозяина то рядом не оказалось, все чармису - убрано
			// нефиг абьюзить чарм  group::perform_group_gain( killer, victim, 1, 100 );
		} else {
			// Просто NPC или PC сам по себе
			perform_group_gain(killer, victim, 1, 100);
		}
	}

	// в сислог иммам идут только смерти в пк (без арен)
	// в файл пишутся все смерти чаров
	// если чар убит палачем то тоже не спамим
	if (!victim->IsNpc() && !(killer && killer->IsFlagged(EPrf::kExecutor))) {
		UpdatePkLogs(ch, victim);

		for (const auto &ch_vict : world[ch->in_room]->people) {
			//Мобы все кто присутствовал при смерти игрока забывают
			if (ch_vict->IsImmortal())
				continue;
			if (!HERE(ch_vict))
				continue;
			if (!ch_vict->IsNpc())
				continue;
			if (ch_vict->IsFlagged(EMobFlag::kMemory)) {
				mob_ai::mobForget(ch_vict, victim);
			}
		}

	}

	if (killer) {
		ch = killer;
	}
	die(victim, ch);
}

/**
 * Разный инит щитов у мобов и чаров.
 * У мобов работают все 3 щита, у чаров только 1 рандомный на текущий удар.
 */
void Damage::SetPostInitShieldFlags(CharData *victim) {
	if (victim->IsNpc() && !IsCharmice(victim)) {
		if (AFF_FLAGGED(victim, EAffect::kFireShield)) {
			flags.set(fight::kVictimFireShield);
		}

		if (AFF_FLAGGED(victim, EAffect::kIceShield)) {
			flags.set(fight::kVictimIceShield);
		}

		if (AFF_FLAGGED(victim, EAffect::kAirShield)) {
			flags.set(fight::kVictimAirShield);
		}
	} else {
		enum { FIRESHIELD, ICESHIELD, AIRSHIELD };
		std::vector<int> shields;

		if (AFF_FLAGGED(victim, EAffect::kFireShield)) {
			shields.push_back(FIRESHIELD);
		}

		if (AFF_FLAGGED(victim, EAffect::kAirShield)) {
			shields.push_back(AIRSHIELD);
		}

		if (AFF_FLAGGED(victim, EAffect::kIceShield)) {
			shields.push_back(ICESHIELD);
		}

		if (shields.empty()) {
			return;
		}

		int shield_num = number(0, static_cast<int>(shields.size() - 1));

		if (shields[shield_num] == FIRESHIELD) {
			flags.set(fight::kVictimFireShield);
		} else if (shields[shield_num] == AIRSHIELD) {
			flags.set(fight::kVictimAirShield);
		} else if (shields[shield_num] == ICESHIELD) {
			flags.set(fight::kVictimIceShield);
		}
	}
}

void Damage::PerformPostInit(CharData *ch, CharData *victim) {
	if (ch_start_pos == EPosition::kUndefined) {
		ch_start_pos = ch->GetPosition();
	}

	if (victim_start_pos == EPosition::kUndefined) {
		victim_start_pos = victim->GetPosition();
	}

	SetPostInitShieldFlags(victim);
}

// обработка щитов, зб, поглощения, сообщения для огн. щита НЕ ЗДЕСЬ
// возвращает сделанный дамаг
int Damage::Process(CharData *ch, CharData *victim) {
	PerformPostInit(ch, victim);
	if (victim->in_room == kNowhere || ch->in_room == kNowhere || ch->in_room != victim->in_room) {
		log("SYSERR: Attempt to damage '%s' in room kNowhere by '%s'.",
			GET_NAME(victim), GET_NAME(ch));
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		return 0;
	}
	if (victim->purged()) { //будем мониторить коредамп
		log("SYSERR: Attempt to damage purged char/mob '%s' in room #%d by '%s'.",
			GET_NAME(victim), GET_ROOM_VNUM(victim->in_room), GET_NAME(ch));
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		return 0;
	}
	if (!check_valid_chars(ch, victim, __FILE__, __LINE__)) {
		log("SYSERR: Attempt to damage purged char/mob ch or vict");
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		return 0;
	}
	if (victim->GetPosition() <= EPosition::kDead) {
		log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
			GET_NAME(victim), GET_ROOM_VNUM(victim->in_room), GET_NAME(ch));
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		die(victim, nullptr);
		return 0;
	}
	if ((ch->IsNpc() && ch->IsFlagged(EMobFlag::kNoFight))
		|| (victim->IsNpc() && victim->IsFlagged(EMobFlag::kNoFight))) {
		return 0;
	}
	if (dam > 0) {
		if (victim->IsGod()) {
			dam = 0;
		} else if (victim->IsImmortal() || GET_GOD_FLAG(victim, EGf::kGodsLike)) {
			dam /= 4;
		} else if (GET_GOD_FLAG(victim, EGf::kGodscurse)) {
			dam *= 2;
		}
	}

	mob_ai::update_mob_memory(ch, victim);

	// If you attack a pet, it hates your guts
	if (!group::same_group(ch, victim)) {
		check_agro_follower(ch, victim);
	}
	if (victim != ch) {
		if (ch->GetPosition() > EPosition::kStun && (ch->GetEnemy() == nullptr)) {
			if (!pk_agro_action(ch, victim)) {
				return 0;
			}
			SetFighting(ch, victim);
			npc_groupbattle(ch);
		}
		// Start the victim fighting the attacker
		if (victim->GetPosition() > EPosition::kDead && (victim->GetEnemy() == nullptr)) {
			SetFighting(victim, ch);
			npc_groupbattle(victim);
		}

		// лошадь сбрасывает седока при уроне
		if (mount::IsOnHorse(ch) && mount::GetHorse(ch) == victim) {
			mount::DropFromHorse(victim);
		} else if (mount::IsOnHorse(victim) && mount::GetHorse(victim) == ch) {
			mount::DropFromHorse(ch);
		}
	}
	Appear(ch);
	Appear(victim);

	if (dam < 0 || ch->in_room == kNowhere || victim->in_room == kNowhere || ch->in_room != victim->in_room) {
		return 0;
	}

	if (ch->GetPosition() <= EPosition::kIncap) {
		return 0;
	}

	if (dam >= 2) {
		if (AFF_FLAGGED(victim, EAffect::kPrismaticAura) && !flags[fight::kIgnorePrism]) {
			if (dmg_type == fight::kPhysDmg) {
				dam *= 2;
			} else if (dmg_type == fight::kMagicDmg) {
				dam /= 2;
			}
		}
		if (AFF_FLAGGED(victim, EAffect::kSanctuary) && !flags[fight::kIgnoreSanct]) {
			if (dmg_type == fight::kPhysDmg) {
				dam /= 2;
			} else if (dmg_type == fight::kMagicDmg) {
				dam *= 2;
			}
		}

		if (victim->IsNpc() && Bonus::is_bonus_active(Bonus::EBonusType::BONUS_DAMAGE)) {
			dam *= Bonus::get_mult_bonus();
		}
	}
	//учет резистов для магического урона
	if (dmg_type == fight::kMagicDmg) {
		if (spell_id > ESpell::kUndefined) {
			dam = ApplyResist(victim, GetResistType(spell_id), dam);
		} else {
			dam = ApplyResist(victim, GetResisTypeWithElement(element), dam);
		};
	};

	// учет положения атакующего и жертвы
	// Include a damage multiplier if victim isn't ready to fight:
	// Position sitting  1.5 x normal
	// Position resting  2.0 x normal
	// Position sleeping 2.5 x normal
	// Position stunned  3.0 x normal
	// Position incap    3.5 x normal
	// Position mortally 4.0 x normal
	// Note, this is a hack because it depends on the particular
	// values of the POSITION_XXX constants.

	// физ дамага атакера из сидячего положения
	if (ch_start_pos < EPosition::kFight && dmg_type == fight::kPhysDmg) {
		dam -= dam * (EPosition::kFight - ch_start_pos) / 4;
	}

	// дамаг не увеличивается если:
	// на жертве есть воздушный щит
	// атака - каст моба (в mage_damage увеличение дамага от позиции было только у колдунов)
	if (victim_start_pos < EPosition::kFight
		&& !flags[fight::kVictimAirShield]
		&& !(dmg_type == fight::kMagicDmg
			&& ch->IsNpc())) {
		dam += dam * (EPosition::kFight - victim_start_pos) / 4;
	}

	// прочие множители

	if (AFF_FLAGGED(victim, EAffect::kHold) && dmg_type == fight::kPhysDmg) {
		if (ch->IsNpc() && !IsCharmice(ch)) {
			dam = dam * 15 / 10;
		} else {
			dam = dam * 125 / 100;
		}
	}

	if (!victim->IsNpc() && IsCharmice(ch)) {
		dam = dam * 8 / 10;
	}

	if (GET_PR(victim) && dmg_type == fight::kPhysDmg) {
		int ResultDam = dam - (dam * GET_PR(victim) / 100);
		ch->send_to_TC(false, true, false,
					   "&CУчет поглощения урона: %d начислено, %d применено.&n\r\n", dam, ResultDam);
		victim->send_to_TC(false, true, false,
						   "&CУчет поглощения урона: %d начислено, %d применено.&n\r\n", dam, ResultDam);
		dam = ResultDam;
	}
	if (!ch->IsImmortal() && AFF_FLAGGED(victim, EAffect::kGodsShield)) {
		if (skill_id == ESkill::kBash) {
			SendSkillMessages(dam, ch, victim, skill_id);
		}
		if (ch != victim) {
			act("Магический кокон полностью поглотил удар $N1.", false, victim, nullptr, ch, kToChar);
			act("Магический кокон вокруг $N1 полностью поглотил ваш удар.", false, ch, nullptr, victim, kToChar);
			act("Магический кокон вокруг $N1 полностью поглотил удар $n1.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
		} else {
			act("Магический кокон полностью поглотил повреждения.", false, ch, nullptr, nullptr, kToChar);
			act("Магический кокон вокруг $N1 полностью поглотил повреждения.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
		}
		return 0;
	}
	// щиты, броня, поглощение
	if (victim != ch) {
		bool shield_full_absorb = CalcMagisShieldsDmgAbsoption(ch, victim);
		CalcArmorDmgAbsorption(victim);
		bool armor_full_absorb = CalcDmgAbsorption(ch, victim);
		if (flags[fight::kCritHit] && (GetRealLevel(victim) >= 5 || !ch->IsNpc())
			&& !AFF_FLAGGED(victim, EAffect::kPrismaticAura)
			&& !flags[fight::kVictimIceShield]) {
			int tmpdam = std::min(victim->get_real_max_hit() / 8, dam * 2);
			tmpdam = ApplyResist(victim, EResist::kVitality, dam);
			dam = std::max(dam, tmpdam); //крит
		}
		// полное поглощение
		if (shield_full_absorb || armor_full_absorb) {
			return 0;
		}
		if (dam > 0)
			ProcessBlink(ch, victim);
	}

	// Внутри magic_shields_dam вызывается dmg::proccess, если чар там умрет, то будет креш
	if (!(ch && victim) || (ch->purged() || victim->purged())) {
		log("Death from CalcMagisShieldsDmgAbsoption");
		return 0;
	}

	if (victim->IsFlagged(EMobFlag::kProtect)) {
		if (victim != ch) {
			act("$n находится под защитой Богов.", false, victim, nullptr, nullptr, kToRoom);
		}
		return 0;
	}
	// внутри есть !боевое везение!, для какого типа дамага - не знаю
	DamageActorParameters params(ch, victim, dam);
	handle_affects(params);
	dam = params.damage;
	DamageVictimParameters params1(ch, victim, dam);
	handle_affects(params1);
	dam = params1.damage;

	// обратка от зеркал/огненного щита
	if (flags[fight::kMagicReflect]) {
		// ограничение для зеркал на 40% от макс хп кастера
		dam = std::min(dam, victim->get_max_hit() * 4 / 10);
		// чтобы не убивало обраткой
		dam = std::min(dam, victim->get_hit() - 1);
	}

	dam = std::clamp(dam, 0, kMaxHits);
	if (dam >= 0) {
		if (dmg_type == fight::kPhysDmg) {
			if (!damage_mtrigger(ch, victim, dam, MUD::Skill(skill_id).GetName(), 1, wielded))
				return 0;
		} else if (dmg_type == fight::kMagicDmg) {
			// spell_id is kUndefined for spell-less magic damage (e.g. mob breath, which
			// is magic melee of an element). Use an empty name rather than looking up an
			// invalid spell.
			const char *dmg_name = (spell_id > ESpell::kUndefined) ? MUD::Spell(spell_id).GetCName() : "";
			if (!damage_mtrigger(ch, victim, dam, dmg_name, 0, wielded))
				return 0;
		} else if (dmg_type == fight::kPoisonDmg) {
			if (!damage_mtrigger(ch, victim, dam, MUD::Spell(spell_id).GetCName(), 2, wielded))
				return 0;
		}
	}
	if (!InTestZone(ch)) {
		gain_battle_exp(ch, victim, dam);
	}

	// real_dam так же идет в обратку от огн.щита
	int real_dam = dam;
	int over_dam = 0;

	if (dam > victim->get_hit() + 11) {
		real_dam = victim->get_hit() + 11;
		over_dam = dam - real_dam;
	}
	// собственно нанесение дамага
	victim->set_hit(victim->get_hit() - dam);
	// Точка инструментации для автономного симулятора баланса (issue #2967):
	// эмитим точные числа на каждый успешный удар. В проде список sink'ов
	// пуст -- HasAnyEventSink() возвращает false, ранний выход без построения
	// Event и без аллокаций.
	if (observability::HasAnyEventSink()) {
		observability::Event ev;
		ev.name = "damage";
		ev.ts_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		ev.attrs["attacker_name"] = observability::EngineStringToUtf8(GET_NAME(ch) ? GET_NAME(ch) : "");
		ev.attrs["victim_name"] = observability::EngineStringToUtf8(GET_NAME(victim) ? GET_NAME(victim) : "");
		ev.attrs["dam"] = static_cast<std::int64_t>(dam);
		ev.attrs["real_dam"] = static_cast<std::int64_t>(real_dam);
		ev.attrs["over_dam"] = static_cast<std::int64_t>(over_dam);
		ev.attrs["victim_hp_after"] = static_cast<std::int64_t>(victim->get_hit());
		ev.attrs["dmg_type"] = static_cast<std::int64_t>(dmg_type);
		ev.attrs["crit"] = flags[fight::kCritHit];
		// spell_id/skill_id выставлены если урон пришёл из заклинания/умения
		// (kUndefined для обычной автоатаки). В виз позволяет отличить
		// melee от cast.
		ev.attrs["spell_id"] = static_cast<std::int64_t>(spell_id);
		ev.attrs["skill_id"] = static_cast<std::int64_t>(skill_id);
		// Чармис/поднятая нежить -- атаковал не сам PC, а его подчинённый.
		// Визуализатору это нужно, чтобы отделить вклад хозяина и слуг.
		ev.attrs["attacker_is_charmie"] = IsCharmice(ch);
		ev.attrs["attacker_master_name"] = observability::EngineStringToUtf8(
			(IsCharmice(ch) && ch->has_master() && GET_NAME(ch->get_master()))
				? GET_NAME(ch->get_master()) : "");
		observability::EmitToAllSinks(ev);
	}
	victim->send_to_TC(false, true, true, "&MПолучен урон = %d&n\r\n", dam);
	ch->send_to_TC(false, true, true, "&MПрименен урон = %d&n\r\n", dam);
	if (dmg_type == fight::kPhysDmg && GET_GOD_FLAG(ch, EGf::kSkillTester) && skill_id != ESkill::kUndefined) {
		log("SKILLTEST:;%s;skill;%s;damage;%d;Luck;%s", GET_NAME(ch), MUD::Skill(skill_id).GetName(), dam, flags[fight::kCritLuck] ? "yes" : "no");
	}
	// если на чармисе вампир
	if (AFF_FLAGGED(ch, EAffect::kVampirism)) {
		ch->set_hit(std::clamp(ch->get_hit() + std::max(1, dam / 10),
							   ch->get_hit(), ch->get_real_max_hit() + ch->get_real_max_hit() * GetRealLevel(ch) / 10));
		// если есть родство душ, то чару отходит по 5% от дамаги к хп
		if (ch->has_master()) {
			if (CanUseFeat(ch->get_master(), EFeat::kSoulLink)) {
				ch->get_master()->set_hit(std::max(ch->get_master()->get_hit(),
												   std::min(ch->get_master()->get_hit() + std::max(1, dam / 20 ),
															ch->get_master()->get_real_max_hit() +
																ch->get_master()->get_real_max_hit() *
																	GetRealLevel(ch->get_master()) / 10)));
			}
		}
	}
	// запись в дметр фактического и овер дамага
	DpsSystem::UpdateDpsStatistics(ch, real_dam, over_dam);
	// запись дамага в список атакеров
	if (victim->IsNpc()) {
		victim->add_attacker(ch, ATTACKER_DAMAGE, real_dam);
	}
	// попытка спасти жертву через ангела
	CheckTutelarSelfSacrfice(ch, victim);

	// обновление позиции после удара и ангела
	update_pos(victim);
	// если вдруг виктим сдох после этого, то произойдет креш, поэтому вставил тут проверочку
	if (!(ch && victim) || (ch->purged() || victim->purged())) {
		log("Error in fight_hit, function process()\r\n");
		return 0;
	}
	// если у чара есть жатва жизни
	if (CanUseFeat(victim, EFeat::kHarvestOfLife)) {
		if (victim->GetPosition() == EPosition::kDead) {
			int souls = victim->get_souls();
			if (souls >= 10) {
				victim->set_hit(0 + souls * 10);
				update_pos(victim);
				SendMsgToChar("&GДуши спасли вас от смерти!&n\r\n", victim);
				victim->set_souls(0);
			}
		}
	}
	// сбивание надува черноков //
	if (spell_id != ESpell::kPoison && dam > 0 && !flags[fight::kMagicReflect]) {
		TryRemoveExtrahits(ch, victim);
	}

	if (dam && flags[fight::kCritHit] && !dam_critic && spell_id != ESpell::kPoison) {
		SendCritHitMsg(ch, victim);
	}

	// инит Damage::brief_shields_
	if (flags.test(fight::kDrawBriefFireShield)
		|| flags.test(fight::kDrawBriefAirShield)
		|| flags.test(fight::kDrawBriefIceShield)
		|| flags.test(fight::kDrawBriefMagMirror)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "&n (%s%s%s%s)",
				 flags.test(fight::kDrawBriefFireShield) ? "&R*&n" : "",
				 flags.test(fight::kDrawBriefAirShield) ? "&C*&n" : "",
				 flags.test(fight::kDrawBriefIceShield) ? "&W*&n" : "",
				 flags.test(fight::kDrawBriefMagMirror) ? "&M*&n" : "");
		brief_shields_ = buf_;
	}
	// сообщения об ударах //
	if (MUD::Skills().IsValid(skill_id)) {
		SendSkillMessages(dam, ch, victim, skill_id, brief_shields_);
	} else if (spell_id > ESpell::kUndefined) {
		SendSkillMessages(dam, ch, victim, spell_id, brief_shields_);
	} else {
		// удар оружием/рукой или серверный урон - всё из контейнера сообщений
		// (для ударов оружием kFightHit* содержит плейсхолдер {intensity}, issue #3322)
		SendSkillMessages(dam, ch, victim, damage_source, brief_shields_);
	}
	/// Use SendMsgToChar -- act() doesn't send message if you are DEAD.
	char_dam_message(dam, ch, victim, flags[fight::kNoFleeDmg]);


	// Проверить, что жертва все еще тут. Может уже сбежала по трусости.
	// Думаю, простой проверки достаточно.
	// Примечание, если сбежал в FIRESHIELD,
	// то обратного повреждения по атакующему не будет
	if (ch->in_room != victim->in_room) {
		return dam;
	}

	// Stop someone from fighting if they're stunned or worse
	/*if ((victim->GetPosition() <= EPosition::kStun)
		&& (victim->GetEnemy() != NULL))
	{
		stop_fighting(victim, victim->GetPosition() <= EPosition::kDead);
	} */

	// жертва умирает //
	if (victim->GetPosition() == EPosition::kDead) {
		ProcessDeath(ch, victim);
		return -1;
	}
	// обратка от огненного щита
	if (fs_damage > 0
		&& victim->GetEnemy()
		&& victim->GetPosition() > EPosition::kStun
		&& victim->in_room != kNowhere) {
		Damage dmg(SpellDmg(ESpell::kFireShield), fs_damage, fight::kUndefDmg);
		dmg.flags.set(fight::kNoFleeDmg);
		dmg.flags.set(fight::kMagicReflect);
		dmg.Process(victim, ch);
	}
	return dam;
}

// * При надуве выше х 1.5 в пк есть 1% того, что весь надув слетит одним ударом.
void TryRemoveExtrahits(CharData *ch, CharData *victim) {
	if (((!ch->IsNpc() && ch != victim)
		|| (ch->has_master()
			&& !ch->get_master()->IsNpc()
			&& ch->get_master() != victim))
		&& !victim->IsNpc()
		&& victim->GetPosition() != EPosition::kDead
		&& victim->get_hit() > victim->get_real_max_hit() * 1.5
		&& number(1, 100) == 5)// пусть будет 5, а то 1 по субъективным ощущениям выпадает как-то часто
	{
		victim->set_hit(victim->get_real_max_hit());
		SendMsgToChar(victim, "%s'Будь%s тощ%s аки прежде' - мелькнула чужая мысль в вашей голове.%s\r\n",
					  kColorWht, grammar::PluralVerbEnding(IS_POLY(victim)), grammar::InstrEnding((victim)->get_sex()), kColorNrm);
		act("Вы прервали золотистую нить, питающую $N3 жизнью.", false, ch, nullptr, victim, kToChar);
		act("$n прервал$g золотистую нить, питающую $N3 жизнью.", false, ch, nullptr, victim, kToNotVict | kToArenaListen);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
