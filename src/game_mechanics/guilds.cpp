/**
\authors Created by Sventovit
\date 14.05.2022.
\brief Модуль механики учителей умений/заклинаний/способностей.
*/

#include "guilds.h"

//#include <iostream>
//#include <boost/algorithm/string.hpp>

#include "boot/boot_constants.h"
#include "color.h"
#include "game_magic/magic_utils.h"
#include "game_magic/spells_info.h"
#include "structs/global_objects.h"
#include "utils/table_wrapper.h"

typedef int special_f(CharData *, void *, int, char *);
extern void ASSIGNMASTER(MobVnum mob, special_f, int learn_info);

/*
int guild_mono(CharData *ch, void *me, int cmd, char *argument) {
	int command = 0, gcount = 0, info_num = 0, found = false, sfound = false, i, bits;
	auto *victim = (CharData *) me;

	if (ch->IsNpc()) {
		return 0;
	}

	if (CMD_IS("учить") || CMD_IS("practice")) {
		command = SCMD_LEARN;
	} else {
		return 0;
	}

	info_num = mob_index[victim->get_rnum()].stored;
	if (info_num <= 0 || info_num > GUILDS_MONO_USED) {
		act("$N сказал$G : 'Извини, $n, я уже в отставке.'", false, ch, nullptr, victim, kToChar);
		return (1);
	}

	info_num--;
	if (!IS_BITS(guild_mono_info[info_num].classes, to_underlying(ch->GetClass()))
		|| !IS_BITS(guild_mono_info[info_num].races, GET_RACE(ch))
		|| !IS_BITS(guild_mono_info[info_num].religion, GET_RELIGION(ch))) {
		act("$N сказал$g : '$n, я не учу таких, как ты.'", false, ch, nullptr, victim, kToChar);
		return 1;
	}

	skip_spaces(&argument);

	switch (command) {
		case SCMD_LEARN:
			if (!*argument) {
				gcount += sprintf(buf, "Я могу научить тебя следующему:\r\n");
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					const auto skill_no = (guild_mono_info[info_num].learn_info + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kUndefined != skill_no && (!ch->get_trained_skill(skill_no)
						|| IS_GRGOD(ch)) && CanGetSkill(ch, skill_no)) {
						gcount += sprintf(buf + gcount, "- умение %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), MUD::Skills(skill_no).GetName(), CCNRM(ch, C_NRM));
						found = true;
					}

					if (!(bits = -2 * bits) || bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}

					const auto spell_no = (guild_mono_info[info_num].learn_info + i)->spell_no;
					if (spell_no > ESpell::kUndefined && (!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) || IS_GRGOD(
						ch))) {
						gcount += sprintf(buf + gcount, "- магия %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						found = true;
					}

					const auto feat_no = (guild_mono_info[info_num].learn_info + i)->feat_no;
					if (feat_no > EFeat::kUndefined && !ch->HaveFeat(feat_no) && CanGetFeat(ch, feat_no)) {
						gcount += sprintf(buf + gcount, "- способность %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Похоже, твои знания полнее моих.'", false, ch, nullptr, victim, kToChar);
					return (1);
				} else {
					SendMsgToChar(buf, ch);
					return (1);
				}
			}

			if (!strn_cmp(argument, "все", strlen(argument)) || !strn_cmp(argument, "all", strlen(argument))) {
				for (i = 0, found = false, sfound = true;
					 (guild_mono_info[info_num].learn_info + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch))
						continue;

					const ESkill skill_no = (guild_mono_info[info_num].learn_info + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kUndefined != skill_no
						&& !ch->get_trained_skill(skill_no))    // sprintf(buf, "$N научил$G вас умению %s\"%s\"\%s",
					{
						//             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
						// act(buf,false,ch,0,victim,TO_CHAR);
						// ch->get_skill(skill_no) = 10;
						sfound = true;
					}

					if (!(bits = -2 * bits) || bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}

					const auto spell_no = (guild_mono_info[info_num].learn_info + i)->spell_no;
					if (spell_no > ESpell::kUndefined &&
						!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)) {
						gcount += sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						act(buf, false, ch, nullptr, victim, kToChar);
						if (IS_SET(bits, ESpellType::kKnow))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kKnow);
						if (IS_SET(bits, ESpellType::kItemCast))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kItemCast);
						if (IS_SET(bits, ESpellType::kRunes))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kRunes);
						if (IS_SET(bits, ESpellType::kPotionCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kPotionCast);
							ch->set_skill(ESkill::kCreatePotion, std::max(10, ch->get_trained_skill(ESkill::kCreatePotion)));
						}
						if (IS_SET(bits, ESpellType::kWandCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kWandCast);
							ch->set_skill(ESkill::kCreateWand, std::max(10, ch->get_trained_skill(ESkill::kCreateWand)));
						}
						if (IS_SET(bits, ESpellType::kScrollCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kScrollCast);
							ch->set_skill(ESkill::kCreateScroll, std::max(10, ch->get_trained_skill(ESkill::kCreateScroll)));
						}
						found = true;
					}

					const auto feat_no = (guild_mono_info[info_num].learn_info + i)->feat_no;
					if (feat_no >= EFeat::kFirst && feat_no <= EFeat::kLast) {
						if (!ch->HaveFeat(feat_no) && CanGetFeat(ch, feat_no)) {
							sfound = true;
						}
					}
				}

				if (sfound) {
					act("$N сказал$G : \r\n"
						"'$n, к сожалению, это может сильно затруднить твое постижение умений.'\r\n"
						"'Выбери лишь самые необходимые тебе умения.'",
						false, ch, nullptr, victim, kToChar);
				}

				if (!found) {
					act("$N ничему новому вас не научил$G.",
						false, ch, nullptr, victim, kToChar);
				}

				return (1);
			}

			const auto feat_no = FindFeatId(argument);
			if (feat_no >= EFeat::kFirst && feat_no <= EFeat::kLast) {
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->feat_no >= EFeat::kFirst; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (feat_no == (guild_mono_info[info_num].learn_info + i)->feat_no) {
						if (ch->HaveFeat(feat_no)) {
							act("$N сказал$g вам : 'Ничем помочь не могу, ты уже владеешь этой способностью.'",
								false, ch, nullptr, victim, kToChar);
						} else if (!CanGetFeat(ch, feat_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'",
								false, ch, nullptr, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас способности %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, nullptr, victim, kToChar);
							ch->SetFeat(feat_no);
						}
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Мне не ведомо такое мастерство.'",
						false, ch, nullptr, victim, kToChar);
				}

				return (1);
			}

			const ESkill skill_no = FixNameAndFindSkillId(argument);
			if (skill_no >= ESkill::kLast && skill_no <= ESkill::kLast) {
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (skill_no == (guild_mono_info[info_num].learn_info + i)->skill_no) {
						if (ch->get_trained_skill(skill_no)) {
							act("$N сказал$g вам : 'Ничем помочь не могу, ты уже владеешь этим умением.'",
								false, ch, nullptr, victim, kToChar);
						} else if (!CanGetSkill(ch, skill_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'",
								false, ch, nullptr, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас умению %s\"%s\"%s",
									CCCYN(ch, C_NRM), MUD::Skills(skill_no).GetName(), CCNRM(ch, C_NRM));
							act(buf, false, ch, nullptr, victim, kToChar);
							ch->set_skill(skill_no, 10);
						}
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Мне не ведомо такое умение.'",
						false, ch, nullptr, victim, kToChar);
				}

				return (1);
			}

			auto spell_no = FixNameAndFindSpellId(argument);
			if (spell_no > ESpell::kUndefined && spell_no <= ESpell::kLast) {
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (spell_no == (guild_mono_info[info_num].learn_info + i)->spell_no) {
						if (!(bits = -2 * to_underlying((guild_mono_info[info_num].learn_info + i)->skill_no))
							|| bits == ESpellType::kTemp) {
							bits = ESpellType::kKnow;
						}

						if ((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) {
							if (IS_SET(bits, ESpellType::kKnow)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить это заклинание, тебе необходимо набрать\r\n"
											 "%s колд(овать) '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, ESpellType::kItemCast)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s смешать '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, ESpellType::kRunes)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s руны '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else {
								strcpy(buf, "Вы уже умеете это.");
							}
							act(buf, false, ch, nullptr, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, nullptr, victim, kToChar);
							if (IS_SET(bits, ESpellType::kKnow)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kKnow);
							}
							if (IS_SET(bits, ESpellType::kItemCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kItemCast);
							}
							if (IS_SET(bits, ESpellType::kRunes)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kRunes);
							}
							if (IS_SET(bits, ESpellType::kPotionCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kPotionCast);
								ch->set_skill(ESkill::kCreatePotion, std::max(10, ch->get_trained_skill(ESkill::kCreatePotion)));
							}
							if (IS_SET(bits, ESpellType::kWandCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kWandCast);
								ch->set_skill(ESkill::kCreateWand, std::max(10, ch->get_trained_skill(ESkill::kCreateWand)));
							}
							if (IS_SET(bits, ESpellType::kScrollCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kScrollCast);
								ch->set_skill(ESkill::kCreateScroll, std::max(10, ch->get_trained_skill(ESkill::kCreateScroll)));
							}
						}
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Я и сам$G не знаю такой магии.'",
						false, ch, nullptr, victim, kToChar);
				}

				return (1);
			}

			act("$N сказал$G вам : 'Я никогда и никого этому не учил$G.'",
				false, ch, nullptr, victim, kToChar);
			return (1);
	}

	return 0;
}

int guild_poly(CharData *ch, void *me, int cmd, char *argument) {
	int command = 0, gcount = 0, info_num = 0, found = false, sfound = false, i, bits;
	auto *victim = (CharData *) me;

	if (ch->IsNpc()) {
		return 0;
	}

	if (CMD_IS("учить") || CMD_IS("practice")) {
		command = SCMD_LEARN;
	} else {
		return 0;
	}

	if ((info_num = mob_index[victim->get_rnum()].stored) <= 0 || info_num > GUILDS_POLY_USED) {
		act("$N сказал$G : 'Извини, $n, я уже в отставке.'",
			false, ch, nullptr, victim, kToChar);
		return 1;
	}

	info_num--;

	skip_spaces(&argument);

	switch (command) {
		case SCMD_LEARN:
			if (!*argument) {
				gcount += sprintf(buf, "Я могу научить тебя следующему:\r\n");
				for (i = 0, found = false; to_underlying((guild_poly_info[info_num] + i)->spell_no) >= 0; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch)))
						continue;

					const ESkill skill_no = (guild_poly_info[info_num] + i)->skill_no;

					bits = to_underlying(skill_no);
					if (ESkill::kUndefined != skill_no &&
						(!ch->get_trained_skill(skill_no) || IS_GRGOD(ch)) && CanGetSkill(ch, skill_no)) {
						gcount += sprintf(buf + gcount, "- умение %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), MUD::Skills(skill_no).GetName(), CCNRM(ch, C_NRM));
						found = true;
					}

					if (!(bits = -2 * bits) || bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}

					const auto spell_no = (guild_poly_info[info_num] + i)->spell_no;
					if (spell_no > ESpell::kUndefined &&
						(!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) || IS_GRGOD(ch))) {
						gcount += sprintf(buf + gcount, "- магия %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						found = true;
					}

					const auto feat_no = (guild_poly_info[info_num] + i)->feat_no;
					if (feat_no >= EFeat::kFirst && feat_no <= EFeat::kLast) {
						if (!ch->HaveFeat(feat_no) && CanGetFeat(ch, feat_no)) {
							gcount += sprintf(buf + gcount, "- способность %s\"%s\"%s\r\n",
											  CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
							found = true;
						}
					}
				}

				if (!found) {
					act("$N сказал$G : 'Похоже, я не смогу тебе помочь.'",
						false, ch, nullptr, victim, kToChar);
					return (1);
				} else {
					SendMsgToChar(buf, ch);
					return (1);
				}
			}

			if (!strn_cmp(argument, "все", strlen(argument))
				|| !strn_cmp(argument, "all", strlen(argument))) {
				for (i = 0, found = false, sfound = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					const ESkill skill_no = (guild_poly_info[info_num] + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kUndefined != skill_no
						&& !ch->get_trained_skill(skill_no))    // sprintf(buf, "$N научил$G вас умению %s\"%s\"\%s",
					{
						//             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
						// act(buf,false,ch,0,victim,TO_CHAR);
						// ch->get_skill(skill_no) = 10;
						sfound = true;
					}

					const auto feat_no = (guild_poly_info[info_num] + i)->feat_no;
					if (feat_no >= EFeat::kFirst && feat_no <= EFeat::kLast) {
						if (!ch->HaveFeat(feat_no) && CanGetFeat(ch, feat_no)) {
							sfound = true;
						}
					}

					if (!(bits = -2 * bits) || bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}

					const auto spell_no = (guild_poly_info[info_num] + i)->spell_no;
					if (spell_no > ESpell::kUndefined &&
						!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)) {
						gcount += sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						act(buf, false, ch, nullptr, victim, kToChar);

						if (IS_SET(bits, ESpellType::kKnow))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kKnow);
						if (IS_SET(bits, ESpellType::kItemCast))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kItemCast);
						if (IS_SET(bits, ESpellType::kRunes))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kRunes);
						if (IS_SET(bits, ESpellType::kPotionCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kPotionCast);
							ch->set_skill(ESkill::kCreatePotion, std::max(10, ch->get_trained_skill(ESkill::kCreatePotion)));
						}
						if (IS_SET(bits, ESpellType::kWandCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kWandCast);
							ch->set_skill(ESkill::kCreateWand, std::max(10, ch->get_trained_skill(ESkill::kCreateWand)));
						}
						if (IS_SET(bits, ESpellType::kScrollCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kScrollCast);
							ch->set_skill(ESkill::kCreateScroll, std::max(10, ch->get_trained_skill(ESkill::kCreateScroll)));
						}
						found = true;
					}
				}

				if (sfound) {
					act("$N сказал$G : \r\n"
						"'$n, к сожалению, это может сильно затруднить твое постижение умений.'\r\n"
						"'Выбери лишь самые необходимые тебе умения.'",
						false, ch, nullptr, victim, kToChar);
				}

				if (!found) {
					act("$N ничему новому вас не научил$G.",
						false, ch, nullptr, victim, kToChar);
				}

				return (1);
			}

			const auto skill_no = FixNameAndFindSkillId(argument);
			if (ESkill::kUndefined != skill_no && skill_no <= ESkill::kLast) {
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					if (skill_no == (guild_poly_info[info_num] + i)->skill_no) {
						if (ch->get_trained_skill(skill_no)) {
							act("$N сказал$G вам : 'Ты уже владеешь этим умением.'",
								false, ch, nullptr, victim, kToChar);
						} else if (!CanGetSkill(ch, skill_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'",
								false, ch, nullptr, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас умению %s\"%s\"%s",
									CCCYN(ch, C_NRM), MUD::Skills(skill_no).GetName(), CCNRM(ch, C_NRM));
							act(buf, false, ch, nullptr, victim, kToChar);
							ch->set_skill(skill_no, 10);
						}
						found = true;
					}
				}

				if (found) {
					return 1;
				}
			}

			auto feat_no = FindFeatId(argument);
			if (feat_no == EFeat::kUndefined) {
				std::string str(argument);
				std::replace_if(str.begin(), str.end(), boost::is_any_of("_:"), ' ');
				feat_no = FindFeatId(str.c_str(), true);
			}

			if (feat_no != EFeat::kUndefined) {
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					if (feat_no == (guild_poly_info[info_num] + i)->feat_no) {
						if (ch->HaveFeat(feat_no)) {
							act("$N сказал$G вам : 'Ничем помочь не могу, ты уже владеешь этой способностью.'",
								false, ch, nullptr, victim, kToChar);
						} else if (!CanGetFeat(ch, feat_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'",
								false, ch, nullptr, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас способности %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, nullptr, victim, kToChar);
							ch->SetFeat(feat_no);
						}
						found = true;
					}
				}

				if (found) {
					return 1;
				}
			}

			const auto spell_no = FixNameAndFindSpellId(argument);
			if (spell_no != ESpell::kUndefined) {
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!(bits = -2 * to_underlying((guild_poly_info[info_num] + i)->skill_no))
						|| bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					if (spell_no == (guild_poly_info[info_num] + i)->spell_no) {
						if ((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) {
							if (IS_SET(bits, ESpellType::kKnow)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить это заклинание, тебе необходимо набрать\r\n"
											 "%s колд(овать) '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, ESpellType::kItemCast)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s смешать '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, ESpellType::kRunes)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s руны '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else {
								strcpy(buf, "Вы уже умеете это.");
							}
							act(buf, false, ch, nullptr, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, nullptr, victim, kToChar);
							if (IS_SET(bits, ESpellType::kKnow)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kKnow);
							}
							if (IS_SET(bits, ESpellType::kItemCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kItemCast);
							}
							if (IS_SET(bits, ESpellType::kRunes)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kRunes);
							}
							if (IS_SET(bits, ESpellType::kPotionCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kPotionCast);
								ch->set_skill(ESkill::kCreatePotion, std::max(10, ch->get_trained_skill(ESkill::kCreatePotion)));
							}
							if (IS_SET(bits, ESpellType::kWandCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kWandCast);
								ch->set_skill(ESkill::kCreateWand, std::max(10, ch->get_trained_skill(ESkill::kCreateWand)));
							}
							if (IS_SET(bits, ESpellType::kScrollCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kScrollCast);
								ch->set_skill(ESkill::kCreateScroll, std::max(10, ch->get_trained_skill(ESkill::kCreateScroll)));
							}
						}
						found = true;
					}
				}

				if (found) {
					return 1;
				}
			}

			act("$N сказал$G вам : 'Я никогда и никого этому не учил$G.'",
				false, ch, nullptr, victim, kToChar);
			return (1);
	}

	return 0;
}*/

