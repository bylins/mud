// Part of Bylins http://www.mud.ru

#include "engine/db/obj_save.h"

#include "gameplay/clans/house.h"
#include "depot.h"
#include "gameplay/communication/parcel.h"
#include "engine/db/world_characters.h"
#include "utils/backtrace.h"

namespace ObjSaveSync {

struct NodeType {
	// уид чара/внум клан-ренты инициатора
	long init_uid;
	// флаг для уида инициатора
	int init_type;
	// уид чара/внум клан-ренты на сейв
	long targ_uid;
	// флаг для уида на сейв
	int targ_type;
};

struct ForceNodeType {
	long uid;
	int type;
};

// список всех пар на будующий сейв
std::list<NodeType> save_list;
// очищенный от дублей список на текущий форс-сейв
std::vector<ForceNodeType> force_list;

void add(long init_uid, long targ_uid, int targ_type) {
	NodeType node;
	node.init_uid = init_uid;
	node.init_type = CHAR_SAVE;
	node.targ_uid = targ_uid;
	node.targ_type = targ_type;
	save_list.push_front(node);
}

void write_file(long uid, int type) {
	if (type == CHAR_SAVE) {
		CharData *ch = find_char(uid);
		if (ch) {
			Crash_crashsave(ch);
			return;
		} else {
			mudlog(fmt::format("Запрошена запись в файл неизвестного игрока, uid {} Создан coredump", uid));
			debug::backtrace(runtime_config.logs(SYSLOG).handle());
		}
	} else if (type == CLAN_SAVE) {
		for (const auto &i : Clan::ClanList) {
			if (i->GetRent() == uid) {
				i->save_chest();
				return;
			}
		}
	} else if (type == PERS_CHEST_SAVE) {
		Depot::save_char_by_uid(uid);
	} else if (type == PARCEL_SAVE) {
		Parcel::save();
	}
}

void add_to_list(long uid, int type) {
	if (type == PARCEL_SAVE) {
		// почта одна на всех - отсекаются дубли по совпадению type
		std::vector<ForceNodeType>::const_iterator i =
			std::find_if(force_list.begin(), force_list.end(),
						 [&](const ForceNodeType &x) {
							 return x.type == type;
						 });

		if (i != force_list.end()) {
			return;
		}
	} else {
		// отсекаются остальные дубли по совпадению и uid, и type
		std::vector<ForceNodeType>::const_iterator i =
			std::find_if(force_list.begin(), force_list.end(),
						 [&](const ForceNodeType &x) {
							 return x.uid == uid && x.type == type;
						 });

		if (i != force_list.end()) {
			return;
		}
	}
	ForceNodeType node;
	node.uid = uid;
	node.type = type;
	force_list.push_back(node);
}

void fill_force_list(long uid, int type) {
	for (std::list<NodeType>::iterator i = save_list.begin();
		 i != save_list.end(); /* empty */) {
		// случай с посылками, у которых вся бд в одном файле
		// в save_list оно только в виде init чар -> targ посылка
		// поэтому дергаем всех, завязанных на посылки в targ
		if ((type == PARCEL_SAVE && i->targ_type == PARCEL_SAVE)
			|| (i->targ_uid == uid && i->targ_type == type)) {
			const long uid = i->init_uid, type = i->init_type;
			save_list.erase(i);
			add_to_list(uid, type);
			fill_force_list(uid, type);
			i = save_list.begin();
		} else if (i->init_uid == uid && i->init_type == type) {
			const int uid = i->targ_uid, type = i->targ_type;
			save_list.erase(i);
			add_to_list(uid, type);
			fill_force_list(uid, type);
			i = save_list.begin();
		} else {
			++i;
		}
	}
}

// весь нужный список генерится за один вызов fill_force_list
// поэтому потом в процессе сейва в цикле не надо дергать новые
// проверки и пытаться записать файлы
bool checking = false;

void check(long uid, int type) {
	if (checking) return;

	log("ObjSaveSync::check start uid %ld type %d", uid, type);
	checking = true;

	fill_force_list(uid, type);
	for (std::vector<ForceNodeType>::const_iterator i = force_list.begin(),
			 iend = force_list.end(); i != iend; ++i) {
		if (i->uid == uid && i->type == type) {
			// пропускаем лишний сейв себя
			continue;
		}
		if (type == PARCEL_SAVE && i->type == PARCEL_SAVE) {
			// у почты нужно совпадение только по типу
			continue;
		}
		write_file(i->uid, i->type);
	}
	force_list.clear();

	checking = false;
	log("ObjSaveSync::check end");
}

} // namespace ObjSaveSync

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
