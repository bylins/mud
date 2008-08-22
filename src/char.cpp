// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <sstream>
#include "char.hpp"
#include "utils.h"
#include "db.h"
#include "mobmax.h"
#include "pk.h"
#include "im.h"
#include "handler.h"
#include "interpreter.h"
#include "boards.h"
#include "privilege.hpp"
#include "skills.h"
#include "constants.h"
#include "dg_scripts.h"
#include "spells.h"
#include "features.hpp"
#include "olc.h"
#include "comm.h"
#include "file_crc.hpp"

extern void save_char_vars(CHAR_DATA * ch);
extern void tascii(int *pointer, int num_planes, char *ascii);

Character::Character()
		: pfilepos(-1),
		nr(NOBODY),
		in_room(0),
		was_in_room(NOWHERE),
		wait(0),
		punctual_wait(0),
		last_comm(0),
		player_specials(0),
		affected(0),
		timed(0),
		timed_feat(0),
		carrying(0),
		desc(0),
		id(0),
		proto_script(0),
		script(0),
		memory(0),
		next_in_room(0),
		next(0),
		next_fighting(0),
		followers(0),
		master(0),
		CasterLevel(0),
		DamageLevel(0),
		pk_list(0),
		helpers(0),
		track_dirs(0),
		CheckAggressive(0),
		ExtractTimer(0),
		Initiative(0),
		BattleCounter(0),
		Protecting(0),
		Touching(0),
		Poisoner(0),
		ing_list(0),
		dl_list(0)
{
	memset(&player, 0, sizeof(char_player_data));
	memset(&add_abils, 0, sizeof(char_played_ability_data));
	memset(&real_abils, 0, sizeof(char_ability_data));
	memset(&points, 0, sizeof(char_point_data));
	memset(&char_specials, 0, sizeof(char_special_data));
	memset(&mob_specials, 0, sizeof(mob_special_data));

	for (int i = 0; i < NUM_WEARS; i++)
		equipment[i] = 0;

	memset(&MemQueue, 0, sizeof(spell_mem_queue));
	memset(&Questing, 0, sizeof(quest_data));
	memset(&MobKill, 0, sizeof(mob_kill_data));
	memset(&Temporary, 0, sizeof(FLAG_DATA));
	memset(&BattleAffects, 0, sizeof(FLAG_DATA));
	memset(&extra_attack, 0, sizeof(extra_attack_type));
	memset(&cast_attack, 0, sizeof(cast_attack_type));

	char_specials.position = POS_STANDING;
	mob_specials.default_pos = POS_STANDING;
}

