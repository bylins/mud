#include <cmd/mercenary.h>
#include <chars/char_player.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <modify.h>
#include <mob_stat.hpp>

int do_social(CHAR_DATA *ch, char *argument);

namespace MERC {
const int BASE_COST = 1000;
CHAR_DATA *findMercboss(int room_rnum) {
  for (const auto tch : world[room_rnum]->people)
    if (GET_MOB_SPEC(tch) == mercenary)
      return tch;
  sprintf(buf1, "[ERROR] MERC::doList - вызвана команда, не найден моб, room %d", room_rnum);
  mudlog(buf1, LogMode::CMP, 1, EOutputStream::SYSLOG, 1);
  return nullptr;
};

void doList(CHAR_DATA *ch, CHAR_DATA *boss, bool isFavList) {
  std::map<int, MERCDATA> *m;
  m = ch->getMercList();
  if (m->empty()) {
    if (ch->get_class() == CLASS_MERCHANT)
      tell_to_char(boss, ch, "Ступай, поначалу, заведи знакомства, потом ко мне приходи.");
    else if (ch->get_class() == CLASS_CHARMMAGE)
      tell_to_char(boss, ch, "Поищи себе марионетку, да потренируйся, а затем ко мне приходи.");
    else if (IS_IMMORTAL(ch))
      tell_to_char(boss, ch, "Не гневайся, боже, но не было у тебя последователей еще.");
    return;
  }
  if (IS_IMMORTAL(ch)) {
    sprintf(buf, "Вот, господи, %s тварей земных, чьим разумом ты владел волею своей:",
            isFavList ? "краткий список" : "полный список");
  } else if (ch->get_class() == CLASS_MERCHANT) {
    sprintf(buf, "%s тех, с кем знакомство ты водишь:",
            isFavList ? "Краткий список" : "Полный список");
  } else if (ch->get_class() == CLASS_CHARMMAGE) {
    sprintf(buf, "Вот %s тварей земных, чьим разумом ты владел с помощью чар колдовских:",
            isFavList ? "краткий список" : "полный список");
  }
  tell_to_char(boss, ch, buf);
  send_to_char(ch,
               " ##   Имя                                                   \r\n"
               "------------------------------------------------------------\r\n");
  std::map<int, MERCDATA>::iterator it = m->begin();
  std::stringstream out;
  std::string format_str = "%3d)  %-54s\r\n";
  std::string mobname;
  for (int num = 1; it != m->end(); ++it, ++num) {
    if (it->second.currRemortAvail == 0)
      continue;
    if (isFavList && it->second.isFavorite == 0)
      continue;
    mobname = mob_stat::print_mob_name(it->first, 54);
    out << boost::str(boost::format(format_str) % num % mobname);
  }
  page_string(ch->desc, out.str());
  send_to_char(ch, "------------------------------------------------------------\r\n");
  sprintf(buf,
          "А всего за %d кун моя ватага приведёт тебе любого из них живым и невредимым.",
          1000 * (ch->get_remort() + 1));
  tell_to_char(boss, ch, buf);
  snprintf(buf, MAX_INPUT_LENGTH, "ухмы %s", GET_NAME(ch));
  do_social(boss, buf);
};

void doStat(CHAR_DATA *ch) {
  if (!ch) return;
  return;
};

void doBring(CHAR_DATA *ch, CHAR_DATA *boss, unsigned int pos, char *bank) {
  CHAR_DATA *mob;
  std::map<int, MERCDATA> *m;
  m = ch->getMercList();
  const int cost = MERC::BASE_COST * (ch->get_remort() + 1);
  mob_rnum rnum;
  std::map<int, MERCDATA>::iterator it = m->begin();
  for (unsigned int num = 1; it != m->end(); ++it, ++num) {
    if (num != pos)
      continue;
    if (it->second.currRemortAvail == 0) {
      sprintf(buf, "Протри глаза, такой персонаж тебе неведом!");
      tell_to_char(boss, ch, buf);
      return;
    }

    if ((!isname(bank, "банк bank") && cost > ch->get_gold()) ||
        (isname(bank, "банк bank") && cost > ch->get_bank())) {
      sprintf(buf, "Мои услуги стоят %d %s - это тебе не по карману.", cost, desc_count(cost, WHAT_MONEYu));
      tell_to_char(boss, ch, buf);
      return;
    }

    if ((rnum = real_mobile(it->first)) < 0) {
      sprintf(buf, "С персонажем стряслось что то, не могу его найти.");
      tell_to_char(boss, ch, buf);
      sprintf(buf1, "[ERROR] MERC::doBring, не найден моб, vnum: %d", it->first);
      mudlog(buf1, LogMode::CMP, 1, EOutputStream::SYSLOG, 1);
      return;
    }
    mob = read_mobile(rnum, REAL);
    char_to_room(mob, ch->in_room);
    if (ch->get_class() == CLASS_CHARMMAGE) {
      act("$n окрикнул$g своих парней и скрыл$u из виду.", TRUE, boss, 0, 0, TO_ROOM);
      act("Спустя некоторое время, взмыленная ватага вернулась, волоча на аркане $n3.", TRUE, mob, 0, 0, TO_ROOM);
    } else {
      act("$n вскочил$g и скрыл$u из виду.", TRUE, boss, 0, 0, TO_ROOM);
      sprintf(buf, "Спустя некоторое время, %s вернул$U, ведя за собой $n3.", boss->get_npc_name().c_str());
      act(buf, TRUE, mob, 0, ch, TO_ROOM);
    }
    if (!WAITLESS(ch)) {
      if (isname(bank, "банк bank"))
        ch->remove_bank(cost);
      else
        ch->remove_gold(cost);
    }
  }
  return;
};

void doForget(CHAR_DATA *ch, CHAR_DATA *boss, unsigned int pos) {
  std::map<int, MERCDATA> *m;
  m = ch->getMercList();
  std::map<int, MERCDATA>::iterator it = m->begin();
  for (unsigned int num = 1; it != m->end(); ++it, ++num) {
    if (num != pos)
      continue;
    if (it->second.currRemortAvail == 0) {
      sprintf(buf, "Протри глаза, такой персонаж тебе неведом!");
      tell_to_char(boss, ch, buf);
      return;
    }
    it->second.isFavorite = !it->second.isFavorite;
    if (it->second.isFavorite)
      sprintf(buf, "Персонаж %s добавлен в список любимчиков.", mob_stat::print_mob_name(it->first, 54).c_str());
    else
      sprintf(buf, "Персонаж %s попал в опалу.", mob_stat::print_mob_name(it->first, 54).c_str());
    tell_to_char(boss, ch, buf);
    return;
  }
  sprintf(buf1, "[ERROR] MERC::doForget, не найден моб, pos: %d", pos);
  mudlog(buf1, LogMode::CMP, 1, EOutputStream::SYSLOG, 1);
  return;
};

unsigned int getPos(char *arg, CHAR_DATA *ch, CHAR_DATA *boss) {
  unsigned int pos = 0;
  std::map<int, MERCDATA> *m;
  m = ch->getMercList();
  try {
    pos = boost::lexical_cast<unsigned>(arg);
  }
  catch (const boost::bad_lexical_cast &) {
    pos = 0;
  }
  if (pos < 1 || pos > m->size() || m->size() == 0) {
    sprintf(buf, "Протри глаза, такой персонаж тебе неведом!");
    tell_to_char(boss, ch, buf);
    return 0;
  }
  return pos;
}

}

