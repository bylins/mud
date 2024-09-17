//#include "celebrates.h"

#include <third_party_libs/fmt/include/fmt/format.h>
#include "third_party_libs/pugixml/pugixml.h"

#include "engine/db/global_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/core/handler.h"
#include "utils/backtrace.h"
#include "weather.h"

extern void ExtractTrigger(Trigger *trig);

namespace celebrates {

const int kCleanPeriod{10};

int tab_day[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};//да и хрен с ним, с 29 февраля!

CelebrateList &mono_celebrates = GlobalObjects::mono_celebrates();
CelebrateList &poly_celebrates = GlobalObjects::poly_celebrates();
CelebrateList &real_celebrates = GlobalObjects::real_celebrates();
CelebrateMobs &attached_mobs = GlobalObjects::attached_mobs();
CelebrateMobs &loaded_mobs = GlobalObjects::loaded_mobs();
CelebrateObjs &attached_objs = GlobalObjects::attached_objs();
CelebrateObjs &loaded_objs = GlobalObjects::loaded_objs();

using CelebrateMobs_list = std::list<CelebrateMobs::mapped_type>;

void MakeCelebratedMobsListFromCelebrateMobs(const CelebrateMobs &input, CelebrateMobs_list &result) {
	result.clear();
	for (const auto &mob : input) {
		result.push_back(mob.second);
	}
}

void AddMobToAttachList(long uid, CharData *mob) {
	attached_mobs[uid] = mob;
}

void AddMobToLoadList(long uid, CharData *mob) {
	loaded_mobs[uid] = mob;
}
void AddObjToAttachList(long uid, ObjData *obj) {
	attached_objs[uid] = obj;
}
void AddObjToLoadList(long uid, ObjData *obj) {
	loaded_objs[uid] = obj;
}

int GetRealHour() {
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	return timeinfo->tm_hour;
}

std::string GetNameMono(int day) {
	if (mono_celebrates.find(day) == mono_celebrates.end())
		return "";
	else if (mono_celebrates[day]->start_at <= time_info.hours && mono_celebrates[day]->finish_at >= time_info.hours)
		return mono_celebrates[day]->celebrate->name;
	else return "";
}

std::string GetNamePoly(int day) {
	if (poly_celebrates.find(day) == poly_celebrates.end())
		return "";
	else if (poly_celebrates[day]->start_at <= time_info.hours && poly_celebrates[day]->finish_at >= time_info.hours)
		return poly_celebrates[day]->celebrate->name;
	else return "";
}

std::string AddRest(CelebrateList::iterator it, const CelebrateDataPtr &celebrate) {
	int days_count = 0;
	while (it->second->celebrate == celebrate) {
		days_count++;
		++it;
	}
	--it;
	--days_count;
	int hours = it->second->finish_at;
	hours -= GetRealHour();
	if (hours < 0) {
		--days_count;
		hours = 24 + hours;
	}

	return fmt::format(". До окончания - дней: {}, часов: {}.", days_count, hours);
}

std::string GetNameReal(int day) {
	auto it = real_celebrates.find(day);
	std::string result;
	if (it != real_celebrates.end()) {
		if (real_celebrates[day]->start_at <= GetRealHour() && real_celebrates[day]->finish_at >= GetRealHour())
			result = real_celebrates[day]->celebrate->name;// + add_rest(it, real_celebrates[day]->celebrate)
	}
	return result;
}

void ParseTrigList(pugi::xml_node node, TrigList *triggers) {
	for (pugi::xml_node trig = node.child("trig"); trig; trig = trig.next_sibling("trig")) {
		int vnum = trig.attribute("vnum").as_int();
		if (!vnum) {
			snprintf(buf, kMaxStringLength, "...celebrates - bad trig (node = %s)", node.name());
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;
		}
		triggers->push_back(vnum);
	}
}

void ParseLoadData(pugi::xml_node node, const LoadPtr &node_data) {
	int vnum = node.attribute("vnum").as_int();
	int max = node.attribute("max").as_int();
	if (!vnum || !max) {
		snprintf(buf, kMaxStringLength, "...celebrates - bad data (node = %s)", node.name());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	node_data->vnum = vnum;
	node_data->max = max;
}

void ParseLoadSection(pugi::xml_node node, const CelebrateDataPtr &holiday) {
	for (pugi::xml_node room = node.child("room"); room; room = room.next_sibling("room")) {
		int vnum = room.attribute("vnum").as_int();
		if (!vnum) {
			snprintf(buf,
					 kMaxStringLength,
					 "...celebrates - bad room (celebrate = %s)",
					 node.attribute("name").value());
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;
		}
		CelebrateRoomPtr tmp_room(new CelebrateRoom);
		tmp_room->vnum = vnum;
		ParseTrigList(room, &tmp_room->triggers);
		for (pugi::xml_node mob = room.child("mob"); mob; mob = mob.next_sibling("mob")) {
			LoadPtr tmp_mob(new ToLoad);
			ParseLoadData(mob, tmp_mob);
			ParseTrigList(mob, &tmp_mob->triggers);
			for (pugi::xml_node obj_in = mob.child("obj"); obj_in; obj_in = obj_in.next_sibling("obj")) {
				LoadPtr tmp_obj_in(new ToLoad);
				ParseLoadData(obj_in, tmp_obj_in);
				ParseTrigList(obj_in, &tmp_obj_in->triggers);
				tmp_mob->objects.push_back(tmp_obj_in);
			}
			tmp_room->mobs.push_back(tmp_mob);
		}
		for (pugi::xml_node obj = room.child("obj"); obj; obj = obj.next_sibling("obj")) {
			LoadPtr tmp_obj(new ToLoad);
			ParseLoadData(obj, tmp_obj);
			ParseTrigList(obj, &tmp_obj->triggers);
			for (pugi::xml_node obj_in = obj.child("obj"); obj_in; obj_in = obj_in.next_sibling("obj")) {
				LoadPtr tmp_obj_in(new ToLoad);
				ParseLoadData(obj_in, tmp_obj_in);
				ParseTrigList(obj_in, &tmp_obj_in->triggers);
				tmp_obj->objects.push_back(tmp_obj_in);
			}
			tmp_room->objects.push_back(tmp_obj);
		}
		holiday->rooms[tmp_room->vnum / 100].push_back(tmp_room);
	}
}
void ParseAttachSection(pugi::xml_node node, const CelebrateDataPtr &holiday) {
	pugi::xml_node attaches = node.child("attaches");
	int vnum;
	for (pugi::xml_node mob = attaches.child("mob"); mob; mob = mob.next_sibling("mob")) {
		vnum = mob.attribute("vnum").as_int();
		if (!vnum) {
			snprintf(buf, kMaxStringLength, "...celebrates - bad attach data (node = %s)", node.name());
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;
		}
		ParseTrigList(mob, &holiday->mobsToAttach[vnum / 100][vnum]);
	}
	for (pugi::xml_node obj = attaches.child("obj"); obj; obj = obj.next_sibling("obj")) {
		vnum = obj.attribute("vnum").as_int();
		if (!vnum) {
			snprintf(buf, kMaxStringLength, "...celebrates - bad attach data (node = %s)", node.name());
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;
		}
		ParseTrigList(obj, &holiday->objsToAttach[vnum / 100][vnum]);
	}
}

int FindRealDayNumber(int day, int month) {
	int d = day;
	for (int i = 0; i < month - 1; i++)
		d += tab_day[i];
	return d;
}

int GetPreviousDay(int day, bool is_real) {
	if (day == 1)
		return is_real ? 365 : kDaysPerMonth * 12;
	else
		return day - 1;
}

int GetNextDay(int day, bool is_real) {
	if ((is_real && day == 365) || (!is_real && day == kDaysPerMonth * 12))
		return 1;
	else
		return day + 1;
}

void LoadCelebrates(pugi::xml_node node_list, CelebrateList &celebrates, bool is_real) {
	for (pugi::xml_node node = node_list.child("celebrate"); node; node = node.next_sibling("celebrate")) {
		int day = node.attribute("day").as_int();
		int month = node.attribute("month").as_int();
		int start = node.attribute("start").as_int();
		int end = node.attribute("end").as_int();
		std::string name = node.attribute("name").value();
		int baseDay;
		if (!day || !month || name.empty()) {
			snprintf(buf, kMaxStringLength, "...celebrates - bad node struct");
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;
		}
		CelebrateDataPtr tmp_holiday(new CelebrateData);
		tmp_holiday->name = name;
		ParseLoadSection(node, tmp_holiday);
		ParseAttachSection(node, tmp_holiday);

		if (is_real)
			baseDay = FindRealDayNumber(day, month);
		else
			baseDay = kDaysPerMonth * (month - 1) + day;
		CelebrateDayPtr tmp_day(new CelebrateDay);
		tmp_day->celebrate = tmp_holiday;
		celebrates[baseDay] = tmp_day;
		if (start > 0) {
			int dayBefore = baseDay;
			while (start > 24) {
				start -= 24;
				dayBefore = GetPreviousDay(dayBefore, is_real);
				celebrates[dayBefore] = tmp_day;
			}
			CelebrateDayPtr tmp_day1(new CelebrateDay);
			tmp_day1->celebrate = tmp_holiday;
			tmp_day1->start_at = 24 - start;
			dayBefore = GetPreviousDay(dayBefore, is_real);
			celebrates[dayBefore] = tmp_day1;
		}
		if (end > 0) {
			int dayAfter = baseDay;
			while (end > 24) {
				end -= 24;
				dayAfter = GetNextDay(dayAfter, is_real);
				celebrates[dayAfter] = tmp_day;
			}
			CelebrateDayPtr tmp_day2(new CelebrateDay);
			tmp_day2->celebrate = tmp_holiday;
			tmp_day2->finish_at = end;
			tmp_day2->last = true;
			dayAfter = GetNextDay(dayAfter, is_real);
			celebrates[dayAfter] = tmp_day2;
		} else
			tmp_day->last = true;
	}
}

void Load() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_MISC"celebrates.xml");
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node node_list = doc.child("celebrates");
	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...celebrates read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node mono_node_list = node_list.child("celebratesMono");//православные праздники
	LoadCelebrates(mono_node_list, mono_celebrates, false);
	pugi::xml_node poly_node_list = node_list.child("celebratesPoly");//языческие праздники
	LoadCelebrates(poly_node_list, poly_celebrates, false);
	pugi::xml_node real_node_list = node_list.child("celebratesReal");//Российские праздники
	LoadCelebrates(real_node_list, real_celebrates, true);
}