Character::~Character()
{
	if (GET_NAME(this))
		log("[FREE CHAR] (%s)", GET_NAME(this));

	int i, j, id = -1;
	struct alias_data *a;
	struct helper_data_type *temp;

	if (!IS_NPC(this) && GET_NAME(this))
	{
		id = get_ptable_by_name(GET_NAME(this));
		if (id >= 0)
		{
			player_table[id].level = (GET_REMORT(this) ? 30 : GET_LEVEL(this));
			player_table[id].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
		}
	}

	if (!IS_NPC(this) || (IS_NPC(this) && GET_MOB_RNUM(this) == -1))
	{	/* if this is a player, or a non-prototyped non-player, free all */
		if (GET_NAME(this))
			free(GET_NAME(this));

		if (GET_PASSWD(this))
			free(GET_PASSWD(this));

		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(this, j))
				free(GET_PAD(this, j));

		if (this->player.title)
			free(this->player.title);

		if (this->player.short_descr)
			free(this->player.short_descr);

		if (this->player.long_descr)
			free(this->player.long_descr);

		if (this->player.description)
			free(this->player.description);

		if (IS_NPC(this) && this->mob_specials.Questor)
			free(this->mob_specials.Questor);

		if (this->Questing.quests)
			free(this->Questing.quests);

		free_mkill(this);

		pk_free_list(this);

		while (this->helpers)
			REMOVE_FROM_LIST(this->helpers, this->helpers, next_helper);
	}
	else if ((i = GET_MOB_RNUM(this)) >= 0)
	{	/* otherwise, free strings only if the string is not pointing at proto */
		if (this->player.name && this->player.name != mob_proto[i].player.name)
			free(this->player.name);

		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(this, j)
					&& (this->player.PNames[j] != mob_proto[i].player.PNames[j]))
				free(this->player.PNames[j]);

		if (this->player.title && this->player.title != mob_proto[i].player.title)
			free(this->player.title);

		if (this->player.short_descr && this->player.short_descr != mob_proto[i].player.short_descr)
			free(this->player.short_descr);

		if (this->player.long_descr && this->player.long_descr != mob_proto[i].player.long_descr)
			free(this->player.long_descr);

		if (this->player.description && this->player.description != mob_proto[i].player.description)
			free(this->player.description);

		if (this->mob_specials.Questor && this->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
			free(this->mob_specials.Questor);
	}

	supress_godsapply = TRUE;
	while (this->affected)
		affect_remove(this, this->affected);
	supress_godsapply = FALSE;

	while (this->timed)
		timed_from_char(this, this->timed);

	if (this->desc)
		this->desc->character = NULL;

	if (this->player_specials != NULL && this->player_specials != &dummy_mob)
	{
		while ((a = GET_ALIASES(this)) != NULL)
		{
			GET_ALIASES(this) = (GET_ALIASES(this))->next;
			free_alias(a);
		}
		if (this->player_specials->poofin)
			free(this->player_specials->poofin);
		if (this->player_specials->poofout)
			free(this->player_specials->poofout);
		/* рецепты */
		while (GET_RSKILL(this) != NULL)
		{
			im_rskill *r;
			r = GET_RSKILL(this)->link;
			free(GET_RSKILL(this));
			GET_RSKILL(this) = r;
		}
		/* порталы */
		while (GET_PORTALS(this) != NULL)
		{
			struct char_portal_type *prt_next;
			prt_next = GET_PORTALS(this)->next;
			free(GET_PORTALS(this));
			GET_PORTALS(this) = prt_next;
		}
// Cleanup punish reasons
		if (MUTE_REASON(this))
			free(MUTE_REASON(this));
		if (DUMB_REASON(this))
			free(DUMB_REASON(this));
		if (HELL_REASON(this))
			free(HELL_REASON(this));
		if (FREEZE_REASON(this))
			free(FREEZE_REASON(this));
		if (NAME_REASON(this))
			free(NAME_REASON(this));
// End reasons cleanup

		if (KARMA(this))
			free(KARMA(this));

		if (GET_LAST_ALL_TELL(this))
			free(GET_LAST_ALL_TELL(this));
		free(GET_LOGS(this));
// shapirus: подчистим за криворукуми кодерами memory leak,
// вызванный неосвобождением фильтра базара...
		if (EXCHANGE_FILTER(this))
			free(EXCHANGE_FILTER(this));
		EXCHANGE_FILTER(this) = NULL;	// на всякий случай
// ...а заодно и игнор лист *смущ :)
		while (IGNORE_LIST(this))
		{
			struct ignore_data *ign_next;
			ign_next = IGNORE_LIST(this)->next;
			free(IGNORE_LIST(this));
			IGNORE_LIST(this) = ign_next;
		}
		IGNORE_LIST(this) = NULL;

		if (GET_CLAN_STATUS(this))
			free(GET_CLAN_STATUS(this));

		// Чистим лист логонов
		while (LOGON_LIST(this))
		{
			struct logon_data *log_next;
			log_next = LOGON_LIST(this)->next;
			free(this->player_specials->logons->ip);
			delete LOGON_LIST(this);
			LOGON_LIST(this) = log_next;
		}
		LOGON_LIST(this) = NULL;

		if (GET_BOARD(this))
			delete GET_BOARD(this);
		GET_BOARD(this) = 0;

		free(this->player_specials);
		this->player_specials = NULL;	// чтобы словить ACCESS VIOLATION !!!
		if (IS_NPC(this))
			log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(this), GET_MOB_VNUM(this));
	}
}

