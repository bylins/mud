// обслуживание функций езды на всяческих жовтоне
//
#include "mount.h"

#include "cmd/follow.h"
#include "handler.h"

void make_horse(CHAR_DATA *horse, CHAR_DATA *ch) {
  AFF_FLAGS(horse).set(EAffectFlag::AFF_HORSE);
  ch->add_follower(horse);
  MOB_FLAGS(horse).unset(MOB_WIMPY);
  MOB_FLAGS(horse).unset(MOB_SENTINEL);
  MOB_FLAGS(horse).unset(MOB_HELPER);
  MOB_FLAGS(horse).unset(MOB_AGGRESSIVE);
  MOB_FLAGS(horse).unset(MOB_MOUNTING);
  AFF_FLAGS(horse).unset(EAffectFlag::AFF_TETHERED);
}

void do_horseon(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  CHAR_DATA *horse;

  if (IS_NPC(ch))
    return;

  if (!ch->get_horse()) {
    send_to_char("У вас нет скакуна.\r\n", ch);
    return;
  }

  if (ch->ahorse()) {
    send_to_char("Не пытайтесь усидеть на двух стульях.\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  if (*arg)
    horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  else
    horse = ch->get_horse();

  if (horse == NULL)
    send_to_char(NOPERSON, ch);
  else if (IN_ROOM(horse) != ch->in_room)
    send_to_char("Ваш скакун далеко от вас.\r\n", ch);
  else if (!IS_HORSE(horse))
    send_to_char("Это не скакун.\r\n", ch);
  else if (horse->get_master() != ch)
    send_to_char("Это не ваш скакун.\r\n", ch);
  else if (GET_POS(horse) < POS_FIGHTING)
    act("$N не сможет вас нести в таком состоянии.", FALSE, ch, 0, horse, TO_CHAR);
  else if (AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED))
    act("Вам стоит отвязать $N3.", FALSE, ch, 0, horse, TO_CHAR);
    //чтоб не вскакивали в ванрумах
  else if (ROOM_FLAGGED(ch->in_room, ROOM_TUNNEL))
    send_to_char("Слишком мало места.\r\n", ch);
  else if (ROOM_FLAGGED(ch->in_room, ROOM_NOHORSE))
    act("$Z $N взбрыкнул$G и отказал$U вас слушаться.", FALSE, ch, 0, horse, TO_CHAR);
  else {
    if (affected_by_spell(ch, SPELL_SNEAK))
      affect_from_char(ch, SPELL_SNEAK);
    if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
      affect_from_char(ch, SPELL_CAMOUFLAGE);
    act("Вы взобрались на спину $N1.", FALSE, ch, 0, horse, TO_CHAR);
    act("$n вскочил$g на $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
    AFF_FLAGS(ch).set(EAffectFlag::AFF_HORSE);
  }
}

void do_horseoff(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
  CHAR_DATA *horse;

  if (IS_NPC(ch))
    return;
  if (!(horse = ch->get_horse())) {
    send_to_char("У вас нет скакуна.\r\n", ch);
    return;
  }

  if (!ch->ahorse()) {
    send_to_char("Вы ведь и так не на лошади.", ch);
    return;
  }
  ch->dismount();

}

void do_horseget(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  CHAR_DATA *horse;

  if (IS_NPC(ch))
    return;

  if (!ch->get_horse()) {
    send_to_char("У вас нет скакуна.\r\n", ch);
    return;
  }

  if (ch->ahorse()) {
    send_to_char("Вы уже сидите на скакуне.\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  if (*arg)
    horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  else
    horse = ch->get_horse();

  if (horse == NULL)
    send_to_char(NOPERSON, ch);
  else if (IN_ROOM(horse) != ch->in_room)
    send_to_char("Ваш скакун далеко от вас.\r\n", ch);
  else if (!IS_HORSE(horse))
    send_to_char("Это не скакун.\r\n", ch);
  else if (horse->get_master() != ch)
    send_to_char("Это не ваш скакун.\r\n", ch);
  else if (!AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED))
    act("А $N и не привязан$A.", FALSE, ch, 0, horse, TO_CHAR);
  else {
    act("Вы отвязали $N3.", FALSE, ch, 0, horse, TO_CHAR);
    act("$n отвязал$g $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
    AFF_FLAGS(horse).unset(EAffectFlag::AFF_TETHERED);
  }
}

void do_horseput(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  CHAR_DATA *horse;

  if (IS_NPC(ch))
    return;
  if (!ch->get_horse()) {
    send_to_char("У вас нет скакуна.\r\n", ch);
    return;
  }

  if (ch->ahorse()) {
    send_to_char("Вам стоит слезть со скакуна.\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  if (*arg)
    horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  else
    horse = ch->get_horse();
  if (horse == NULL)
    send_to_char(NOPERSON, ch);
  else if (IN_ROOM(horse) != ch->in_room)
    send_to_char("Ваш скакун далеко от вас.\r\n", ch);
  else if (!IS_HORSE(horse))
    send_to_char("Это не скакун.\r\n", ch);
  else if (horse->get_master() != ch)
    send_to_char("Это не ваш скакун.\r\n", ch);
  else if (AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED))
    act("А $N уже и так привязан$A.", FALSE, ch, 0, horse, TO_CHAR);
  else {
    act("Вы привязали $N3.", FALSE, ch, 0, horse, TO_CHAR);
    act("$n привязал$g $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
    AFF_FLAGS(horse).set(EAffectFlag::AFF_TETHERED);
  }
}

void do_horsetake(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  CHAR_DATA *horse = NULL;

  if (IS_NPC(ch))
    return;

  if (ch->get_horse()) {
    send_to_char("Зачем вам столько скакунов?\r\n", ch);
    return;
  }

  if (ch->is_morphed()) {
    send_to_char("И как вы собираетесь это проделать без рук и без ног, одними лапами?\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  if (*arg) {
    horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  }

  if (horse == NULL) {
    send_to_char(NOPERSON, ch);
    return;
  } else if (!IS_NPC(horse)) {
    send_to_char("Господи, не чуди...\r\n", ch);
    return;
  }
    // Исправил ошибку не дававшую воровать коняжек. -- Четырь (13.10.10)
  else if (!IS_GOD(ch)
      && !MOB_FLAGGED(horse, MOB_MOUNTING)
      && !(horse->has_master()
          && AFF_FLAGGED(horse, EAffectFlag::AFF_HORSE))) {
    act("Вы не сможете оседлать $N3.", FALSE, ch, 0, horse, TO_CHAR);
    return;
  } else if (ch->get_horse()) {
    if (ch->get_horse() == horse)
      act("Не стоит седлать $S еще раз.", FALSE, ch, 0, horse, TO_CHAR);
    else
      send_to_char("Вам не усидеть сразу на двух скакунах.\r\n", ch);
    return;
  } else if (GET_POS(horse) < POS_STANDING) {
    act("$N не сможет стать вашим скакуном.", FALSE, ch, 0, horse, TO_CHAR);
    return;
  } else if (IS_HORSE(horse)) {
    if (!IS_IMMORTAL(ch)) {
      send_to_char("Это не ваш скакун.\r\n", ch);
      return;
    }
  }
  if (stop_follower(horse, SF_EMPTY))
    return;
  act("Вы оседлали $N3.", FALSE, ch, 0, horse, TO_CHAR);
  act("$n оседлал$g $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
  make_horse(horse, ch);
}

void do_givehorse(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  CHAR_DATA *horse, *victim;

  if (IS_NPC(ch))
    return;

  if (!(horse = ch->get_horse())) {
    send_to_char("Да нету у вас скакуна.\r\n", ch);
    return;
  }
  if (!ch->has_horse(true)) {
    send_to_char("Ваш скакун далеко от вас.\r\n", ch);
    return;
  }
  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Кому вы хотите передать скакуна?\r\n", ch);
    return;
  }
  if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    send_to_char("Вам некому передать скакуна.\r\n", ch);
    return;
  } else if (IS_NPC(victim)) {
    send_to_char("Он и без этого обойдется.\r\n", ch);
    return;
  }
  if (victim->get_horse()) {
    act("У $N1 уже есть скакун.\r\n", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  if (ch->ahorse()) {
    send_to_char("Вам стоит слезть со скакуна.\r\n", ch);
    return;
  }
  if (AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED)) {
    send_to_char("Вам стоит прежде отвязать своего скакуна.\r\n", ch);
    return;
  }
  // Долбанные умертвия при передаче рассыпаются и весело роняют мад на проходе по последователям чара -- Krodo
  if (stop_follower(horse, SF_EMPTY))
    return;
  act("Вы передали своего скакуна $N2.", FALSE, ch, 0, victim, TO_CHAR);
  act("$n передал$g вам своего скакуна.", FALSE, ch, 0, victim, TO_VICT);
  act("$n передал$g своего скакуна $N2.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
  make_horse(horse, victim);
}

void do_stophorse(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
  CHAR_DATA *horse;

  if (IS_NPC(ch))
    return;

  if (!(horse = ch->get_horse())) {
    send_to_char("Да нету у вас скакуна.\r\n", ch);
    return;
  }
  if (!ch->has_horse(true)) {
    send_to_char("Ваш скакун далеко от вас.\r\n", ch);
    return;
  }
  if (ch->ahorse()) {
    send_to_char("Вам стоит слезть со скакуна.\r\n", ch);
    return;
  }
  if (AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED)) {
    send_to_char("Вам стоит прежде отвязать своего скакуна.\r\n", ch);
    return;
  }
  if (stop_follower(horse, SF_EMPTY))
    return;
  act("Вы отпустили $N3.", FALSE, ch, 0, horse, TO_CHAR);
  act("$n отпустил$g $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
  if (GET_MOB_VNUM(horse) == HORSE_VNUM) {
    act("$n убежал$g в свою конюшню.\r\n", FALSE, horse, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
    extract_char(horse, FALSE);
  }
}