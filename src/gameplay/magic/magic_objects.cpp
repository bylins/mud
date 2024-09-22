// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "spells.h"

#include "engine/entities/obj_data.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/poison.h"
#include "engine/entities/char_data.h"
#include "engine/core/utils_char_obj.inl"
#include "spells_info.h"
#include "engine/db/global_objects.h"

/*
Система следующая:
хотим что-то сделать при касте на шмотку - пишем в mag_alter_objs()
надо что-то сделать при снятии обкаста - check_spell_remove()
если надо постоянный обкаст - ставим таймер на -1 в timed_spell.add()
надо проверить есть ли каст на шмотке - timed_spell.check_spell(spell_num)
*/

////////////////////////////////////////////////////////////////////////////////
namespace {

///
/// Удаление временного флага со шмотки obj (с проверкой прототипа).
/// \param flag - ITEM_XXX
///
void remove_tmp_extra(ObjData *obj, EObjFlag flag) {
	auto proto = GetObjectPrototype(GET_OBJ_VNUM(obj));
	if (!proto->has_flag(flag)) {
		obj->unset_extraflag(flag);
	}
}

/**
 * Проверка надо ли что-то делать со шмоткой или писать чару
 * при снятии заклинания со шмотки.
 */
void PrepareSpellRemoving(ObjData *obj, ESpell spell_id, bool send_message) {
	if (!obj) {
		log("SYSERROR: NULL object %s:%d, spell = %d", __FILE__, __LINE__, to_underlying(spell_id));
		return;
	}

	// если что-то надо сделать со шмоткой при снятии обкаста
	switch (spell_id) {
		case ESpell::kAconitumPoison:
		case ESpell::kScopolaPoison:
		case ESpell::kBelenaPoison:
		case ESpell::kDaturaPoison: break;

		case ESpell::kFly: remove_tmp_extra(obj, EObjFlag::kFlying);
			break;

		case ESpell::kLight: remove_tmp_extra(obj, EObjFlag::kGlow);
			break;
		default: break;
	} // switch

	// онлайн уведомление чару
	if (send_message
		&& (obj->get_carried_by()
			|| obj->get_worn_by())) {
		CharData *ch = obj->get_carried_by() ? obj->get_carried_by() : obj->get_worn_by();
		switch (spell_id) {
			case ESpell::kAconitumPoison:
			case ESpell::kScopolaPoison:
			case ESpell::kBelenaPoison:
			case ESpell::kDaturaPoison:
				SendMsgToChar(ch, "С %s испарились последние капельки яда.\r\n",
							  GET_OBJ_PNAME(obj, 1).c_str());
				break;

			case ESpell::kFly:
				SendMsgToChar(ch, "Ваш%s %s перестал%s парить в воздухе.\r\n",
							  GET_OBJ_VIS_SUF_7(obj, ch),
							  GET_OBJ_PNAME(obj, 0).c_str(),
							  GET_OBJ_VIS_SUF_1(obj, ch));
				break;

			case ESpell::kLight:
				SendMsgToChar(ch, "Ваш%s %s перестал%s светиться.\r\n",
							  GET_OBJ_VIS_SUF_7(obj, ch),
							  GET_OBJ_PNAME(obj, 0).c_str(),
							  GET_OBJ_VIS_SUF_1(obj, ch));
				break;
			default: break;
		}
	}
}

// * Распечатка строки с заклинанием и таймером при осмотре шмотки.
std::string print_spell_str(ESpell spell_id, int timer) {
	if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
		log("SYSERROR: %s, spell = %d, time = %d", __func__, to_underlying(spell_id), timer);
		return "";
	}

	std::stringstream out;
	switch (spell_id) {
		case ESpell::kAconitumPoison:
		case ESpell::kScopolaPoison:
		case ESpell::kBelenaPoison:
		case ESpell::kDaturaPoison:
			out << kColorGrn << "Отравлено " << GetPoisonName(spell_id) << " еще " << timer << " "
				<< GetDeclensionInNumber(timer, EWhat::kMinU) << ".\r\n" << kColorNrm;
			break;

		default:
			if (timer == -1) {
				out << kColorCyn << "Наложено постоянное заклинание '"
					<< MUD::Spell(spell_id).GetName() << "'" << ".\r\n" << kColorNrm;
			} else {
				out << kColorCyn << "Наложено заклинание '"
					<< MUD::Spell(spell_id).GetName() << "' (" << FormatTimeToStr(timer, true) << ").\r\n"
					<< kColorNrm;
			}
			break;
	}
	return out.str();
}

} // namespace
////////////////////////////////////////////////////////////////////////////////

/**
 * Удаление заклинания со шмотки с проверкой на действия/сообщения
 * при снятии обкаста.
 */
void TimedSpell::del(ObjData *obj, ESpell spell_id, bool message) {
	auto i = spell_list_.find(spell_id);
	if (i != spell_list_.end()) {
		PrepareSpellRemoving(obj, spell_id, message);
		spell_list_.erase(i);
	}
}

/**
* Сет доп.спела с таймером на шмотку.
* \param time = -1 для постоянного обкаста
*/
void TimedSpell::add(ObjData *obj, ESpell spell_id, int time) {
	// замещение ядов друг другом
	if (spell_id == ESpell::kAconitumPoison
		|| spell_id == ESpell::kScopolaPoison
		|| spell_id == ESpell::kBelenaPoison
		|| spell_id == ESpell::kDaturaPoison) {
		del(obj, ESpell::kAconitumPoison, false);
		del(obj, ESpell::kScopolaPoison, false);
		del(obj, ESpell::kBelenaPoison, false);
		del(obj, ESpell::kDaturaPoison, false);
	}

	spell_list_[spell_id] = time;
}

// * Вывод оставшегося времени яда на пушке при осмотре.
std::string TimedSpell::diag_to_char() {
	if (spell_list_.empty()) {
		return "";
	}

	std::string out;
	for (auto & i : spell_list_) {
		out += print_spell_str(i.first, i.second);
	}
	return out;
}

/**
 * Проверка на обкаст шмотки любым видом яда.
 * \return -1 если яда нет, spell_num если есть.
 */
ESpell TimedSpell::IsSpellPoisoned() const {
	for (auto i : spell_list_) {
		if (IsSpellPoison(i.first)) {
			return i.first;
		}
	}
	return ESpell::kUndefined;
}

// * Для сейва обкаста.
bool TimedSpell::empty() const {
	return spell_list_.empty();
}

// * Сохранение строки в файл.
std::string TimedSpell::print() const {
	std::stringstream out;

	out << "TSpl: ";
	for (auto i : spell_list_) {
		out << to_underlying(i.first) << " " << i.second << "\n";
	}
	out << "~\n";

	return out.str();
}

// * Поиск заклинания по spell_num.
bool TimedSpell::check_spell(ESpell spell_id) const {
	auto i = spell_list_.find(spell_id);
	if (i != spell_list_.end()) {
		return true;
	}
	return false;
}

/**
* Тик доп.спеллов на шмотке (раз в минуту).
* \param time по дефолту = 1.
*/
void TimedSpell::dec_timer(ObjData *obj, int time) {
	for (auto i = spell_list_.begin(); i != spell_list_.end(); /* empty */) {
		if (i->second != -1) {
			i->second -= time;
			if (i->second <= 0) {
				PrepareSpellRemoving(obj, i->first, true);
				spell_list_.erase(i++);
			} else {
				++i;
			}
		} else {
			++i;
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