/**
*
*/
int Character::get_skill(int skill_num)
{
	if (Privilege::check_skills(this, skill_num))
	{
		CharSkillsType::iterator it = skills.find(skill_num);
		if (it != skills.end())
			return it->second;
	}
	return 0;
}

/**
* Нулевой скилл мы не сетим, а при обнулении уже имеющегося удалем эту запись.
*/
void Character::set_skill(int skill_num, int percent)
{
	if (skill_num > MAX_SKILL_NUM)
	{
		log("SYSERROR: неизвесный номер скилла %d в set_skill.", skill_num);
		return;
	}

	CharSkillsType::iterator it = skills.find(skill_num);
	if (it != skills.end())
	{
		if (percent)
			it->second = percent;
		else
			skills.erase(it);
	}
	else if (percent)
		skills[skill_num] = percent;
}

/**
*
*/
void Character::clear_skills()
{
	skills.clear();
}

/**
*
*/
void Character::check_max_skills()
{
	for (CharSkillsType::iterator it = skills.begin(); it != skills.end(); ++it)
	{
		int skill_num = it->first;
		if (skill_num != SKILL_SATTACK)
		{
			int max = wis_app[GET_REAL_WIS(this)].max_learn_l20 * (GET_LEVEL(this) + 1) / 20;
			if (max > MAX_EXP_PERCENT)
				max = MAX_EXP_PERCENT;
			int sval = this->get_skill(skill_num) - max - GET_REMORT(this) * 5;
			if (sval < 0)
				sval = 0;
			if ((this->get_skill(skill_num) - sval) > (wis_app[GET_REAL_WIS(this)].max_learn_l20 * GET_LEVEL(this) / 20))
			{
				this->set_skill(skill_num, ((wis_app[GET_REAL_WIS(this)].max_learn_l20 * GET_LEVEL(this) / 20) + sval));
			}
		}
	}
}