int DoGuildLearn(CharData *ch, void *me, int cmd, char *argument) {
	if (ch->IsNpc()) {
		return 0;
	}
	if (!CMD_IS("учить") && !CMD_IS("practice")) {
		return 0;
	}

	/*
	 *  Это не слишком красиво, потому что кто-нибудь может затереть поле stored в инлексе и гильдия перестанет работать,
	 *  но позволяет не искать каждый раз тренера по всем гильдиям. По уму, нужно, чтобы поле stored как-то конструировалось
	 *  в комплекте с самой спецфункцией, и независимо его нельзя было бы перезаписать.
	 */
	auto *trainer = (CharData *) me;
	Vnum guild_vnum{-1};
	if (auto rnum = trainer->get_rnum(); rnum >= 0) {
		guild_vnum = mob_index[rnum].stored;
	}
	const auto &guild = MUD::Guilds(guild_vnum);

	if (guild.GetId() < 0) {
		act("$N сказал$G : 'Извини, $n, я уже в отставке.'", false, ch, nullptr, trainer, kToChar);
		err_log("try to call DoGuildLearn wuthout assigned guild vnum.");
		return 0;
	}

	guild.Process(trainer, ch, argument);
	return 1;
}

namespace guilds {

using DataNode = parser_wrapper::DataNode;
using ItemPtr = GuildInfoBuilder::ItemPtr;

void GuildsLoader::AssignGuildsToTrainers() {
	for (const auto &guild: MUD::Guilds()) {
		guild.AssignToTrainers();
	}
}

void GuildsLoader::Load(DataNode data) {
	MUD::Guilds().Init(data.Children());
	AssignGuildsToTrainers();
}

void GuildsLoader::Reload(DataNode data) {
	MUD::Guilds().Reload(data.Children());
	AssignGuildsToTrainers();
}

const std::string &GuildInfo::GetMessage(EGuildMsg msg_id) {
	static const std::unordered_map<EGuildMsg, std::string> guild_msgs = {
		{EGuildMsg::kGreeting, "$N сказал$G: 'Я могу научить тебя следующему:'"},
		{EGuildMsg::kSkill, "умение"},
		{EGuildMsg::kSpell, "заклинание"},
		{EGuildMsg::kFeat, "способность"},
		{EGuildMsg::kDidNotTeach, "$N сказал$G: 'Я никогда и никого ЭТОМУ не учил$G!'"},
		{EGuildMsg::kInquiry, "$n о чем-то спросил$g $N3."},
		{EGuildMsg::kCannotToChar, "$N сказал$G: 'Я не могу тебя этому научить'."},
		{EGuildMsg::kCannotToRoom, "$N сказал$G $n2: 'Я не могу тебя этому научить'."},
		{EGuildMsg::kAskToChar, "Вы попросились в ученики к $N2."},
		{EGuildMsg::kAskToRoom, "$n попросил$u в ученики к $N2."},
		{EGuildMsg::kDoLearnToChar, "Вы получили несколько наставлений и мудрых советов от $N1."},
		{EGuildMsg::kDoLearnToRoom, "$N дал$G $n2 несколько наставлений."},
		{EGuildMsg::kAllSkills,
		 "$N сказал$G: '$n, нельзя научиться всем умениям или способностям сразу. Выбери необходимые!'"},
		{EGuildMsg::kListEmpty, "$N сказал$G : 'Похоже, $n, я не смогу тебе помочь'."},
		{EGuildMsg::kError, "У кодера какие-то проблемы."},
	};

	if (guild_msgs.contains(msg_id)) {
		return guild_msgs.at(msg_id);
	} else {
		return guild_msgs.at(EGuildMsg::kError);
	}
}

ItemPtr GuildInfoBuilder::Build(DataNode &node) {
	try {
		return ParseGuild(node);
	} catch (std::exception &e) {
		err_log("Guild parsing error: '%s'", e.what());
		return nullptr;
	}
}

ItemPtr GuildInfoBuilder::ParseGuild(DataNode node) {
	auto vnum = std::clamp(parse::ReadAsInt(node.GetValue("vnum")), 0, kMaxProtoNumber);
	auto mode = SkillInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	std::string text_id{"kUndefined"};
	std::string name{"undefined"};
	try {
		text_id = parse::ReadAsStr(node.GetValue("text_id"));
		name = parse::ReadAsStr(node.GetValue("name"));
	} catch (...) {
	}

	auto guild_info = std::make_shared<GuildInfo>(vnum, text_id, name, mode);

	if (node.GoToChild("trainers")) {
		parse::ReadAsIntSet(guild_info->trainers_, node.GetValue("vnums"));
	}

	if (node.GoToSibling("talents")) {
		ParseTalents(guild_info, node);
	}

	return guild_info;
}

void GuildInfoBuilder::ParseTalents(ItemPtr &info, DataNode &node) {
	static const std::string skill_element{"skill"};
	static const std::string spell_element{"spell"};
	static const std::string feat_element{"feat"};

	for (auto &talent_node: node.Children()) {
		std::unordered_set<ECharClass> char_classes;
		if (talent_node.GoToChild("class")) {
			parse::ReadAsConstantsSet<ECharClass>(char_classes, talent_node.GetValue("val"));
			talent_node.GoToParent();
		}

		if (talent_node.GetName() == skill_element) {
			auto talent_id = parse::ReadAsConstant<ESkill>(talent_node.GetValue("id"));
			info->learning_talents_.emplace_back(std::make_unique<GuildInfo::GuildSkill>(
				GuildInfo::ETalent::kSkill, talent_id, char_classes));
		} else if (talent_node.GetName() == spell_element) {
			auto talent_id = parse::ReadAsConstant<ESpell>(talent_node.GetValue("id"));
			info->learning_talents_.emplace_back(std::make_unique<GuildInfo::GuildSpell>(
				GuildInfo::ETalent::kSpell, talent_id, char_classes));
		} else if (talent_node.GetName() == feat_element) {
			auto talent_id = parse::ReadAsConstant<EFeat>(talent_node.GetValue("id"));
			info->learning_talents_.emplace_back(std::make_unique<GuildInfo::GuildFeat>(
				GuildInfo::ETalent::kFeat, talent_id, char_classes));
		}

		char_classes.clear();
	}
}

void GuildInfo::AssignToTrainers() const {
	for (const auto trainer_vnum: trainers_) {
		ASSIGNMASTER(trainer_vnum, DoGuildLearn, GetId());
	}
};

void GuildInfo::DisplayMenu(CharData *trainer, CharData *ch) const {
	std::ostringstream out;
	auto count{0};
	table_wrapper::Table table;
	for (const auto &talent: learning_talents_) {
		if (talent->IsUnlearnable(ch)) {
			continue;
		}

		++count;

		table << (KCYN + std::to_string(count) + KNRM + ")" + KGRN);
		switch (talent->GetTalentType()) {
			case ETalent::kSkill: table << GetMessage(EGuildMsg::kSkill);
				break;
			case ETalent::kSpell: table << GetMessage(EGuildMsg::kSpell);
				break;
			case ETalent::kFeat: table << GetMessage(EGuildMsg::kFeat);
				break;
		}
		table << ("'" + static_cast<std::string>(talent->GetName()) + "'" + KNRM);
		table << "100 кун" << table_wrapper::kEndRow;
	}

	act(GetMessage(EGuildMsg::kAskToChar), false, ch, nullptr, trainer, kToChar);
	act(GetMessage(EGuildMsg::kAskToRoom), false, ch, nullptr, trainer, kToRoom);
	if (count) {
		act(GetMessage(EGuildMsg::kGreeting), false, ch, nullptr, trainer, kToChar);
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToStream(out, table);
		out << std::endl;
		SendMsgToChar(out.str(), ch);
	} else {
		act(GetMessage(EGuildMsg::kListEmpty), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kListEmpty), false, ch, nullptr, trainer, kToRoom);
	}
}

