/**
\file print_armor.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/global_objects.h"
#include "engine/ui/color.h"
#include "engine/ui/modify.h"

struct FilterType {
  FilterType() : type(-1), wear(EWearFlag::kUndefined), wear_message(-1), material(-1) {};
  int type;
  EWearFlag wear;
  int wear_message;
  int material;
  std::vector<int> affect; // аффекты weap
  std::vector<int> affect2; // аффекты apply
  std::vector<int> affect3; // экстрафлаг
};

void DoPrintArmor(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || (!IS_GRGOD(ch) && !ch->IsFlagged(EPrf::kCoderinfo))) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	FilterType filter;
	char tmpbuf[kMaxInputLength];
	bool find_param = false;
	while (*argument) {
		switch (*argument) {
			case 'М': argument = one_argument(++argument, tmpbuf);
				if (utils::IsAbbr(tmpbuf, "булат")) {
					filter.material = EObjMaterial::kBulat;
				} else if (utils::IsAbbr(tmpbuf, "бронза")) {
					filter.material = EObjMaterial::kBronze;
				} else if (utils::IsAbbr(tmpbuf, "железо")) {
					filter.material = EObjMaterial::kIron;
				} else if (utils::IsAbbr(tmpbuf, "сталь")) {
					filter.material = EObjMaterial::kSteel;
				} else if (utils::IsAbbr(tmpbuf, "кованая.сталь")) {
					filter.material = EObjMaterial::kForgedSteel;
				} else if (utils::IsAbbr(tmpbuf, "драг.металл")) {
					filter.material = EObjMaterial::kPreciousMetel;
				} else if (utils::IsAbbr(tmpbuf, "кристалл")) {
					filter.material = EObjMaterial::kCrystal;
				} else if (utils::IsAbbr(tmpbuf, "дерево")) {
					filter.material = EObjMaterial::kWood;
				} else if (utils::IsAbbr(tmpbuf, "прочное.дерево")) {
					filter.material = EObjMaterial::kHardWood;
				} else if (utils::IsAbbr(tmpbuf, "керамика")) {
					filter.material = EObjMaterial::kCeramic;
				} else if (utils::IsAbbr(tmpbuf, "стекло")) {
					filter.material = EObjMaterial::kGlass;
				} else if (utils::IsAbbr(tmpbuf, "камень")) {
					filter.material = EObjMaterial::kStone;
				} else if (utils::IsAbbr(tmpbuf, "кость")) {
					filter.material = EObjMaterial::kBone;
				} else if (utils::IsAbbr(tmpbuf, "ткань")) {
					filter.material = EObjMaterial::kCloth;
				} else if (utils::IsAbbr(tmpbuf, "кожа")) {
					filter.material = EObjMaterial::kSkin;
				} else if (utils::IsAbbr(tmpbuf, "органика")) {
					filter.material = EObjMaterial::kOrganic;
				} else if (utils::IsAbbr(tmpbuf, "береста")) {
					filter.material = EObjMaterial::kPaper;
				} else if (utils::IsAbbr(tmpbuf, "драг.камень")) {
					filter.material = EObjMaterial::kDiamond;
				} else {
					SendMsgToChar("Неверный материал предмета.\r\n", ch);
					return;
				}
				find_param = true;
				break;
			case 'Т': argument = one_argument(++argument, tmpbuf);
				if (utils::IsAbbr(tmpbuf, "броня") || utils::IsAbbr(tmpbuf, "armor")) {
					filter.type = EObjType::kArmor;
				} else if (utils::IsAbbr(tmpbuf, "легкие") || utils::IsAbbr(tmpbuf, "легкая")) {
					filter.type = EObjType::kLightArmor;
				} else if (utils::IsAbbr(tmpbuf, "средние") || utils::IsAbbr(tmpbuf, "средняя")) {
					filter.type = EObjType::kMediumArmor;
				} else if (utils::IsAbbr(tmpbuf, "тяжелые") || utils::IsAbbr(tmpbuf, "тяжелая")) {
					filter.type = EObjType::kHeavyArmor;
				} else {
					SendMsgToChar("Неверный тип предмета.\r\n", ch);
					return;
				}
				find_param = true;
				break;
			case 'О': argument = one_argument(++argument, tmpbuf);
				if (utils::IsAbbr(tmpbuf, "тело")) {
					filter.wear = EWearFlag::kBody;
					filter.wear_message = 3;
				} else if (utils::IsAbbr(tmpbuf, "голова")) {
					filter.wear = EWearFlag::kHead;
					filter.wear_message = 4;
				} else if (utils::IsAbbr(tmpbuf, "ноги")) {
					filter.wear = EWearFlag::kLegs;
					filter.wear_message = 5;
				} else if (utils::IsAbbr(tmpbuf, "ступни")) {
					filter.wear = EWearFlag::kFeet;
					filter.wear_message = 6;
				} else if (utils::IsAbbr(tmpbuf, "кисти")) {
					filter.wear = EWearFlag::kHands;
					filter.wear_message = 7;
				} else if (utils::IsAbbr(tmpbuf, "руки")) {
					filter.wear = EWearFlag::kArms;
					filter.wear_message = 8;
				} else {
					SendMsgToChar("Неверное место одевания предмета.\r\n", ch);
					return;
				}
				find_param = true;
				break;
			case 'А': {
				bool tmp_find = false;
				argument = one_argument(++argument, tmpbuf);
				if (!strlen(tmpbuf)) {
					SendMsgToChar("Неверный аффект предмета.\r\n", ch);
					return;
				}
				if (filter.affect.size() + filter.affect2.size() + filter.affect3.size() >= 3) {
					break;
				}
				switch (*tmpbuf) {
					case '1': sprintf(tmpbuf, "можно вплавить 1 камень");
						break;
					case '2': sprintf(tmpbuf, "можно вплавить 2 камня");
						break;
					case '3': sprintf(tmpbuf, "можно вплавить 3 камня");
						break;
					default: break;
				}
				utils::ConvertToLow(tmpbuf);
				size_t len = strlen(tmpbuf);
				int num = 0;

				for (int flag = 0; flag < 4; ++flag) {
					for (/* тут ничего не надо */; *weapon_affects[num] != '\n'; ++num) {
						if (strlen(weapon_affects[num]) < len)
							continue;
						if (!strncmp(weapon_affects[num], tmpbuf, len)) {
							filter.affect.push_back(num);
							tmp_find = true;
							break;
						}
					}
					if (tmp_find) {
						break;
					}
					++num;
				}
				if (!tmp_find) {
					for (num = 0; *apply_types[num] != '\n'; ++num) {
						if (strlen(apply_types[num]) < len)
							continue;
						if (!strncmp(apply_types[num], tmpbuf, len)) {
							filter.affect2.push_back(num);
							tmp_find = true;
							break;
						}
					}
				}
				// поиск по экстрафлагу
				if (!tmp_find) {
					num = 0;
					for (int flag = 0; flag < 4; ++flag) {
						for (/* тут ничего не надо */; *extra_bits[num] != '\n'; ++num) {
							if (strlen(extra_bits[num]) < len)
								continue;
							if (!strncmp(extra_bits[num], tmpbuf, len)) {
								filter.affect3.push_back(num);
								tmp_find = true;
								break;
							}
						}
						if (tmp_find) {
							break;
						}
						num++;
					}
				}
				if (!tmp_find) {
					sprintf(buf, "Неверный аффект предмета: '%s'.\r\n", tmpbuf);
					SendMsgToChar(buf, ch);
					return;
				}
				find_param = true;
				break;
			}
			default: ++argument;
		}
	}
	if (!find_param) {
		SendMsgToChar("Формат команды:\r\n"
					  "   armor Т[броня|легкие|средние|тяжелые] О[тело|голова|ногиступни|кисти|руки] А[аффект] М[материал]\r\n",
					  ch);
		return;
	}
	std::string buffer = "Выборка по следующим параметрам: ";
	if (filter.material >= 0) {
		buffer += material_name[filter.material];
		buffer += " ";
	}
	if (filter.type >= 0) {
		buffer += item_types[filter.type];
		buffer += " ";
	}
	if (filter.wear != EWearFlag::kUndefined) {
		buffer += wear_bits[filter.wear_message];
		buffer += " ";
	}
	if (!filter.affect.empty()) {
		for (const auto it : filter.affect) {
			buffer += weapon_affects[it];
			buffer += " ";
		}
	}
	if (!filter.affect2.empty()) {
		for (const auto it : filter.affect2) {
			buffer += apply_types[it];
			buffer += " ";
		}
	}
	if (!filter.affect3.empty()) {
		for (const auto it : filter.affect3) {
			buffer += extra_bits[it];
			buffer += " ";
		}
	}
	buffer += "\r\nСредний уровень мобов в зоне | внум предмета  | материал | имя предмета + аффекты если есть\r\n";
	SendMsgToChar(buffer, ch);

	std::multimap<int /* zone lvl */, int /* obj rnum */> tmp_list;
	for (const auto &i : obj_proto) {
		// материал
		if (filter.material >= 0 && filter.material != i->get_material()) {
			continue;
		}
		// тип
		if (filter.type >= 0 && filter.type != i->get_type()) {
			continue;
		}
		// куда можно одеть
		if (filter.wear != EWearFlag::kUndefined
			&& !i->has_wear_flag(filter.wear)) {
			continue;
		}
		// аффекты
		bool find = true;
		if (!filter.affect.empty()) {
			for (int it : filter.affect) {
				if (!CompareBits(i->get_affect_flags(), weapon_affects, it)) {
					find = false;
					break;
				}
			}
			// аффект не найден, продолжать смысла нет
			if (!find) {
				continue;
			}
		}

		if (!filter.affect2.empty()) {
			for (auto it = filter.affect2.begin(); it != filter.affect2.end() && find; ++it) {
				find = false;
				for (int k = 0; k < kMaxObjAffect; ++k) {
					if (i->get_affected(k).location == *it) {
						find = true;
						break;
					}
				}
			}
			// доп.свойство не найдено, продолжать смысла нет
			if (!find) {
				continue;
			}
		}
		if (!filter.affect3.empty()) {
			for (auto it = filter.affect3.begin(); it != filter.affect3.end() && find; ++it) {
				//find = true;
				if (!CompareBits(i->get_extra_flags(), extra_bits, *it)) {
					find = false;
					break;
				}
			}
			// экстрафлаг не найден, продолжать смысла нет
			if (!find) {
				continue;
			}
		}

		if (find) {
			const auto vnum = i->get_vnum() / 100;
			for (auto & nr : zone_table) {
				if (vnum == nr.vnum) {
					tmp_list.insert(std::make_pair(nr.mob_level, i->get_rnum()));
				}
			}
		}
	}

	std::ostringstream out;
	for (auto it = tmp_list.rbegin(), iend = tmp_list.rend(); it != iend; ++it) {
		const auto& obj = obj_proto[it->second];
		out << "   "
			<< std::setw(2) << it->first << " | "
			<< std::setw(7) << obj->get_vnum() << " | "
			<< std::setw(14) << material_name[obj->get_material()] << " | "
			<< obj->get_PName(ECase::kNom) << "\r\n";

		for (int i = 0; i < kMaxObjAffect; i++) {
			auto drndice = obj->get_affected(i).location;
			int drsdice = obj->get_affected(i).modifier;
			if (drndice == EApply::kNone || !drsdice) {
				continue;
			}
			sprinttype(drndice, apply_types, buf2);
			bool negative = IsNegativeApply(drndice);
			if (!negative && drsdice < 0) {
				negative = true;
			} else if (negative && drsdice < 0) {
				negative = false;
			}
			snprintf(buf, kMaxStringLength, "   %s%s%s%s%s%d%s\r\n",
					 kColorCyn, buf2, kColorNrm,
					 kColorCyn,
					 negative ? " ухудшает на " : " улучшает на ", abs(drsdice), kColorNrm);
			out << "      |         |                | " << buf;
		}
	}
	if (!out.str().empty()) {
		SendMsgToChar(ch, "Всего найдено предметов: %zu\r\n\r\n", tmp_list.size());
		page_string(ch->desc, out.str());
	} else {
		SendMsgToChar("Ничего не найдено.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
