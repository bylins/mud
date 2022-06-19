#include "memorize.h"

#include "game_magic/spells_info.h"
#include "handler.h"
#include "color.h"
#include "game_classes/classes_spell_slots.h"
#include "game_magic/magic_utils.h"
#include "structs/global_objects.h"

using classes::CalcCircleSlotsAmount;

void show_wizdom(CharData *ch, int bitset);

void do_memorize(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char *s;

	// get: blank, spell name, target name
	if (!argument || !(*argument)) {
		show_wizdom(ch, 0x07);
		return;
	}
	if (IS_IMMORTAL(ch)) {
		SendMsgToChar("Господи, хоть ты не подкалывай!\r\n", ch);
		return;
	}
	if (!*argument) {
		SendMsgToChar("Какое заклинание вы хотите заучить?\r\n", ch);
		return;
	}
	s = strtok(argument, "'*!");
	if (!str_cmp(s, argument)) {
		SendMsgToChar("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}

	auto spell_id = FixNameAndFindSpellId(s);
	if (spell_id == ESpell::kUndefined) {
		SendMsgToChar("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	const auto spell = MUD::Class(ch->GetClass()).spells[spell_id];
	if (GetRealLevel(ch) < CalcMinSpellLvl(ch, spell_id)
		|| GET_REAL_REMORT(ch) < spell.GetMinRemort()
		|| CalcCircleSlotsAmount(ch, spell.GetCircle()) <= 0) {
		SendMsgToChar("Рано еще вам бросаться такими словами!\r\n", ch);
		return;
	};
	if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow | ESpellType::kTemp)) {
		SendMsgToChar("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
		return;
	}
	MemQ_remember(ch, spell_id);
}

void show_wizdom(CharData *ch, int bitset) {
	char names[kMaxMemoryCircle][kMaxStringLength];
	int slots[kMaxMemoryCircle], i, max_slot, count, slot_num, is_full, gcount = 0, imax_slot = 0;
	for (i = 1; i <= kMaxMemoryCircle; i++) {
		*names[i - 1] = '\0';
		slots[i - 1] = 0;
		if (CalcCircleSlotsAmount(ch, i))
			imax_slot = i;
	}
	if (bitset & 0x01) {
		is_full = 0;
		max_slot = 0;
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			if (!GET_SPELL_TYPE(ch, spell_id)) {
				continue;
			}
			if (MUD::Spell(spell_id).IsInvalid()) {
				continue;
			}
			count = GET_SPELL_MEM(ch, spell_id);
			if (IS_IMMORTAL(ch))
				count = 10;
			if (!count)
				continue;
			slot_num = MUD::Class(ch->GetClass()).spells[spell_id].GetCircle() - 1;
			max_slot = std::max(slot_num, max_slot);
			slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
					"%2s|[%2d] %-35s|",
					slots[slot_num] % 88 < 10 ? "\r\n" : "  ",
					count,
					MUD::Spell(spell_id).GetCName());
			is_full++;
		};
		gcount += sprintf(buf2 + gcount, "  %sВы знаете следующие заклинания :%s", KICYN, KNRM);
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
		struct SpellMemQueueItem *q;
		char timestr[16];
		is_full = 0;
		for (i = 0; i < kMaxMemoryCircle; i++) {
			*names[i] = '\0';
			slots[i] = 0;
		}

		if (!ch->mem_queue.Empty()) {
			ESpell cnt [to_underlying(ESpell::kLast) + 1];
			memset(cnt, 0, to_underlying(ESpell::kLast) + 1);
			timestr[0] = 0;
			if (!IS_MANA_CASTER(ch)) {
				int div, min, sec;
				div = CalcManaGain(ch);
				if (div > 0) {
					sec = std::max(0, 1 + GET_MEM_CURRENT(ch) - ch->mem_queue.stored);    // sec/div -- время мема в мин
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

			for (q = ch->mem_queue.queue; q; q = q->next) {
				++cnt[to_underlying(q->spell_id)];
			}

			for (q = ch->mem_queue.queue; q; q = q->next) {
				auto spell_id = q->spell_id;
				auto index = to_underlying(spell_id);
				if (cnt[index] == ESpell::kUndefined) {
					continue;
				}
				slot_num = MUD::Class(ch->GetClass()).spells[spell_id].GetCircle() - 1;
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
						"%2s|[%2d] %-30s%5s|",
						slots[slot_num] % 88 < 10 ? "\r\n" : "  ",
						to_underlying(cnt[index]),
						MUD::Spell(spell_id).GetCName(), q == ch->mem_queue.queue ? timestr : "");
				cnt[index] = ESpell::kUndefined;
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
			slot_num = std::max(0, CalcCircleSlotsAmount(ch, i + 1) - s[i]);
			gcount += sprintf(buf2 + gcount, "%s%2d-%2d%s  ",
					slot_num ? CCICYN(ch, C_NRM) : "", i + 1, slot_num, slot_num ? CCNRM(ch, C_NRM) : "");
		}
		sprintf(buf2 + gcount, "\r\n");
	}
	//page_string(ch->desc, buf2, 1);
	SendMsgToChar(buf2, ch);
}