void GuildInfo::Process(CharData *trainer, CharData *ch, const std::string &argument) const {
	if (argument.empty()) {
		DisplayMenu(trainer, ch);
		return;
	}

	act(GetMessage(EGuildMsg::kInquiry), false, ch, nullptr, trainer, kToRoom);
	try {
		std::size_t talent_num = std::stoi(argument);
		talent_num = std::clamp(talent_num, 1UL, learning_talents_.size());
		LearnWithTalentNum(trainer, ch, talent_num);
	} catch (std::exception &) {
		LearnWithTalentName(trainer, ch, argument);
	}
};

void GuildInfo::LearnWithTalentNum(CharData *trainer, CharData *ch, std::size_t talent_num) const {
/*	if (talent_num <= 0 || talent_num >= learning_talents_.size()) {
		act(GetMessage(EGuildMsg::kDidNotTeach),
			false, ch, nullptr, trainer, kToChar | kToRoom);
	}*/

	--talent_num;
	auto result = std::count_if(learning_talents_.begin(), std::next(learning_talents_.begin(), talent_num),
								[ch](const TalentPtr &talent) {
									if (talent->IsUnlearnable(ch)) {
										return true;
									}
									return false;
								});
	talent_num += result;

	if (talent_num >= learning_talents_.size() || learning_talents_[talent_num]->IsUnlearnable(ch)) {
		act(GetMessage(EGuildMsg::kCannotToChar), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kCannotToRoom), false, ch, nullptr, trainer, kToRoom);
		return;
	}

	act(GetMessage(EGuildMsg::kDoLearnToChar), false, ch, nullptr, trainer, kToChar);
	act(GetMessage(EGuildMsg::kDoLearnToRoom), false, ch, nullptr, trainer, kToRoom);
	SendMsgToChar(ch, "Найден талант '%s'.", static_cast<std::string>(learning_talents_[talent_num]->GetName()).c_str());
	// Do learn

