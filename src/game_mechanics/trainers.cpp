/**
\authors Created by Sventovit
\date 14.05.2022.
\brief Модуль механики учителей умений/заклинаний/способностей.
*/

#include "trainers.h"

#include <boost/algorithm/string.hpp>

#include "boot/boot_constants.h"
#include "color.h"
#include "game_magic/magic_utils.h"
#include "structs/global_objects.h"

//extern int guild_info[][3];
typedef int special_f(CharData *, void *, int, char *);
extern void ASSIGNMASTER(MobVnum mob, special_f, int learn_info);

struct guild_learn_type {
	EFeat feat_no;
	ESkill skill_no;
	ESpell spell_no;
	int level;
};

struct guild_mono_type {
	guild_mono_type() : races(0), classes(0), religion(0), alignment(0), learn_info(nullptr) {};
	Bitvector races;
	Bitvector classes;
	Bitvector religion;
	Bitvector alignment;
	struct guild_learn_type *learn_info;
};

struct guild_poly_type {
	Bitvector races;        // bitvector //
	Bitvector classes;        // bitvector //
	Bitvector religion;        // bitvector //
	Bitvector alignment;        // bitvector //
	EFeat feat_no;
	ESkill skill_no;
	ESpell spell_no;
	int level;
};

int GUILDS_MONO_USED = 0;
int GUILDS_POLY_USED = 0;

struct guild_mono_type *guild_mono_info = nullptr;
struct guild_poly_type **guild_poly_info = nullptr;

