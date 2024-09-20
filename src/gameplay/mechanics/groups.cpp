/**
\file groups.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Код групп игроков и мобов.
\detail Группы, вступление, покидание, дележка опыта - должно быть тут.
*/

#include "gameplay/mechanics/groups.h"

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/magic/magic.h"
#include "engine/network/msdp/msdp_constants.h"
#include "gameplay/clans/house.h"
#include "gameplay/economics/currencies.h"

#include <third_party_libs/fmt/include/fmt/format.h>

#include <ranges>

bool same_group(CharData *ch, CharData *tch) {
	if (!ch || !tch)
		return false;

	// Добавлены проверки чтобы не любой заследовавшийся моб считался согруппником (Купала)
	if (ch->IsNpc()
		&& ch->has_master()
		&& !ch->get_master()->IsNpc()
		&& (IS_HORSE(ch)
			|| AFF_FLAGGED(ch, EAffect::kCharmed)
			|| ch->IsFlagged(EMobFlag::kTutelar)
			|| ch->IsFlagged(EMobFlag::kMentalShadow))) {
		ch = ch->get_master();
	}

	if (tch->IsNpc()
		&& tch->has_master()
		&& !tch->get_master()->IsNpc()
		&& (IS_HORSE(tch)
			|| AFF_FLAGGED(tch, EAffect::kCharmed)
			|| tch->IsFlagged(EMobFlag::kTutelar)
			|| tch->IsFlagged(EMobFlag::kMentalShadow))) {
		tch = tch->get_master();
	}

	// NPC's always in same group
	if ((ch->IsNpc() && tch->IsNpc())
		|| ch == tch) {
		return true;
	}

	if (!AFF_FLAGGED(ch, EAffect::kGroup)
		|| !AFF_FLAGGED(tch, EAffect::kGroup)) {
		return false;
	}

	if (ch->get_master() == tch
		|| tch->get_master() == ch
		|| (ch->has_master()
			&& ch->get_master() == tch->get_master())) {
		return true;
	}

	return false;
}

int max_group_size(CharData *ch) {
	int bonus_commander = 0;
//	if (AFF_FLAGGED(ch, EAffectFlag::AFF_COMMANDER))
//		bonus_commander = VPOSI((ch->get_skill(ESkill::kLeadership) - 120) / 10, 0, 8);
	bonus_commander = VPOSI((ch->GetSkill(ESkill::kLeadership) - 200) / 8, 0, 8);
	return kMaxGroupedFollowers + (int) VPOSI((ch->GetSkill(ESkill::kLeadership) - 80) / 5, 0, 4) + bonus_commander;
}

bool is_group_member(CharData *ch, CharData *vict) {
	if (vict->IsNpc()
		|| !AFF_FLAGGED(vict, EAffect::kGroup)
		|| vict->get_master() != ch) {
		return false;
	} else {
		return true;
	}
}

int perform_group(CharData *ch, CharData *vict) {
	if (AFF_FLAGGED(vict, EAffect::kGroup)
		|| AFF_FLAGGED(vict, EAffect::kCharmed)
		|| vict->IsFlagged(EMobFlag::kTutelar)
		|| vict->IsFlagged(EMobFlag::kMentalShadow)
		|| IS_HORSE(vict)) {
		return (false);
	}

	AFF_FLAGS(vict).set(EAffect::kGroup);
	if (ch != vict) {
		act("$N принят$A в члены вашего кружка (тьфу-ты, группы :).", false, ch, nullptr, vict, kToChar);
		act("Вы приняты в группу $n1.", false, ch, nullptr, vict, kToVict);
		act("$N принят$A в группу $n1.", false, ch, nullptr, vict, kToNotVict | kToArenaListen);
	}
	return (true);
}