int GetMudDay() {
	return time_info.month * kDaysPerMonth + time_info.day + 1;
}

int GetRealDay() {
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	return FindRealDayNumber(timeinfo->tm_mday, timeinfo->tm_mon + 1);
}

bool IsActive(CelebrateList::iterator it, bool is_real) {
	int hours = is_real ? GetRealHour() : time_info.hours;
	int day = is_real ? GetRealDay() : GetMudDay();
	CelebrateList::const_iterator celebrate;
	if (is_real) {
		celebrate = real_celebrates.find(day);
		if (celebrate == real_celebrates.end())
			return false;
		if (celebrate->second->celebrate == it->second->celebrate && hours <= it->second->finish_at)
			return true;
	} else {
		celebrate = poly_celebrates.find(day);
		if (celebrate == poly_celebrates.end())
			return false;
		if (celebrate != poly_celebrates.end() && celebrate->second->celebrate == it->second->celebrate
			&& hours <= it->second->finish_at)
			return true;
		celebrate = mono_celebrates.find(day);
		if (celebrate == mono_celebrates.end())
			return false;
		if (celebrate != mono_celebrates.end() && celebrate->second->celebrate == it->second->celebrate
			&& hours <= it->second->finish_at)
			return true;
	}
	return false;
}

CelebrateDataPtr GetMonoCelebrate() {
	int day = GetMudDay();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (mono_celebrates.find(day) != mono_celebrates.end()) {
		if (IsActive(mono_celebrates.find(day), false)) {
			result = mono_celebrates[day]->celebrate;
			mono_celebrates[day]->celebrate->is_clean = false;
		}
	}
	return result;
};