/**
*
*/
void Character::save_char()
{
	FILE *saved;
	char filename[MAX_STRING_LENGTH];
	room_rnum location;
	int i;
	time_t li;
	AFFECT_DATA *aff, tmp_aff[MAX_AFFECT];
	OBJ_DATA *char_eq[NUM_WEARS];
	struct timed_type *skj;
	struct char_portal_type *prt;
	int tmp = time(0) - this->player.time.logon;
	if (!now_entrycount)
		if (IS_NPC(this) || GET_PFILEPOS(this) < 0)
			return;

	log("Save char %s", GET_NAME(this));
	save_char_vars(this);

	/* Запись чара в новом формате */
	get_filename(GET_NAME(this), filename, PLAYERS_FILE);
	if (!(saved = fopen(filename, "w")))
	{
		perror("Unable open charfile");
		return;
	}
	/* подготовка */
	/* снимаем все возможные аффекты  */
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(this, i))
		{
			char_eq[i] = unequip_char(this, i | 0x80);
#ifndef NO_EXTRANEOUS_TRIGGERS
			remove_otrigger(char_eq[i], this);
#endif
		}
		else
			char_eq[i] = NULL;
	}

	for (aff = this->affected, i = 0; i < MAX_AFFECT; i++)
	{
		if (aff)
		{
			if (aff->type == SPELL_ARMAGEDDON || aff->type < 1 || aff->type > LAST_USED_SPELL)
				i--;
			else
			{
				tmp_aff[i] = *aff;
				tmp_aff[i].next = 0;
			}
			aff = aff->next;
		}
		else
		{
			tmp_aff[i].type = 0;	/* Zero signifies not used */
			tmp_aff[i].duration = 0;
			tmp_aff[i].modifier = 0;
			tmp_aff[i].location = 0;
			tmp_aff[i].bitvector = 0;
			tmp_aff[i].next = 0;
		}
	}

	/*
	 * remove the affections so that the raw values are stored; otherwise the
	 * effects are doubled when the char logs back in.
	 */
	supress_godsapply = TRUE;
	while (this->affected)
		affect_remove(this, this->affected);
	supress_godsapply = FALSE;

	if ((i >= MAX_AFFECT) && aff && aff->next)
		log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

	// первыми идут поля, необходимые при ребуте мада, тут без необходимости трогать ничего не надо
	if (GET_NAME(this))
		fprintf(saved, "Name: %s\n", GET_NAME(this));
	fprintf(saved, "Levl: %d\n", GET_LEVEL(this));
	fprintf(saved, "Clas: %d\n", GET_CLASS(this));
	fprintf(saved, "UIN : %d\n", GET_UNIQUE(this));
	fprintf(saved, "LstL: %ld\n", static_cast<long int>(LAST_LOGON(this)));
	fprintf(saved, "Id  : %ld\n", GET_IDNUM(this));
	fprintf(saved, "Exp : %ld\n", GET_EXP(this));
	if (GET_REMORT(this) > 0)
		fprintf(saved, "Rmrt: %d\n", GET_REMORT(this));
	// флаги
	*buf = '\0';
	tascii(&PLR_FLAGS(this, 0), 4, buf);
	fprintf(saved, "Act : %s\n", buf);
	// это пишем обязательно посленим, потому что после него ничего не прочитается
	fprintf(saved, "Rebt: следующие далее поля при перезагрузке не парсятся\n\n");
	// дальше пишем как хотим и что хотим

	if (GET_PAD(this, 0))
		fprintf(saved, "NmI : %s\n", GET_PAD(this, 0));
	if (GET_PAD(this, 0))
		fprintf(saved, "NmR : %s\n", GET_PAD(this, 1));
	if (GET_PAD(this, 0))
		fprintf(saved, "NmD : %s\n", GET_PAD(this, 2));
	if (GET_PAD(this, 0))
		fprintf(saved, "NmV : %s\n", GET_PAD(this, 3));
	if (GET_PAD(this, 0))
		fprintf(saved, "NmT : %s\n", GET_PAD(this, 4));
	if (GET_PAD(this, 0))
		fprintf(saved, "NmP : %s\n", GET_PAD(this, 5));
	if (GET_PASSWD(this))
		fprintf(saved, "Pass: %s\n", GET_PASSWD(this));
	if (GET_EMAIL(this))
		fprintf(saved, "EMal: %s\n", GET_EMAIL(this));
	if (GET_TITLE(this))
		fprintf(saved, "Titl: %s\n", GET_TITLE(this));
	if (this->player.description && *this->player.description)
	{
		strcpy(buf, this->player.description);
		kill_ems(buf);
		fprintf(saved, "Desc:\n%s~\n", buf);
	}
	if (POOFIN(this))
		fprintf(saved, "PfIn: %s\n", POOFIN(this));
	if (POOFOUT(this))
		fprintf(saved, "PfOt: %s\n", POOFOUT(this));
	fprintf(saved, "Sex : %d %s\n", GET_SEX(this), genders[(int) GET_SEX(this)]);
	fprintf(saved, "Kin : %d %s\n", GET_KIN(this), kin_name[GET_KIN(this)][(int) GET_SEX(this)]);
	if ((location = real_room(GET_HOME(this))) != NOWHERE)
		fprintf(saved, "Home: %d %s\n", GET_HOME(this), world[(location)]->name);
	li = this->player.time.birth;
	fprintf(saved, "Brth: %ld %s\n", static_cast<long int>(li), ctime(&li));
	// Gunner
	// time.logon ЦАБ=---Lг- -= -Е-г ┐ёЮ-L-- - А┐АБг-Ц = time(0) -= БгLЦИ┐L ---г-Б
	//-Юг-О - АгLЦ-г=Е А -=Г=L= ┐ёЮК
	tmp += this->player.time.played;
	fprintf(saved, "Plyd: %d\n", tmp);
	// Gunner end
	li = this->player.time.logon;
	fprintf(saved, "Last: %ld %s\n", static_cast<long int>(li), ctime(&li));
	if (this->desc)
		strcpy(buf, this->desc->host);
	else
		strcpy(buf, "Unknown");
	fprintf(saved, "Host: %s\n", buf);
	fprintf(saved, "Hite: %d\n", GET_HEIGHT(this));
	fprintf(saved, "Wate: %d\n", GET_WEIGHT(this));
	fprintf(saved, "Size: %d\n", GET_SIZE(this));
	/* структуры */
	fprintf(saved, "Alin: %d\n", GET_ALIGNMENT(this));
	*buf = '\0';
	tascii(&AFF_FLAGS(this, 0), 4, buf);
	fprintf(saved, "Aff : %s\n", buf);

	/* дальше не по порядку */
	/* статсы */
	fprintf(saved, "Str : %d\n", GET_STR(this));
	fprintf(saved, "Int : %d\n", GET_INT(this));
	fprintf(saved, "Wis : %d\n", GET_WIS(this));
	fprintf(saved, "Dex : %d\n", GET_DEX(this));
	fprintf(saved, "Con : %d\n", GET_CON(this));
	fprintf(saved, "Cha : %d\n", GET_CHA(this));

	/* способности - added by Gorrah */
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "Feat:\n");
		for (i = 1; i < MAX_FEATS; i++)
		{
			if (HAVE_FEAT(this, i))
				fprintf(saved, "%d %s\n", i, feat_info[i].name);
		}
		fprintf(saved, "0 0\n");
	}

	/* Задержки на cпособности */
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "FtTm:\n");
		for (skj = this->timed_feat; skj; skj = skj->next)
		{
			fprintf(saved, "%d %d %s\n", skj->skill, skj->time, feat_info[skj->skill].name);
		}
		fprintf(saved, "0 0\n");
	}

	/* Скиллы и задержки на скилы */
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "Skil:\n");
		for (CharSkillsType::iterator it = skills.begin(); it != skills.end(); ++it)
		{
			fprintf(saved, "%d %d %s\n", it->first, it->second, skill_info[it->first].name);
		}
		fprintf(saved, "0 0\n");

		fprintf(saved, "SkTm:\n");
		for (skj = this->timed; skj; skj = skj->next)
		{
			fprintf(saved, "%d %d\n", skj->skill, skj->time);
		}
		fprintf(saved, "0 0\n");
	}

	/* спелы */
	// волхвам всеравно известны тупо все спеллы, смысла их писать не вижу
	if (GET_LEVEL(this) < LVL_IMMORT && GET_CLASS(this) != CLASS_DRUID)
	{
		fprintf(saved, "Spel:\n");
		for (i = 1; i <= MAX_SPELLS; i++)
			if (GET_SPELL_TYPE(this, i))
				fprintf(saved, "%d %d %s\n", i, GET_SPELL_TYPE(this, i), spell_info[i].name);
		fprintf(saved, "0 0\n");
	}

	/* Замемленые спелы */
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "SpMe:\n");
		for (i = 1; i <= MAX_SPELLS; i++)
		{
			if (GET_SPELL_MEM(this, i))
				fprintf(saved, "%d %d\n", i, GET_SPELL_MEM(this, i));
		}
		fprintf(saved, "0 0\n");
	}

	/* Рецепты */
