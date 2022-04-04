/* ************************************************************************
*   File: stuff.cpp                                     Part of Bylins    *
*  Usage: stuff load table handling                                       *
*                                                                         *
*  $Author$                                                          *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "stuff.h"

#include <cmath>

#include "world_objects.h"
#include "obj_prototypes.h"
#include "handler.h"
#include "corpse.h"
#include "color.h"
#include "game_mechanics/sets_drop.h"
#include "structs/global_objects.h"


extern std::vector<RandomObj> random_objs;
extern void set_obj_eff(ObjData *itemobj, const EApply type, int mod);
extern void set_obj_aff(ObjData *itemobj, const EAffect bitv);
extern int planebit(const char *str, int *plane, int *bit);

void LoadRandomObj(ObjData *obj) {
	// костыли, привет
	int plane, bit;
	for (auto robj : random_objs) {
		if (robj.vnum == GET_OBJ_VNUM(obj)) {
			obj->set_weight(number(robj.min_weight, robj.max_weight));
			obj->set_cost(number(robj.min_price, robj.max_price));
			obj->set_maximum_durability(number(robj.min_stability, robj.max_stability));
			obj->set_val(0, number(robj.value0_min, robj.value0_max));
			obj->set_val(1, number(robj.value1_min, robj.value1_max));
			obj->set_val(2, number(robj.value2_min, robj.value2_max));
			obj->set_val(3, number(robj.value3_min, robj.value3_max));
			for (auto nw : robj.not_wear) {
				if (nw.second < number(0, 1000)) {
					int numb = planebit(nw.first.c_str(), &plane, &bit);
					if (numb <= 0)
						return;
					obj->toggle_no_flag(plane, 1 << bit);
				}
			}
			for (auto aff : robj.affects) {
				if (aff.second < number(0, 1000)) {
					int numb = planebit(aff.first.c_str(), &plane, &bit);
					if (numb <= 0)
						return;
					obj->toggle_affect_flag(plane, 1 << bit);
				}
			}
			for (auto aff : robj.extraffect) {
				if (aff.chance < number(0, 1000)) {
					obj->set_affected_location(aff.number,
											   static_cast<EApply>(number(aff.min_val, aff.max_val)));
				}
			}
		}
	}
}

void oload_class::init() {
	std::string cppstr;
	std::istringstream isstream;
	bool in_block = false;
	ObjVnum ovnum;
	MobVnum mvnum;
	int oqty, lprob;

	clear();

	std::ifstream fp(LIB_MISC "stuff.lst");

	if (!fp) {
		cppstr = "oload_class:: Unable open input file !!!";
		mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
		return;
	}

	while (!fp.eof()) {
		getline(fp, cppstr);

		if (cppstr.empty() || cppstr[0] == ';')
			continue;
		if (cppstr[0] == '#') {
			cppstr.erase(cppstr.begin());

			if (cppstr.empty()) {
				cppstr = "oload_class:: Error in line '#' expected '#<RIGHT_obj_vnum>' !!!";
				mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				in_block = false;
				continue;
			}

			isstream.str(cppstr);
			isstream >> std::noskipws >> ovnum;

			if (!isstream.eof() || real_object(ovnum) < 0) {
				isstream.clear();
				cppstr = "oload_class:: Error in line '#" + cppstr + "' expected '#<RIGHT_obj_vnum>' !!!";
				mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				in_block = false;
				continue;
			}

			isstream.clear();

			in_block = true;
		} else if (in_block) {
			oqty = lprob = -1;

			isstream.str(cppstr);
			isstream >> std::skipws >> mvnum >> oqty >> lprob;

			if (lprob < 0 || lprob > MAX_LOAD_PROB || oqty < 0 || real_mobile(mvnum) < 0 || !isstream.eof()) {
				isstream.clear();
				cppstr = "oload_class:: Error in line '" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				cppstr =
					"oload_class:: \texpected '<RIGHT_mob_vnum>\t<0 <= obj_qty>\t<0 <= load_prob <= MAX_LOAD_PROB>' !!!";
				mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				continue;
			}

			isstream.clear();

			add_elem(mvnum, ovnum, obj_load_info(oqty, lprob));
		} else {
			cppstr = "oload_class:: Error in line '" + cppstr + "' expected '#<RIGHT_obj_vnum>' !!!";
			mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
		}
	}
}

oload_class oload_table;

ObjRnum ornum_by_info(const std::pair<ObjVnum, obj_load_info> &it) {
	ObjRnum i = real_object(it.first);
	ObjRnum resutl_obj = number(1, MAX_LOAD_PROB) <= it.second.load_prob
						  ? (it.first >= 0 && i >= 0
							 ? (obj_proto.actual_count(i) < it.second.obj_qty
								? i
								: kNothing)
							 : kNothing)
						  : kNothing;
	if (resutl_obj != kNothing) {
		log("Current load_prob: %d/%d, obj #%d (setload)", it.second.load_prob, MAX_LOAD_PROB, it.first);
	}
	return resutl_obj;
}

int get_stat_mod(int stat) {
	int mod = 0;
	switch (stat) {
		case EApply::kStr:
		case EApply::kDex:
		case EApply::kInt:
		case EApply::kWis:
		case EApply::kCon:
		case EApply::kCha: mod = 1;
			break;
		case EApply::kAc: mod = -10;
			break;
		case EApply::kHitroll: mod = 2;
			break;
		case EApply::kDamroll: mod = 3;
			break;
		case EApply::kSavingWill:
		case EApply::kSavingCritical:
		case EApply::kSavingStability:
		case EApply::kSavingReflex: mod = -10;
			break;
		case EApply::kHpRegen:
		case EApply::kMamaRegen: mod = 10;
			break;
		case EApply::kMorale:
		case EApply::kInitiative: mod = 3;
			break;
		case EApply::kAbsorbe:
		case EApply::kResistMind:
		case EApply::kCastSuccess: mod = 5;
			break;
		case EApply::kAffectResist:
		case EApply::kMagicResist: mod = 1;
			break;
	}
	return mod;
}

void generate_book_upgrd(ObjData *obj) {
	const auto skill_list = make_array<ESkill>(
		ESkill::kBackstab, ESkill::kPunctual, ESkill::kBash, ESkill::kHammer,
		ESkill::kOverwhelm, ESkill::kAddshot, ESkill::kAwake, ESkill::kNoParryHit,
		ESkill::kWarcry, ESkill::kIronwind, ESkill::kStrangle);

	auto skill_id = skill_list[number(0, skill_list.size() - 1)];
	std::string book_name = MUD::Skills()[skill_id].name;

	obj->set_val(1, to_underlying(skill_id));
	obj->set_aliases("книга секретов умения: " + book_name);
	obj->set_short_description("книга секретов умения: " + book_name);
	obj->set_description("Книга секретов умения: " + book_name + " лежит здесь.");

	obj->set_PName(0, "книга секретов умения: " + book_name);
	obj->set_PName(1, "книги секретов умения: " + book_name);
	obj->set_PName(2, "книге секретов умения: " + book_name);
	obj->set_PName(3, "книгу секретов умения: " + book_name);
	obj->set_PName(4, "книгой секретов умения: " + book_name);
	obj->set_PName(5, "книге секретов умения: " + book_name);
}

void generate_warrior_enchant(ObjData *obj) {
	const auto main_list = make_array<EApply>(
		EApply::kStr, EApply::kDex, EApply::kCon, EApply::kAc, EApply::kDamroll);

	const auto second_list = make_array<EApply>(
		EApply::kHitroll, EApply::kSavingWill, EApply::kSavingCritical,
		EApply::kSavingStability, EApply::kHpRegen, EApply::kSavingReflex,
		EApply::kMorale, EApply::kInitiative, EApply::kAbsorbe, EApply::kAffectResist, EApply::kMagicResist);

	switch (GET_OBJ_VNUM(obj)) {
		case GlobalDrop::WARR1_ENCHANT_VNUM: {
			auto stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
			set_obj_eff(obj, stat, get_stat_mod(stat));
			break;
		}
		case GlobalDrop::WARR2_ENCHANT_VNUM: {
			auto stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
			set_obj_eff(obj, stat, get_stat_mod(stat));

			stat = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
			set_obj_eff(obj, stat, get_stat_mod(stat));
			break;
		}
		case GlobalDrop::WARR3_ENCHANT_VNUM: {
			auto stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
			set_obj_eff(obj, stat, get_stat_mod(stat) * 2);        //Double effect from main_stat

			stat = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
			set_obj_eff(obj, stat, get_stat_mod(stat));
			break;
		}
		default: sprintf(buf2, "SYSERR: Unknown vnum warrior enchant object: %d", GET_OBJ_VNUM(obj));
			mudlog(buf2, BRF, kLvlImmortal, SYSLOG, true);
			break;
	}
}

void generate_magic_enchant(ObjData *obj) {
	const auto main_list = make_array<EApply>(
		EApply::kStr, EApply::kDex, EApply::kCon, EApply::kInt, EApply::kWis,
		EApply::kCha, EApply::kAc, EApply::kDamroll, EApply::kAffectResist, EApply::kMagicResist);

	const auto second_list = make_array<EApply>(
		EApply::kHitroll, EApply::kSavingWill, EApply::kSavingCritical,
		EApply::kSavingStability, EApply::kHpRegen, EApply::kSavingReflex,
		EApply::kMorale, EApply::kInitiative, EApply::kAbsorbe, EApply::kAffectResist, EApply::kMagicResist,
		EApply::kMamaRegen, EApply::kCastSuccess, EApply::kResistMind, EApply::kDamroll);

	switch (GET_OBJ_VNUM(obj)) {
		case GlobalDrop::MAGIC1_ENCHANT_VNUM: {
			EApply effect = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
			set_obj_eff(obj, effect, get_stat_mod(effect));
			break;
		}
		case GlobalDrop::MAGIC2_ENCHANT_VNUM: {
			auto effect = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
			set_obj_eff(obj, effect, get_stat_mod(effect) * 2);

			effect = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
			set_obj_eff(obj, effect, get_stat_mod(effect));
			break;
		}
		case GlobalDrop::MAGIC3_ENCHANT_VNUM: {
			auto stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
			set_obj_eff(obj, stat, get_stat_mod(stat) * 2);

			stat = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
			set_obj_eff(obj, stat, get_stat_mod(stat));

			if (number(0, 1) == 0) {
				stat = main_list[number(0, static_cast<int>(main_list.size()) - 1)];
				set_obj_eff(obj, stat, get_stat_mod(stat) * 2);
			} else {
				stat = second_list[number(0, static_cast<int>(second_list.size()) - 1)];
				set_obj_eff(obj, stat, get_stat_mod(stat));
			}
			break;
		}
		default: sprintf(buf2, "SYSERR: Unknown vnum magic enchant object: %d", GET_OBJ_VNUM(obj));
			mudlog(buf2, BRF, kLvlImmortal, SYSLOG, true);
			break;
	}
}

/**
 * \param setload = true - лоад через систему дропа сетов
 *        setload = false - лоад через глобал дроп
 */
