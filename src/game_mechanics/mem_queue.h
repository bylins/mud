/**
\authors Created by Sventovit
\date 02.05.2022.
\brief Функции и структуры очереди заучивания заклинаний
\details Весь код по работе с очередью заучивания заклинаний должен располагаться в данном модуле.
*/

#ifndef BYLINS_SRC_GAME_MECHANICS_MEM_QUEUE_H_
#define BYLINS_SRC_GAME_MECHANICS_MEM_QUEUE_H_

#include "game_magic/spells_constants.h"

struct SpellMemQueueItem {
	ESpell spell_id{ESpell::kUndefined};
	struct SpellMemQueueItem *next{nullptr};
};

// очередь запоминания заклинаний
struct SpellMemQueue {
	SpellMemQueue() = default;
	~SpellMemQueue();

	SpellMemQueueItem *queue{nullptr};
	int stored{0};	// накоплено манны
	int total{0};	// полное время мема всей очереди

	[[nodiscard]] bool Empty() const { return queue == nullptr; };
	void Clear();
};

int CalcSpellManacost(CharData *ch, ESpell spell_id);
void MemQ_init(CharData *ch);
void MemQ_flush(CharData *ch);
ESpell MemQ_learn(CharData *ch);
//inline ESpell MemQ_learn(const CharData::shared_ptr &ch) { return MemQ_learn(ch.get()); }
void MemQ_remember(CharData *ch, ESpell spell_id);
void MemQ_forget(CharData *ch, ESpell spell_id);
int *MemQ_slots(CharData *ch);
void forget_all_spells(CharData *ch);

#endif //BYLINS_SRC_GAME_MECHANICS_MEM_QUEUE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
