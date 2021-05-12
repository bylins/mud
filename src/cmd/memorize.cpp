#include "memorize.h"

#include "spells.info.h"
#include "handler.h"
#include "im.h"
#include "screen.h"
#include "limits.hpp"
#include "classes/spell.slots.hpp"
#include "spells.h"

#include "magic.utils.hpp" //включен ради функци поиска спеллов, которые по-хорошеиу должны быть где-то в утилитах.

using PlayerClass::slot_for_char;

void show_wizdom(CHAR_DATA *ch, int bitset);
void do_memorize(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);

void do_memorize(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  char *s;
  int spellnum;

  // get: blank, spell name, target name
  if (!argument || !(*argument)) {
    show_wizdom(ch, 0x07);
    return;
  }
  if (IS_IMMORTAL(ch)) {
    send_to_char("Господи, хоть ты не подкалывай!\r\n", ch);
    return;
  }
  s = strtok(argument, "'*!");
  if (s == NULL) {
    send_to_char("Какое заклинание вы хотите заучить?\r\n", ch);
    return;
  }
  s = strtok(NULL, "'*!");
  if (s == NULL) {
    send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
    return;
  }
  spellnum = fix_name_and_find_spell_num(s);

  if (spellnum < 1 || spellnum > MAX_SPELLS) {
    send_to_char("И откуда вы набрались таких выражений?\r\n", ch);
    return;
  }
  // Caster is lower than spell level
  if (GET_LEVEL(ch) < MIN_CAST_LEV(spell_info[spellnum], ch)
      || GET_REMORT(ch) < MIN_CAST_REM(spell_info[spellnum], ch)
      || slot_for_char(ch, spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0) {
    send_to_char("Рано еще вам бросаться такими словами!\r\n", ch);
    return;
  };
  if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP)) {
    send_to_char("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
    return;
  }
  MemQ_remember(ch, spellnum);
  return;
}

void show_wizdom(CHAR_DATA *ch, int bitset) {
  char names[MAX_SLOT][MAX_STRING_LENGTH];
  int slots[MAX_SLOT], i, max_slot, count, slot_num, is_full, gcount = 0, imax_slot = 0;
  for (i = 1; i <= MAX_SLOT; i++) {
    *names[i - 1] = '\0';
    slots[i - 1] = 0;
    if (slot_for_char(ch, i))
      imax_slot = i;
  }
  if (bitset & 0x01) {
    is_full = 0;
    for (i = 1, max_slot = 0; i <= MAX_SPELLS; i++) {
      if (!GET_SPELL_TYPE(ch, i))
        continue;
      if (!spell_info[i].name || *spell_info[i].name == '!')
        continue;
      count = GET_SPELL_MEM(ch, i);
      if (IS_IMMORTAL(ch))
        count = 10;
      if (!count)
        continue;
      slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
      max_slot = MAX(slot_num, max_slot);
      slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
                                 "%2s|[%2d] %-31s|",
                                 slots[slot_num] % 80 <
                                     10 ? "\r\n" : "  ", count, spell_info[i].name);
      is_full++;
    };

    gcount +=
        sprintf(buf2 + gcount, "  %sВы знаете следующие заклинания :%s", KICYN, KNRM);
    if (is_full) {
      for (i = 0; i < max_slot + 1; i++) {
        if (slots[i]) {
          gcount += sprintf(buf2 + gcount, "\r\nКруг %d", i + 1);
          gcount += sprintf(buf2 + gcount, "%s", names[i]);
        }
      }
    } else {
      gcount += sprintf(buf2 + gcount, "\r\nСейчас у вас нет заученных заклинаний.");
    }
    gcount += sprintf(buf2 + gcount, "\r\n");
  }
  if (bitset & 0x02) {
    struct spell_mem_queue_item *q;
    char timestr[16];
    is_full = 0;
    for (i = 0; i < MAX_SLOT; i++) {
      *names[i] = '\0';
      slots[i] = 0;
    }

    if (!MEMQUEUE_EMPTY(ch)) {
      unsigned char cnt[MAX_SPELLS + 1];
      memset(cnt, 0, MAX_SPELLS + 1);
      timestr[0] = 0;
      if (!IS_MANA_CASTER(ch)) {
        int div, min, sec;
        div = mana_gain(ch);
        if (div > 0) {
          sec = MAX(0, 1 + GET_MEM_CURRENT(ch) - GET_MEM_COMPLETED(ch));    // sec/div -- время мема в мин
          sec = sec * 60 / div;    // время мема в сек
          min = sec / 60;
          sec %= 60;
          if (min > 99)
            sprintf(timestr, "&g%5d&n", min);
          else
            sprintf(timestr, "&g%2d:%02d&n", min, sec);
        } else {
          sprintf(timestr, "&r    -&n");
        }
      }

      for (q = ch->MemQueue.queue; q; q = q->link) {
        ++cnt[q->spellnum];
      }

      for (q = ch->MemQueue.queue; q; q = q->link) {
        i = q->spellnum;
        if (cnt[i] == 0)
          continue;
        slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
        slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
                                   "%2s|[%2d] %-26s%5s|",
                                   slots[slot_num] % 80 <
                                       10 ? "\r\n" : "  ", cnt[i],
                                   spell_info[i].name, q == ch->MemQueue.queue ? timestr : "");
        cnt[i] = 0;
      }

      gcount +=
          sprintf(buf2 + gcount,
                  "  %sВы запоминаете следующие заклинания :%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
      for (i = 0; i < imax_slot; i++) {
        if (slots[i]) {
          gcount += sprintf(buf2 + gcount, "\r\nКруг %d", i + 1);
          gcount += sprintf(buf2 + gcount, "%s", names[i]);
        }
      }
    } else
      gcount += sprintf(buf2 + gcount, "\r\nВы ничего не запоминаете.");
    gcount += sprintf(buf2 + gcount, "\r\n");
  }

  if ((bitset & 0x04) && imax_slot) {
    int *s = MemQ_slots(ch);
    gcount += sprintf(buf2 + gcount, "  %sСвободно :%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    for (i = 0; i < imax_slot; i++) {
      slot_num = MAX(0, slot_for_char(ch, i + 1) - s[i]);
      gcount += sprintf(buf2 + gcount, "%s%2d-%2d%s  ",
                        slot_num ? CCICYN(ch, C_NRM) : "",
                        i + 1, slot_num, slot_num ? CCNRM(ch, C_NRM) : "");
    }
    sprintf(buf2 + gcount, "\r\n");
  }
  //page_string(ch->desc, buf2, 1);
  send_to_char(buf2, ch);
}
