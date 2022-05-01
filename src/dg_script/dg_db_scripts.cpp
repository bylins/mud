/* ************************************************************************
*  File: dg_db_scripts.cpp                                 Part of Bylins *
*                                                                         *
*  Usage: Contains routines to handle db functions for scripts and trigs  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                         *
*  $Date$                                           *
*  $Revision$                                                   *
************************************************************************ */
#include "dg_db_scripts.h"

#include "obj_prototypes.h"
#include "dg_scripts.h"
#include "handler.h"
#include "dg_event.h"
#include "game_magic/magic.h"
#include "game_magic/magic_temp_spells.h"
#include "structs/global_objects.h"

#include <stack>

//External functions
extern void extract_trigger(Trigger *trig);

//внум_триггера : [внум_триггера_который_прикрепил_данный тригер : [перечисление к чему прикрепленно (внумы объектов/мобов/комнат)]]
trigger_to_owners_map_t owner_trig;

extern int top_of_trigt;

extern IndexData *mob_index;

// TODO: Get rid of me
char *dirty_indent_trigger(char *cmd, int *level) {
	static std::stack<std::string> indent_stack;

	*level = std::max(0, *level);
	if (*level == 0) {
		std::stack<std::string> empty_stack;
		indent_stack.swap(empty_stack);
	}

	// уровни вложения
	int currlev, nextlev;
	currlev = nextlev = *level;

	if (!cmd) {
		return cmd;
	}

	// Удаляем впереди идущие пробелы.
	char *ptr = cmd;
	skip_spaces(&ptr);

	// ptr содержит строку без первых пробелов.
	if (!strn_cmp("case ", ptr, 5) || !strn_cmp("default", ptr, 7)) {
		// последовательные case (или default после case) без break
		if (!indent_stack.empty()
			&& !strn_cmp("case ", indent_stack.top().c_str(), 5)) {
			--currlev;
		} else {
			indent_stack.push(ptr);
		}
		nextlev = currlev + 1;
	} else if (!strn_cmp("if ", ptr, 3) || !strn_cmp("while ", ptr, 6)
		|| !strn_cmp("foreach ", ptr, 8) || !strn_cmp("switch ", ptr, 7)) {
		++nextlev;
		indent_stack.push(ptr);
	} else if (!strn_cmp("elseif ", ptr, 7) || !strn_cmp("else", ptr, 4)) {
		--currlev;
	} else if (!strn_cmp("break", ptr, 5) || !strn_cmp("end", ptr, 3)
		|| !strn_cmp("done", ptr, 4)) {
		// в switch завершающий break можно опускать и сразу писать done|end
		if ((!strn_cmp("done", ptr, 4) || !strn_cmp("end", ptr, 3))
			&& !indent_stack.empty()
			&& (!strn_cmp("case ", indent_stack.top().c_str(), 5)
				|| !strn_cmp("default", indent_stack.top().c_str(), 7))) {
			--currlev;
			--nextlev;
			indent_stack.pop();
		}
		if (!indent_stack.empty()) {
			indent_stack.pop();
		}
		--nextlev;
		--currlev;
	}

	if (nextlev < 0) nextlev = 0;
	if (currlev < 0) currlev = 0;

	// Вставляем дополнительные пробелы

	char *tmp = (char *) malloc(currlev * 2 + 1);
	memset(tmp, 0x20, currlev * 2);
	tmp[currlev * 2] = '\0';

	tmp = str_add(tmp, ptr);

	cmd = (char *) realloc(cmd, strlen(tmp) + 1);
	cmd = strcpy(cmd, tmp);

	free(tmp);

	*level = nextlev;
	return cmd;
}

void indent_trigger(std::string &cmd, int *level) {
	char *cmd_copy = str_dup(cmd.c_str());;
	cmd_copy = dirty_indent_trigger(cmd_copy, level);
	cmd = cmd_copy;
	free(cmd_copy);
}

/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
Trigger *read_trigger(int nr) {
	IndexData *index;
	if (nr >= top_of_trigt || nr == -1) {
		return nullptr;
	}

	if ((index = trig_index[nr]) == nullptr) {
		return nullptr;
	}

	Trigger *trig = new Trigger(*index->proto);
	index->total_online++;

	return trig;
}

// vnum_owner - триг, который приаттачил данный триг
// vnum_trig - внум приатаченного трига
// vnum - к кому приатачился триг
void add_trig_to_owner(int vnum_owner, int vnum_trig, int vnum) {
	if (owner_trig[vnum_trig].find(vnum_owner) != owner_trig[vnum_trig].end()) {
		const auto &triggers_set = owner_trig[vnum_trig][vnum_owner];
		const bool flag_trig = triggers_set.find(vnum) != triggers_set.end();

		if (!flag_trig) {
			owner_trig[vnum_trig][vnum_owner].insert(vnum);
		}
	} else {
		triggers_set_t tmp_vector = {vnum};
		owner_trig[vnum_trig].emplace(-1, tmp_vector);
	}
}

