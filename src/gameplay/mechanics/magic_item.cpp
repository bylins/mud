/**
 \file magic_item.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: magic-item recipe / rune mechanic (extracted from spells.cpp).
*/

#include "gameplay/mechanics/magic_item.h"
#include "gameplay/magic/spells.h"
#include "gameplay/magic/magic.h"
#include "gameplay/magic/magic_internal.h"
#include "gameplay/crafting/im.h"
#include "engine/db/global_objects.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/clans/house.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/communication/parcel.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/stuff.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/minions.h"
#include "utils/logger.h"
#include <map>
#include <sstream>
#include <cmath>

typedef std::map<EIngredientFlag, std::string> EIngredientFlag_name_by_value_t;
typedef std::map<const std::string, EIngredientFlag> EIngredientFlag_value_by_name_t;
EIngredientFlag_name_by_value_t EIngredientFlag_name_by_value;
EIngredientFlag_value_by_name_t EIngredientFlag_value_by_name;

void init_EIngredientFlag_ITEM_NAMES() {
	EIngredientFlag_name_by_value.clear();
	EIngredientFlag_value_by_name.clear();

	EIngredientFlag_name_by_value[EIngredientFlag::kItemRunes] = "kItemRunes";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemCheckUses] = "kItemCheckUses";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemCheckLag] = "kItemCheckLag";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemCheckLevel] = "kItemCheckLevel";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemDecayEmpty] = "kItemDecayEmpty";

	for (const auto &i : EIngredientFlag_name_by_value) {
		EIngredientFlag_value_by_name[i.second] = i.first;
	}
}

