/* ************************************************************************
*   File: track.cpp                                     Part of Bylins    *
*  ************************************************************************/

#include "track.h"

#include "graph.h"
#include "handler.h"
#include "structs/global_objects.h"

const char *track_when[] = {"совсем свежие",
							"свежие",
							"менее полудневной давности",
							"примерно полудневной давности",
							"почти дневной давности",
							"примерно дневной давности",
							"совсем старые"
};

int age_track(CharData * /*ch*/, int time, int calc_track) {
	int when = 0;

	if (calc_track >= number(1, 50)) {
		if (time & 0x03)    // 2
			when = 0;
		else if (time & 0x1F)    // 5
			when = 1;
		else if (time & 0x3FF)    // 10
			when = 2;
		else if (time & 0x7FFF)    // 15
			when = 3;
		else if (time & 0xFFFFF)    // 20
			when = 4;
		else if (time & 0x3FFFFFF)    // 26
			when = 5;
		else
			when = 6;
	} else
		when = number(0, 6);
	return (when);
}

// * Functions and Commands which use the above functions. *
int go_track(CharData *ch, CharData *victim, const ESkill skill_no) {
	int percent, dir;
	int if_sense;

	if (AFF_FLAGGED(victim, EAffect::kNoTrack) && (skill_no != ESkill::kSense)) {
		return kBfsError;
	}
	// 101 is a complete failure, no matter what the proficiency.
	//Временная затычка. Перевести на резисты
	//Изменил макс скилл со 100 до 200, чтобы не ломать алгоритм, в данном значении вернем старое значение.
	if_sense = (skill_no == ESkill::kSense) ? 100 : 0;
	percent = number(0, MUD::Skill(skill_no).difficulty - if_sense);
	//current_skillpercent = GET_SKILL(ch, ESkill::kSense);
	if ((!victim->IsNpc()) && (!IS_GOD(ch)) && (!ch->IsNpc())) //Если цель чар и ищет не бог
	{
		percent = MIN(99, number(0, GET_REAL_REMORT(victim)) + percent);
	}
	if (percent > CalcCurrentSkill(ch, skill_no, victim)) {
		int tries = 10;
		// Find a random direction. :)
		do {
			dir = number(0, EDirection::kMaxDirNum - 1);
		} while (!CAN_GO(ch, dir) && --tries);
		return dir;
	}

	// They passed the skill check.
	return find_first_step(ch->in_room, victim->in_room, ch);
}

void do_track(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict = nullptr;
	struct TrackData *track;
	int found = false, calc_track = 0, track_t, i;
	char name[kMaxInputLength];

	// The character must have the track skill.
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kTrack)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы как крот.\r\n", ch);
		return;
	}

	if (!check_moves(ch, CanUseFeat(ch, EFeat::kTracker) ? kTrackMoves / 2 : kTrackMoves))
		return;

	calc_track = CalcCurrentSkill(ch, ESkill::kTrack, nullptr);
	act("Похоже, $n кого-то выслеживает.", false, ch, nullptr, nullptr, kToRoom);
	one_argument(argument, arg);

	// No argument - show all
	if (!*arg) {
		for (track = world[ch->in_room]->track; track; track = track->next) {
			*name = '\0';
			if (IS_SET(track->track_info, TRACK_NPC)) {
				strcpy(name, GET_NAME(mob_proto + track->who));
			} else {
				for (std::size_t c = 0; c < player_table.size(); c++) {
					if (player_table[c].id() == track->who) {
						strcpy(name, player_table[c].name());
						break;
					}
				}
			}

			if (*name && calc_track > number(1, 40)) {
				CAP(name);
				for (track_t = i = 0; i < EDirection::kMaxDirNum; i++) {
					track_t |= track->time_outgone[i];
					track_t |= track->time_income[i];
				}
				sprintf(buf, "%s : следы %s.\r\n", name,
						track_when[age_track(ch, track_t, calc_track)]);
				SendMsgToChar(buf, ch);
				found = true;
			}
		}
		if (!found)
			SendMsgToChar("Вы не видите ничьих следов.\r\n", ch);
		return;
	}

	if ((vict = get_char_vis(ch, arg, EFind::kCharInRoom))) {
		act("Вы же в одной комнате с $N4!", false, ch, nullptr, vict, kToChar);
		return;
	}

	// found victim
	for (track = world[ch->in_room]->track; track; track = track->next) {
		*name = '\0';
		if (IS_SET(track->track_info, TRACK_NPC)) {
			strcpy(name, GET_NAME(mob_proto + track->who));
		} else {
			for (std::size_t c = 0; c < player_table.size(); c++) {
				if (player_table[c].id() == track->who) {
					strcpy(name, player_table[c].name());
					break;
				}
			}
		}

		if (*name && isname(arg, name))
			break;
		else
			*name = '\0';
	}

	if (calc_track < number(1, 40) || !*name || ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoTrack)) {
		SendMsgToChar("Вы не видите похожих следов.\r\n", ch);
		return;
	}

	ch->track_dirs = 0;
	CAP(name);
	sprintf(buf, "%s:\r\n", name);

	for (int c = 0; c < EDirection::kMaxDirNum; c++) {
		if ((track && track->time_income[c]
			&& calc_track >= number(0, MUD::Skill(ESkill::kTrack).difficulty))
			|| (!track && calc_track < number(0, MUD::Skill(ESkill::kTrack).difficulty))) {
			found = true;
			sprintf(buf + strlen(buf), "- %s следы ведут %s\r\n",
					track_when[age_track
						(ch,
						 track ? track->
							 time_income[c] : (1 << number(0, 25)), calc_track)], DirsFrom[Reverse[c]]);
		}
		if ((track && track->time_outgone[c]
			&& calc_track >= number(0, MUD::Skill(ESkill::kTrack).difficulty))
			|| (!track && calc_track < number(0, MUD::Skill(ESkill::kTrack).difficulty))) {
			found = true;
			SET_BIT(ch->track_dirs, 1 << c);
			sprintf(buf + strlen(buf), "- %s следы ведут %s\r\n",
					track_when[age_track
						(ch,
						 track ? track->
							 time_outgone[c] : (1 << number(0, 25)), calc_track)], DirsTo[c]);
		}
	}

	if (!found) {
		sprintf(buf, "След неожиданно оборвался.\r\n");
	}
	SendMsgToChar(buf, ch);
}

