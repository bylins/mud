#ifndef BYLINS_AFFECT_DATA_H
#define BYLINS_AFFECT_DATA_H

#include "affect_contants.h"
#include "structs/flag_data.h"
#include "structs/structs.h"
#include "structs/info_container.h"
#include "structs/messages_data.h"
#include "boot/cfg_manager.h"

#include <vector>
#include <list>
#include <bitset>
#include <string>
#include <fstream>
#include <iterator>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <array>

// An affect structure. //
class IAffectHandler;

template<typename TLocation>
class Affect {
 public:
	using shared_ptr = std::shared_ptr<Affect<TLocation>>;

	Affect() = default;
	[[nodiscard]] bool removable() const;

	/*
	 * Идея в следующем. Аффект однозначно _идентифицируется_ своим id'ом, который один и только один.
	 * Именно id определяет нахвание аффекта, сообщения, которые появлятся при его наложении и снятии.
	 * Но одновременно id'ы является битовым флагом в affect_bist и именно эти биты определяют _действие_
	 * аффекта. Немного извращенно, но зато не требует прямо сейчас переписать пол-движка.
	 */
	ESpell type{ESpell::kUndefined}; // The type of spell that caused this      //
	int duration{0};    // For how long its effects will last      //
	int modifier{0};        // This is added to appropriate ability     //
	TLocation location{static_cast<TLocation>(0)};        // Tells which ability to change(APPLY_XXX) //
	Bitvector flags{0u};
	Bitvector affect_bits{0u};        // Tells which bits to set (AFF_XXX) //
	FlagData aff;			// Такое впечатление, что это нигде не используется
	long caster_id{0L}; //Unique caster ID //
	bool must_handled{false}; // Указывает муду что для аффекта должен быть вызван обработчик (пока только для комнат) //
	sh_int apply_time{0}; // Указывает сколько аффект висит (пока используется только в комнатах) //
	std::shared_ptr<IAffectHandler> handler; //обработчик аффектов
};

template<>
bool Affect<EApply>::removable() const;

// Возможно эту структуру следует перенести в отдельный модуль для обкаста предметов.
struct obj_affected_type {
	EApply location;    // Which ability to change (APPLY_XXX) //
	int modifier;                // How much it changes by              //

	obj_affected_type() : location(EApply::kNone), modifier(0) {}

	obj_affected_type(EApply __location, int __modifier)
		: location(__location), modifier(__modifier) {}

	// для сравнения в sedit
	bool operator!=(const obj_affected_type &r) const {
		return (location != r.location || modifier != r.modifier);
	}
	bool operator==(const obj_affected_type &r) const {
		return !(*this != r);
	}
};

void UpdateAffectOnPulse(CharData *ch, int count);
void player_affect_update();
void battle_affect_update(CharData *ch);
void mobile_affect_update();

void affect_total(CharData *ch);
void affect_modify(CharData *ch, EApply loc, int mod, EAffect bitv, bool add);
void affect_to_char(CharData *ch, const Affect<EApply> &af);
void RemoveAffectFromChar(CharData *ch, ESpell spell_id);
bool IsAffectedBySpell(CharData *ch, ESpell type);
void ImposeAffect(CharData *ch, const Affect<EApply> &af);
void ImposeAffect(CharData *ch, Affect<EApply> &af, bool add_dur, bool max_dur, bool add_mod, bool max_mod);
void reset_affects(CharData *ch);
bool no_bad_affects(ObjData *obj);

namespace affects {

class AffectsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

class AffectInfo : public info_container::BaseItem<EAffect> {
	friend class AffectInfoBuilder;
	std::string name_{"!undefined!"};
	MessagesData<EAffectMsg> messages_;

 public:
	AffectInfo() = default;
	AffectInfo(EAffect id, EItemMode mode)
		: BaseItem<EAffect>(id, mode) {};

	const MessagesData<EAffectMsg> &Messages() const { return messages_; };
	const std::string &GetMsg(EAffectMsg id) const { return messages_.GetMsg(id); };
	const std::string &Name() { return name_; };
	void Print(std::ostringstream &buffer) const;
};

class AffectInfoBuilder : public info_container::IItemBuilder<AffectInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseObligatoryValues(parser_wrapper::DataNode &node);
	static void ParseDispensableValues(ItemPtr &item_ptr, parser_wrapper::DataNode &node);
};

using AffectsInfo = info_container::InfoContainer<EAffect, AffectInfo, AffectInfoBuilder>;

} // namespace affects

#endif //BYLINS_AFFECT_DATA_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
