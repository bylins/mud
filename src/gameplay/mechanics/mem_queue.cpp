/**
\authors Created by Sventovit
\date 02.05.2022.
\brief Функции и структуры очереди заучивания заклинаний
\details Весь код по работе с очередью заучивания заклинаний должен располагаться в данном модуле.
*/

#include "mem_queue.h"

#include "engine/ui/color.h"
#include "engine/entities/char_data.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/classes/classes_spell_slots.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/magic/spells_info.h"
#include "engine/db/global_objects.h"

const double kManaCostModifier = 0.5;

//	Коэффициент изменения мема относительно скилла магии.
int koef_skill_magic(int percent_skill) {
//	Выделяем процент на который меняется мем
	return ((800 - percent_skill) / 8);

//	return 0;
}

float druid_manacost_modifier[] {
	0.f,
	9.875f,	
	9.750f,	
	9.625f,	
	9.500f,	
	9.375f,	
	9.250f,	
	9.125f,	
	9.000f,
	8.875f,	
	8.750f,	
	8.625f,	
	8.500f,	
	8.375f,	
	8.250f,	
	8.125f,	
	8.000f,
	7.875f,	
	7.750f,	
	7.625f,	
	7.500f,	
	7.375f,	
	7.250f,	
	7.125f,	
	7.000f,
	6.750f,	//25
	6.640f,	
	6.530f,	
	6.420f,	
	6.310f,	
	6.200f,	
	6.090f,	
	5.980f,	
	5.870f,	
	5.760f,	
	5.650f,	
	5.540f,	
	5.430f,	
	5.320f,	
	5.210f,	
	5.100f,	
	4.990f,	
	4.880f,	
	4.770f,	
	4.660f,	
	4.550f,	
	4.440f,	
	4.330f,	
	4.220f,	
	4.110f,	
	4.0f,	//50
	3.98f,	
	3.96f,	
	3.94f,	
	3.92f,	
	3.90f,	
	3.88f,	
	3.86f,	
	3.84f,	
	3.82f,	
	3.80f,	
	3.78f,	
	3.76f,	
	3.74f,	
	3.72f,	
	3.70f,	
	3.68f,	
	3.66f,	
	3.64f,	
	3.62f,	
	3.60f,	
	3.58f,	
	3.56f,	
	3.54f,	
	3.52f,	
	3.50f,	//75
	3.48f,	
	3.46f,	
	3.44f,	
	3.42f,	
	3.40f,	
	3.38f,	
	3.36f,	
	3.34f,	
	3.32f,	
	3.30f,	
	3.28f,	
	3.26f,	
	3.24f,	
	3.22f,	
	3.20f,	
	3.18f,	
	3.16f,	
	3.14f,	
	3.12f,	
	3.10f,	
	3.08f,	
	3.06f,	
	3.04f,	
	3.02f,	
	3.0f	//100
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
			kColorBoldCyn, MUD::Spell(spell_id).GetCName(), kColorNrm);
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
				kColorBoldMag, MUD::Spell(spell_id).GetCName(), kColorNrm);
	else
		sprintf(buf, "Вы занесли заклинание \"%s%s%s\" в свои резы.\r\n",
				kColorBoldMag, MUD::Spell(spell_id).GetCName(), kColorNrm);
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
				kColorBoldMag, MUD::Spell(spell_id).GetCName(), kColorNrm);
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

