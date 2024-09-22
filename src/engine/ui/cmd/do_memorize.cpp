#include "do_memorize.h"

#include "gameplay/magic/spells_info.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "gameplay/classes/classes_spell_slots.h"
#include "gameplay/magic/magic_utils.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/game_limits.h"

#include <third_party_libs/fmt/include/fmt/format.h>

using classes::CalcCircleSlotsAmount;

void show_wizdom(CharData *ch, int bitset);

void do_memorize(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char *s;

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
		SendMsgToChar("Название заклинания должно быть заключено в символы : * или !\r\n", ch);
		return;
	}
	auto spell_id = FixNameAndFindSpellId(s);
	if (spell_id == ESpell::kUndefined) {
		SendMsgToChar("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}
	const auto spell = MUD::Class(ch->GetClass()).spells[spell_id];
	if (GetRealLevel(ch) < CalcMinSpellLvl(ch, spell_id)
		|| GetRealRemort(ch) < spell.GetMinRemort()
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
	int slot_num;
	std::map<int, std::list<ESpell>> list_spell;
	std::stringstream ss;

	if (bitset & 0x01) {
		int count = 0;

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
			list_spell[slot_num].push_back(spell_id);
		}
		if (!list_spell.empty()) {
			ss << "  &CВы знаете следующие заклинания :&n";
			for (auto i : list_spell) {
				int columns = 0;
				ss << "\r\nКруг: " << i.first + 1;
				for (auto it : list_spell[i.first]) {
					ss << fmt::format("{}|[{:>2}] {:<35}|", ((++columns % 2) ? "\r\n" : " "), GET_SPELL_MEM(ch, it), MUD::Spell(it).GetCName());
				}
				if (ss.str().back() == '\n')
					ss.seekp(-2, std::ios_base::end);
			}
		} else {
			ss << "Сейчас у вас нет заученных заклинаний.";
		}
		ss << "\r\n";
	}
	if (bitset & 0x02) {
		struct SpellMemQueueItem *q;
		char timestr[16];

		list_spell.clear();
		if (!ch->mem_queue.Empty()) {
			ESpell cnt [to_underlying(ESpell::kLast) + 1];
			for (int i = 0; i < to_underlying(ESpell::kLast) + 1; i++)
				cnt[i] = ESpell::kUndefined;
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
						snprintf(timestr, sizeof(timestr), "&g%2d:%02d&n", min, sec);
				} else {
					sprintf(timestr, "&r    -&n");
				}
			}

			for (q = ch->mem_queue.queue; q; q = q->next) {
				++cnt[to_underlying(q->spell_id)];
			}

			for (q = ch->mem_queue.queue; q; q = q->next) {
				auto spell_id = q->spell_id;

				slot_num = MUD::Class(ch->GetClass()).spells[spell_id].GetCircle() - 1;
				list_spell[slot_num].push_back(spell_id);
			}
			if (!list_spell.empty()) {	
				ss << "  &cВы запоминаете следующие заклинания :&n";
				for (auto i : list_spell) {
					int columns = 0;
					std::map<ESpell, int> spell_count;
				
					for (auto it : list_spell[i.first]) {
						spell_count[it]++;
					}
					
					ss << "\r\nКруг: " << i.first + 1;
					for (auto it : spell_count) {
						ss << fmt::format("{}|[{:>2}] {:<35} {:<5}|", ((++columns % 2) ? "\r\n" : " "), 
								it.second, MUD::Spell(it.first).GetCName(), (ch->mem_queue.queue->spell_id == it.first ? timestr : ""));
					}
					if (ss.str().back() == '\n')
						ss.seekp(-2, std::ios_base::end);
				}
			}
		} else {
			ss << "\r\nВы ничего не запоминаете.";
		}
		ss << "\r\n";
	}

	int imax_slot = 0;

	for (int i = 1; i <= kMaxMemoryCircle; i++) {
		if (CalcCircleSlotsAmount(ch, i))
			imax_slot = i;
	}
	if ((bitset & 0x04) && imax_slot) {
		int *s = MemQ_slots(ch);
		ss << "  &CСвободно :&n\r\n";
		for (int i = 0; i < imax_slot; i++) {
			slot_num = std::max(0, CalcCircleSlotsAmount(ch, i + 1) - s[i]);
			ss << fmt::format("{}{:>2}-{:>2}{}  ",
					(slot_num ? kColorBoldCyn : ""), i + 1, slot_num, (slot_num ? kColorNrm : ""));
		}
		ss << "\r\n";
	}
	SendMsgToChar(ss.str(), ch);
}
