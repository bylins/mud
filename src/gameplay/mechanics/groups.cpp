/**
\file groups.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Код групп игроков и мобов.
\detail Группы, вступление, покидание, дележка опыта - должно быть тут.
*/

#include "gameplay/mechanics/groups.h"
#include "utils/grammar/gender.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/mount.h"

#include "engine/entities/char_data.h"
#include "engine/core/target_resolver.h"
#include "engine/ui/color.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/magic/magic.h"
#include "engine/network/msdp/msdp_constants.h"
#include "gameplay/clans/house.h"
#include "gameplay/economics/currencies.h"
#include "engine/db/global_objects.h"
#include "gameplay/skills/leadership.h"

#include <fmt/format.h>

#include <ranges>
#include "gameplay/mechanics/sight.h"

bool group::same_group(CharData *ch, CharData *tch) {
	if (!ch || !tch)
		return false;

	// Добавлены проверки чтобы не любой заследовавшийся моб считался согруппником (Купала)
	if (ch->IsNpc()
		&& ch->has_master()
		&& !ch->get_master()->IsNpc()
		&& (mount::IsHorse(ch)
			|| ch->IsFlagged(EMobFlag::kCompanion))) {
		ch = ch->get_master();
	}

	if (tch->IsNpc()
		&& tch->has_master()
		&& !tch->get_master()->IsNpc()
		&& (mount::IsHorse(tch)
			|| tch->IsFlagged(EMobFlag::kCompanion))) {
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

int group::max_group_size(CharData *ch) {
	return kMaxGroupedFollowers + CalcLeadershipGroupSizeBonus(ch);
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

int group::perform_group(CharData *ch, CharData *vict) {
	if (AFF_FLAGGED(vict, EAffect::kGroup)
		|| vict->IsFlagged(EMobFlag::kCompanion)
		|| mount::IsHorse(vict)
		|| IsAffected(ch, EAffect::kFrenzy)
		|| IsAffected(vict, EAffect::kFrenzy)) {
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
		|| ch->followers.empty()) {
		return;
	}

	CharData *leader = vict;
	if (!leader) {
		// лидер умер, ищем согрупника с максимальным скиллом лидерки
		for (auto *l : ch->followers) {
			if (!is_group_member(ch, l))
				continue;
			if (!leader)
				leader = l;
			else if (GetSkill(l, ESkill::kLeadership) > GetSkill(leader, ESkill::kLeadership))
				leader = l;
		}
	}

	if (!leader) {
		return;
	}

	// для реследования используем стандартные функции
	std::vector<CharData *> temp_list;
	auto followers_copy = ch->followers;
	for (auto *l : followers_copy) {
		if (!is_group_member(ch, l)) {
			continue;
		} else {
			CharData *temp_vict = l;
			if (temp_vict->has_master() && follow::StopFollower(temp_vict, follow::kSfSilence)) {
				continue;
			}

			if (temp_vict != leader) {
				temp_list.push_back(temp_vict);
			}
		}
	}

	if (!temp_list.empty()) {
		for (auto & it : std::ranges::reverse_view(temp_list)) {
			follow::AddFollowerSilently(leader, it);
		}
	}

	// бывшего лидера последним закидываем обратно в группу, если он живой
	if (vict) {
		// флаг группы надо снять, иначе при регрупе не будет писаться о старом лидере
		//AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
		group::RemoveGroupFlags(ch);
		follow::AddFollowerSilently(leader, ch);
	}

	if (leader->followers.empty()) {
		return;
	}

	ch->dps_copy(leader);
	group::perform_group(leader, leader);
	int num_followers = 0;
	for (auto *f : leader->followers) {
		if (num_followers < group::max_group_size(leader)) {
			if (group::perform_group(leader, f))
				++num_followers;
		} else {
			SendMsgToChar("Вы больше никого не можете принять в группу.\r\n", ch);
			return;
		}
	}
}

void group::print_one_line(CharData *ch, CharData *k, int leader, int header) {
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
	  affects += mount::IsOnHorse(k) ? "&YВ" : " ";

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
	  debuf += IsAffected(k, EAffect::kFever) ? "&cЛ" : " ";

	  return debuf;
	};

	auto generate_mem_string = [](CharData *k) -> std::string {
	  int ok, ok2, div;
	  if ((!IS_MANA_CASTER(k) && !k->mem_queue.Empty()) ||
		  (IS_MANA_CASTER(k) && k->mem_queue.stored < Mana(GetRealWis(k)))) {
		  div = CalcManaGain(k);
		  if (div > 0) {
			  if (!IS_MANA_CASTER(k)) {
				  ok2 = std::max(0, 1 + k->mem_queue.total - k->mem_queue.stored);
				  ok2 = ok2 * 60 / div;    // время мема в сек
			  } else {
				  ok2 = std::max(0, 1 + Mana(GetRealWis(k)) - k->mem_queue.stored);
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
			buffer << "Персонаж            | Здоровье | Рядом | Аффект  |  Дебаф  | Положение\r\n";

		buffer << fmt::format("&B{:<20}&n|", k->get_name().substr(0, 20));

		buffer << fmt::format("{}", GetWarmValueColor(k->get_hit(), k->get_real_max_hit()));
		buffer << fmt::format("{:<10}&n|", WORD_STATE[posi_value(k->get_hit(), k->get_real_max_hit()) + 1]);
		buffer << fmt::format(" {:^7} &n|", ch->in_room == k->in_room ? "&gДа" : "&rНет");

		// АФФЕКТЫ
		buffer << fmt::format(" {:<5}", generate_affects_string(k));
		buffer << fmt::format("{:<1} &n|", IsCharmExpiring(k) ? "&nТ" : " ");

		// ДЕБАФЫ
		buffer << fmt::format(" {:<7} &n|", generate_debuf_string(k));

		buffer << fmt::format(" {:<10}\r\n", position_types[(int) k->GetPosition()]);

		SendMsgToChar(buffer.str().c_str(), ch);

	} else {
		std::ostringstream buffer;
		if (!header)
			buffer << "Персонаж            | Здоровье | Энергия | Рядом | Учить | Аффект  |  Дебаф  |  Кто  | Строй | Положение \r\n";

		std::string health_color = GetWarmValueColor(k->get_hit(), k->get_real_max_hit());
		std::string move_color = GetWarmValueColor(k->get_move(), k->get_real_max_move());

		buffer << fmt::format("&B{:<20}&n|", k->get_name());

		buffer << fmt::format("{}", health_color);
		buffer << fmt::format("{:<10}&n|", WORD_STATE[posi_value(k->get_hit(), k->get_real_max_hit()) + 1]);

		buffer << fmt::format("{}", move_color);
		buffer << fmt::format("{:^9}&n|", MOVE_STATE[posi_value(k->get_move(), k->get_real_max_move()) + 1]);

		buffer << fmt::format(" {:^7} &n|", ch->in_room == k->in_room ? "&gДа" : "&rНет");

		buffer << fmt::format(" {:^5} &n|", generate_mem_string(k));
		buffer << fmt::format(" {:<5}  &n|", generate_affects_string(k));
		buffer << fmt::format(" {:<7} &n|", generate_debuf_string(k));

		buffer << fmt::format(" {:^5} &n|", leader ? "Лидер" : "");
		buffer << fmt::format(" {:^5} &n|", k->IsFlagged(EPrf::kSkirmisher) ? " &gДа  " : "Нет");
		buffer << fmt::format(" {:<10}\r\n", k->GetEnemy()  ? "Сражается" : mount::IsOnHorse(k) ? "Верхом" : position_types[(int) k->GetPosition()]);

		SendMsgToChar(buffer.str().c_str(), ch);
	}
}

void group::print_list_group(CharData *ch) {
	CharData *k;
	int count = 1;
	k = (ch->has_master() ? ch->get_master() : ch);
	if (AFF_FLAGGED(ch, EAffect::kGroup)) {
		SendMsgToChar("Ваша группа состоит из:\r\n", ch);
		if (AFF_FLAGGED(k, EAffect::kGroup)) {
			sprintf(buf1, "Лидер: %s\r\n", GET_NAME(k));
			SendMsgToChar(buf1, ch);
		}

		for (auto *f : k->followers) {
			if (!AFF_FLAGGED(f, EAffect::kGroup)) {
				continue;
			}
			sprintf(buf1, "%d. Согруппник: %s\r\n", count, GET_NAME(f));
			SendMsgToChar(buf1, ch);
			count++;
		}
	} else {
		SendMsgToChar("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
	}
}

void group::print_group(CharData *ch) {
	int gfound = 0, cfound = 0;
	CharData *k;

	k = ch->has_master() ? ch->get_master() : ch;
	if (!ch->IsNpc())
		ch->desc->msdp_report(msdp::constants::GROUP);

	if (AFF_FLAGGED(ch, EAffect::kGroup)) {
		SendMsgToChar("Ваша группа состоит из:\r\n", ch);
		if (AFF_FLAGGED(k, EAffect::kGroup)) {
			group::print_one_line(ch, k, true, gfound++);
		}

		for (auto *f : k->followers) {
			if (!AFF_FLAGGED(f, EAffect::kGroup)) {
				continue;
			}
			print_one_line(ch, f, false, gfound++);
		}
	}

	for (auto *f : ch->followers) {
		if (!(f->IsFlagged(EMobFlag::kCompanion))) {
			continue;
		}
		if (!cfound)
			SendMsgToChar("Ваши последователи:\r\n", ch);
		print_one_line(ch, f, false, cfound++);
	}
	if (!gfound && !cfound) {
		SendMsgToChar("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kShowGroup)) {
		cfound = 0;
		for (auto *g : k->followers) {
			for (auto *f : g->followers) {
				if (!(f->IsFlagged(EMobFlag::kCompanion))
					|| !AFF_FLAGGED(ch, EAffect::kGroup)) {
					continue;
				}

				if (f->get_master() == ch
					|| !AFF_FLAGGED(f->get_master(), EAffect::kGroup)) {
					continue;
				}

				if (ch->IsFlagged(EPrf::kNoClones)
					&& f->IsNpc()
					&& (f->IsFlagged(EMobFlag::kClone)
						|| GET_MOB_VNUM(f) == kMobKeeper)) {
					continue;
				}

				if (!cfound) {
					SendMsgToChar("Последователи членов вашей группы:\r\n", ch);
				}
				group::print_one_line(ch, f, false, cfound++);
			}

			if (ch->has_master()) {
				if (!(g->IsFlagged(EMobFlag::kCompanion))
					|| !AFF_FLAGGED(ch, EAffect::kGroup)) {
					continue;
				}

				if (ch->IsFlagged(EPrf::kNoClones)
					&& g->IsNpc()
					&& (g->IsFlagged(EMobFlag::kClone)
						|| GET_MOB_VNUM(g) == kMobKeeper)) {
					continue;
				}

				if (!cfound) {
					SendMsgToChar("Последователи членов вашей группы:\r\n", ch);
				}
				group::print_one_line(ch, g, false, cfound++);
			}
		}
	}
}

void group::GoGroup(CharData *ch, char *argument) {
	int f_number = 0;
	for (auto *f : ch->followers) {
		if (AFF_FLAGGED(f, EAffect::kGroup)) {
			f_number++;
		}
	}

	CharData *vict;
	if (!str_cmp(buf, "all")
		|| !str_cmp(buf, "все")) {
		int found = 0;
		for (auto *f : ch->followers) {
			if ((f_number + found) >= group::max_group_size(ch)) {
				SendMsgToChar("Вы больше никого не можете принять в группу.\r\n", ch);
				break;
			}
			if (IsAffected(f, EAffect::kFrenzy)) {
				continue;
			}
			found += group::perform_group(ch, f);
		}

		if (!found) {
			SendMsgToChar("Все, кто за вами следуют, уже включены в вашу группу.\r\n", ch);
		} else {
			group::perform_group(ch, ch);
		}

		return;
	} else if (!str_cmp(buf, "leader") || !str_cmp(buf, "лидер")) {
		vict = target_resolver::FindPlayerVis(ch, argument);
		if (vict
			&& vict->IsNpc()
			&& vict->IsFlagged(EMobFlag::kClone)
			&& AFF_FLAGGED(vict, EAffect::kCharmed)
			&& vict->has_master()
			&& !vict->get_master()->IsNpc()) {
			if (sight::CanSee(ch, vict->get_master())) {
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

	vict = target_resolver::FindCharInRoom(ch, buf);

	if (!vict) {
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
	} else if ((vict->get_master() != ch) && (vict != ch)) {
		act("$N2 нужно следовать за вами, чтобы стать членом вашей группы.", false, ch, nullptr, vict, kToChar);
	} else if (IsAffected(vict, EAffect::kFrenzy)) {
		act("$N слишком агрессивн$W и непредсказуем$W! Нельзя брать $s!", false, ch, nullptr, vict, kToChar);
	} else {
		if (!AFF_FLAGGED(vict, EAffect::kGroup)) {
			if (vict->IsFlagged(EMobFlag::kCompanion) || mount::IsHorse(vict)) {
				SendMsgToChar("Только равноправные персонажи могут быть включены в группу.\r\n", ch);
				SendMsgToChar("Только равноправные персонажи могут быть включены в группу.\r\n", vict);
			};
			if (f_number >= group::max_group_size(ch)) {
				SendMsgToChar("Вы больше никого не можете принять в группу.\r\n", ch);
				return;
			}
			group::perform_group(ch, ch);
			group::perform_group(ch, vict);
		} else if (ch != vict) {
			act("$N исключен$A из состава вашей группы.", false, ch, nullptr, vict, kToChar);
			act("Вы исключены из группы $n1!", false, ch, nullptr, vict, kToVict);
			act("$N был$G исключен$A из группы $n1!", false, ch, nullptr, vict, kToNotVict | kToArenaListen);
			//AFF_FLAGS(vict).unset(EAffectFlag::AFF_GROUP);
			group::RemoveGroupFlags(vict);
		}
	}
}

void group::GoUngroup(CharData *ch, char *argument) {
	CharData *tch;
	if (!*argument) {
		sprintf(buf2, "Вы исключены из группы %s.\r\n", GET_PAD(ch, 1));
		auto copy = ch->followers;
		for (auto *f : copy) {
			if (AFF_FLAGGED(f, EAffect::kGroup)) {
				//AFF_FLAGS(f->ch).unset(EAffectFlag::AFF_GROUP);
				group::RemoveGroupFlags(f);
				SendMsgToChar(buf2, f);
				if (!AFF_FLAGGED(f, EAffect::kCharmed)
					&& !(f->IsNpc()
						&& AFF_FLAGGED(f, EAffect::kHorse))) {
					follow::StopFollower(f, follow::kSfEmpty);
				}
			}
		}
		AFF_FLAGS(ch).unset(EAffect::kGroup);
		group::RemoveGroupFlags(ch);
		SendMsgToChar("Вы распустили группу.\r\n", ch);
		return;
	}
	auto copy2 = ch->followers;
	for (auto *f : copy2) {
		tch = f;
		if (isname(argument, tch->GetCharAliases())
			&& !AFF_FLAGGED(tch, EAffect::kCharmed)
			&& !mount::IsHorse(tch)) {
			//AFF_FLAGS(tch).unset(EAffectFlag::AFF_GROUP);
			group::RemoveGroupFlags(tch);
			act("$N более не член вашей группы.", false, ch, nullptr, tch, kToChar);
			act("Вы исключены из группы $n1!", false, ch, nullptr, tch, kToVict);
			act("$N был$G изгнан$A из группы $n1!", false, ch, nullptr, tch, kToNotVict | kToArenaListen);
			follow::StopFollower(tch, follow::kSfEmpty);
			return;
		}
	}
	SendMsgToChar("Этот игрок не входит в состав вашей группы.\r\n", ch);
}

void do_report(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *k;

	if (!*argument) {
		if (!AFF_FLAGGED(ch, EAffect::kGroup) && !AFF_FLAGGED(ch, EAffect::kCharmed)) {
			SendMsgToChar("И перед кем вы отчитываетесь?\r\n", ch);
			return;
		}
		if (IS_MANA_CASTER(ch)) {
			sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %d(%d)M\r\n",
					GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1),
					ch->get_hit(), ch->get_real_max_hit(),
					ch->get_move(), ch->get_real_max_move(),
					ch->mem_queue.stored, Mana(GetRealWis(ch)));
		} else if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
			int loyalty = 0;

			for (const auto &aff : ch->affected) {
				if (IS_SET(aff->battleflag, kAfCharmBond)) {
					loyalty = aff->duration / 2;
					break;
				}
			}
			sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %dL\r\n",
					GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1),
					ch->get_hit(), ch->get_real_max_hit(),
					ch->get_move(), ch->get_real_max_move(),
					loyalty);
		} else {
			sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V\r\n",
					GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1),
					ch->get_hit(), ch->get_real_max_hit(),
					ch->get_move(), ch->get_real_max_move());
		}
		utils::CAP(buf);
		k = ch->has_master() ? ch->get_master() : ch;
		for (auto *f : k->followers) {
			if (AFF_FLAGGED(f, EAffect::kGroup)
				&& f != ch
				&& !AFF_FLAGGED(f, EAffect::kDeafness)) {
				SendMsgToChar(buf, f);
			}
		}
		if (k != ch && !AFF_FLAGGED(k, EAffect::kDeafness)) {
			SendMsgToChar(buf, k);
		}
		SendMsgToChar("Вы доложили о состоянии всем членам вашей группы.\r\n", ch);
	} else {
		if (CompareParam(argument, "умения")) {
			if (ch->followers.empty()) {
				SendMsgToChar(ch, "За вами никто не следует.");
				return;
			}
			for (auto *f : ch->followers) {
				if (IsCharmice(f)) {
					std::string str;

					SendMsgToChar(ch, "%s доложил%s свои умения:", utils::CAP(f->get_name()).c_str(), grammar::SexEnding((f)->get_sex(), 1));
					for (const auto &skill : MUD::Skills()) {
						if (skill.IsValid() && GetSkill(f, skill.GetId())) {
							str += fmt::format(" {},", skill.GetName());
						}
					}
					if (str.back() == ',') {
						str.pop_back();
						str.push_back('.');
					}
					SendMsgToChar(ch, "&C%s&n\r\n", str.c_str());
				} else {
					SendMsgToChar(ch, "%s не подчиняется вам, и не хочет ничего докладывать.\r\n", utils::CAP(f->get_name()).c_str());
				}
			}
		} else {
			SendMsgToChar(ch, "Кому чего доложить?\r\n");
		}
	}
}

void group::do_split(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	do_split(ch, argument, 0, 0, 0);
}

void group::do_split(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, int currency_vnum) {
	int amount, num, share, rest;
	CharData *k;

	if (ch->IsNpc())
		return;

	one_argument(argument, buf);


	if (is_number(buf)) {
		amount = atoi(buf);
		if (amount <= 0) {
			SendMsgToChar("И как вы это планируете сделать?\r\n", ch);
			return;
		}

		if (amount > currencies::GetHand(*ch, currency_vnum)) {
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

		for (auto *f : k->followers) {
			if (AFF_FLAGGED(f, EAffect::kGroup)
				&& !f->IsNpc()
				&& f->in_room == ch->in_room) {
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

		currencies::RemoveHand(*ch, currency_vnum, share * (num - 1));

		sprintf(buf, "%s разделил%s %d %s; вам досталось %d.\r\n",
				GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1), amount, MUD::Currency(currency_vnum).GetNameWithAmount(amount, grammar::ECase::kAcc).c_str(), share);
		if (AFF_FLAGGED(k, EAffect::kGroup) && k->in_room == ch->in_room && !k->IsNpc() && k != ch) {
			SendMsgToChar(buf, k);
			currencies::AddHand(*k, currency_vnum, share - (currency_vnum == currencies::kGoldVnum ? ClanSystem::do_gold_tax(k, share) : 0), false, true);
		}
		for (auto *f : k->followers) {
			if (AFF_FLAGGED(f, EAffect::kGroup)
				&& !f->IsNpc()
				&& f->in_room == ch->in_room
				&& f != ch) {
				SendMsgToChar(buf, f);
				currencies::AddHand(*f, currency_vnum, share - (currency_vnum == currencies::kGoldVnum ? ClanSystem::do_gold_tax(f, share) : 0), false, true);
			}
		}
		sprintf(buf, "Вы разделили %d %s на %d  -  по %d каждому.\r\n",
				amount, MUD::Currency(currency_vnum).GetNameWithAmount(amount, grammar::ECase::kAcc).c_str(), num, share);
		if (rest) {
			sprintf(buf + strlen(buf),
					"Как истинный еврей вы оставили %d %s (которые не смогли разделить нацело) себе.\r\n",
					rest, MUD::Currency(currency_vnum).GetNameWithAmount(rest, grammar::ECase::kAcc).c_str());
		}

		SendMsgToChar(buf, ch);
		// клан-налог лутера с той части, которая пошла каждому в группе
		if (currency_vnum == currencies::kGoldVnum) {
			const long clan_tax = ClanSystem::do_gold_tax(ch, share);
			currencies::RemoveHand(*ch, currencies::kGold, clan_tax);
		}
	} else {
		SendMsgToChar("Сколько и чего вы хотите разделить?\r\n", ch);
		return;
	}
}

// issue.chardata-cleaning: was CharData::removeGroupFlags.
void group::RemoveGroupFlags(CharData *ch) {
	AFF_FLAGS(ch).unset(EAffect::kGroup);
	ch->UnsetFlag(EPrf::kSkirmisher);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