void do_hidetrack(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	struct TrackData *track[EDirection::kMaxDirNum + 1], *temp;
	int percent, prob, i, croom, found = false, dir, rdir;

	if (ch->IsNpc() || !ch->GetSkill(ESkill::kHideTrack)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	croom = ch->in_room;

	for (dir = 0; dir < EDirection::kMaxDirNum; dir++) {
		track[dir] = nullptr;
		rdir = Reverse[dir];
		if (EXITDATA(croom, dir) &&
			EXITDATA(EXITDATA(croom, dir)->to_room(), rdir) &&
			EXITDATA(EXITDATA(croom, dir)->to_room(), rdir)->to_room() == croom) {
			for (temp = world[EXITDATA(croom, dir)->to_room()]->track; temp; temp = temp->next)
				if (!IS_SET(temp->track_info, TRACK_NPC)
					&& GET_IDNUM(ch) == temp->who && !IS_SET(temp->track_info, TRACK_HIDE)
					&& IS_SET(temp->time_outgone[rdir], 3)) {
					found = true;
					track[dir] = temp;
					break;
				}
		}
	}

	track[EDirection::kMaxDirNum] = nullptr;
	for (temp = world[ch->in_room]->track; temp; temp = temp->next)
		if (!IS_SET(temp->track_info, TRACK_NPC) &&
			GET_IDNUM(ch) == temp->who && !IS_SET(temp->track_info, TRACK_HIDE)) {
			found = true;
			track[EDirection::kMaxDirNum] = temp;
			break;
		}

	if (!found) {
		SendMsgToChar("Вы не видите своих следов.\r\n", ch);
		return;
	}
	if (!check_moves(ch, CanUseFeat(ch, EFeat::kStealthy) ? kHidetrackMoves / 2 : kHidetrackMoves))
		return;
	percent = number(1, MUD::Skill(ESkill::kHideTrack).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kHideTrack, nullptr);
	if (percent > prob) {
		SendMsgToChar("Вы безуспешно попытались замести свои следы.\r\n", ch);
		if (!number(0, 25 - IsTimedBySkill(ch, ESkill::kHideTrack) ? 0 : 15))
			ImproveSkill(ch, ESkill::kHideTrack, false, nullptr);
	} else {
		SendMsgToChar("Вы успешно замели свои следы.\r\n", ch);
		if (!number(0, 25 - IsTimedBySkill(ch, ESkill::kHideTrack) ? 0 : 15))
			ImproveSkill(ch, ESkill::kHideTrack, true, nullptr);
		prob -= percent;
		for (i = 0; i <= EDirection::kMaxDirNum; i++)
			if (track[i]) {
				if (i < EDirection::kMaxDirNum)
					track[i]->time_outgone[Reverse[i]] <<= MIN(31, prob);
				else
					for (rdir = 0; rdir < EDirection::kMaxDirNum; rdir++) {
						track[i]->time_income[rdir] <<= MIN(31, prob);
						track[i]->time_outgone[rdir] <<= MIN(31, prob);
					}
				//sprintf(buf,"Заметены следы %d\r\n",i);
				//SendMsgToChar(buf,ch);
			}
	}
	SetWaitState(ch, kRealSec / 2);
	for (i = 0; i <= EDirection::kMaxDirNum; i++)
		if (track[i])
			SET_BIT(track[i]->track_info, TRACK_HIDE);
}
