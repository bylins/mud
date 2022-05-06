/**
\authors Created by Sventovit
\date 02.05.2022.
\brief Функции и структуры очереди заучивания заклинаний
\details Весь код по работе с очередью заучивания заклинаний должен располагаться в данном модуле.
*/

#include "mem_queue.h"

#include "color.h"
#include "game_limits.h"
#include "game_classes/classes_spell_slots.h"
#include "game_magic/magic_utils.h"
#include "game_magic/spells_info.h"
#include "structs/global_objects.h"

const double kManaCostModifier = 0.5;

//	Коэффициент изменения мема относительно скилла магии.
int koef_skill_magic(int percent_skill) {
//	Выделяем процент на который меняется мем
	return ((800 - percent_skill) / 8);

//	return 0;
}

int CalcSpellManacost(const CharData *ch, ESpell spell_id) {
	int result = 0;
	if (IS_IMMORTAL(ch)) {
		return 1;
	}

	if (IS_MANA_CASTER(ch) && GetRealLevel(ch) >= CalcRequiredLevel(ch, spell_id)) {
		result = static_cast<int>(kManaCostModifier
			* (float) mana_gain_cs[VPOSI(55 - GET_REAL_INT(ch), 10, 50)]
			/ (float) int_app[VPOSI(55 - GET_REAL_INT(ch), 10, 50)].mana_per_tic
			* 60
			* std::max(spell_info[spell_id].mana_max
						   - (spell_info[spell_id].mana_change
							   * (GetRealLevel(ch)
								   - spell_create[spell_id].runes.min_caster_level)),
					   spell_info[spell_id].mana_min));
	} else {
		if (!IS_MANA_CASTER(ch) && GetRealLevel(ch) >= CalcMinSpellLvl(ch, spell_id)
			&& GET_REAL_REMORT(ch) >= MUD::Classes(ch->GetClass()).spells[spell_id].GetMinRemort()) {
			result = std::max(spell_info[spell_id].mana_max - (spell_info[spell_id].mana_change *
								  (GetRealLevel(ch) - CalcMinSpellLvl(ch, spell_id))),
							  spell_info[spell_id].mana_min);
			auto class_mem_mod = MUD::Classes(ch->GetClass()).spells[spell_id].GetMemMod();
			if (class_mem_mod < 0) {
				result *= (100 - std::min(99, abs(class_mem_mod)))/100;
			} else {
				result *= 100/(100 - std::min(99, abs(class_mem_mod)));
			}
//		Меняем мем на коэффициент скилла магии
// \todo ABYRVALG Нужно ввести общую для витязя и купца способность, а эту похабень убрать.
			if (ch->GetClass() == ECharClass::kPaladine || ch->GetClass() == ECharClass::kMerchant) {
				return result;
			}
		}
	}
	if (result > 0)
		return result * koef_skill_magic(ch->GetSkill(GetMagicSkillId(spell_id))) / 100;
		// при скилле 200 + 25%, чем меньше тем лучше
	else
		return 99999;
}

void MemQ_init(CharData *ch) {
	ch->mem_queue->stored = 0;
	ch->mem_queue->total = 0;
	ch->mem_queue->queue = nullptr;
}

void MemQ_flush(CharData *ch) {
	struct SpellMemQueueItem *i;
	while (ch->mem_queue->queue) {
		i = ch->mem_queue->queue;
		ch->mem_queue->queue = i->next;
		free(i);
	}
	MemQ_init(ch);
}

ESpell MemQ_learn(CharData *ch) {
	SpellMemQueueItem *i;
	if (ch->mem_queue->Empty()) {
		return ESpell::kUndefined;
	}
	auto num = GET_MEM_CURRENT(ch);
	ch->mem_queue->stored -= num;
	ch->mem_queue->total -= num;
	auto spell_id = ch->mem_queue->queue->spell_id;
	i = ch->mem_queue->queue;
	ch->mem_queue->queue = i->next;
	free(i);
	sprintf(buf, "Вы выучили заклинание \"%s%s%s\".\r\n",
			CCICYN(ch, C_NRM), spell_info[spell_id].name, CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);
	return spell_id;
}