CelebrateDataPtr GetPolyCelebrate() {
	int day = GetMudDay();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (poly_celebrates.find(day) != poly_celebrates.end()) {
		if (IsActive(poly_celebrates.find(day), false)) {
			result = poly_celebrates[day]->celebrate;
			poly_celebrates[day]->celebrate->is_clean = false;
		}
	}
	return result;
};

CelebrateDataPtr GetRealCelebrate() {
	int day = GetRealDay();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (real_celebrates.find(day) != real_celebrates.end()) {
		if (IsActive(real_celebrates.find(day), true)) {
			result = real_celebrates[day]->celebrate;
			real_celebrates[day]->celebrate->is_clean = false;
		}
	}
	return result;
};

class ScChecker {
 public:
  explicit ScChecker(const TrigList &trigs);
  void SetInsideLoop() { m_inside_loop = true; }
  void SetCurrentTrigger(const TrigList::value_type current_trigger) { m_current_trigger = current_trigger; }
  void ReportNullSc() const;

 private:
  [[nodiscard]] auto GetInsideLoop() const { return m_inside_loop; }
  [[nodiscard]] const auto &TriggersList() const { return m_triggers_list; }
  [[nodiscard]] auto GetCurrentTrigger() const { return m_current_trigger; }

  bool m_inside_loop;
  TrigList m_triggers_list;
  TrigList::value_type m_current_trigger;
};