template<>
EIngredientFlag ITEM_BY_NAME(const std::string &name) {
	if (EIngredientFlag_name_by_value.empty()) {
		init_EIngredientFlag_ITEM_NAMES();
	}
	return EIngredientFlag_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<EIngredientFlag>(const EIngredientFlag item) {
	if (EIngredientFlag_name_by_value.empty()) {
		init_EIngredientFlag_ITEM_NAMES();
	}
	return EIngredientFlag_name_by_value.at(item);
}

extern im_type *imtypes;
extern int top_imtypes;

// vnum -> times-used, for the rune-usage reporting below.
static std::map<int /* vnum */, int /* count */> rune_list;

// magic-item lag/level requirement flags (used only by mag_item_ok).
constexpr Bitvector kMiLag1S = 1 << 0;
constexpr Bitvector kMiLag2S = 1 << 1;
constexpr Bitvector kMiLag4S = 1 << 2;
constexpr Bitvector kMiLag8S = 1 << 3;
constexpr Bitvector kMiLag16S = 1 << 4;
constexpr Bitvector kMiLag32S = 1 << 5;
constexpr Bitvector kMiLag64S = 1 << 6;
constexpr Bitvector kMiLag128S = 1 << 7;
constexpr Bitvector kMiLevel1 = 1 << 8;
constexpr Bitvector kMiLevel2 = 1 << 9;
constexpr Bitvector kMiLevel4 = 1 << 10;
constexpr Bitvector kMiLevel8 = 1 << 11;
constexpr Bitvector kMiLevel16 = 1 << 12;
constexpr Bitvector kMiLevel32 = 1 << 13;

static void AddRuneStats(CharData *ch, int vnum, int spelltype) {
	if (ch->IsNpc() || ESpellType::kRunes != spelltype) {
		return;
	}
	std::map<int, int>::iterator i = rune_list.find(vnum);
	if (rune_list.end() != i) {
		i->second += 1;
	} else {
		rune_list[vnum] = 1;
	}
}

static void extract_item(CharData *ch, ObjData *obj, int spelltype) {
	int extract = false;
	if (!obj) {
		return;
	}

	obj->set_val(3, time(nullptr));

	if (IS_SET(obj->get_spec_param(), kItemCheckUses)) {
		obj->dec_val(2);
		if (GET_OBJ_VAL(obj, 2) <= 0
			&& IS_SET(obj->get_spec_param(), kItemDecayEmpty)) {
			extract = true;
		}
	} else if (spelltype != ESpellType::kRunes) {
		extract = true;
	}

	if (extract) {
		if (spelltype == ESpellType::kRunes) {
			snprintf(buf, kMaxStringLength, "$o%s рассыпал$U у вас в руках.",
					 char_get_custom_label(obj, ch).c_str());
			act(buf, false, ch, obj, nullptr, kToChar);
		}
		RemoveObjFromChar(obj);
		ExtractObjFromWorld(obj);
	}
}

int CheckRecipeValues(CharData *ch, ESpell spell_id, ESpellType spell_type, int showrecipe) {
	int item0 = -1, item1 = -1, item2 = -1, obj_num = -1;
	struct SpellCreateItem *items;

	if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
		return (false);
	}
	if (!spell_create.contains(spell_id)) {
		SendMsgToChar("Вам не доступно это заклинание.\r\n", ch);
		return (false);
	}
	if (spell_type == ESpellType::kItemCast) {
		items = &spell_create[spell_id].items;
	} else if (spell_type == ESpellType::kPotionCast) {
		items = &spell_create[spell_id].potion;
	} else if (spell_type == ESpellType::kWandCast) {
		items = &spell_create[spell_id].wand;
	} else if (spell_type == ESpellType::kScrollCast) {
		items = &spell_create[spell_id].scroll;
	} else if (spell_type == ESpellType::kRunes) {
		items = &spell_create[spell_id].runes;
	} else
		return (false);

	if (((obj_num = GetObjRnum(items->rnumber)) < 0 &&
		spell_type != ESpellType::kItemCast && spell_type != ESpellType::kRunes) ||
		((item0 = GetObjRnum(items->items[0])) +
			(item1 = GetObjRnum(items->items[1])) + (item2 = GetObjRnum(items->items[2])) < -2)) {
		if (showrecipe)
			SendMsgToChar("Боги хранят в секрете этот рецепт.\n\r", ch);
		return (false);
	}

	if (!showrecipe)
		return (true);
	else {
		strcpy(buf, "Вам потребуется :\r\n");
		if (item0 >= 0) {
			strcat(buf, kColorBoldRed);
			strcat(buf, obj_proto[item0]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}
		if (item1 >= 0) {
			strcat(buf, kColorBoldYel);
			strcat(buf, obj_proto[item1]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}
		if (item2 >= 0) {
			strcat(buf, kColorBoldGrn);
			strcat(buf, obj_proto[item2]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}
		if (obj_num >= 0 && (spell_type == ESpellType::kItemCast || spell_type == ESpellType::kRunes)) {
			strcat(buf, kColorBoldBlu);
			strcat(buf, obj_proto[obj_num]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}

		strcat(buf, kColorNrm);
		if (spell_type == ESpellType::kItemCast || spell_type == ESpellType::kRunes) {
			strcat(buf, "для создания магии '");
			strcat(buf, MUD::Spell(spell_id).GetCName());
			strcat(buf, "'.");
		} else {
			strcat(buf, "для создания ");
			strcat(buf, obj_proto[obj_num]->get_PName(ECase::kGen).c_str());
		}
		act(buf, false, ch, nullptr, nullptr, kToChar);
	}

	return (true);
}

static bool mag_item_ok(CharData *ch, ObjData *obj, int spelltype) {
	int num = 0;

	if (spelltype == ESpellType::kRunes
		&& obj->get_type() != EObjType::kMagicIngredient) {
		return false;
	}

	if (obj->get_type() == EObjType::kMagicIngredient) {
		if ((!IS_SET(obj->get_spec_param(), kItemRunes) && spelltype == ESpellType::kRunes)
			|| (IS_SET(obj->get_spec_param(), kItemRunes) && spelltype != ESpellType::kRunes)) {
			return false;
		}
	}

	if (IS_SET(obj->get_spec_param(), kItemCheckUses)
		&& GET_OBJ_VAL(obj, 2) <= 0) {
		return false;
	}

	if (IS_SET(obj->get_spec_param(), kItemCheckLag)) {
		num = 0;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag1S))
			num += 1;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag2S))
			num += 2;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag4S))
			num += 4;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag8S))
			num += 8;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag16S))
			num += 16;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag32S))
			num += 32;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag64S))
			num += 64;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag128S))
			num += 128;
		if (GET_OBJ_VAL(obj, 3) + num - 5 * GetRealRemort(ch) >= time(nullptr))
			return false;
	}

	if (IS_SET(obj->get_spec_param(), kItemCheckLevel)) {
		num = 0;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel1))
			num += 1;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel2))
			num += 2;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel4))
			num += 4;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel8))
			num += 8;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel16))
			num += 16;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel32))
			num += 32;
		if (GetRealLevel(ch) + GetRealRemort(ch) < num)
			return false;
	}

	return true;
}