int mercenary(CHAR_DATA *ch, void * /*me*/, int cmd, char *argument) {
  if (!ch || !ch->desc || IS_NPC(ch))
    return 0;
  if (!(CMD_IS("наемник") || CMD_IS("mercenary")))
    return 0;

  CHAR_DATA *boss = MERC::findMercboss(ch->in_room);
  if (!boss) return 0;

  if (!IS_IMMORTAL(ch) && ch->get_class() != CLASS_MERCHANT && ch->get_class() != CLASS_CHARMMAGE) {
    tell_to_char(boss, ch, "Эти знания тебе недоступны, ступай с миром");
    return 0;
  }

  char subCmd[MAX_INPUT_LENGTH];
  char cmdParam[MAX_INPUT_LENGTH];
  char bank[MAX_INPUT_LENGTH];
  unsigned int pos;

  three_arguments(argument, subCmd, cmdParam, bank);
  if (is_abbrev(subCmd, "стат") || is_abbrev(subCmd, "stat")) {
    return (1);
  } else if (is_abbrev(subCmd, "список") || is_abbrev(subCmd, "list")) {
    if (is_abbrev(cmdParam, "полный") || is_abbrev(cmdParam, "full"))
      MERC::doList(ch, boss, false);
    else
      MERC::doList(ch, boss, true);
    return (1);
  } else if (is_abbrev(subCmd, "привести") || is_abbrev(subCmd, "bring")) {
    pos = MERC::getPos(cmdParam, ch, boss);
    if (pos == 0) return (1);
    MERC::doBring(ch, boss, pos, bank);
    return (1);
  } else if (is_abbrev(subCmd, "фаворит") || is_abbrev(subCmd, "favorite")) {
    pos = MERC::getPos(cmdParam, ch, boss);
    if (pos == 0) return (1);
    MERC::doForget(ch, boss, pos);
    return (1);
  } else
    tell_to_char(boss, ch, "Доступные команды: список (полный), привести <номер> (банк), фаворит <номер>");
  return (1);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

