/**
\file dead_load.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 03.09.2024.
\brief Система загрузки предметов по списку DL непосредственно в мобе. Необходимо удалить после введения таблиц лута.
*/

#include "dead_load.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/handler.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/global_objects.h"
#include "stable_objs.h"

#include <third_party_libs/fmt/include/fmt/format.h>

namespace dead_load {

void CopyDeadLoadList(OnDeadLoadList **pdst, OnDeadLoadList *src) {
	OnDeadLoadList::iterator p;
	if (src == nullptr) {
		*pdst = nullptr;
		return;
	} else {
		*pdst = new OnDeadLoadList;
		p = src->begin();
		while (p != src->end()) {
			(*pdst)->push_back(*p);
			p++;
		}
	}
}

bool LoadObjFromDeadLoad(ObjData *corpse, CharData *ch, CharData *chr, EDeadLoadType load_type) {

	if (mob_proto[ch->get_rnum()].dl_list.empty()) {
		return false;
	}

	auto p = mob_proto[ch->get_rnum()].dl_list.begin();

	bool last_load{true};
	bool load{false};
	while (p != mob_proto[ch->get_rnum()].dl_list.end()) {
		switch ((*p).load_type) {
			case kAnyway: last_load = load;
			case kAnywaySaveState: break;
			case kPreviuosSuccess: last_load = load && last_load;
				break;
			case kPreviuosSuccessSaveState: load = load && last_load;
				break;
		}

		// Блокируем лоад в зависимости от значения смецпараметра
		load = ((*p).spec_param == load_type);
		if (load) {
			const auto tobj = world_objects.create_from_prototype_by_vnum((*p).obj_vnum);
			if (!tobj) {
				auto msg = fmt::format("Попытка загрузки в труп (VNUM:{}) несуществующего объекта (VNUM:{}).",
									   GET_MOB_VNUM(ch), (*p).obj_vnum);
				mudlog(msg, NRM, kLvlBuilder, ERRLOG, true);
			} else {
				// Проверяем мах_ин_ворлд и вероятность загрузки, если это необходимо для такого DL_LOAD_TYPE
				bool miw;
				if (GetObjMIW(tobj->get_rnum()) >= obj_proto.actual_count(tobj->get_rnum())
					|| GetObjMIW(tobj->get_rnum()) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
					|| stable_objs::IsTimerUnlimited(tobj.get())) {
					miw = true;
				} else {
					miw = false;
				}

				switch (load_type) {
					case kOrdinary:    //Обычная загрузка - без выкрутасов
						load = (miw && (number(1, 100) <= (*p).load_prob));
						break;

					case kProgression:    //Загрузка с убывающей до 0.01 вероятностью
						load = ((miw && (number(1, 100) <= (*p).load_prob)) || (number(1, 100) <= 1));
						break;

					case kSkin:    //Загрузка по применению "освежевать"
						load = ((miw && (number(1, 100) <= (*p).load_prob)) || (number(1, 100) <= 1));
						load &= (chr != nullptr);
						break;

					default:
						auto msg = fmt::format("Неизвестный тип загрузки объекта у моба (VNUM:%d)", GET_MOB_VNUM(ch));
						mudlog(msg, NRM, kLvlBuilder, ERRLOG, true);
						mudlog("", LGH, kLvlImmortal, SYSLOG, true);
						break;
				}
				if (load) {
					tobj->set_vnum_zone_from(GetZoneVnumByCharPlace(ch));
					if (load_type == kSkin) {
						ResolveTagsInObjName(tobj.get(), ch);
					}
					// Добавлена проверка на отсутствие трупа
					if (ch->IsFlagged(EMobFlag::kCorpse)) {
						act("На земле остал$U лежать $o.", false, ch, tobj.get(), nullptr, kToRoom);
						PlaceObjToRoom(tobj.get(), ch->in_room);
					} else {
						if ((load_type == kSkin) && (corpse->get_carried_by() == chr)) {
							can_carry_obj(chr, tobj.get());
						} else {
							PlaceObjIntoObj(tobj.get(), corpse);
						}
					}
				} else {
					ExtractObjFromWorld(tobj.get());
					load = false;
				}

			}
		}
		p++;
	}

	return true;
}

bool ParseDeadLoadLine(OnDeadLoadList &dl_list, char *line) {
	// Формат парсинга D {номер прототипа} {вероятность загрузки} {спец поле - тип загрузки}
	int vnum, prob, type, spec;
	if (sscanf(line, "%d %d %d %d", &vnum, &prob, &type, &spec) != 4) {
		log("SYSERR: Parse death load list (bad param count).");
		return false;
	}

	struct LoadingItem new_item;
	new_item.obj_vnum = vnum;
	new_item.load_prob = prob;
	new_item.load_type = type;
	new_item.spec_param = spec;

	dl_list.push_back(new_item);
	return true;
}

int ResolveTagsInObjName(ObjData *obj, CharData *ch) {
	// ищем метку @p , @p1 ... и заменяем на падежи.
	int i, k;
	for (i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
		std::string obj_pad = GET_OBJ_PNAME(obj_proto[GET_OBJ_RNUM(obj)], i);
		size_t j = obj_pad.find("@p");
		if (std::string::npos != j && 0 < j) {
			// Родитель найден прописываем его.
			k = atoi(obj_pad.substr(j + 2, j + 3).c_str());
			obj_pad.replace(j, 3, GET_PAD(ch, k));

			obj->set_PName(i, obj_pad);
			// Если имя в именительном то дублируем запись
			if (i == 0) {
				obj->set_short_description(obj_pad);
				obj->set_aliases(obj_pad); // ставим алиасы
			}
		}
	}
	obj->set_is_rename(true); // присвоим флажок что у шмотки заменены падежи
	return true;
}

} // namespace dead_load

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