/**
* Смена лидера группы на персонажа с макс лидеркой.
* Сам лидер при этом остается в группе, если он живой.
* \param vict - новый лидер, если смена происходит по команде 'гр лидер имя',
* старый лидер соответственно группится обратно, если нулевой, то считаем, что
* произошла смерть старого лидера и новый выбирается по наибольшей лидерке.
*/
void change_leader(CharData *ch, CharData *vict) {
	if (ch->IsNpc()
		|| ch->has_master()
		|| !ch->followers) {
		return;
	}

	CharData *leader = vict;
	if (!leader) {
		// лидер умер, ищем согрупника с максимальным скиллом лидерки
		for (struct FollowerType *l = ch->followers; l; l = l->next) {
			if (!is_group_member(ch, l->follower))
				continue;
			if (!leader)
				leader = l->follower;
			else if (l->follower->GetSkill(ESkill::kLeadership) > leader->GetSkill(ESkill::kLeadership))
				leader = l->follower;
		}
	}

	if (!leader) {
		return;
	}

	// для реследования используем стандартные функции
	std::vector<CharData *> temp_list;
	for (struct FollowerType *n = nullptr, *l = ch->followers; l; l = n) {
		n = l->next;
		if (!is_group_member(ch, l->follower)) {
			continue;
		} else {
			CharData *temp_vict = l->follower;
			if (temp_vict->has_master() && stop_follower(temp_vict, kSfSilence)) {
				continue;
			}

			if (temp_vict != leader) {
				temp_list.push_back(temp_vict);
			}
		}
	}

	if (!temp_list.empty()) {
		for (auto & it : std::ranges::reverse_view(temp_list)) {
			leader->add_follower_silently(it);
		}
	}

	// бывшего лидера последним закидываем обратно в группу, если он живой
	if (vict) {
		// флаг группы надо снять, иначе при регрупе не будет писаться о старом лидере
		//AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
		ch->removeGroupFlags();
		leader->add_follower_silently(ch);
	}

	if (!leader->followers) {
		return;
	}

	ch->dps_copy(leader);
	perform_group(leader, leader);
	int followers = 0;
	for (struct FollowerType *f = leader->followers; f; f = f->next) {
		if (followers < max_group_size(leader)) {
			if (perform_group(leader, f->follower))
				++followers;
		} else {
			SendMsgToChar("Вы больше никого не можете принять в группу.\r\n", ch);
			return;
		}
	}
}