ScChecker::ScChecker(const TrigList &trigs) : m_inside_loop(false), m_triggers_list(trigs), m_current_trigger(0) {
}

void ScChecker::ReportNullSc() const {
	std::string joined_triggers_list;
	joinList(TriggersList(), joined_triggers_list);

	std::stringstream ss;
	ss << "Перехвачен кордамп. Список триггеров, переданных в функцию RemoveTriggers(...): ["
	   << joined_triggers_list << "].";
	if (GetInsideLoop()) {
		ss << " Обратите внимание, что кордамп перехвачен ВНУТРИ цикла на триггере " << GetCurrentTrigger();
	}
	mudlog(ss.str().c_str(), DEF, -1, ERRLOG, true);
	ss << "\nТекущий стек будет распечатан в ERRLOG, выполнение функции будет прервано.";
	debug::backtrace(runtime_config.logs(ERRLOG).handle());
	mudlog(ss.str().c_str(), DEF, kLvlImplementator, ERRLOG, false);
}

void RemoveTriggers(const TrigList &trigs, Script *sc) {
	ScChecker checker(trigs);
	if (nullptr == sc) {
		checker.ReportNullSc();
		return;
	}

	for (int trig : trigs) {
		if (nullptr == sc) {
			checker.SetInsideLoop();
			checker.SetCurrentTrigger(trig);
			checker.ReportNullSc();
			return;
		}

		Trigger *removed = sc->trig_list.remove_by_vnum(trig);
		if (removed) {
			ExtractTrigger(removed);
			SCRIPT_TYPES(sc) = sc->trig_list.get_type();
		}
	}
}

void RemoveFromObjLists(long uid) {
	auto it = attached_objs.find(uid);
	if (it != attached_objs.end()) {
		attached_objs.erase(it);
	}

	it = loaded_objs.find(uid);
	if (it != loaded_objs.end()) {
		loaded_objs.erase(it);
	}
}

