/**
\authors Created by Sventovit
\date 02.05.2022.
\brief Функции и структуры очереди заучивания заклинаний
\details Весь код по работе с очередью заучивания заклинаний должен располагаться в данном модуле.
*/

#include "mem_queue.h"

#include "color.h"
#include "entities/char_data.h"
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

float druid_manacost_modifier[] {
	9.875,	
	9.750,	
	9.625,	
	9.500,	
	9.375,	
	9.250,	
	9.125,	
	9.000,
	8.875,	
	8.750,	
	8.625,	
	8.500,	
	8.375,	
	8.250,	
	8.125,	
	8.000,
	7.875,	
	7.750,	
	7.625,	
	7.500,	
	7.375,	
	7.250,	
	7.125,	
	7.000,
	6.750,	//25
	6.640,	
	6.530,	
	6.420,	
	6.310,	
	6.200,	
	6.090,	
	5.980,	
	5.870,	
	5.760,	
	5.650,	
	5.540,	
	5.430,	
	5.320,	
	5.210,	
	5.100,	
	4.990,	
	4.880,	
	4.770,	
	4.660,	
	4.550,	
	4.440,	
	4.330,	
	4.220,	
	4.110,	
	4.0,	//50
	3.98,	
	3.96,	
	3.94,	
	3.92,	
	3.90,	
	3.88,	
	3.86,	
	3.84,	
	3.82,	
	3.80,	
	3.78,	
	3.76,	
	3.74,	
	3.72,	
	3.70,	
	3.68,	
	3.66,	
	3.64,	
	3.62,	
	3.60,	
	3.58,	
	3.56,	
	3.54,	
	3.52,	
	3.50,	//75
	3.48,	
	3.46,	
	3.44,	
	3.42,	
	3.40,	
	3.38,	
	3.36,	
	3.34,	
	3.32,	
	3.30,	
	3.28,	
	3.26,	
	3.24,	
	3.22,	
	3.20,	
	3.18,	
	3.16,	
	3.14,	
	3.12,	
	3.10,	
	3.08,	
	3.06,	
	3.04,	
	3.02,	
	3.0	//100
};