void init_guilds() {
	FILE *magic;
	char name[kMaxInputLength],
		line[256], line1[256], line2[256], line3[256], line4[256], line5[256], line6[256], *pos;
	int i, num, type = 0, lines = 0, level, pgcount = 0, mgcount = 0;
	ESkill skill_id = ESkill::kUndefined;
	std::unique_ptr<struct guild_poly_type, decltype(free) *> poly_guild(nullptr, free);
	struct guild_mono_type mono_guild;
	std::unique_ptr<struct guild_learn_type, decltype(free) *> mono_guild_learn(nullptr, free);

	if (!(magic = fopen(LIB_MISC "guilds.lst", "r"))) {
		log("Cann't open guilds list file...");
		return;
	}
	auto spell_id{ESpell::kUndefined};
	auto feat_id{EFeat::kUndefined};
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		log("<%s>", name);
		if ((lines = sscanf(name, "%s %s %s %s %s %s %s", line, line1, line2, line3, line4, line5, line6)) == 0)
			continue;
		// log("%d",lines);

		if (!strn_cmp(line, "monoguild", strlen(line))
			|| !strn_cmp(line, "одноклассовая", strlen(line))) {
			type = 1;
			if (lines < 5) {
				log("Bad format for monoguild header, #s #s #s #s #s need...");
				graceful_exit(1);
			}
			mono_guild_learn.reset();
			mono_guild.learn_info = nullptr;
			mono_guild.races = 0;
			mono_guild.classes = 0;
			mono_guild.religion = 0;
			mono_guild.alignment = 0;
			mgcount = 0;
			for (i = 0; *(line1 + i); i++)
				if (strchr("!1xX", *(line1 + i)))
					SET_BIT(mono_guild.races, (1 << i));
			for (i = 0; *(line2 + i); i++)
				if (strchr("!1xX", *(line2 + i)))
					SET_BIT(mono_guild.classes, (1 << i));
			for (i = 0; *(line3 + i); i++)
				if (strchr("!1xX", *(line3 + i)))
					SET_BIT(mono_guild.religion, (1 << i));
			for (i = 0; *(line4 + i); i++)
				if (strchr("!1xX", *(line4 + i)))
					SET_BIT(mono_guild.alignment, (1 << i));
		} else if (!strn_cmp(line, "polyguild", strlen(line))
			|| !strn_cmp(line, "многоклассовая", strlen(line))) {
			type = 2;
			poly_guild.reset();
			pgcount = 0;
		} else if (!strn_cmp(line, "master", strlen(line))
			|| !strn_cmp(line, "учитель", strlen(line))) {
			if ((num = atoi(line1)) == 0 || real_mobile(num) < 0) {
				log("WARNING: Can't assign master %s in guilds.lst. Skipped.", line1);
				continue;
			}

			if (!((type == 1 && mono_guild_learn) || type == 11) &&
				!((type == 2 && poly_guild) || type == 12)) {
				log("WARNING: Can't define guild info for master %s. Skipped.", line1);
				continue;
			}
			if (type == 1 || type == 11) {
				if (type == 1) {
					if (!guild_mono_info) {
						CREATE(guild_mono_info, GUILDS_MONO_USED + 1);
					} else {
						RECREATE(guild_mono_info, GUILDS_MONO_USED + 1);
					}
					log("Create mono guild %d", GUILDS_MONO_USED + 1);
					mono_guild.learn_info = mono_guild_learn.release();
					RECREATE(mono_guild.learn_info, mgcount + 1);
					(mono_guild.learn_info + mgcount)->skill_no = ESkill::kUndefined;
					(mono_guild.learn_info + mgcount)->feat_no = EFeat::kUndefined;
					(mono_guild.learn_info + mgcount)->spell_no = ESpell::kUndefined;
					(mono_guild.learn_info + mgcount)->level = -1;
					guild_mono_info[GUILDS_MONO_USED] = mono_guild;
					GUILDS_MONO_USED++;
					type = 11;
				}
				log("Assign mono guild %d to mobile %s", GUILDS_MONO_USED, line1);
				ASSIGNMASTER(num, guild_mono, GUILDS_MONO_USED);
			}
			if (type == 2 || type == 12) {
				if (type == 2) {
					if (!guild_poly_info) {
						CREATE(guild_poly_info, GUILDS_POLY_USED + 1);
					} else {
						RECREATE(guild_poly_info, GUILDS_POLY_USED + 1);
					}
					log("Create poly guild %d", GUILDS_POLY_USED + 1);
					auto ptr = poly_guild.release();
					RECREATE(ptr, pgcount + 1);
					(ptr + pgcount)->feat_no = EFeat::kUndefined;
					(ptr + pgcount)->skill_no = ESkill::kUndefined;
					(ptr + pgcount)->spell_no = ESpell::kUndefined;
					(ptr + pgcount)->level = -1;
					guild_poly_info[GUILDS_POLY_USED] = ptr;
					GUILDS_POLY_USED++;
					type = 12;
				}
				//log("Assign poly guild %d to mobile %s", GUILDS_POLY_USED, line1);
				ASSIGNMASTER(num, guild_poly, GUILDS_POLY_USED);
			}
		} else if (type == 1) {
			if (lines < 3) {
				log("You need use 3 arguments for monoguild");
				graceful_exit(1);
			}
			spell_id = static_cast<ESpell>(atoi(line));
			if (spell_id < ESpell::kFirst  || spell_id > ESpell::kLast) {
				spell_id = FixNameAndFindSpellId(line);
			}

			skill_id = static_cast<ESkill>(atoi(line1));
			if (MUD::Skills().IsInvalid(skill_id)) {
				skill_id = FixNameAndFindSkillId(line1);
			}

			feat_id = static_cast<EFeat>(atoi(line));
			if (feat_id < EFeat::kFirst || feat_id >= EFeat::kLast) {
				if ((pos = strchr(line1, '.')))
					*pos = ' ';
				feat_id = FindFeatId(line1);
			}

			if (MUD::Skills().IsInvalid(skill_id) && spell_id < ESpell::kFirst && feat_id < EFeat::kFirst) {
				log("Unknown skill, spell or feat for monoguild");
				graceful_exit(1);
			}

			if ((level = atoi(line2)) == 0 || level >= kLvlImmortal) {
				log("Use 1-%d level for guilds", kLvlImmortal);
				graceful_exit(1);
			}

			auto ptr = mono_guild_learn.release();
			if (!ptr) {
				CREATE(ptr, mgcount + 1);
			} else {
				RECREATE(ptr, mgcount + 1);
			}
			mono_guild_learn.reset(ptr);

			ptr += mgcount;
			ptr->spell_no = spell_id;
			ptr->skill_no = skill_id;
			ptr->feat_no = feat_id;
			ptr->level = level;
			mgcount++;
		} else if (type == 2) {
			if (lines < 7) {
				log("You need use 7 arguments for polyguild");
				graceful_exit(1);
			}
			auto ptr = poly_guild.release();
			if (!ptr) {
				CREATE(ptr, pgcount + 1);
			} else {
				RECREATE(ptr, pgcount + 1);
			}
			poly_guild.reset(ptr);

			ptr += pgcount;
			ptr->races = 0;
			ptr->classes = 0;
			ptr->religion = 0;
			ptr->alignment = 0;
			for (i = 0; *(line + i); i++)
				if (strchr("!1xX", *(line + i)))
					SET_BIT(ptr->races, (1 << i));
			for (i = 0; *(line1 + i); i++)
				if (strchr("!1xX", *(line1 + i)))
					SET_BIT(ptr->classes, (1 << i));
			for (i = 0; *(line2 + i); i++)
				if (strchr("!1xX", *(line2 + i)))
					SET_BIT(ptr->religion, (1 << i));
			for (i = 0; *(line3 + i); i++)
				if (strchr("!1xX", *(line3 + i)))
					SET_BIT(ptr->alignment, (1 << i));

			spell_id = static_cast<ESpell>(atoi(line4));
			if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
				spell_id = FixNameAndFindSpellId(line4);
			}

			skill_id = static_cast<ESkill>(atoi(line5));
			if (MUD::Skills().IsInvalid(skill_id)) {
				skill_id = FixNameAndFindSkillId(line5);
			}

			feat_id = static_cast<EFeat>(atoi(line4));
			if (feat_id < EFeat::kFirst || feat_id > EFeat::kLast) {
				if ((pos = strchr(line5, '.')))
					*pos = ' ';

				feat_id = FindFeatId(line1);
				sprintf(buf, "feature number 2: %d", to_underlying(feat_id));
				feat_id = FindFeatId(line5);
			}
			if (MUD::Skills().IsInvalid(skill_id) && spell_id < ESpell::kFirst && feat_id < EFeat::kFirst) {
				log("Unknown skill, spell or feat for polyguild - \'%s\'", line5);
				graceful_exit(1);
			}
			if ((level = atoi(line6)) == 0 || level >= kLvlImmortal) {
				log("Use 1-%d level for guilds", kLvlImmortal);
				graceful_exit(1);
			}
			ptr->spell_no = spell_id;
			ptr->skill_no = skill_id;
			ptr->feat_no = feat_id;
			ptr->level = level;
			pgcount++;
		}
	}
	fclose(magic);
}