void print_one_line(CharData *ch, CharData *k, int leader, int header) {
	const char *WORD_STATE[] = {"При смерти",
								"Оч.тяж.ран",
								"Оч.тяж.ран",
								" Тяж.ранен",
								" Тяж.ранен",
								"  Ранен   ",
								"  Ранен   ",
								"  Ранен   ",
								"Лег.ранен ",
								"Лег.ранен ",
								"Слег.ранен",
								" Невредим "
	};
	const char *MOVE_STATE[] = {"Истощен",
								"Истощен",
								"О.устал",
								" Устал ",
								" Устал ",
								"Л.устал",
								"Л.устал",
								"Хорошо ",
								"Хорошо ",
								"Хорошо ",
								"Отдохн.",
								" Полон "
	};
	const char *POS_STATE[] = {"Умер",
							   "Истекает кровью",
							   "При смерти",
							   "Без сознания",
							   "Спит",
							   "Отдыхает",
							   "Сидит",
							   "Сражается",
							   "Стоит"
	};

	auto generate_affects_string = [](CharData *k) -> std::string {
	  std::string affects;
	  affects += AFF_FLAGGED(k, EAffect::kSanctuary) ? "&RО" : (AFF_FLAGGED(k, EAffect::kPrismaticAura) ? "&RП" : " ");
	  affects += AFF_FLAGGED(k, EAffect::kWaterBreath) ? "&gД" : " ";
	  affects += AFF_FLAGGED(k, EAffect::kInvisible) ? "&CН" : " ";
	  affects += (AFF_FLAGGED(k, EAffect::kSingleLight)
		  || AFF_FLAGGED(k, EAffect::kHolyLight)
		  || (GET_EQ(k, EEquipPos::kLight)
			  && GET_OBJ_VAL(GET_EQ(k, EEquipPos::kLight), 2))) ? "&YС" : " ";
	  affects += AFF_FLAGGED(k, EAffect::kFly) ? "&BЛ" : " ";

	  return affects;
	};

	auto generate_debuf_string = [](CharData *k) -> std::string {
	  std::string debuf;
	  debuf += AFF_FLAGGED(k, EAffect::kHold) ? "&RХ" : " ";
	  debuf += AFF_FLAGGED(k, EAffect::kSleep) ? "&BС" : " ";
	  debuf += AFF_FLAGGED(k, EAffect::kSilence) ? "&yМ" : " ";
	  debuf += AFF_FLAGGED(k, EAffect::kDeafness) ? "&gГ" : " ";
	  debuf += AFF_FLAGGED(k, EAffect::kBlind) ? "&YБ" : " ";
	  debuf += AFF_FLAGGED(k, EAffect::kCurse) ? "&mП" : " ";
	  debuf += IsAffectedBySpell(k, ESpell::kFever) ? "&cЛ" : " ";

	  return debuf;
	};

	auto generate_mem_string = [](CharData *k) -> std::string {
	  int ok, ok2, div;
	  if ((!IS_MANA_CASTER(k) && !k->mem_queue.Empty()) ||
		  (IS_MANA_CASTER(k) && k->mem_queue.stored < GET_MAX_MANA(k))) {
		  div = CalcManaGain(k);
		  if (div > 0) {
			  if (!IS_MANA_CASTER(k)) {
				  ok2 = std::max(0, 1 + k->mem_queue.total - k->mem_queue.stored);
				  ok2 = ok2 * 60 / div;    // время мема в сек
			  } else {
				  ok2 = std::max(0, 1 + GET_MAX_MANA(k) - k->mem_queue.stored);
				  ok2 = ok2 / div;    // время восстановления в секундах
			  }
			  ok = ok2 / 60;
			  ok2 %= 60;
			  if (ok > 99)
				  return fmt::format("&g{:02}", ok);
			  else
				  return fmt::format("&g{:02}:{:02}", ok, ok2);
		  } else {
			  return "&r  -  ";
		  }
	  } else
		  return " ";
	};


	if (k->IsNpc()) {
		std::ostringstream buffer;
		if (!header)
			buffer << "Персонаж            | Здоровье | Рядом | Аффект |  Дебаф  | Положение\r\n";

		buffer << fmt::format("&B{:<20}&n|", k->get_name().substr(0, 20));

		buffer << fmt::format("{}", GetWarmValueColor(GET_HIT(k), GET_REAL_MAX_HIT(k)));
		buffer << fmt::format("{:<10}&n|", WORD_STATE[posi_value(GET_HIT(k), GET_REAL_MAX_HIT(k)) + 1]);
		buffer << fmt::format(" {:^7} &n|", ch->in_room == k->in_room ? "&gДа" : "&rНет");

		// АФФЕКТЫ
		buffer << fmt::format(" {:<5}", generate_affects_string(k));
		buffer << fmt::format("{:<1} &n|", k->low_charm() ? "&nТ" : " ");

		// ДЕБАФЫ
		buffer << fmt::format(" {:<7} &n|", generate_debuf_string(k));

		buffer << fmt::format(" {:<10}\r\n", POS_STATE[(int) k->GetPosition()]);

		SendMsgToChar(buffer.str().c_str(), ch);

	} else {
		std::ostringstream buffer;
		if (!header)
			buffer << "Персонаж            | Здоровье | Энергия | Рядом | Учить | Аффект |  Дебаф  |  Кто  | Строй | Положение \r\n";

		std::string health_color = GetWarmValueColor(GET_HIT(k), GET_REAL_MAX_HIT(k));
		std::string move_color = GetWarmValueColor(GET_MOVE(k), GET_REAL_MAX_MOVE(k));

		buffer << fmt::format("&B{:<20}&n|", k->get_name());

		buffer << fmt::format("{}", health_color);
		buffer << fmt::format("{:<10}&n|", WORD_STATE[posi_value(GET_HIT(k), GET_REAL_MAX_HIT(k)) + 1]);

		buffer << fmt::format("{}", move_color);
		buffer << fmt::format("{:^9}&n|", MOVE_STATE[posi_value(GET_MOVE(k), GET_REAL_MAX_MOVE(k)) + 1]);

		buffer << fmt::format(" {:^7} &n|", ch->in_room == k->in_room ? "&gДа" : "&rНет");

		buffer << fmt::format(" {:^5} &n|", generate_mem_string(k));
		buffer << fmt::format(" {:<5}  &n|", generate_affects_string(k));
		buffer << fmt::format(" {:<7} &n|", generate_debuf_string(k));

		buffer << fmt::format(" {:^5} &n|", leader ? "Лидер" : "");
		buffer << fmt::format(" {:^5} &n|", k->IsFlagged(EPrf::kSkirmisher) ? " &gДа  " : "Нет");
		buffer << fmt::format(" {:<10}\r\n", POS_STATE[(int) k->GetPosition()]);

		SendMsgToChar(buffer.str().c_str(), ch);
	}
}