int CalcSpellManacost(CharData *ch, ESpell spell_id) {
	int result = 0;

	if (IS_IMMORTAL(ch)) {
		return 1;
	}
	if (IS_MANA_CASTER(ch) && GetRealLevel(ch) >= MagusCastRequiredLevel(ch, spell_id)) {
		result = static_cast<int>(druid_manacost_modifier[GetRealInt(ch)]
			* std::max(MUD::Spell(spell_id).GetMaxMana() 
					- (MUD::Spell(spell_id).GetManaChange() * (GetRealLevel(ch) - CalcMinRuneSpellLvl(ch, spell_id))),  
				MUD::Spell(spell_id).GetMinMana()));

			int tmp =  std::max(MUD::Spell(spell_id).GetMaxMana() 
					- (MUD::Spell(spell_id).GetManaChange() * (GetRealLevel(ch) - CalcMinRuneSpellLvl(ch, spell_id))),  
				MUD::Spell(spell_id).GetMinMana());
		ch->send_to_TC(false, true, true, "&MМакс манна %d, маначенж %d minrunespelllevel %d&n\r\n", MUD::Spell(spell_id).GetMaxMana(),
				MUD::Spell(spell_id).GetManaChange(), CalcMinRuneSpellLvl(ch, spell_id));
		ch->send_to_TC(false, true, true, "&MПотрачено манны множитель %f  затраты %d всего = %d&n\r\n", druid_manacost_modifier[GetRealInt(ch)], tmp, result );
//		log("manacost Макс манна %d, маначенж %d minrunespelllevel %d\r\n", MUD::Spell(spell_id).GetMaxMana(),
//				MUD::Spell(spell_id).GetManaChange(), CalcMinRuneSpellLvl(ch, spell_id));
//		log("manacost Потрачено манны множитель %f  затраты %d всего = %d\r\n", druid_manacost_modifier[GetRealInt(ch)], tmp, result );

	} else {
		if (!IS_MANA_CASTER(ch)
					&& GetRealLevel(ch) >= CalcMinSpellLvl(ch, spell_id)
					&& GetRealRemort(ch) >= MUD::Class(ch->GetClass()).spells[spell_id].GetMinRemort()) {
			result = std::max(MUD::Spell(spell_id).GetMaxMana() 
					- (MUD::Spell(spell_id).GetManaChange() * (GetRealLevel(ch) - CalcMinSpellLvl(ch, spell_id))),
				MUD::Spell(spell_id).GetMinMana());
			auto class_mem_mod = MUD::Class(ch->GetClass()).spells[spell_id].GetMemMod();
			if (class_mem_mod < 0) {
				result = result*(100 - abs(class_mem_mod))/100;
			} else {
				result = result*100/(100 - abs(class_mem_mod));
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
	ch->mem_queue.stored = 0;
	ch->mem_queue.total = 0;
	ch->mem_queue.queue = nullptr;
}

void MemQ_flush(CharData *ch) {
	struct SpellMemQueueItem *i;
	while (ch->mem_queue.queue) {
		i = ch->mem_queue.queue;
		ch->mem_queue.queue = i->next;
		free(i);
	}
	MemQ_init(ch);
}

ESpell MemQ_learn(CharData *ch) {
	SpellMemQueueItem *i;
	if (ch->mem_queue.Empty()) {
		return ESpell::kUndefined;
	}
	auto num = GET_MEM_CURRENT(ch);
	ch->mem_queue.stored -= num;
	ch->mem_queue.total -= num;
	auto spell_id = ch->mem_queue.queue->spell_id;
	i = ch->mem_queue.queue;
	ch->mem_queue.queue = i->next;
	free(i);
	sprintf(buf, "Вы выучили заклинание \"%s%s%s\".\r\n",
			CCICYN(ch, C_NRM), MUD::Spell(spell_id).GetCName(), CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);
	return spell_id;
}

void MemQ_remember(CharData *ch, ESpell spell_id) {
	int *slots;
	int slotcnt, slotn;
	struct SpellMemQueueItem *i, **pi = &ch->mem_queue.queue;

	// проверить количество слотов
	slots = MemQ_slots(ch);
	slotn = MUD::Class(ch->GetClass()).spells[spell_id].GetCircle() - 1;
	slotcnt = classes::CalcCircleSlotsAmount(ch, slotn + 1);
	slotcnt -= slots[slotn];    // кол-во свободных слотов

	if (slotcnt <= 0) {
		SendMsgToChar("У вас нет свободных ячеек этого круга.", ch);
		return;
	}

	if (GET_RELIGION(ch) == kReligionMono)
		sprintf(buf, "Вы дописали заклинание \"%s%s%s\" в свой часослов.\r\n",
				CCIMAG(ch, C_NRM), MUD::Spell(spell_id).GetCName(), CCNRM(ch, C_NRM));
	else
		sprintf(buf, "Вы занесли заклинание \"%s%s%s\" в свои резы.\r\n",
				CCIMAG(ch, C_NRM), MUD::Spell(spell_id).GetCName(), CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);

	ch->mem_queue.total += CalcSpellManacost(ch, spell_id);
	while (*pi)
		pi = &((*pi)->next);
	CREATE(i, 1);
	*pi = i;
	i->spell_id = spell_id;
	i->next = nullptr;
}

void MemQ_forget(CharData *ch, ESpell spell_id) {
	struct SpellMemQueueItem **q = nullptr, **i;

	for (i = &ch->mem_queue.queue; *i; i = &(i[0]->next)) {
		if (i[0]->spell_id == spell_id)
			q = i;
	}

	if (q == nullptr) {
		SendMsgToChar("Вы и не собирались заучить это заклинание.\r\n", ch);
	} else {
		struct SpellMemQueueItem *ptr;
		if (q == &ch->mem_queue.queue)
			ch->mem_queue.stored = 0;
		ch->mem_queue.total = std::max(0, ch->mem_queue.total - CalcSpellManacost(ch, spell_id));
		ptr = q[0];
		q[0] = q[0]->next;
		free(ptr);
		sprintf(buf,
				"Вы вычеркнули заклинание \"%s%s%s\" из списка для запоминания.\r\n",
				CCIMAG(ch, C_NRM), MUD::Spell(spell_id).GetCName(), CCNRM(ch, C_NRM));
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
		sloti = MUD::Class(ch->GetClass()).spells[spell_id].GetCircle() - 1;
		if (CalcMinSpellLvl(ch, spell_id) > GetRealLevel(ch) ||
			MUD::Class(ch->GetClass()).spells[spell_id].GetMinRemort() > GetRealRemort(ch)) {
			GET_SPELL_MEM(ch, spell_id) = 0;
			continue;
		}
		slots[sloti] -= spell_mem;
		if (slots[sloti] < 0) {
			GET_SPELL_MEM(ch, spell_id) += slots[sloti];
			slots[sloti] = 0;
		}

	}

	for (q = &ch->mem_queue.queue; q[0];) {
		sloti = MUD::Class(ch->GetClass()).spells[q[0]->spell_id].GetCircle() - 1;
		if (sloti >= 0 && sloti <= 10) {
			--slots[sloti];
			if (slots[sloti] >= 0 && CalcMinSpellLvl(ch, q[0]->spell_id) <= GetRealLevel(ch) &&
				MUD::Class(ch->GetClass()).spells[q[0]->spell_id].GetMinRemort() <= GetRealRemort(ch)) {
				q = &(q[0]->next);
			} else {
				if (q == &ch->mem_queue.queue)
					ch->mem_queue.stored = 0;
				ch->mem_queue.total = std::max(0, ch->mem_queue.total - CalcSpellManacost(ch, q[0]->spell_id));
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
	Clear();
}

void SpellMemQueue::Clear() {
	if (Empty()) {
		return;
	}

	while (queue->next) {
		auto item = queue->next;
		queue->next = item->next;
		free(item);
	}
	free(queue);

	queue = nullptr;
	stored = 0;
	total = 0;
}

void forget_all_spells(CharData *ch) {
	using classes::CalcCircleSlotsAmount;

	ch->mem_queue.stored = 0;
	int slots[kMaxMemoryCircle];
	int max_slot = 0;
	for (unsigned i = 0; i < kMaxMemoryCircle; ++i) {
		slots[i] = CalcCircleSlotsAmount(ch, i + 1);
		if (slots[i]) max_slot = i + 1;
	}
	struct SpellMemQueueItem *qi_cur, **qi = &ch->mem_queue.queue;
	while (*qi) {
		--slots[MUD::Class(ch->GetClass()).spells[(*(qi))->spell_id].GetCircle() - 1];
		qi = &((*qi)->next);
	}
	int slotn;

	for (auto spell_id = ESpell::kFirst ; spell_id <= ESpell::kLast; ++spell_id) {
		if (ch->IsFlagged(EPrf::kAutomem) && GET_SPELL_MEM(ch, spell_id)) {
			slotn = MUD::Class(ch->GetClass()).spells[spell_id].GetCircle() - 1;
			for (unsigned j = 0; (slots[slotn] > 0 && j < GET_SPELL_MEM(ch, spell_id)); ++j, --slots[slotn]) {
				ch->mem_queue.total += CalcSpellManacost(ch, spell_id);
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