#define SCMD_LEARN 1

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
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
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
}

// ********************************************************************
// *  Special procedures for mobiles                                  *
// ********************************************************************

/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.cpp if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
/*int guild_info[][3] =
	{
		// Midgaard
		{ECharClass::kConjurer, 3017, SCMD_SOUTH},
		{ECharClass::kSorcerer, 3004, SCMD_NORTH},
		{ECharClass::kThief, 3027, SCMD_EAST},
		{ECharClass::kWarrior, 3021, SCMD_EAST},

		// Brass Dragon
		{-999 *//* all *//* , 5065, SCMD_WEST},

		// this must go last -- add new guards above!
		{-1, -1, -1}
	};*/

/*int guild_guard(CharData *ch, void *me, int cmd, char * *//*argument*//*) {
	int i;
	CharData *guard = (CharData *) me;
	const char *buf = "Охранник остановил вас, преградив дорогу.\r\n";
	const char *buf2 = "Охранник остановил $n, преградив $m дорогу.";

	if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, EAffect::kBlind)
		|| AFF_FLAGGED(guard, EAffect::kHold))
		return (false);

	if (GetRealLevel(ch) >= kLvlImmortal)
		return (false);

	for (i = 0; guild_info[i][0] != -1; i++) {
		if ((ch->IsNpc() || ch->GetClass() != guild_info[i][0]) &&
			GET_ROOM_VNUM(ch->in_room) == guild_info[i][1] && cmd == guild_info[i][2]) {
			SendMsgToChar(buf, ch);
			act(buf2, false, ch, 0, 0, kToRoom);
			return (true);
		}
	}

	return (false);
}*/

/*
namespace trainers {

using DataNode = parser_wrapper::DataNode;
using Optional = TrainerInfoBuilder::ItemOptional;

void TrainersLoader::Load(DataNode data) {
	MUD::Classes().Init(data.Children());
}

void TrainersLoader::Reload(DataNode data) {
	MUD::Classes().Reload(data.Children());
}

Optional TrainerInfoBuilder::Build(DataNode &node) {
	auto trainer_info =  std::make_optional(std::make_shared<TrainerInfo>());
	try {
		ParseTrainer(trainer_info, node);
	} catch (std::exception &e) {
		err_log("Trainer info parsing error: '%s'", e.what());
		trainer_info = std::nullopt;
	}
	return trainer_info;
}

void TrainerInfoBuilder::ParseTrainer(Optional &info, DataNode &node) {
	try {
		info.value()->id_ = parse::ReadAsConstant<ECharClass>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect class id (%s).", e.what());
		info = std::nullopt;
		return;
	}

	try {
		info.value()->mode = parse::ReadAsConstant<EItemMode>(node.GetValue("mode"));
	} catch (std::exception &) {
	}

	if (node.GoToChild("name")) {
		ParseName(info, node);
	}

	if (node.GoToSibling("stats")) {
		ParseStats(info, node);
	}

	if (node.GoToSibling("skills")) {
		ParseSkills(info, node);
	}

	if (node.GoToSibling("spells")) {
		ParseSpells(info, node);
	}

	if (node.GoToSibling("feats")) {
		ParseFeats(info, node);
	}
}

}
*/
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