void RemoveFromMobLists(long uid) {
	auto it = attached_mobs.find(uid);
	if (it != attached_mobs.end())
		attached_mobs.erase(it);
	it = loaded_mobs.find(uid);
	if (it != loaded_mobs.end())
		loaded_mobs.erase(it);
}

bool MakeClean(const CelebrateDataPtr &celebrate) {
	CelebrateObjs::iterator obj_it;
	CelebrateMobs::iterator mob_it;

	for (mob_it = attached_mobs.begin(); mob_it != attached_mobs.end(); ++mob_it) {
		const auto rnum = mob_it->second->get_rnum();
		int vnum = mob_index[rnum].vnum;
		for (auto &it : celebrate->mobsToAttach) {
			if (it.second.find(vnum) != it.second.end()) {
				RemoveTriggers(it.second[vnum], mob_it->second->script.get());
			}
		}

		attached_mobs.erase(mob_it);
		if (attached_mobs.empty()) {
			break;
		}
	}

	for (obj_it = attached_objs.begin(); obj_it != attached_objs.end(); ++obj_it) {
		int vnum = obj_it->second->get_rnum();
		for (auto &it : celebrate->objsToAttach) {
			if (it.second.find(vnum) != it.second.end()) {
				RemoveTriggers(it.second[vnum], obj_it->second->get_script().get());
			}
		}

		attached_objs.erase(obj_it);
		if (attached_objs.empty()) {
			break;
		}
	}

	CelebrateMobs_list loaded_mobs_copy;
	MakeCelebratedMobsListFromCelebrateMobs(loaded_mobs, loaded_mobs_copy);

	for (const auto &mob : loaded_mobs_copy) {
		const auto rnum = mob->get_rnum();
		const int vnum = mob_index[rnum].vnum;
		for (auto &rooms : celebrate->rooms) {
			for (auto & room : rooms.second) {
				for (auto &it : room->mobs) {
					if (it->vnum == vnum) {
						// This function is very bad because modifies global variable loaded_mobs
						// Initially loop was over this global variable. Therefore, this loop constantly crashed.
						// I've changed loop variable to the copy of loaded_mobs, but it's still a bad approach.
						ExtractCharFromWorld(mob, 0);
					}
				}
			}
		}
	}

	for (obj_it = loaded_objs.begin(); obj_it != loaded_objs.end(); ++obj_it) {
		const int vnum = obj_it->second->get_rnum();
		for (auto &rooms : celebrate->rooms) {
			for (auto &room : rooms.second) {
				for (auto &object : room->objects) {
					if (object->vnum == vnum) {
						ExtractObjFromWorld(obj_it->second);
					}
				}
			}
		}

		if (loaded_objs.empty()) {
			break;
		}
	}

	for (auto &rooms : celebrate->rooms) {
		for (auto &room : rooms.second) {
			if (!room->triggers.empty()) {
				const int rnum = GetRoomRnum(room->vnum);
				RemoveTriggers(room->triggers, world[rnum]->script.get());
			}
		}
	}

	return true;//пока не знаю зачем
}

void ClearRealCelebrates(CelebrateList celebrates) {
	for (auto it = celebrates.begin(); it != celebrates.end(); ++it) {
		if (!it->second->celebrate->is_clean
			&& !IsActive(it, true)
			&& it->second->last) {
			if (MakeClean(it->second->celebrate)) {
				it->second->celebrate->is_clean = true;
			}
		}
	}
}

void ClearMudCelebrates(CelebrateList celebrates) {
	for (auto it = celebrates.begin(); it != celebrates.end(); ++it) {
		if (!it->second->celebrate->is_clean
			&& !IsActive(it, false)
			&& it->second->last) {
			if (MakeClean(it->second->celebrate)) {
				it->second->celebrate->is_clean = true;
			}
		}
	}
}

void Sanitize() {
	ClearRealCelebrates(real_celebrates);
	ClearMudCelebrates(poly_celebrates);
	ClearMudCelebrates(mono_celebrates);
}