void dg_obj_trigger(char *line, ObjData *obj) {
	char junk[8];
	int vnum, rnum, count;

	count = sscanf(line, "%s %d", junk, &vnum);

	if (count != 2)    // should do a better job of making this message
	{
		log("SYSERR: Error assigning trigger!");
		return;
	}

	rnum = real_trigger(vnum);
	if (rnum < 0) {
		sprintf(line, "SYSERR: Trigger vnum #%d asked for but non-existant!", vnum);
		log("%s", line);
		return;
	}

	// для начала определяем, есть ли такой внум у нас в контейнере
	if (owner_trig.find(vnum) == owner_trig.end()) {
		owner_to_triggers_map_t tmp_map;
		owner_trig.emplace(vnum, tmp_map);
	}
	add_trig_to_owner(-1, vnum, GET_OBJ_VNUM(obj));

	obj->add_proto_script(vnum);
}

extern CharData *mob_proto;

void assign_triggers(void *i, int type) {
	CharData *mob;
	ObjData *obj;
	RoomData *room;
	int rnum;
	char buf[256];

	switch (type) {
		case MOB_TRIGGER: mob = (CharData *) i;
			for (const auto trigger_vnum : *mob_proto[GET_MOB_RNUM(mob)].proto_script) {
				rnum = real_trigger(trigger_vnum);
				if (rnum == -1) {
					const auto rnum = mob->get_rnum();
					sprintf(buf, "SYSERR: trigger #%d non-existent, for mob #%d", trigger_vnum, mob_index[rnum].vnum);
					log("%s", buf);
				} else {
					if (trig_index[rnum]->proto->get_attach_type() != MOB_TRIGGER) {
						const auto rnum = mob->get_rnum();
						sprintf(buf, "SYSERR: trigger #%d has wrong attach_type: %d, for mob #%d",
								trigger_vnum,
								static_cast<int>(trig_index[rnum]->proto->get_attach_type()),
								mob_index[rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
					} else {
						auto trig = read_trigger(rnum);
						if (add_trigger(SCRIPT(mob).get(), trig, -1)) {
							if (owner_trig.find(trigger_vnum) == owner_trig.end()) {
								owner_to_triggers_map_t tmp_map;
								owner_trig.emplace(trigger_vnum, tmp_map);
							}
							add_trig_to_owner(-1, trigger_vnum, GET_MOB_VNUM(mob));
						} else {
							extract_trigger(trig);
						}
					}
				}
			}
			break;

		case OBJ_TRIGGER: obj = (ObjData *) i;
			for (const auto trigger_vnum : obj_proto.proto_script(GET_OBJ_RNUM(obj))) {
				rnum = real_trigger(trigger_vnum);
				if (rnum == -1) {
					sprintf(buf, "SYSERR: trigger #%d non-existent, for obj #%d",
							trigger_vnum, obj->get_vnum());
					log("%s", buf);
				} else {
					if (trig_index[rnum]->proto->get_attach_type() != OBJ_TRIGGER) {
						sprintf(buf, "SYSERR: trigger #%d has wrong attach_type: %d, for obj #%d",
								trigger_vnum,
								static_cast<int>(trig_index[rnum]->proto->get_attach_type()),
								obj->get_vnum());
						mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
					} else {
						auto trig = read_trigger(rnum);
						if (add_trigger(obj->get_script().get(), trig, -1)) {
							if (owner_trig.find(trigger_vnum) == owner_trig.end()) {
								owner_to_triggers_map_t tmp_map;
								owner_trig.emplace(trigger_vnum, tmp_map);
							}
							add_trig_to_owner(-1, trigger_vnum, GET_OBJ_VNUM(obj));
						} else {
							extract_trigger(trig);
						}
					}
				}
			}
			break;

		case WLD_TRIGGER: room = (RoomData *) i;
			for (const auto trigger_vnum : *room->proto_script) {
				rnum = real_trigger(trigger_vnum);
				if (rnum == -1) {
					sprintf(buf, "SYSERR: trigger #%d non-existant, for room #%d",
							trigger_vnum, room->room_vn);
					log("%s", buf);
				} else {
					if (trig_index[rnum]->proto->get_attach_type() != WLD_TRIGGER) {
						sprintf(buf, "SYSERR: trigger #%d has wrong attach_type: %d, for room #%d",
								trigger_vnum, static_cast<int>(trig_index[rnum]->proto->get_attach_type()),
								room->room_vn);
						mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
					} else {
						auto trig = read_trigger(rnum);
						if (add_trigger(SCRIPT(room).get(), trig, -1)) {
							if (owner_trig.find(trigger_vnum) == owner_trig.end()) {
								owner_to_triggers_map_t tmp_map;
								owner_trig.emplace(trigger_vnum, tmp_map);
							}
							add_trig_to_owner(-1, trigger_vnum, room->room_vn);
						} else {
							extract_trigger(trig);
						}
					}
				}
			}
			break;

		default: log("SYSERR: unknown type for assign_triggers()");
			break;
	}
}

void trg_featturn(CharData *ch, EFeat feat_id, int featdiff, int vnum) {
	if (ch->HaveFeat(feat_id)) {
		if (featdiff)
			return;
		else {
			sprintf(buf, "Вы утратили способность '%s'.\r\n", feat_info[feat_id].name);
			SendMsgToChar(buf, ch);
			log("Remove %s to %s (trigfeatturn) trigvnum %d", feat_info[feat_id].name, GET_NAME(ch), vnum);
			ch->UnsetFeat(feat_id);
		}
	} else {
		if (featdiff) {
			if (MUD::Classes(ch->GetClass()).feats.IsAvailable(feat_id)) {
				sprintf(buf, "Вы обрели способность '%s'.\r\n", feat_info[feat_id].name);
				SendMsgToChar(buf, ch);
				log("Add %s to %s (trigfeatturn) trigvnum %d", feat_info[feat_id].name, GET_NAME(ch), vnum);
				ch->SetFeat(feat_id);
			}
		};
	}
}

void trg_skillturn(CharData *ch, const ESkill skill_id, int skilldiff, int vnum) {
	const auto ch_class = static_cast<ECharClass>(GET_CLASS(ch));
	if (ch->get_trained_skill(skill_id)) {
		if (skilldiff) {
			return;
		}
		ch->set_skill(skill_id, 0);
		SendMsgToChar(ch, "Вас лишили умения '%s'.\r\n", MUD::Skills(skill_id).GetName());
		log("Remove %s from %s (trigskillturn)", MUD::Skills(skill_id).GetName(), GET_NAME(ch));
	} else if (skilldiff && MUD::Classes(ch_class).skills[skill_id].IsAvailable()) {
		ch->set_skill(skill_id, 5);
		SendMsgToChar(ch, "Вы изучили умение '%s'.\r\n", MUD::Skills(skill_id).GetName());
		log("Add %s to %s (trigskillturn) trigvnum %d", MUD::Skills(skill_id).GetName(), GET_NAME(ch), vnum);
	}
}

void AddSkill(CharData *ch, const ESkill skillnum, int skilldiff, int vnum) {
	int skill = ch->get_trained_skill(skillnum);
	ch->set_skill(skillnum, std::clamp(skill + skilldiff, 1, MUD::Skills(skillnum).cap));

	if (skill > ch->get_trained_skill(skillnum)) {
		SendMsgToChar(ch, "Ваше умение '%s' понизилось.\r\n", MUD::Skills(skillnum).GetName());
		log("Decrease %s to %s from %d to %d (diff %d)(trigskilladd) trigvnum %d",
			MUD::Skills(skillnum).GetName(), GET_NAME(ch), skill,
			ch->get_trained_skill(skillnum), skilldiff, vnum);
	} else if (skill < ch->get_trained_skill(skillnum)) {
		SendMsgToChar(ch, "Вы повысили свое умение '%s'.\r\n", MUD::Skills(skillnum).GetName());
		log("Raise %s to %s from %d to %d (diff %d)(trigskilladd) trigvnum %d",
			MUD::Skills(skillnum).GetName(), GET_NAME(ch), skill,
			ch->get_trained_skill(skillnum), skilldiff, vnum);
	} else {
		SendMsgToChar(ch, "Ваше умение '%s' не изменилось.\r\n", MUD::Skills(skillnum).GetName());
		log("Unchanged %s to %s from %d to %d (diff %d)(trigskilladd) trigvnum %d",
			MUD::Skills(skillnum).GetName(), GET_NAME(ch), skill,
			ch->get_trained_skill(skillnum), skilldiff, vnum);
	}
}

void trg_spellturn(CharData *ch, ESpell spell_id, int spelldiff, int vnum) {
	int spell = GET_SPELL_TYPE(ch, spell_id);

	if (!IsAbleToGetSpell(ch, spell_id)) {
		log("Error trying to add %s to %s (trigspell) trigvnum %d", GetSpellName(spell_id), GET_NAME(ch), vnum);
		return;
	}

	if (spell & ESpellType::kKnow) {
		if (spelldiff) return;

		REMOVE_BIT(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow);
		if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp))
			GET_SPELL_MEM(ch, spell_id) = 0;
		SendMsgToChar(ch, "Вы начисто забыли заклинание '%s'.\r\n", GetSpellName(spell_id));
		log("Remove %s from %s (trigspell) trigvnum %d", GetSpellName(spell_id), GET_NAME(ch), vnum);
	} else if (spelldiff) {
		SET_BIT(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow);
		SendMsgToChar(ch, "Вы постигли заклинание '%s'.\r\n", GetSpellName(spell_id));
		log("Add %s to %s (trigspell) trigvnum %d", GetSpellName(spell_id), GET_NAME(ch), vnum);
	}
}