void MemQ_remember(CharData *ch, ESpell spell_id) {
	int *slots;
	int slotcnt, slotn;
	struct SpellMemQueueItem *i, **pi = &ch->mem_queue->queue;

	// проверить количество слотов
	slots = MemQ_slots(ch);
	slotn = MUD::Classes(ch->GetClass()).spells[spell_id].GetCircle() - 1;
	slotcnt = classes::CalcCircleSlotsAmount(ch, slotn + 1);
	slotcnt -= slots[slotn];    // кол-во свободных слотов

	if (slotcnt <= 0) {
		SendMsgToChar("У вас нет свободных ячеек этого круга.", ch);
		return;
	}

	if (GET_RELIGION(ch) == kReligionMono)
		sprintf(buf, "Вы дописали заклинание \"%s%s%s\" в свой часослов.\r\n",
				CCIMAG(ch, C_NRM), spell_info[spell_id].name, CCNRM(ch, C_NRM));
	else
		sprintf(buf, "Вы занесли заклинание \"%s%s%s\" в свои резы.\r\n",
				CCIMAG(ch, C_NRM), spell_info[spell_id].name, CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);

	ch->mem_queue->total += CalcSpellManacost(ch, spell_id);
	while (*pi)
		pi = &((*pi)->next);
	CREATE(i, 1);
	*pi = i;
	i->spell_id = spell_id;
	i->next = nullptr;
}

void MemQ_forget(CharData *ch, ESpell spell_id) {
	struct SpellMemQueueItem **q = nullptr, **i;

	for (i = &ch->mem_queue->queue; *i; i = &(i[0]->next)) {
		if (i[0]->spell_id == spell_id)
			q = i;
	}

	if (q == nullptr) {
		SendMsgToChar("Вы и не собирались заучить это заклинание.\r\n", ch);
	} else {
		struct SpellMemQueueItem *ptr;
		if (q == &ch->mem_queue->queue)
			ch->mem_queue->stored = 0;
		ch->mem_queue->total = std::max(0, ch->mem_queue->total - CalcSpellManacost(ch, spell_id));
		ptr = q[0];
		q[0] = q[0]->next;
		free(ptr);
		sprintf(buf,
				"Вы вычеркнули заклинание \"%s%s%s\" из списка для запоминания.\r\n",
				CCIMAG(ch, C_NRM), spell_info[spell_id].name, CCNRM(ch, C_NRM));
		SendMsgToChar(buf, ch);
	}
}