/*	--talent_num;
	for (const auto &talent: learning_talents_) {
		if (talent_num < 0) {
			act(GetMessage(EGuildMsg::kCannotToChar),
				false, ch, nullptr, trainer, kToChar);
			act(GetMessage(EGuildMsg::kCannotToRoom),
				false, ch, nullptr, trainer, kToRoom);
			return;
		}
		if (!talent_num) {
			if (talent->IsUnlearnable(ch)) {
				act(GetMessage(EGuildMsg::kCannotToChar),
					false, ch, nullptr, trainer, kToChar);
				act(GetMessage(EGuildMsg::kCannotToRoom),
					false, ch, nullptr, trainer, kToRoom);
				return;
			}
			SendMsgToChar(ch, "Найден талант '%s'.", static_cast<std::string>(talent->GetName()).c_str());
			// Do learn
			break;
		}
		if (talent->IsUnlearnable(ch)) {
			continue;
		}
		--talent_num;
	}*/
}

void GuildInfo::LearnWithTalentName(CharData *trainer, CharData *ch, const std::string &talent_name) const {
	if (utils::IsAbbrev(talent_name, "все") || utils::IsAbbrev(talent_name, "all")) {
		SendMsgToChar(ch, "Введено '%s' - учим все подряд.", talent_name.c_str());
	}

	auto result = std::find_if(learning_talents_.begin(), learning_talents_.end(),
							   [ch, &talent_name](const TalentPtr &talent) {
								   if (talent->IsUnlearnable(ch)) {
									   return false;
								   }
								   if (IsEquivalent(talent_name,
													static_cast<std::string>(talent->GetName()))) {
									   return true;
								   }
								   return false;
							   });

	if (result != learning_talents_.end()) {
		act(GetMessage(EGuildMsg::kDoLearnToChar), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kDoLearnToRoom), false, ch, nullptr, trainer, kToRoom);
		SendMsgToChar(ch, "Найден талант '%s'.", static_cast<std::string>((*result)->GetName()).c_str());
		// Do learn
		return;
	} else {
		act(GetMessage(EGuildMsg::kListEmpty), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kListEmpty), false, ch, nullptr, trainer, kToRoom);
	}
}