void trg_spellturntemp(CharData *ch, ESpell spell_id, int spelldiff, int vnum) {
	if (!IsAbleToGetSpell(ch, spell_id)) {
		log("Error trying to add %s to %s (trigspelltemp) trigvnum %d", GetSpellName(spell_id), GET_NAME(ch), vnum);
		return;
	}

	Temporary_Spells::AddSpell(ch, spell_id, time(nullptr), spelldiff);
	SendMsgToChar(ch, "Вы дополнительно можете использовать заклинание '%s' некоторое время.\r\n", GetSpellName(spell_id));
	log("Add %s for %d seconds to %s (trigspelltemp) trigvnum %d", GetSpellName(spell_id), spelldiff, GET_NAME(ch), vnum);
}

void trg_spelladd(CharData *ch, ESpell spell_id, int spelldiff, int vnum) {
	int spell = GET_SPELL_MEM(ch, spell_id);
	GET_SPELL_MEM(ch, spell_id) = MAX(0, MIN(spell + spelldiff, 50));

	if (spell > GET_SPELL_MEM(ch, spell_id)) {
		if (GET_SPELL_MEM(ch, spell_id)) {
			log("Remove custom spell %s to %s (trigspell) trigvnum %d", GetSpellName(spell_id), GET_NAME(ch), vnum);
			sprintf(buf, "Вы забыли часть заклинаний '%s'.\r\n", GetSpellName(spell_id));
		} else {
			sprintf(buf, "Вы забыли все заклинания '%s'.\r\n", GetSpellName(spell_id));
			log("Remove all spells %s to %s (trigspell) trigvnum %d", GetSpellName(spell_id), GET_NAME(ch), vnum);
		}
		SendMsgToChar(buf, ch);
	} else if (spell < GET_SPELL_MEM(ch, spell_id)) {
		SendMsgToChar(ch, "Вы выучили несколько заклинаний '%s'.\r\n", GetSpellName(spell_id));
		log("Add %s to %s (trigspell) trigvnum %d", GetSpellName(spell_id), GET_NAME(ch), vnum);
	}
}

