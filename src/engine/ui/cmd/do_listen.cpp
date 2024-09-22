/**
\file listen.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/sight.h"
#include "engine/db/global_objects.h"
#include "engine/core/handler.h"

void hear_in_direction(CharData *ch, int dir, int info_is);

void DoListen(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int i;

	if (!ch->desc)
		return;

	if (AFF_FLAGGED(ch, EAffect::kDeafness)) {
		SendMsgToChar("Вы глухи и все равно ничего не услышите.\r\n", ch);
		return;
	}

	if (ch->GetPosition() < EPosition::kSleep)
		SendMsgToChar("Вам начали слышаться голоса предков, зовущие вас к себе.\r\n", ch);
	if (ch->GetPosition() == EPosition::kSleep)
		SendMsgToChar("Морфей медленно задумчиво провел рукой по струнам и заиграл колыбельную.\r\n", ch);
	else if (ch->GetSkill(ESkill::kHearing)) {
		if (check_moves(ch, kHearingMoves)) {
			SendMsgToChar("Вы начали сосредоточенно прислушиваться.\r\n", ch);
			for (i = 0; i < EDirection::kMaxDirNum; i++)
				hear_in_direction(ch, i, 0);
			if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike)))
				SetWaitState(ch, 1 * kBattleRound);
		}
	} else
		SendMsgToChar("Выучите сначала как это следует делать.\r\n", ch);
}

void hear_in_direction(CharData *ch, int dir, int info_is) {
	int count = 0, percent = 0, probe = 0;
	RoomData::exit_data_ptr rdata;
	int fight_count = 0;
	std::string tmpstr;

	if (AFF_FLAGGED(ch, EAffect::kDeafness)) {
		SendMsgToChar("Вы забыли, что вы глухи?\r\n", ch);
		return;
	}
	if (CAN_GO(ch, dir)
		|| (EXIT(ch, dir)
			&& EXIT(ch, dir)->to_room() != kNowhere)) {
		rdata = EXIT(ch, dir);
		count += sprintf(buf, "%s%s:%s ", kColorYel, dirs_rus[dir], kColorNrm);
		count += sprintf(buf + count, "\r\n%s", kColorGrn);
		SendMsgToChar(buf, ch);
		count = 0;
		for (const auto tch : world[rdata->to_room()]->people) {
			percent = number(1, MUD::Skill(ESkill::kHearing).difficulty);
			probe = CalcCurrentSkill(ch, ESkill::kHearing, tch);
			TrainSkill(ch, ESkill::kHearing, probe >= percent, tch);
			// Если сражаются то слышем только борьбу.
			if (tch->GetEnemy()) {
				if (tch->IsNpc()) {
					tmpstr += " Вы слышите шум чьей-то борьбы.\r\n";
				} else {
					tmpstr += " Вы слышите звуки чьих-то ударов.\r\n";
				}
				fight_count++;
				continue;
			}

			if ((probe >= percent
				|| ((!AFF_FLAGGED(tch, EAffect::kSneak) || !AFF_FLAGGED(tch, EAffect::kHide))
					&& (probe > percent * 2)))
				&& (percent < 100 || IS_IMMORTAL(ch))
				&& !fight_count) {
				if (tch->IsNpc()) {
					if (GET_RACE(tch) == ENpcRace::kConstruct) {
						if (GetRealLevel(tch) < 5)
							tmpstr += " Вы слышите чье-то тихое поскрипывание.\r\n";
						else if (GetRealLevel(tch) < 15)
							tmpstr += " Вы слышите чей-то скрип.\r\n";
						else if (GetRealLevel(tch) < 25)
							tmpstr += " Вы слышите чей-то громкий скрип.\r\n";
						else
							tmpstr += " Вы слышите чей-то грозный скрип.\r\n";
					} else if (real_sector(ch->in_room) != ESector::kUnderwater) {
						if (GetRealLevel(tch) < 5)
							tmpstr += " Вы слышите чью-то тихую возню.\r\n";
						else if (GetRealLevel(tch) < 15)
							tmpstr += " Вы слышите чье-то сопение.\r\n";
						else if (GetRealLevel(tch) < 25)
							tmpstr += " Вы слышите чье-то громкое дыхание.\r\n";
						else
							tmpstr += " Вы слышите чье-то грозное дыхание.\r\n";
					} else {
						if (GetRealLevel(tch) < 5)
							tmpstr += " Вы слышите тихое бульканье.\r\n";
						else if (GetRealLevel(tch) < 15)
							tmpstr += " Вы слышите бульканье.\r\n";
						else if (GetRealLevel(tch) < 25)
							tmpstr += " Вы слышите громкое бульканье.\r\n";
						else
							tmpstr += " Вы слышите грозное пузырение.\r\n";
					}
				} else {
					tmpstr += " Вы слышите чье-то присутствие.\r\n";
				}
				count++;
			}
		}

		if ((!count) && (!fight_count)) {
			SendMsgToChar(" Тишина и покой.\r\n", ch);
		} else {
			SendMsgToChar(tmpstr.c_str(), ch);
		}

		SendMsgToChar(kColorNrm, ch);
	} else {
		if (info_is & EXIT_SHOW_WALL) {
			SendMsgToChar("И что вы там хотите услышать?\r\n", ch);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