void obj_to_corpse(ObjData *corpse, CharData *ch, int rnum, bool setload) {
	const auto o = world_objects.create_from_prototype_by_rnum(rnum);
	if (!o) {
		log("SYSERROR: null from read_object rnum=%d (%s:%d)", rnum, __FILE__, __LINE__);
		return;
	}

	log("Load obj #%d by %s in room #%d (%s)",
		o->get_vnum(), GET_NAME(ch), GET_ROOM_VNUM(ch->in_room),
		setload ? "setload" : "globaldrop");

	if (!setload) {
		switch (o->get_vnum()) {
			case GlobalDrop::kSkillUpgradeBookVnum: generate_book_upgrd(o.get());
				break;

			case GlobalDrop::WARR1_ENCHANT_VNUM:
			case GlobalDrop::WARR2_ENCHANT_VNUM:
			case GlobalDrop::WARR3_ENCHANT_VNUM: generate_warrior_enchant(o.get());
				break;

			case GlobalDrop::MAGIC1_ENCHANT_VNUM:
			case GlobalDrop::MAGIC2_ENCHANT_VNUM:
			case GlobalDrop::MAGIC3_ENCHANT_VNUM: generate_magic_enchant(o.get());
				break;
		}
	} else {
		for (const auto tch : world[ch->in_room]->people) {
			send_to_char(tch, "%sДиво дивное, чудо чудное!%s\r\n", CCGRN(tch, C_NRM), CCNRM(tch, C_NRM));
		}
	}
	o->set_vnum_zone_from(99999);
	if (MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
		obj_to_room(o.get(), ch->in_room);
	} else {
		obj_to_obj(o.get(), corpse);
	}

	if (!obj_decay(o.get())) {
		if (o->get_in_room() != kNowhere) {
			act("На земле остал$U лежать $o.", false, ch, o.get(), 0, kToRoom);
		}
		load_otrigger(o.get());
	}
}