void print_list_group(CharData *ch) {
	CharData *k;
	struct FollowerType *f;
	int count = 1;
	k = (ch->has_master() ? ch->get_master() : ch);
	if (AFF_FLAGGED(ch, EAffect::kGroup)) {
		SendMsgToChar("Ваша группа состоит из:\r\n", ch);
		if (AFF_FLAGGED(k, EAffect::kGroup)) {
			sprintf(buf1, "Лидер: %s\r\n", GET_NAME(k));
			SendMsgToChar(buf1, ch);
		}

		for (f = k->followers; f; f = f->next) {
			if (!AFF_FLAGGED(f->follower, EAffect::kGroup)) {
				continue;
			}
			sprintf(buf1, "%d. Согруппник: %s\r\n", count, GET_NAME(f->follower));
			SendMsgToChar(buf1, ch);
			count++;
		}
	} else {
		SendMsgToChar("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
	}
}

void print_group(CharData *ch) {
	int gfound = 0, cfound = 0;
	CharData *k;
	struct FollowerType *f, *g;

	k = ch->has_master() ? ch->get_master() : ch;
	if (!ch->IsNpc())
		ch->desc->msdp_report(msdp::constants::GROUP);

	if (AFF_FLAGGED(ch, EAffect::kGroup)) {
		SendMsgToChar("Ваша группа состоит из:\r\n", ch);
		if (AFF_FLAGGED(k, EAffect::kGroup)) {
			print_one_line(ch, k, true, gfound++);
		}

		for (f = k->followers; f; f = f->next) {
			if (!AFF_FLAGGED(f->follower, EAffect::kGroup)) {
				continue;
			}
			print_one_line(ch, f->follower, false, gfound++);
		}
	}

	for (f = ch->followers; f; f = f->next) {
		if (!(AFF_FLAGGED(f->follower, EAffect::kCharmed)
			|| f->follower->IsFlagged(EMobFlag::kTutelar) || f->follower->IsFlagged(EMobFlag::kMentalShadow))) {
			continue;
		}
		if (!cfound)
			SendMsgToChar("Ваши последователи:\r\n", ch);
		print_one_line(ch, f->follower, false, cfound++);
	}
	if (!gfound && !cfound) {
		SendMsgToChar("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kShowGroup)) {
		for (g = k->followers, cfound = 0; g; g = g->next) {
			for (f = g->follower->followers; f; f = f->next) {
				if (!(AFF_FLAGGED(f->follower, EAffect::kCharmed)
					|| f->follower->IsFlagged(EMobFlag::kTutelar) || f->follower->IsFlagged(EMobFlag::kMentalShadow))
					|| !AFF_FLAGGED(ch, EAffect::kGroup)) {
					continue;
				}

				if (f->follower->get_master() == ch
					|| !AFF_FLAGGED(f->follower->get_master(), EAffect::kGroup)) {
					continue;
				}

				if (ch->IsFlagged(EPrf::kNoClones)
					&& f->follower->IsNpc()
					&& (f->follower->IsFlagged(EMobFlag::kClone)
						|| GET_MOB_VNUM(f->follower) == kMobKeeper)) {
					continue;
				}

				if (!cfound) {
					SendMsgToChar("Последователи членов вашей группы:\r\n", ch);
				}
				print_one_line(ch, f->follower, false, cfound++);
			}

			if (ch->has_master()) {
				if (!(AFF_FLAGGED(g->follower, EAffect::kCharmed)
					|| g->follower->IsFlagged(EMobFlag::kTutelar) || g->follower->IsFlagged(EMobFlag::kMentalShadow))
					|| !AFF_FLAGGED(ch, EAffect::kGroup)) {
					continue;
				}

				if (ch->IsFlagged(EPrf::kNoClones)
					&& g->follower->IsNpc()
					&& (g->follower->IsFlagged(EMobFlag::kClone)
						|| GET_MOB_VNUM(g->follower) == kMobKeeper)) {
					continue;
				}

				if (!cfound) {
					SendMsgToChar("Последователи членов вашей группы:\r\n", ch);
				}
				print_one_line(ch, g->follower, false, cfound++);
			}
		}
	}
}

void GoGroup(CharData *ch, char *argument) {
	int f_number;
	struct FollowerType *f;
	for (f_number = 0, f = ch->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->follower, EAffect::kGroup)) {
			f_number++;
		}
	}

	CharData *vict;
	if (!str_cmp(buf, "all")
		|| !str_cmp(buf, "все")) {
		perform_group(ch, ch);
		int found;
		for (found = 0, f = ch->followers; f; f = f->next) {
			if ((f_number + found) >= max_group_size(ch)) {
				SendMsgToChar("Вы больше никого не можете принять в группу.\r\n", ch);
				return;
			}
			found += perform_group(ch, f->follower);
		}

		if (!found) {
			SendMsgToChar("Все, кто за вами следуют, уже включены в вашу группу.\r\n", ch);
		}

		return;
	} else if (!str_cmp(buf, "leader") || !str_cmp(buf, "лидер")) {
		vict = get_player_vis(ch, argument, EFind::kCharInWorld);
		if (vict
			&& vict->IsNpc()
			&& vict->IsFlagged(EMobFlag::kClone)
			&& AFF_FLAGGED(vict, EAffect::kCharmed)
			&& vict->has_master()
			&& !vict->get_master()->IsNpc()) {
			if (CAN_SEE(ch, vict->get_master())) {
				vict = vict->get_master();
			} else {
				vict = nullptr;
			}
		}

		if (!vict) {
			SendMsgToChar("Нет такого персонажа.\r\n", ch);
			return;
		} else if (vict == ch) {
			SendMsgToChar("Вы и так лидер группы...\r\n", ch);
			return;
		} else if (!AFF_FLAGGED(vict, EAffect::kGroup)
			|| vict->get_master() != ch) {
			SendMsgToChar(ch, "%s не является членом вашей группы.\r\n", GET_NAME(vict));
			return;
		}
		change_leader(ch, vict);
		return;
	}

	if (!(vict = get_char_vis(ch, buf, EFind::kCharInRoom))) {
		SendMsgToChar(NOPERSON, ch);
	} else if ((vict->get_master() != ch) && (vict != ch)) {
		act("$N2 нужно следовать за вами, чтобы стать членом вашей группы.", false, ch, nullptr, vict, kToChar);
	} else {
		if (!AFF_FLAGGED(vict, EAffect::kGroup)) {
			if (AFF_FLAGGED(vict, EAffect::kCharmed) || vict->IsFlagged(EMobFlag::kTutelar)
				|| vict->IsFlagged(EMobFlag::kMentalShadow) || IS_HORSE(vict)) {
				SendMsgToChar("Только равноправные персонажи могут быть включены в группу.\r\n", ch);
				SendMsgToChar("Только равноправные персонажи могут быть включены в группу.\r\n", vict);
			};
			if (f_number >= max_group_size(ch)) {
				SendMsgToChar("Вы больше никого не можете принять в группу.\r\n", ch);
				return;
			}
			perform_group(ch, ch);
			perform_group(ch, vict);
		} else if (ch != vict) {
			act("$N исключен$A из состава вашей группы.", false, ch, nullptr, vict, kToChar);
			act("Вы исключены из группы $n1!", false, ch, nullptr, vict, kToVict);
			act("$N был$G исключен$A из группы $n1!", false, ch, nullptr, vict, kToNotVict | kToArenaListen);
			//AFF_FLAGS(vict).unset(EAffectFlag::AFF_GROUP);
			vict->removeGroupFlags();
		}
	}
}