void GuildInfo::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << "Print guild:" << std::endl
		   << " Vnum: " << KGRN << GetId() << KNRM << std::endl
		   << " TextId: " << KGRN << GetTextId() << KNRM << std::endl
		   << " Name: " << KGRN << name_ << KNRM << std::endl;

	if (!trainers_.empty()) {
		buffer << " Trainers vnums: " << KGRN;
		for (const auto vnum: trainers_) {
			buffer << vnum << ", ";
		}
		buffer.seekp(-2, std::ios_base::end);
		buffer << "." << KNRM << std::endl;
	}

	if (!learning_talents_.empty()) {
		buffer << " Trained talents: " << std::endl;
		table_wrapper::Table table;
		table << table_wrapper::kHeader << "Id" << "Name" << "Classes" << table_wrapper::kEndRow;
		for (const auto &talent: learning_talents_) {
			table << talent->GetIdAsStr() << talent->GetName() << talent->GetClassesList() << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToStream(buffer, table);
	}

	buffer << std::endl;
}

bool GuildInfo::IGuildTalent::IsLearnable(CharData *ch) const {
	if (ch->IsNpc()) {
		return false;
	}
	return ((trained_classes_.empty() || trained_classes_.contains(ch->GetClass())) && IsAvailable(ch));
}