int CheckRecipeItems(CharData *ch, ESpell spell_id, ESpellType spell_type, int extract, CharData *tch) {
	ObjData *obj0 = nullptr, *obj1 = nullptr, *obj2 = nullptr, *obj3 = nullptr, *objo = nullptr;
	int item0 = -1, item1 = -1, item2 = -1, item3 = -1;
	int create = 0, obj_num = -1, percent = 0, num = 0;
	auto skill_id{ESkill::kUndefined};
	struct SpellCreateItem *items;

	if (spell_id <= ESpell::kUndefined) {
		return false;
	}
	if (!spell_create.contains(spell_id))
		return false;
	if (spell_type == ESpellType::kItemCast) {
		items = &spell_create[spell_id].items;
	} else if (spell_type == ESpellType::kPotionCast) {
		items = &spell_create[spell_id].potion;
		skill_id = ESkill::kCreatePotion;
		create = 1;
	} else if (spell_type == ESpellType::kWandCast) {
		items = &spell_create[spell_id].wand;
		skill_id = ESkill::kCreateWand;
		create = 1;
	} else if (spell_type == ESpellType::kScrollCast) {
		items = &spell_create[spell_id].scroll;
		skill_id = ESkill::kCreateScroll;
		create = 1;
	} else if (spell_type == ESpellType::kRunes) {
		items = &spell_create[spell_id].runes;
	} else {
		return (false);
	}
	item3 = items->rnumber;
	item0 = items->items[0];
	item1 = items->items[1];
	item2 = items->items[2];
	const int item0_rnum = item0 >= 0 ? GetObjRnum(item0) : -1;
	const int item1_rnum = item1 >= 0 ? GetObjRnum(item1) : -1;
	const int item2_rnum = item2 >= 0 ? GetObjRnum(item2) : -1;
	const int item3_rnum = item3 >= 0 ? GetObjRnum(item3) : -1;

	for (auto obj = ch->carrying; obj; obj = obj->get_next_content()) {
		if (item0 >= 0 && item0_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item0_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr)); //в вал3 сохраним время когда мы заюзали руну
			obj0 = obj;
			item0 = -2;
			objo = obj0;
			num++;
		} else if (item1 >= 0 && item1_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item1_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr));
			obj1 = obj;
			item1 = -2;
			objo = obj1;
			num++;
		} else if (item2 >= 0 && item2_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item2_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr));
			obj2 = obj;
			item2 = -2;
			objo = obj2;
			num++;
		} else if (item3 >= 0 && item3_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item3_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr));
			obj3 = obj;
			item3 = -2;
			objo = obj3;
			num++;
		}
	}

	if (!objo ||
		(items->items[0] >= 0 && item0 >= 0) ||
		(items->items[1] >= 0 && item1 >= 0) ||
		(items->items[2] >= 0 && item2 >= 0) || (items->rnumber >= 0 && item3 >= 0)) {
		return (false);
	}

	if (extract) {
		if (spell_type == ESpellType::kRunes) {
			strcpy(buf, "Вы сложили ");
		} else {
			strcpy(buf, "Вы взяли ");
		}

		ObjData::shared_ptr obj;
		if (create) {
			obj = world_objects.create_from_prototype_by_vnum(obj_num);
			if (!obj) {
				return false;
			} else {
				percent = number(1, MUD::Skill(skill_id).difficulty);
				auto prob = CalcCurrentSkill(ch, skill_id, nullptr);

				if (MUD::Skills().IsValid(skill_id) && percent > prob) {
					percent = -1;
				}
			}
		}

		if (item0 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj0->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj0, 1), spell_type);
		}

		if (item1 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj1->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj1, 1), spell_type);
		}

		if (item2 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj2->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj2, 1), spell_type);
		}

		if (item3 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj3->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj3, 1), spell_type);
		}

		strcat(buf, kColorNrm);

		if (create) {
			if (percent >= 0) {
				strcat(buf, " и создали $o3.");
				act(buf, false, ch, obj.get(), nullptr, kToChar);
				act("$n создал$g $o3.", false, ch, obj.get(), nullptr, kToRoom | kToArenaListen);
				PlaceObjToInventory(obj.get(), ch);
			} else {
				strcat(buf, " и попытались создать $o3.\r\n" "Ничего не вышло.");
				act(buf, false, ch, obj.get(), nullptr, kToChar);
				ExtractObjFromWorld(obj.get());
			}
		} else {
			if (spell_type == ESpellType::kItemCast) {
				strcat(buf, "и создали магическую смесь.\r\n");
				act(buf, false, ch, nullptr, nullptr, kToChar);
				act("$n смешал$g что-то в своей ноше.\r\n"
					"Вы почувствовали резкий запах.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			} else if (spell_type == ESpellType::kRunes) {
				sprintf(buf + strlen(buf),
						"котор%s вспыхнул%s ярким светом.%s",
						num > 1 ? "ые" : GET_OBJ_SUF_3(objo), num > 1 ? "и" : GET_OBJ_SUF_1(objo),
						ch->IsFlagged(EPrf::kCompact) ? "" : "\r\n");
				act(buf, false, ch, nullptr, nullptr, kToChar);
				act("$n сложил$g руны, которые вспыхнули ярким пламенем.",
					true, ch, nullptr, nullptr, kToRoom);
				sprintf(buf, "$n сложил$g руны в заклинание '%s'%s%s.",
						MUD::Spell(spell_id).GetCName(),
						(tch && tch != ch ? " на " : ""),
						(tch && tch != ch ? GET_PAD(tch, 1) : ""));
				act(buf, true, ch, nullptr, nullptr, kToArenaListen);
				auto magic_skill = GetMagicSkillId(spell_id);
				if (MUD::Skills().IsValid(magic_skill)) {
					TrainSkill(ch, magic_skill, true, tch);
				}
			}
		}
		extract_item(ch, obj0, spell_type);
		extract_item(ch, obj1, spell_type);
		extract_item(ch, obj2, spell_type);
		extract_item(ch, obj3, spell_type);
	}
	return (true);
}

void print_rune_stats(CharData *ch) {
	if (!ch->IsGrGod()) {
		SendMsgToChar(ch, "Только для иммов 33+.\r\n");
		return;
	}

	std::multimap<int, int> tmp_list;
	for (std::map<int, int>::const_iterator i = rune_list.begin(),
			 iend = rune_list.end(); i != iend; ++i) {
		tmp_list.insert(std::make_pair(i->second, i->first));
	}
	std::stringstream out;
	out << "Rune stats:\r\n" << "vnum -> count\r\n" << "--------------\r\n";

	for (std::multimap<int, int>::const_reverse_iterator i = tmp_list.rbegin(),
			 iend = tmp_list.rend(); i != iend; ++i) {
		
		out << i->second << " -> " << i->first << "\r\n";
	}
	SendMsgToChar(out.str(), ch);
}

void print_rune_log() {
	for (std::map<int, int>::const_iterator i = rune_list.begin(),
			 iend = rune_list.end(); i != iend; ++i) {
		log("RuneUsed: %d %d", i->first, i->second);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
