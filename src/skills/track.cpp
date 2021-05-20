/* ************************************************************************
*   File: track.cpp                                     Part of Bylins    *
*  ************************************************************************/

#include "track.h"

#include "graph.h"
#include "handler.h"
#include "skills_info.h"

const char *track_when[] = {"совсем свежие",
                            "свежие",
                            "менее полудневной давности",
                            "примерно полудневной давности",
                            "почти дневной давности",
                            "примерно дневной давности",
                            "совсем старые"
};

int age_track(CHAR_DATA * /*ch*/, int time, int calc_track) {
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
int go_track(CHAR_DATA *ch, CHAR_DATA *victim, const ESkill skill_no) {
  int percent, dir;
  int if_sense;

  if (AFF_FLAGGED(victim, EAffectFlag::AFF_NOTRACK) && (skill_no != SKILL_SENSE)) {
    return BFS_ERROR;
  }
  // 101 is a complete failure, no matter what the proficiency.
  //Временная затычка. Перевести на резисты
  //Изменил макс скилл со 100 до 200, чтобы не ломать алгоритм, в данном значении вернем старое значение.
  if_sense = (skill_no == SKILL_SENSE) ? 100 : 0;
  percent = number(0, skill_info[skill_no].difficulty - if_sense);
  //current_skillpercent = GET_SKILL(ch, SKILL_SENSE);
  if ((!IS_NPC(victim)) && (!IS_GOD(ch)) && (!IS_NPC(ch))) //Если цель чар и ищет не бог
  {
    percent = MIN(99, number(0, GET_REMORT(victim)) + percent);
  }
  if (percent > CalculateCurrentSkill(ch, skill_no, victim)) {
    int tries = 10;
    // Find a random direction. :)
    do {
      dir = number(0, NUM_OF_DIRS - 1);
    } while (!CAN_GO(ch, dir) && --tries);
    return dir;
  }

  // They passed the skill check.
  return find_first_step(ch->in_room, victim->in_room, ch);
}

void do_track(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  CHAR_DATA *vict = NULL;
  struct track_data *track;
  int found = FALSE, calc_track = 0, track_t, i;
  char name[MAX_INPUT_LENGTH];

  // The character must have the track skill.
  if (IS_NPC(ch) || !ch->get_skill(SKILL_TRACK)) {
    send_to_char("Но вы не знаете как.\r\n", ch);
    return;
  }

  if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
    send_to_char("Вы слепы как крот.\r\n", ch);
    return;
  }

  if (!check_moves(ch, can_use_feat(ch, TRACKER_FEAT) ? TRACK_MOVES / 2 : TRACK_MOVES))
    return;

  calc_track = CalculateCurrentSkill(ch, SKILL_TRACK, 0);
  act("Похоже, $n кого-то выслеживает.", FALSE, ch, 0, 0, TO_ROOM);
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
        for (track_t = i = 0; i < NUM_OF_DIRS; i++) {
          track_t |= track->time_outgone[i];
          track_t |= track->time_income[i];
        }
        sprintf(buf, "%s : следы %s.\r\n", name,
                track_when[age_track(ch, track_t, calc_track)]);
        send_to_char(buf, ch);
        found = TRUE;
      }
    }
    if (!found)
      send_to_char("Вы не видите ничьих следов.\r\n", ch);
    return;
  }

  if ((vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    act("Вы же в одной комнате с $N4!", FALSE, ch, 0, vict, TO_CHAR);
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

  if (calc_track < number(1, 40) || !*name || ROOM_FLAGGED(ch->in_room, ROOM_NOTRACK)) {
    send_to_char("Вы не видите похожих следов.\r\n", ch);
    return;
  }

  ch->track_dirs = 0;
  CAP(name);
  sprintf(buf, "%s:\r\n", name);

  for (int c = 0; c < NUM_OF_DIRS; c++) {
    if ((track && track->time_income[c]
        && calc_track >= number(0, skill_info[SKILL_TRACK].difficulty))
        || (!track && calc_track < number(0, skill_info[SKILL_TRACK].difficulty))) {
      found = TRUE;
      sprintf(buf + strlen(buf), "- %s следы ведут %s\r\n",
              track_when[age_track
                  (ch,
                   track ? track->
                       time_income[c] : (1 << number(0, 25)), calc_track)], DirsFrom[Reverse[c]]);
    }
    if ((track && track->time_outgone[c]
        && calc_track >= number(0, skill_info[SKILL_TRACK].difficulty))
        || (!track && calc_track < number(0, skill_info[SKILL_TRACK].difficulty))) {
      found = TRUE;
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
  send_to_char(buf, ch);
}

void do_hidetrack(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
  struct track_data *track[NUM_OF_DIRS + 1], *temp;
  int percent, prob, i, croom, found = FALSE, dir, rdir;

  if (IS_NPC(ch) || !ch->get_skill(SKILL_HIDETRACK)) {
    send_to_char("Но вы не знаете как.\r\n", ch);
    return;
  }

  croom = ch->in_room;

  for (dir = 0; dir < NUM_OF_DIRS; dir++) {
    track[dir] = NULL;
    rdir = Reverse[dir];
    if (EXITDATA(croom, dir) &&
        EXITDATA(EXITDATA(croom, dir)->to_room(), rdir) &&
        EXITDATA(EXITDATA(croom, dir)->to_room(), rdir)->to_room() == croom) {
      for (temp = world[EXITDATA(croom, dir)->to_room()]->track; temp; temp = temp->next)
        if (!IS_SET(temp->track_info, TRACK_NPC)
            && GET_IDNUM(ch) == temp->who && !IS_SET(temp->track_info, TRACK_HIDE)
            && IS_SET(temp->time_outgone[rdir], 3)) {
          found = TRUE;
          track[dir] = temp;
          break;
        }
    }
  }

  track[NUM_OF_DIRS] = NULL;
  for (temp = world[ch->in_room]->track; temp; temp = temp->next)
    if (!IS_SET(temp->track_info, TRACK_NPC) &&
        GET_IDNUM(ch) == temp->who && !IS_SET(temp->track_info, TRACK_HIDE)) {
      found = TRUE;
      track[NUM_OF_DIRS] = temp;
      break;
    }

  if (!found) {
    send_to_char("Вы не видите своих следов.\r\n", ch);
    return;
  }
  if (!check_moves(ch, can_use_feat(ch, STEALTHY_FEAT) ? HIDETRACK_MOVES / 2 : HIDETRACK_MOVES))
    return;
  percent = number(1, skill_info[SKILL_HIDETRACK].difficulty);
  prob = CalculateCurrentSkill(ch, SKILL_HIDETRACK, 0);
  if (percent > prob) {
    send_to_char("Вы безуспешно попытались замести свои следы.\r\n", ch);
    if (!number(0, 25 - timed_by_skill(ch, SKILL_HIDETRACK) ? 0 : 15))
      ImproveSkill(ch, SKILL_HIDETRACK, FALSE, 0);
  } else {
    send_to_char("Вы успешно замели свои следы.\r\n", ch);
    if (!number(0, 25 - timed_by_skill(ch, SKILL_HIDETRACK) ? 0 : 15))
      ImproveSkill(ch, SKILL_HIDETRACK, TRUE, 0);
    prob -= percent;
    for (i = 0; i <= NUM_OF_DIRS; i++)
      if (track[i]) {
        if (i < NUM_OF_DIRS)
          track[i]->time_outgone[Reverse[i]] <<= MIN(31, prob);
        else
          for (rdir = 0; rdir < NUM_OF_DIRS; rdir++) {
            track[i]->time_income[rdir] <<= MIN(31, prob);
            track[i]->time_outgone[rdir] <<= MIN(31, prob);
          }
        //sprintf(buf,"Заметены следы %d\r\n",i);
        //send_to_char(buf,ch);
      }
  }

  for (i = 0; i <= NUM_OF_DIRS; i++)
    if (track[i])
      SET_BIT(track[i]->track_info, TRACK_HIDE);
}