void GoUngroup(CharData *ch, char *argument) {
	struct FollowerType *f, *next_fol;
	CharData *tch;
	if (!*argument) {
		sprintf(buf2, "Вы исключены из группы %s.\r\n", GET_PAD(ch, 1));
		for (f = ch->followers; f; f = next_fol) {
			next_fol = f->next;
			if (AFF_FLAGGED(f->follower, EAffect::kGroup)) {
				//AFF_FLAGS(f->ch).unset(EAffectFlag::AFF_GROUP);
				f->follower->removeGroupFlags();
				SendMsgToChar(buf2, f->follower);
				if (!AFF_FLAGGED(f->follower, EAffect::kCharmed)
					&& !(f->follower->IsNpc()
						&& AFF_FLAGGED(f->follower, EAffect::kHorse))) {
					stop_follower(f->follower, kSfEmpty);
				}
			}
		}
		AFF_FLAGS(ch).unset(EAffect::kGroup);
		ch->removeGroupFlags();
		SendMsgToChar("Вы распустили группу.\r\n", ch);
		return;
	}
	for (f = ch->followers; f; f = next_fol) {
		next_fol = f->next;
		tch = f->follower;
		if (isname(argument, tch->GetCharAliases())
			&& !AFF_FLAGGED(tch, EAffect::kCharmed)
			&& !IS_HORSE(tch)) {
			//AFF_FLAGS(tch).unset(EAffectFlag::AFF_GROUP);
			tch->removeGroupFlags();
			act("$N более не член вашей группы.", false, ch, nullptr, tch, kToChar);
			act("Вы исключены из группы $n1!", false, ch, nullptr, tch, kToVict);
			act("$N был$G изгнан$A из группы $n1!", false, ch, nullptr, tch, kToNotVict | kToArenaListen);
			stop_follower(tch, kSfEmpty);
			return;
		}
	}
	SendMsgToChar("Этот игрок не входит в состав вашей группы.\r\n", ch);
}