void trg_spellitem(CharData *ch, ESpell spell_id, int spelldiff, ESpellType spell_type) {
	char type[kMaxStringLength];

	if ((spelldiff && IS_SET(GET_SPELL_TYPE(ch, spell_id), spell_type)) ||
		(!spelldiff && !IS_SET(GET_SPELL_TYPE(ch, spell_id), spell_type)))
		return;
	if (!spelldiff) {
		REMOVE_BIT(GET_SPELL_TYPE(ch, spell_id), spell_type);
		switch (spell_type) {
			case ESpellType::kScrollCast: strcpy(type, "создания свитка");
				break;
			case ESpellType::kPotionCast: strcpy(type, "приготовления напитка");
				break;
			case ESpellType::kWandCast: strcpy(type, "изготовления посоха");
				break;
			case ESpellType::kItemCast: strcpy(type, "предметной магии");
				break;
			case ESpellType::kRunes: strcpy(type, "использования рун");
				break;
			default: break;
		};
		std::stringstream buffer;
		buffer << "Вы утратили умение " << type << " '" << GetSpellName(spell_id) << "'";
		SendMsgToChar(buffer.str(), ch);

	} else {
		SET_BIT(GET_SPELL_TYPE(ch, spell_id), spell_type);
		switch (spell_type) {
			case ESpellType::kScrollCast:
				if (!ch->GetSkill(ESkill::kCreateScroll))
					ch->set_skill(ESkill::kCreateScroll, 5);
				strcpy(type, "создания свитка");
				break;
			case ESpellType::kPotionCast:
				if (!ch->GetSkill(ESkill::kCreatePotion))
					ch->set_skill(ESkill::kCreatePotion, 5);
				strcpy(type, "приготовления напитка");
				break;
			case ESpellType::kWandCast:
				if (!ch->GetSkill(ESkill::kCreateWand))
					ch->set_skill(ESkill::kCreateWand, 5);
				strcpy(type, "изготовления посоха");
				break;
			case ESpellType::kItemCast: strcpy(type, "предметной магии");
				break;
			case ESpellType::kRunes: strcpy(type, "использования рун");
				break;
			default: break;
		}
		std::stringstream buffer;
		buffer << "Вы приобрели умение " << type << " '" << GetSpellName(spell_id) << "'";
		SendMsgToChar(buffer.str(), ch);
		CheckRecipeItems(ch, spell_id, spell_type, true);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