void ProcessLoadCelebrate(CelebrateDataPtr &celebrate, int vnum) {
	CelebrateRoomsList::iterator room;
	LoadList::iterator load, load_in;

	log("Processing celebrate %s load section for zone %d", celebrate->name.c_str(), vnum);

	if (celebrate->rooms.find(vnum) != celebrate->rooms.end()) {
		for (room = celebrate->rooms[vnum].begin(); room != celebrate->rooms[vnum].end(); ++room) {
			RoomRnum rn = GetRoomRnum((*room)->vnum);
			if (rn != kNowhere) {
				for (int &trigger : (*room)->triggers) {
					auto trig = read_trigger(GetTriggerRnum(trigger));
					if (!add_trigger(world[rn]->script.get(), trig, -1)) {
						ExtractTrigger(trig);
					}
				}
			}

			for (load = (*room)->mobs.begin(); load != (*room)->mobs.end(); ++load) {
				CharData *mob;
				int i = GetMobRnum((*load)->vnum);
				if (i > 0
					&& mob_index[i].total_online < (*load)->max) {
					mob = ReadMobile(i, kReal);
					if (mob) {
						for (int &trigger : (*load)->triggers) {
							auto trig = read_trigger(GetTriggerRnum(trigger));
							if (!add_trigger(SCRIPT(mob).get(), trig, -1)) {
								ExtractTrigger(trig);
							}
						}
						load_mtrigger(mob);
						PlaceCharToRoom(mob, GetRoomRnum((*room)->vnum));
						AddMobToLoadList(mob->get_uid(), mob);
						for (load_in = (*load)->objects.begin(); load_in != (*load)->objects.end(); ++load_in) {
							ObjRnum rnum = GetObjRnum((*load_in)->vnum);

							if (obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world()) {
								const auto obj = world_objects.create_from_prototype_by_vnum((*load_in)->vnum);
								if (obj) {
									PlaceObjToInventory(obj.get(), mob);
									obj->set_vnum_zone_from(zone_table[world[mob->in_room]->zone_rn].vnum);

									for (int &trigger : (*load_in)->triggers) {
										auto trig = read_trigger(GetTriggerRnum(trigger));
										if (!add_trigger(obj->get_script().get(), trig, -1)) {
											ExtractTrigger(trig);
										}
									}

									load_otrigger(obj.get());
									AddObjToLoadList(obj->get_unique_id(), obj.get());
								} else {
									log("{Error] Processing celebrate %s while loading obj %d",
										celebrate->name.c_str(),
										(*load_in)->vnum);
								}
							}
						}
					} else {
						log("{Error] Processing celebrate %s while loading mob %d",
							celebrate->name.c_str(),
							(*load)->vnum);
					}
				}
			}
			for (load = (*room)->objects.begin(); load != (*room)->objects.end(); ++load) {
				ObjData *obj_room;
				ObjRnum rnum = GetObjRnum((*load)->vnum);
				if (rnum == -1) {
					log("{Error] Processing celebrate %s while loading obj %d", celebrate->name.c_str(), (*load)->vnum);
					return;
				}
				int obj_in_room = 0;

				for (obj_room = world[rn]->contents; obj_room; obj_room = obj_room->get_next_content()) {
					if (rnum == GET_OBJ_RNUM(obj_room)) {
						obj_in_room++;
					}
				}

				if ((obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world())
					&& (obj_in_room < (*load)->max)) {
					const auto obj = world_objects.create_from_prototype_by_vnum((*load)->vnum);
					if (obj) {
						for (int &trigger : (*load)->triggers) {
							auto trig = read_trigger(GetTriggerRnum(trigger));
							if (!add_trigger(obj->get_script().get(), trig, -1)) {
								ExtractTrigger(trig);
							}
						}
						load_otrigger(obj.get());
						AddObjToLoadList(obj->get_unique_id(), obj.get());

						PlaceObjToRoom(obj.get(), GetRoomRnum((*room)->vnum));

						for (load_in = (*load)->objects.begin(); load_in != (*load)->objects.end(); ++load_in) {
							ObjRnum current_obj_rnum = GetObjRnum((*load_in)->vnum);

							if (obj_proto.actual_count(current_obj_rnum)
								< obj_proto[current_obj_rnum]->get_max_in_world()) {
								const auto obj_in = world_objects.create_from_prototype_by_vnum((*load_in)->vnum);
								if (obj_in
									&& GET_OBJ_TYPE(obj) == EObjType::kContainer) {
									PlaceObjIntoObj(obj_in.get(), obj.get());
									obj_in->set_vnum_zone_from(GET_OBJ_VNUM_ZONE_FROM(obj));

									for (int &trigger : (*load_in)->triggers) {
										auto trig = read_trigger(GetTriggerRnum(trigger));
										if (!add_trigger(obj_in->get_script().get(), trig, -1)) {
											ExtractTrigger(trig);
										}
									}

									load_otrigger(obj_in.get());
									AddObjToLoadList(obj->get_unique_id(), obj.get());
								} else {
									log("{Error] Processing celebrate %s while loading obj %d",
										celebrate->name.c_str(),
										(*load_in)->vnum);
								}
							}
						}
					} else {
						log("{Error] Processing celebrate %s while loading mob %d",
							celebrate->name.c_str(),
							(*load)->vnum);
					}
				}
			}
		}
	}
}