void do_report(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	CharData *k;
	struct FollowerType *f;

	if (!AFF_FLAGGED(ch, EAffect::kGroup) && !AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar("И перед кем вы отчитываетесь?\r\n", ch);
		return;
	}
	if (IS_MANA_CASTER(ch)) {
		sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %d(%d)M\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch),
				GET_HIT(ch), GET_REAL_MAX_HIT(ch),
				GET_MOVE(ch), GET_REAL_MAX_MOVE(ch),
				ch->mem_queue.stored, GET_MAX_MANA(ch));
	} else if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		int loyalty = 0;
		for (const auto &aff : ch->affected) {
			if (aff->type == ESpell::kCharm) {
				loyalty = aff->duration / 2;
				break;
			}
		}
		sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %dL\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch),
				GET_HIT(ch), GET_REAL_MAX_HIT(ch),
				GET_MOVE(ch), GET_REAL_MAX_MOVE(ch),
				loyalty);
	} else {
		sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch),
				GET_HIT(ch), GET_REAL_MAX_HIT(ch),
				GET_MOVE(ch), GET_REAL_MAX_MOVE(ch));
	}
	CAP(buf);
	k = ch->has_master() ? ch->get_master() : ch;
	for (f = k->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->follower, EAffect::kGroup)
			&& f->follower != ch
			&& !AFF_FLAGGED(f->follower, EAffect::kDeafness)) {
			SendMsgToChar(buf, f->follower);
		}
	}

	if (k != ch && !AFF_FLAGGED(k, EAffect::kDeafness)) {
		SendMsgToChar(buf, k);
	}
	SendMsgToChar("Вы доложили о состоянии всем членам вашей группы.\r\n", ch);
}