int *MemQ_slots(CharData *ch) {
	struct SpellMemQueueItem **q, *qt;
	static int slots[kMaxMemoryCircle];

	// инициализация
	for (auto i = 0; i < kMaxMemoryCircle; ++i) {
		slots[i] = classes::CalcCircleSlotsAmount(ch, i + 1);
	}

	// ABYRVALG непонятно, зачем тут цикл через декремент. Поменял на инкремент, надеюсь, глюков не будет
	//for (auto spell_id = ESpell::kLast; spell_id >= ESpell::kFirst; --spell_id) {
	auto sloti{0};
	for (auto spell_id = ESpell::kFirst ; spell_id <= ESpell::kLast; ++spell_id) {
		if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow | ESpellType::kTemp))
			continue;
		auto spell_mem = GET_SPELL_MEM(ch, spell_id);
		if (spell_mem == 0) {
			continue;
		}
		sloti = MUD::Classes(ch->GetClass()).spells[spell_id].GetCircle() - 1;
		if (CalcMinSpellLvl(ch, spell_id) > GetRealLevel(ch) ||
			MUD::Classes(ch->GetClass()).spells[spell_id].GetMinRemort() > GET_REAL_REMORT(ch)) {
			GET_SPELL_MEM(ch, spell_id) = 0;
			continue;
		}
		slots[sloti] -= spell_mem;
		if (slots[sloti] < 0) {
			GET_SPELL_MEM(ch, spell_id) += slots[sloti];
			slots[sloti] = 0;
		}

	}

	for (q = &ch->mem_queue->queue; q[0];) {
		sloti = MUD::Classes(ch->GetClass()).spells[q[0]->spell_id].GetCircle() - 1;
		if (sloti >= 0 && sloti <= 10) {
			--slots[sloti];
			if (slots[sloti] >= 0 && CalcMinSpellLvl(ch, q[0]->spell_id) <= GetRealLevel(ch) &&
				MUD::Classes(ch->GetClass()).spells[q[0]->spell_id].GetMinRemort() <= GET_REAL_REMORT(ch)) {
				q = &(q[0]->next);
			} else {
				if (q == &ch->mem_queue->queue)
					ch->mem_queue->stored = 0;
				ch->mem_queue->total = std::max(0, ch->mem_queue->total - CalcSpellManacost(ch, q[0]->spell_id));
				++slots[sloti];
				qt = q[0];
				q[0] = q[0]->next;
				free(qt);
			}
		}
	}

	for (auto i = 0; i < kMaxMemoryCircle; ++i) {
		slots[i] = classes::CalcCircleSlotsAmount(ch, i + 1) - slots[i];
	}

	return slots;
}

SpellMemQueue::~SpellMemQueue() {
	while (queue->next) {
		auto item = queue->next;
		queue->next = item->next;
		free(item);
	}
	free(queue);
}

void forget_all_spells(CharData *ch) {
	using classes::CalcCircleSlotsAmount;

	ch->mem_queue->stored = 0;
	int slots[kMaxMemoryCircle];
	int max_slot = 0;
	for (unsigned i = 0; i < kMaxMemoryCircle; ++i) {
		slots[i] = CalcCircleSlotsAmount(ch, i + 1);
		if (slots[i]) max_slot = i + 1;
	}
	struct SpellMemQueueItem *qi_cur, **qi = &ch->mem_queue->queue;
	while (*qi) {
		--slots[MUD::Classes(ch->GetClass()).spells[(*(qi))->spell_id].GetCircle() - 1];
		qi = &((*qi)->next);
	}
	int slotn;

	for (auto spell_id = ESpell::kFirst ; spell_id <= ESpell::kLast; ++spell_id) {
		if (PRF_FLAGGED(ch, EPrf::kAutomem) && GET_SPELL_MEM(ch, spell_id)) {
			slotn = MUD::Classes(ch->GetClass()).spells[spell_id].GetCircle() - 1;
			for (unsigned j = 0; (slots[slotn] > 0 && j < GET_SPELL_MEM(ch, spell_id)); ++j, --slots[slotn]) {
				ch->mem_queue->total += CalcSpellManacost(ch, spell_id);
				CREATE(qi_cur, 1);
				*qi = qi_cur;
				qi_cur->spell_id = spell_id;
				qi_cur->next = nullptr;
				qi = &qi_cur->next;
			}
		}
		GET_SPELL_MEM(ch, spell_id) = 0;
	}
	if (max_slot) {
		Affect<EApply> af;
		af.type = ESpell::kRecallSpells;
		af.location = EApply::kNone;
		af.modifier = 1; // номер круга, который восстанавливаем
		//добавим 1 проход про запас, иначе неуспевает отмемиться последний круг -- аффект спадает раньше

		af.duration = CalcDuration(ch, max_slot*kRecallSpellsInterval + kSecsPerPlayerAffect, 0, 0, 0, 0);
		af.bitvector = to_underlying(EAffect::kMemorizeSpells);
		af.battleflag = kAfPulsedec | kAfDeadkeep;
		ImposeAffect(ch, af, false, false, false, false);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