//    if (GET_LEVEL(this) < LVL_IMMORT)
	{
		im_rskill *rs;
		im_recipe *r;
		fprintf(saved, "Rcps:\n");
		for (rs = GET_RSKILL(this); rs; rs = rs->link)
		{
			if (rs->perc <= 0)
				continue;
			r = &imrecipes[rs->rid];
			fprintf(saved, "%d %d %s\n", r->id, rs->perc, r->name);
		}
		fprintf(saved, "-1 -1\n");
	}

	fprintf(saved, "Hrol: %d\n", GET_HR(this));
	fprintf(saved, "Drol: %d\n", GET_DR(this));
	fprintf(saved, "Ac  : %d\n", GET_AC(this));

	fprintf(saved, "Hit : %d/%d\n", GET_HIT(this), GET_MAX_HIT(this));
	fprintf(saved, "Mana: %d/%d\n", GET_MEM_COMPLETED(this), GET_MEM_TOTAL(this));
	fprintf(saved, "Move: %d/%d\n", GET_MOVE(this), GET_MAX_MOVE(this));
	fprintf(saved, "Gold: %d\n", get_gold(this));
	fprintf(saved, "Bank: %ld\n", get_bank_gold(this));
	fprintf(saved, "PK  : %ld\n", IS_KILLER(this));

	fprintf(saved, "Wimp: %d\n", GET_WIMP_LEV(this));
	fprintf(saved, "Frez: %d\n", GET_FREEZE_LEV(this));
	fprintf(saved, "Invs: %d\n", GET_INVIS_LEV(this));
	fprintf(saved, "Room: %d\n", GET_LOADROOM(this));

	fprintf(saved, "Badp: %d\n", GET_BAD_PWS(this));

	if (GET_BOARD(this))
		for (int i = 0; i < BOARD_TOTAL; ++i)
			fprintf(saved, "Br%02d: %ld\n", i + 1, static_cast<long int>(GET_BOARD_DATE(this, i)));

	for (int i = 0; i <= START_STATS_TOTAL; ++i)
		fprintf(saved, "St%02d: %i\n", i, GET_START_STAT(this, i));

	if (GET_LEVEL(this) < LVL_IMMORT)
		fprintf(saved, "Hung: %d\n", GET_COND(this, FULL));
	if (GET_LEVEL(this) < LVL_IMMORT)
		fprintf(saved, "Thir: %d\n", GET_COND(this, THIRST));
	if (GET_LEVEL(this) < LVL_IMMORT)
		fprintf(saved, "Drnk: %d\n", GET_COND(this, DRUNK));

	fprintf(saved, "Reli: %d %s\n", GET_RELIGION(this), religion_name[GET_RELIGION(this)][(int) GET_SEX(this)]);
	fprintf(saved, "Race: %d %s\n", GET_RACE(this), race_name[GET_RACE(this)][(int) GET_SEX(this)]);
	fprintf(saved, "DrSt: %d\n", GET_DRUNK_STATE(this));
	fprintf(saved, "Glor: %d\n", GET_GLORY(this));
	fprintf(saved, "Olc : %d\n", GET_OLC_ZONE(this));
	*buf = '\0';
	tascii(&PRF_FLAGS(this, 0), 4, buf);
	fprintf(saved, "Pref: %s\n", buf);

	if (MUTE_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_MUTE))
		fprintf(saved, "PMut: %ld %d %ld %s~\n", MUTE_DURATION(this), GET_MUTE_LEV(this), MUTE_GODID(this), MUTE_REASON(this));
	if (NAME_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_NAMED))
		fprintf(saved, "PNam: %ld %d %ld %s~\n", NAME_DURATION(this), GET_NAME_LEV(this), NAME_GODID(this), NAME_REASON(this));
	if (DUMB_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_DUMB))
		fprintf(saved, "PDum: %ld %d %ld %s~\n", DUMB_DURATION(this), GET_DUMB_LEV(this), DUMB_GODID(this), DUMB_REASON(this));
	if (HELL_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_HELLED))
		fprintf(saved, "PHel: %ld %d %ld %s~\n", HELL_DURATION(this), GET_HELL_LEV(this), HELL_GODID(this), HELL_REASON(this));
	if (GCURSE_DURATION(this) > 0)
		fprintf(saved, "PGcs: %ld %d %ld %s~\n", GCURSE_DURATION(this), GET_GCURSE_LEV(this), GCURSE_GODID(this), GCURSE_REASON(this));
	if (FREEZE_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_FROZEN))
		fprintf(saved, "PFrz: %ld %d %ld %s~\n", FREEZE_DURATION(this), GET_FREEZE_LEV(this), FREEZE_GODID(this), FREEZE_REASON(this));
	if (UNREG_DURATION(this) > 0)
		fprintf(saved, "PUnr: %ld %d %ld %s~\n", UNREG_DURATION(this), GET_UNREG_LEV(this), UNREG_GODID(this), UNREG_REASON(this));


	if (KARMA(this) > 0)
	{
		strcpy(buf, KARMA(this));
		kill_ems(buf);
		fprintf(saved, "Karm:\n%s~\n", buf);
	}
	if (LOGON_LIST(this) > 0)
	{
		log("Saving logon list.");
		struct logon_data * next_log = LOGON_LIST(this);
		std::stringstream buffer;
		while (next_log)
		{
			buffer << next_log->ip << " " << next_log->count << " " << next_log->lasttime << "\n";
			next_log = next_log->next;
		}
		fprintf(saved, "LogL:\n%s~\n", buffer.str().c_str());
	}
	fprintf(saved, "GdFl: %ld\n", this->player_specials->saved.GodsLike);
	fprintf(saved, "NamG: %d\n", NAME_GOD(this));
	fprintf(saved, "NaID: %ld\n", NAME_ID_GOD(this));
	fprintf(saved, "StrL: %d\n", STRING_LENGTH(this));
	fprintf(saved, "StrW: %d\n", STRING_WIDTH(this));
	if (EXCHANGE_FILTER(this))
		fprintf(saved, "ExFl: %s\n", EXCHANGE_FILTER(this));

	// shapirus: игнор лист
	{
		struct ignore_data *cur = IGNORE_LIST(this);
		if (cur)
		{
			for (; cur; cur = cur->next)
			{
				if (!cur->id)
					continue;
				fprintf(saved, "Ignr: [%ld]%ld\n", cur->mode, cur->id);
			}
		}
	}

	/* affected_type */
	if (tmp_aff[0].type > 0)
	{
		fprintf(saved, "Affs:\n");
		for (i = 0; i < MAX_AFFECT; i++)
		{
			aff = &tmp_aff[i];
			if (aff->type)
				fprintf(saved, "%d %d %d %d %d %s\n", aff->type, aff->duration,
						aff->modifier, aff->location, (int) aff->bitvector, spell_name(aff->type));
		}
		fprintf(saved, "0 0 0 0 0\n");
	}

	/* порталы */
	for (prt = GET_PORTALS(this); prt; prt = prt->next)
	{
		fprintf(saved, "Prtl: %d\n", prt->vnum);
	}
	for (i = 0; i < NLOG; ++i)
	{
		if (!GET_LOGS(this))
		{
			log("SYSERR: Saving NULL logs for char %s", GET_NAME(this));
			break;
		}
		fprintf(saved, "Logs: %d %d\n", i, GET_LOGS(this)[i]);
	}
	/* Квесты */
	if (this->Questing.quests)
	{
		for (i = 0; i < this->Questing.count; i++)
		{
			fprintf(saved, "Qst : %d\n", *(this->Questing.quests + i));
		}
	}

	save_mkill(this, saved);
	save_pkills(this, saved);

	fclose(saved);
	FileCRC::check_crc(filename, FileCRC::UPDATE_PLAYER, GET_UNIQUE(this));

	/* восстанавливаем аффекты */
	/* add spell and eq affections back in now */
	for (i = 0; i < MAX_AFFECT; i++)
	{
		if (tmp_aff[i].type)
			affect_to_char(this, &tmp_aff[i]);
	}

	for (i = 0; i < NUM_WEARS; i++)
	{
		if (char_eq[i])
		{
#ifndef NO_EXTRANEOUS_TRIGGERS
			if (wear_otrigger(char_eq[i], this, i))
#endif
				equip_char(this, char_eq[i], i | 0x80 | 0x40);
#ifndef NO_EXTRANEOUS_TRIGGERS
			else
				obj_to_char(char_eq[i], this);
#endif
		}
	}
	affect_total(this);

	if ((i = get_ptable_by_name(GET_NAME(this))) >= 0)
	{
		player_table[i].last_logon = LAST_LOGON(this);
		player_table[i].level = GET_LEVEL(this);
	}
}

/**
*
*/
void set_god_skills(CHAR_DATA *ch)
{
	for (int i = 1; i <= MAX_SKILL_NUM; i++)
		ch->set_skill(i, 100);
}