void obj_load_on_death(ObjData *corpse, CharData *ch) {
	if (ch == nullptr
		|| !ch->is_npc()
		|| (!MOB_FLAGGED(ch, EMobFlag::kCorpse)
			&& corpse == nullptr)) {
		return;
	}

	const int rnum = SetsDrop::check_mob(GET_MOB_RNUM(ch));
	if (rnum > 0) {
		obj_to_corpse(corpse, ch, rnum, true);
	}

	if (GlobalDrop::check_mob(corpse, ch)) {
		return;
	}

	oload_class::const_iterator p = oload_table.find(GET_MOB_VNUM(ch));

	if (p == oload_table.end()) {
		return;
	}

	std::vector<ObjRnum> v(p->second.size());
	std::transform(p->second.begin(), p->second.end(), v.begin(), ornum_by_info);

	for (size_t i = 0; i < v.size(); i++) {
		if (v[i] >= 0) {
			obj_to_corpse(corpse, ch, v[i], false);
		}
	}
}

// готовим прототипы шмоток для зверюшек (Кудояр)
void create_charmice_stuff(CharData *ch, const ESkill skill_id, int diff) {
	const auto obj = world_objects.create_blank();
	int position = 0;
	obj->set_aliases("острые когти");
	const std::string descr = std::string("острые когти ") + ch->get_pad(1);
	obj->set_short_description(descr);
	obj->set_description("Острые когти лежат здесь.");
	obj->set_ex_description(descr.c_str(), "Острые когти лежат здесь.");
	obj->set_PName(0, "острые когти");
	obj->set_PName(1, "острых когтей");
	obj->set_PName(2, "острым когтям");
	obj->set_PName(3, "острые когти");
	obj->set_PName(4, "острыми когтями");
	obj->set_PName(5, "острых когтях");
	obj->set_sex(ESex::kPoly);
	obj->set_type(EObjType::kWeapon);
	// среднее оружки
	obj->set_val(1, floorf(diff/18.0)); // при 100 скила куб. = 5  	при 200 скила = 11
	obj->set_val(2, floorf(diff/27.0)); // при 100 скила граней = d4  при 200 скила = d7
	//подсчет среднего оружия	// итог средне при 100 скила = 12,5  при 200 скила = 44
	obj->set_cost(1);
	obj->set_rent_off(1);
	obj->set_rent_on(1);
	obj->set_timer(9999);
	//ставим флаги на шмотки
	obj->set_wear_flags(to_underlying(EWearFlag::kTake));
	obj->set_wear_flags(to_underlying(EWearFlag::kUndefined)); // в теории никак не взять одежку
	// obj->set_no_flag(ENoFlag::ITEM_NO_MONO); // пресекаем всяких абузеров
	// obj->set_no_flag(ENoFlag::ITEM_NO_POLY); // 
	obj->set_extra_flag(EObjFlag::kNosell);
	obj->set_extra_flag(EObjFlag::kNolocate);
	obj->set_extra_flag(EObjFlag::kDecay);
	obj->set_extra_flag(EObjFlag::kNodisarm);
	obj->set_extra_flag(EObjFlag::kBless);
	obj->set_extra_flag(EObjFlag::kNodrop);

	obj->set_maximum_durability(5000);
	obj->set_current_durability(5000);
	obj->set_material(EObjMaterial::kCrystal);

	obj->set_weight(floorf(diff/9.0));

	switch (skill_id)
	{
	case ESkill::kClubs: // дубины
		obj->set_val(3, 12);
		obj->set_skill(141);
		obj->set_extra_flag(EObjFlag::kThrowing);
		obj->set_affected(0, EApply::kStr, floorf(diff/12.0));
		obj->set_affected(1, EApply::kSavingStability, -floorf(diff/4.0));
		create_charmice_stuff(ch, ESkill::kIncorrect, diff);
		position = 16;
		break;
	case ESkill::kSpades: // копья
		obj->set_val(3, 11);
		obj->set_skill(148);
		obj->set_extra_flag(EObjFlag::kThrowing);
		create_charmice_stuff(ch, ESkill::kShieldBlock, diff);
		create_charmice_stuff(ch, ESkill::kIncorrect, diff);
		position = 16;
		break;
	case ESkill::kPicks: // стабер
		obj->set_val(3, 11);
		obj->set_skill(147);
		obj->set_affected(0, EApply::kStr, floorf(diff/16.0));
		obj->set_affected(1, EApply::kDex, floorf(diff/10.0));
		create_charmice_stuff(ch, ESkill::kIncorrect, diff);
		position = 16;
		break;
	case ESkill::kAxes: // секиры
		obj->set_val(3, 8);
		obj->set_skill(142);
		obj->set_affected(0, EApply::kStr, floorf(diff/12.0));
		obj->set_affected(1, EApply::kDex, floorf(diff/15.0));
		obj->set_affected(2, EApply::kDamroll, floorf(diff/10.0));
		obj->set_affected(3, EApply::kHp, 5*(diff));
		create_charmice_stuff(ch, ESkill::kShieldBlock, diff);
		create_charmice_stuff(ch, ESkill::kIncorrect, diff);
		position = 16;
		break;
	case ESkill::kBows: // луки
		obj->set_val(3, 2);
		obj->set_skill(154);
		obj->set_affected(0, EApply::kStr, floorf(diff/20.0));
		obj->set_affected(1, EApply::kDex, floorf(diff/15.0));
		create_charmice_stuff(ch, ESkill::kIncorrect, diff);
		position = 18;
		break;
	case ESkill::kTwohands: // двуруч
		obj->set_val(3, 1);
		obj->set_skill(146);
		obj->set_weight(floorf(diff/4.0)); // 50 вес при 200% скила
		obj->set_affected(0, EApply::kStr, floorf(diff/15.0));
		obj->set_affected(1, EApply::kDamroll, floorf(diff/13.0));
		create_charmice_stuff(ch, ESkill::kIncorrect, diff);
		position = 18;
		break;
	case ESkill::kPunch: // кулачка
		obj->set_type(EObjType::kArmor);
		obj->set_affected(0, EApply::kDamroll, floorf(diff/10.0));
		create_charmice_stuff(ch, ESkill::kIncorrect, diff);
		position = 9;
		break;
	case ESkill::kLongBlades: // длинные
		obj->set_val(3, 10);
		obj->set_skill(143);
		obj->set_affected(0, EApply::kStr, floorf(diff/15.0));
		obj->set_affected(1, EApply::kDex, floorf(diff/12.0));
		obj->set_affected(2, EApply::kSavingReflex, -floorf(diff/3.5));
		create_charmice_stuff(ch, ESkill::kIncorrect, -1); // так изощренно создаем обувку(-1), итак кэйсов наплодил
		create_charmice_stuff(ch, ESkill::kIncorrect, diff);
		position = 16;
		break;
	case ESkill::kShieldBlock: // блок щитом ? делаем щит
		obj->set_type(EObjType::kArmor);
		obj->set_description("Роговые пластины лежат здесь.");
		obj->set_ex_description(descr.c_str(), "Роговые пластины лежат здесь.");
		obj->set_aliases("роговые пластины");
		obj->set_short_description("роговые пластины");
		obj->set_PName(0, "роговые пластины");
		obj->set_PName(1, "роговых пластин");
		obj->set_PName(2, "роговым пластинам");
		obj->set_PName(3, "роговые пластины");
		obj->set_PName(4, "роговыми пластинами");
		obj->set_PName(5, "роговых пластинах");
		obj->set_val(1, floorf(diff/13.0));
		obj->set_val(2, floorf(diff/8.0));
		obj->set_affected(0, EApply::kSavingStability, -floorf(diff/3.0));
		obj->set_affected(1, EApply::kSavingCritical, -floorf(diff/3.5));
		obj->set_affected(2, EApply::kSavingReflex, -floorf(diff/3.0));
		obj->set_affected(3, EApply::kSavingWill, -floorf(diff/3.5));
		position = 11; // слот щит
		break;		
	default: //ESkill::kIncorrect / тут шкура(армор)
		obj->set_sex(ESex::kFemale);
		obj->set_description("Прочная шкура лежит здесь.");
		obj->set_ex_description(descr.c_str(), "Прочная шкура лежит здесь.");
		obj->set_aliases("прочная шкура");
		obj->set_short_description("прочная шкура");
		obj->set_PName(0, "прочная шкура");
		obj->set_PName(1, "прочной шкурой");
		obj->set_PName(2, "прочной шкуре");
		obj->set_PName(3, "прочную шкуру");
		obj->set_PName(4, "прочной шкурой");
		obj->set_PName(5, "прочной шкуре");
		obj->set_type(EObjType::kArmor);
		if (diff == -1) { // тут делаем сапоги 
			obj->set_sex(ESex::kPoly);
			obj->set_weight(50);
			obj->set_description("Оторванная лапа зверя лежит здесь.");
			obj->set_ex_description(descr.c_str(), "Оторванная лапа зверя лежит здесь.");
			obj->set_aliases("огромные лапы");
			obj->set_short_description("огромные лапы");
			obj->set_PName(0, "огромные лапы");
			obj->set_PName(1, "огромных лап");
			obj->set_PName(2, "огромным лапам");
			obj->set_PName(3, "огромные лапы");
			obj->set_PName(4, "огромными лапами");
			obj->set_PName(5, "огромных лапах");
			position = 8; // слот ступни
			break;
		}
		obj->set_val(1, floorf(diff/11.0));
		obj->set_val(2, floorf(diff/7.0));
		obj->set_affected(0, EApply::kSavingStability, -floorf(diff*0.7));
		obj->set_affected(1, EApply::kSavingCritical, -floorf(diff*0.7));
		obj->set_affected(2, EApply::kSavingReflex, -floorf(diff*0.7));
		obj->set_affected(3, EApply::kSavingWill, -floorf(diff*0.6));
		obj->set_affected(4, EApply::kMagicResist, floorf(diff*0.16));
		obj->set_affected(5, EApply::kPhysicResist, floorf(diff*0.15));
		position = 5; // слот тело
		break;
	}
	// одеваем шмотки
	equip_char(ch, obj.get(), position, CharEquipFlags());
}






// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