std::string GuildInfo::IGuildTalent::GetClassesList() const {
	std::ostringstream buffer;
	if (!trained_classes_.empty()) {
		for (const auto class_id: trained_classes_) {
			buffer << NAME_BY_ITEM(class_id) << ", ";
		}
		buffer.seekp(-2, std::ios_base::end);
		buffer << ".";
	} else {
		buffer << "all";
	}

	return buffer.str();
}

const std::string &GuildInfo::GuildSkill::GetIdAsStr() const {
	return NAME_BY_ITEM<ESkill>(id_);
}

std::string_view GuildInfo::GuildSkill::GetName() const {
	return MUD::Skills(id_).name;
}

bool GuildInfo::GuildSkill::IsAvailable(CharData *ch) const {
	return CanGetSkill(ch, id_);
}

const std::string &GuildInfo::GuildSpell::GetIdAsStr() const {
	return NAME_BY_ITEM<ESpell>(id_);
}

std::string_view GuildInfo::GuildSpell::GetName() const {
	return spell_info[id_].name;
}

bool GuildInfo::GuildSpell::IsAvailable(CharData *ch) const {
	return CanGetSpell(ch, id_);
}

const std::string &GuildInfo::GuildFeat::GetIdAsStr() const {
	return NAME_BY_ITEM<EFeat>(id_);
}

std::string_view GuildInfo::GuildFeat::GetName() const {
	return feat_info[id_].name;
}

bool GuildInfo::GuildFeat::IsAvailable(CharData *ch) const {
	return CanGetFeat(ch, id_);
}

}
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