void do_split(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	do_split(ch, argument, 0, 0, 0);
}

void do_split(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, int currency) {
	int amount, num, share, rest;
	CharData *k;
	struct FollowerType *f;

	if (ch->IsNpc())
		return;

	one_argument(argument, buf);

	EWhat what_currency;

	switch (currency) {
		case currency::ICE : what_currency = EWhat::kIceU;
			break;
		default : what_currency = EWhat::kMoneyU;
			break;
	}

	if (is_number(buf)) {
		amount = atoi(buf);
		if (amount <= 0) {
			SendMsgToChar("И как вы это планируете сделать?\r\n", ch);
			return;
		}

		if (amount > ch->get_gold() && currency == currency::GOLD) {
			SendMsgToChar("И где бы взять вам столько денег?.\r\n", ch);
			return;
		}
		k = ch->has_master() ? ch->get_master() : ch;

		if (AFF_FLAGGED(k, EAffect::kGroup)
			&& (k->in_room == ch->in_room)) {
			num = 1;
		} else {
			num = 0;
		}

		for (f = k->followers; f; f = f->next) {
			if (AFF_FLAGGED(f->follower, EAffect::kGroup)
				&& !f->follower->IsNpc()
				&& f->follower->in_room == ch->in_room) {
				num++;
			}
		}

		if (num && AFF_FLAGGED(ch, EAffect::kGroup)) {
			share = amount / num;
			rest = amount % num;
		} else {
			SendMsgToChar("С кем вы хотите разделить это добро?\r\n", ch);
			return;
		}
		//MONEY_HACK

		switch (currency) {
			case currency::ICE : ch->sub_ice_currency(share * (num - 1));
				break;
			case currency::GOLD : ch->remove_gold(share * (num - 1));
				break;
		}

		sprintf(buf, "%s разделил%s %d %s; вам досталось %d.\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch), amount, GetDeclensionInNumber(amount, what_currency), share);
		if (AFF_FLAGGED(k, EAffect::kGroup) && k->in_room == ch->in_room && !k->IsNpc() && k != ch) {
			SendMsgToChar(buf, k);
			switch (currency) {
				case currency::ICE : {
					k->add_ice_currency(share);
					break;
				}
				case currency::GOLD : {
					k->add_gold(share, true, true);
					break;
				}
			}
		}
		for (f = k->followers; f; f = f->next) {
			if (AFF_FLAGGED(f->follower, EAffect::kGroup)
				&& !f->follower->IsNpc()
				&& f->follower->in_room == ch->in_room
				&& f->follower != ch) {
				SendMsgToChar(buf, f->follower);
				switch (currency) {
					case currency::ICE : f->follower->add_ice_currency(share);
						break;
					case currency::GOLD : f->follower->add_gold(share, true, true);
						break;
				}
			}
		}
		sprintf(buf, "Вы разделили %d %s на %d  -  по %d каждому.\r\n",
				amount, GetDeclensionInNumber(amount, what_currency), num, share);
		if (rest) {
			sprintf(buf + strlen(buf),
					"Как истинный еврей вы оставили %d %s (которые не смогли разделить нацело) себе.\r\n",
					rest, GetDeclensionInNumber(rest, what_currency));
		}

		SendMsgToChar(buf, ch);
		// клан-налог лутера с той части, которая пошла каждому в группе
		if (currency == currency::GOLD) {
			const long clan_tax = ClanSystem::do_gold_tax(ch, share);
			ch->remove_gold(clan_tax);
		}
	} else {
		SendMsgToChar("Сколько и чего вы хотите разделить?\r\n", ch);
		return;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