void ProcessAttachCelebrate(CelebrateDataPtr &celebrate, int zone_vnum) {
	log("Processing celebrate %s attach section for zone %d", celebrate->name.c_str(), zone_vnum);

	if (celebrate->mobsToAttach.find(zone_vnum) != celebrate->mobsToAttach.end()) {
		//поскольку единственным доступным способом получить всех мобов одного внума является
		//обход всего списка мобов в мире, то будем хотя бы 1 раз его обходить
		AttachList list = celebrate->mobsToAttach[zone_vnum];
		for (const auto &ch : character_list) {
			const auto rnum = ch->get_rnum();
			if (rnum > 0
				&& list.find(mob_index[rnum].vnum) != list.end()) {
				for (int &it : list[mob_index[rnum].vnum]) {
					auto trig = read_trigger(GetTriggerRnum(it));
					if (!add_trigger(SCRIPT(ch).get(), trig, -1)) {
						ExtractTrigger(trig);
					}
				}

				AddMobToAttachList(ch->get_uid(), ch.get());
			}
		}
	}

	if (celebrate->objsToAttach.find(zone_vnum) != celebrate->objsToAttach.end()) {
		AttachList list = celebrate->objsToAttach[zone_vnum];

		world_objects.foreach([&](const ObjData::shared_ptr &o) {
		  if (o->get_rnum() > 0 && list.find(o->get_rnum()) != list.end()) {
			  for (auto it = list[o->get_rnum()].begin(); it != list[o->get_rnum()].end(); ++it) {
				  auto trig = read_trigger(GetTriggerRnum(*it));
				  if (!add_trigger(o->get_script().get(), trig, -1)) {
					  ExtractTrigger(trig);
				  }
			  }

			  AddObjToAttachList(o->get_unique_id(), o.get());
		  }
		});
	}
}

void ProcessCelebrates(int vnum) {
	CelebrateDataPtr mono = GetMonoCelebrate();
	CelebrateDataPtr poly = GetPolyCelebrate();
	CelebrateDataPtr real = GetRealCelebrate();

	if (mono) {
		ProcessLoadCelebrate(mono, vnum);
		ProcessAttachCelebrate(mono, vnum);
	}

	if (poly) {
		ProcessLoadCelebrate(poly, vnum);
		ProcessAttachCelebrate(poly, vnum);
	}
	if (real) {
		ProcessLoadCelebrate(real, vnum);
		ProcessAttachCelebrate(real, vnum);
	}
}

} // namespace Celebrates

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
