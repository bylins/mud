// Part of Bylins http://www.mud.ru

#include "fight.h"
#include "fight_local.hpp"
#include "utils.h"
#include "char.hpp"
#include "skills.h"
#include "handler.h"
#include "db.h"
#include "room.hpp"
#include "spells.h"
#include "dg_scripts.h"
#include "corpse.hpp"
#include "house.h"
#include "pk.h"
#include "stuff.hpp"
#include "sets_drop.hpp"
#include "top.h"
#include "constants.h"
#include "screen.h"
#include "magic.h"
#include "mob_stat.hpp"

// extern
void perform_drop_gold(CHAR_DATA * ch, int amount, byte mode, room_rnum RDR);
int level_exp(CHAR_DATA * ch, int chlevel);
int max_exp_gain_pc(CHAR_DATA * ch);
int max_exp_loss_pc(CHAR_DATA * ch);
void get_from_container(CHAR_DATA * ch, OBJ_DATA * cont, char *arg, int mode, int amount, bool autoloot);
int slot_for_char(CHAR_DATA * ch, int i);
int mag_manacost(CHAR_DATA * ch, int spellnum);
ACMD(do_flee);

extern int material_value[];
extern int max_exp_gain_npc;

//интервал в секундах между восстановлением кругов после рипа
extern const unsigned RECALL_SPELLS_INTERVAL;
const unsigned RECALL_SPELLS_INTERVAL = 28;

void process_mobmax(CHAR_DATA *ch, CHAR_DATA *killer)
{
	CHAR_DATA *master = 0;

	if (IS_NPC(killer)
		&& (AFF_FLAGGED(killer, AFF_CHARM) || MOB_FLAGGED(killer, MOB_ANGEL))
		&& killer->master)
	{
		master = killer->master;
	}
	else if (!IS_NPC(killer))
		master = killer;

	// На этот момент master - PC
	if (master)
	{
		if (AFF_FLAGGED(master, AFF_GROUP))
		{
			int cnt = 0;

			// master - член группы, переходим на лидера группы
			if (master->master)
				master = master->master;

			if (IN_ROOM(master) == IN_ROOM(killer))
			{
				// лидер группы в тойже комнате, что и убивец
				cnt = 1;
			}

			for (struct follow_type *f = master->followers; f; f = f->next)
			{
				if (AFF_FLAGGED(f->follower, AFF_GROUP)
					&& IN_ROOM(f->follower) == IN_ROOM(killer))
				{
					if (!number(0, cnt))
					{
						master = f->follower;
					}
					++cnt;
				}
			}
		}
		master->mobmax_add(master, GET_MOB_VNUM(ch), 1, GET_LEVEL(ch));
	}
}

// 29.11.09 Увеличиваем счетчики рипов (с) Василиса
//edited by WorM
void update_die_counts(CHAR_DATA *ch, CHAR_DATA *killer, int dec_exp)
{
	//настоящий убийца мастер чармиса/коня/ангела
	CHAR_DATA *rkiller = killer;

	if (rkiller
		&& IS_NPC(rkiller)
		&& (IS_CHARMICE(rkiller)
			|| IS_HORSE(rkiller)
			|| MOB_FLAGGED(killer, MOB_ANGEL)))
	{
		if(rkiller->master)
		{
			rkiller = rkiller->master;
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH,
				"die: %s killed by %s (without master)",
				GET_PAD(ch,0), GET_PAD(rkiller,0));
			mudlog(buf, LGH, LVL_IMMORT, SYSLOG, TRUE);
			rkiller = NULL;
		}
	}
	if (!IS_NPC(ch))
	{
		if (rkiller && rkiller != ch)
		{
			if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA)) //Рип на арене
			{
				GET_RIP_ARENA(ch)= GET_RIP_ARENA(ch)+1;
				GET_WIN_ARENA(killer)= GET_WIN_ARENA(killer)+1;
				if (dec_exp)
				{
					GET_EXP_ARENA(ch)=GET_EXP_ARENA(ch)+dec_exp; //Если чар в бд
				}
			}
			else if (IS_NPC(rkiller))
			{
				//Рип от моба
				GET_RIP_MOB(ch)= GET_RIP_MOB(ch)+1;
				GET_RIP_MOBTHIS(ch)= GET_RIP_MOBTHIS(ch)+1;
				if (dec_exp)
				{
					GET_EXP_MOB(ch)= GET_EXP_MOB(ch)+dec_exp;
					GET_EXP_MOBTHIS(ch)= GET_EXP_MOBTHIS(ch)+dec_exp;
				}
			}
			else if (!IS_NPC(rkiller))
			{
				//Рип в ПК
				GET_RIP_PK(ch)= GET_RIP_PK(ch)+1;
				GET_RIP_PKTHIS(ch)= GET_RIP_PKTHIS(ch)+1;
				if (dec_exp)
				{
					GET_EXP_PK(ch)= GET_EXP_PK(ch)+dec_exp;
					GET_EXP_PKTHIS(ch)= GET_EXP_PKTHIS(ch)+dec_exp;
				}
			}
		}
		else if ((!rkiller || (rkiller && rkiller == ch)) &&
			(ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) ||
			ROOM_FLAGGED(IN_ROOM(ch), ROOM_SLOWDEATH) ||
			ROOM_FLAGGED(IN_ROOM(ch), ROOM_ICEDEATH)))
		{
			//Рип в дт
			GET_RIP_DT(ch)= GET_RIP_DT(ch)+1;
			GET_RIP_DTTHIS(ch)= GET_RIP_DTTHIS(ch)+1;
			if (dec_exp)
			{
				GET_EXP_DT(ch)= GET_EXP_DT(ch)+dec_exp;
				GET_EXP_DTTHIS(ch)= GET_EXP_DTTHIS(ch)+dec_exp;
			}
		}
		else// if (!rkiller || (rkiller && rkiller == ch))
		{
			//Рип по стечению обстоятельств
			GET_RIP_OTHER(ch)= GET_RIP_OTHER(ch)+1;
			GET_RIP_OTHERTHIS(ch)= GET_RIP_OTHERTHIS(ch)+1;
			if (dec_exp)
			{
				GET_EXP_OTHER(ch)= GET_EXP_OTHER(ch)+dec_exp;
				GET_EXP_OTHERTHIS(ch)= GET_EXP_OTHERTHIS(ch)+dec_exp;
			}
		}
	}
}
//end by WorM
//конец правки (с) Василиса

void update_leadership(CHAR_DATA *ch, CHAR_DATA *killer)
{
	// train LEADERSHIP
	if (IS_NPC(ch) && killer) // Убили моба
	{
		if (!IS_NPC(killer) // Убил загрупленный чар
			&& AFF_FLAGGED(killer, AFF_GROUP)
			&& killer->master
			&& killer->master->get_skill(SKILL_LEADERSHIP) > 0
			&& IN_ROOM(killer) == IN_ROOM(killer->master))
		{
			improove_skill(killer->master, SKILL_LEADERSHIP, number(0, 1), ch);
		}
		else if (IS_NPC(killer) // Убил чармис загрупленного чара
			&& IS_CHARMICE(killer)
			&& killer->master
			&& AFF_FLAGGED(killer->master, AFF_GROUP))
		{
			if (killer->master->master // Владелец чармиса НЕ лидер
				&& killer->master->master->get_skill(SKILL_LEADERSHIP) > 0
				&& IN_ROOM(killer) == IN_ROOM(killer->master)
				&& IN_ROOM(killer) == IN_ROOM(killer->master->master))
			{
				improove_skill(killer->master->master, SKILL_LEADERSHIP, number(0, 1), ch);
			}
		}
	}

	// decrease LEADERSHIP
	if (!IS_NPC(ch) // Член группы убит мобом
		&& killer
		&& IS_NPC(killer)
		&& AFF_FLAGGED(ch, AFF_GROUP)
		&& ch->master
		&& IN_ROOM(ch) == IN_ROOM(ch->master)
		&& ch->master->get_inborn_skill(SKILL_LEADERSHIP) > 1)
	{
		ch->master->set_skill(SKILL_LEADERSHIP,
			ch->master->get_trained_skill(SKILL_LEADERSHIP) - 1);
	}
}

void die(CHAR_DATA *ch, CHAR_DATA *killer)
{
	int dec_exp = 0, e = GET_EXP(ch);

	if (!IS_NPC(ch) && (IN_ROOM(ch) == NOWHERE))
	{
		log("SYSERR: %s is dying in room NOWHERE.", GET_NAME(ch));
		return;
	}

	if (IS_NPC(ch)
		|| !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA)
		|| RENTABLE(ch))
	{
		if (!(IS_NPC(ch)
			|| IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, GF_GODSLIKE)
			|| (killer && PRF_FLAGGED(killer, PRF_EXECUTOR))))//если убил не палач
		{
			dec_exp = (level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch))) / (3 + MIN(3, GET_REMORT(ch) / 5));
			gain_exp(ch, -dec_exp);
			dec_exp = e - GET_EXP(ch);
			sprintf(buf, "Вы потеряли %d %s опыта.\r\n",
				dec_exp, desc_count(dec_exp, WHAT_POINT));
			send_to_char(buf, ch);
		}

		// Вычисляем замакс по мобам
		// Решил немножко переделать, чтобы короче получилось,
		// кроме того, исправил ошибку с присутствием лидера в комнате
		if (IS_NPC(ch) && killer)
		{
			process_mobmax(ch, killer);
		}

		if (killer)
		{
			update_leadership(ch, killer);
		}
	}

	update_die_counts(ch, killer, dec_exp);
	raw_kill(ch, killer);
}

// * Снятие аффектов с чара при смерти/уходе в дт.
void reset_affects(CHAR_DATA *ch)
{
	AFFECT_DATA *af, *naf;

	for (af = ch->affected; af; af = naf)
	{
		naf = af->next;
		if (!IS_SET(af->battleflag, AF_DEADKEEP))
			affect_remove(ch, af);
	}

	GET_COND(ch, DRUNK) = 0; // Чтобы не шатало без аффекта "под мухой"
	affect_total(ch);
}

void forget_all_spells(CHAR_DATA *ch)
{
	GET_MEM_COMPLETED(ch) = 0;
	int slots[MAX_SLOT];
	int max_slot = 0;
	for (unsigned i = 0; i < MAX_SLOT; ++i)
	{
		slots[i] = slot_for_char(ch, i + 1);
		if (slots[i]) max_slot = i+1;
	}
	struct spell_mem_queue_item *qi_cur, ** qi = &ch->MemQueue.queue;
	while (*qi)
	{
		--slots[spell_info[(*(qi))->spellnum].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1];
		qi = &((*qi)->link);
	}
	int slotn;

	for (int i = 0; i < MAX_SPELLS + 1; i++)
	{
		if (PRF_FLAGGED(ch, PRF_AUTOMEM) && ch->real_abils.SplMem[i])
		{
			slotn = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
			for (unsigned j=0; (slots[slotn]>0 && j<ch->real_abils.SplMem[i]); ++j, --slots[slotn])
			{
				ch->MemQueue.total += mag_manacost(ch, i);
				CREATE(qi_cur, struct spell_mem_queue_item, 1);
				*qi = qi_cur;
				qi_cur->spellnum = i;
				qi_cur->link = NULL;
				qi = &qi_cur->link;
			}
		}
		ch->real_abils.SplMem[i] = 0;
	}
	if (max_slot)
	{
		AFFECT_DATA af;
		af.type = SPELL_RECALL_SPELLS;
		af.location = APPLY_NONE;
		af.modifier = 1; // номер круга, который восстанавливаем
		//добавим 1 проход про запас, иначе неуспевает отмемиться последний круг -- аффект спадает раньше
		af.duration = pc_duration(ch, max_slot*RECALL_SPELLS_INTERVAL+SECS_PER_PLAYER_AFFECT, 0, 0, 0, 0);
		af.bitvector = AFF_RECALL_SPELLS;
		af.battleflag = AF_PULSEDEC | AF_DEADKEEP;
		affect_join(ch, &af, 0, 0, 0, 0);
	}
}

/* Функция используемая при "автограбеже" и "автолуте",
   чтобы не было лута под холдом или в слепи             */
int can_loot(CHAR_DATA * ch)
{
	if (ch != NULL)
	{
		if (!IS_NPC(ch)
			&& GET_MOB_HOLD(ch) == 0 // если под холдом
			&& !AFF_FLAGGED(ch, AFF_STOPFIGHT) // парализован точкой
			&& !AFF_FLAGGED(ch, AFF_BLIND)	// слеп
			&& (GET_POS(ch) >= POS_RESTING)) // мертв, умирает, без сознания, спит
		{
			return TRUE;
		}
	}
	return FALSE;
}

void death_cry(CHAR_DATA * ch)
{
	int door;
	act("Кровушка стынет в жилах от предсмертного крика $n1.",
		FALSE, ch, 0, 0, TO_ROOM | CHECK_DEAF | TO_ARENA_LISTEN);

	for (door = 0; door < NUM_OF_DIRS; door++)
	{
		if (CAN_GO(ch, door))
		{
			CHAR_DATA *people = world[world[IN_ROOM(ch)]->dir_option[door]->to_room]->people;
			if (people)
			{
				act("Кровушка стынет в жилах от чьего-то предсмертного крика.",
					FALSE, people, 0, 0, TO_CHAR | CHECK_DEAF);
				act("Кровушка стынет в жилах от чьего-то предсмертного крика.",
					FALSE, people, 0, 0, TO_ROOM | CHECK_DEAF);
			}
		}
	}
}

void arena_kill(CHAR_DATA *ch, CHAR_DATA *killer)
{
	make_arena_corpse(ch, killer);
	//Если убил палач то все деньги перекачивают к нему
	if(killer && PRF_FLAGGED(killer, PRF_EXECUTOR))
	{
		killer->set_gold(ch->get_gold() + killer->get_gold());
		ch->set_gold(0);
	}
	change_fighting(ch, TRUE);
	GET_HIT(ch) = 1;
	GET_POS(ch) = POS_SITTING;
	char_from_room(ch);
	int to_room = real_room(GET_LOADROOM(ch));
	// тут придется ручками тащить чара за ворота, если ему в замке не рады
	if (!Clan::MayEnter(ch, to_room, HCE_PORTAL))
	{
		to_room = Clan::CloseRent(to_room);
	}
	if (to_room == NOWHERE)
	{
		SET_BIT(PLR_FLAGS(ch, PLR_HELLED), PLR_HELLED);
		HELL_DURATION(ch) = time(0) + 6;
		to_room = r_helled_start_room;
	}
	char_to_room(ch, to_room);
	look_at_room(ch, to_room);
	act("$n со стонами упал$g с небес...", FALSE, ch, 0, 0, TO_ROOM);
}

void auto_loot(CHAR_DATA *ch, CHAR_DATA *killer, OBJ_DATA *corpse, int local_gold)
{
	char obj[256];

	if (IS_NPC(ch)
		&& !IS_NPC(killer)
		&& PRF_FLAGGED(killer, PRF_AUTOLOOT)
		&& (corpse != NULL)
		&& can_loot(killer))
	{
		sprintf(obj, "all");
		get_from_container(killer, corpse, obj, FIND_OBJ_INV, 1, true);
	}
	else if (IS_NPC(ch)
		&& !IS_NPC(killer)
		&& local_gold
		&& PRF_FLAGGED(killer, PRF_AUTOMONEY)
		&& (corpse != NULL)
		&& can_loot(killer))
	{
		sprintf(obj, "all.coin");
		get_from_container(killer, corpse, obj, FIND_OBJ_INV, 1, false);
	}
	else if (IS_NPC(ch)
		&& IS_NPC(killer)
		&& (AFF_FLAGGED(killer, AFF_CHARM) || MOB_FLAGGED(killer, MOB_ANGEL))
		&& (corpse != NULL)
		&& killer->master
		&& killer->in_room == killer->master->in_room
		&& PRF_FLAGGED(killer->master, PRF_AUTOLOOT)
		&& can_loot(killer->master))
	{
		sprintf(obj, "all");
		get_from_container(killer->master, corpse, obj, FIND_OBJ_INV, 1, true);
	}
	else if (IS_NPC(ch)
		&& IS_NPC(killer)
		&& local_gold
		&& (AFF_FLAGGED(killer, AFF_CHARM) || MOB_FLAGGED(killer, MOB_ANGEL))
		&& (corpse != NULL)
		&& killer->master
		&& killer->in_room == killer->master->in_room
		&& PRF_FLAGGED(killer->master, PRF_AUTOMONEY)
		&& can_loot(killer->master))
	{
		sprintf(obj, "all.coin");
		get_from_container(killer->master, corpse, obj, FIND_OBJ_INV, 1, false);
	}
}

void check_spell_capable(CHAR_DATA *ch, CHAR_DATA *killer)
{
	if(IS_NPC(ch)
		&& killer
		&& killer != ch
		&& MOB_FLAGGED(ch, MOB_CLONE)
		&& ch->master
		&& affected_by_spell(ch, SPELL_CAPABLE))
	{
		act("Чары, наложенные на $n3, тускло засветились и стали превращаться в нечто опасное.",
			FALSE, ch, 0, killer, TO_ROOM | TO_ARENA_LISTEN);
		int pos = GET_POS(ch);
		GET_POS(ch) = POS_STANDING;
		call_magic(ch, killer, NULL, world[IN_ROOM(ch)], ch->mob_specials.capable_spell,
			GET_LEVEL(ch), CAST_SPELL);
		GET_POS(ch) = pos;
	}
}

void clear_mobs_memory(CHAR_DATA *ch)
{
	for (CHAR_DATA *hitter = character_list; hitter; hitter = hitter->next)
	{
		if (IS_NPC(hitter) && MEMORY(hitter))
		{
			forget(hitter, ch);
		}
	}
}

void real_kill(CHAR_DATA *ch, CHAR_DATA *killer)
{
	const long local_gold = ch->get_gold();
	OBJ_DATA *corpse = make_corpse(ch, killer);
	bloody::handle_corpse(corpse, ch, killer);

	// Перенес вызов pk_revenge_action из die, чтобы на момент создания
	// трупа месть на убийцу была еще жива
	if (IS_NPC(ch) || !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) || RENTABLE(ch))
	{
		pk_revenge_action(killer, ch);
	}

	if (!IS_NPC(ch))
	{
		forget_all_spells(ch);
		clear_mobs_memory(ch);
		// Если убит в бою - то может выйти из игры
		RENTABLE(ch) = 0;
		AGRESSOR(ch) = 0;
		AGRO(ch) = 0;
	}
	else
	{
		if (killer && (!IS_NPC(killer) || IS_CHARMICE(killer)))
		{
			log("Killed: %d %d %ld", GET_LEVEL(ch), GET_MAX_HIT(ch), GET_EXP(ch));
			obj_load_on_death(corpse, ch);
		}
		if (MOB_FLAGGED(ch, MOB_CORPSE))
		{
			perform_drop_gold(ch, local_gold, SCMD_DROP, 0);
			ch->set_gold(0);
		}
		dl_load_obj(corpse, ch, NULL, DL_ORDINARY);
		dl_load_obj(corpse, ch, NULL, DL_PROGRESSION);
	}

	// Теперь реализация режимов "автограбеж" и "брать куны" происходит не в damage,
	// а здесь, после создания соответствующего трупа. Кроме того,
	// если убил чармис и хозяин в комнате, то автолут происходит хозяину
	if ((ch != NULL) && (killer != NULL))
	{
		auto_loot(ch, killer, corpse, local_gold);
	}
}

void raw_kill(CHAR_DATA *ch, CHAR_DATA *killer)
{
	check_spell_capable(ch, killer);

	if (ch->get_fighting())
		stop_fighting(ch, TRUE);

	for (CHAR_DATA *hitter = combat_list; hitter; hitter = hitter->next_fighting)
	{
		if (hitter->get_fighting() == ch)
		{
			WAIT_STATE(hitter, 0);
		}
	}

	reset_affects(ch);

	if ((!killer || death_mtrigger(ch, killer)) && IN_ROOM(ch) != NOWHERE)
	{
		death_cry(ch);
	}

	if (IN_ROOM(ch) != NOWHERE)
	{
		if (!IS_NPC(ch)
			&& ((!RENTABLE(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA))
				|| (killer && PRF_FLAGGED(killer, PRF_EXECUTOR))))
		{
			//Если убили на арене или палач
			arena_kill(ch, killer);
		}
		else
		{
			real_kill(ch, killer);
			extract_char(ch, TRUE);
		}
	}
}

int get_extend_exp(int exp, CHAR_DATA * ch, CHAR_DATA * victim)
{
	int base, diff;
	int koef;

	if (!IS_NPC(victim) || IS_NPC(ch))
		return (exp);

	for (koef = 100, base = 0, diff = ch->mobmax_get(GET_MOB_VNUM(victim));
			base < diff && koef > 5; base++, koef = koef * 95 / 100);

	exp = exp * MAX(5, koef) / 100;
	exp /= MAX(1, GET_REMORT(ch) - MAX_EXP_COEFFICIENTS_USED - 1);

	return (exp);
}

// When ch kills victim
void change_alignment(CHAR_DATA * ch, CHAR_DATA * victim)
{
	/*
	 * new alignment change algorithm: if you kill a monster with alignment A,
	 * you move 1/16th of the way to having alignment -A.  Simple and fast.
	 */
	GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}

/*++
   Функция начисления опыта
      ch - кому опыт начислять
           Вызов этой функции для NPC смысла не имеет, но все равно
           какие-то проверки внутри зачем то делаются
--*/
void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef)
{
	if (!EXTRA_FLAGGED(victim, EXTRA_GRP_KILL_COUNT)
		&& !IS_NPC(ch)
		&& !IS_IMMORTAL(ch)
		&& IS_NPC(victim)
		&& !IS_CHARMICE(victim)
		&& !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
	{
		mob_stat::add_mob(victim, members);
		SET_BIT(EXTRA_FLAGS(victim, EXTRA_GRP_KILL_COUNT), EXTRA_GRP_KILL_COUNT);
	}
	else if (IS_NPC(ch) && !IS_NPC(victim)
		&& !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
	{
		mob_stat::add_mob(ch, 0);
	}

// Странно, но для NPC эта функция тоже должна работать
//  if (IS_NPC(ch) || !OK_GAIN_EXP(ch,victim))
	if (!OK_GAIN_EXP(ch, victim))
	{
		send_to_char("Ваше деяние никто не оценил.\r\n", ch);
		return;
	}

	// 1. Опыт делится поровну на всех
	int exp = GET_EXP(victim) / MAX(members, 1);

	if(victim->get_zone_group() > 1 && members < victim->get_zone_group())
	{
		// в случае груп-зоны своего рода планка на мин кол-во человек в группе
		exp = GET_EXP(victim) / victim->get_zone_group();
	}

	// 2. Учитывается коэффициент (лидерство, разность уровней)
	//    На мой взгляд его правильней использовать тут а не в конце процедуры,
	//    хотя в большинстве случаев это все равно
	exp = exp * koef / 100;

	// 3. Вычисление опыта для PC и NPC
	if (IS_NPC(ch))
	{
		exp = MIN(max_exp_gain_npc, exp);
		exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
	}
	else
	{
		exp = MIN(max_exp_gain_pc(ch), get_extend_exp(exp, ch, victim));
	}

	// 4. Последняя проверка
	exp = MAX(1, exp);

	if (exp > 1)
	{
		send_to_char(ch, "Ваш опыт повысился на %d %s.\r\n",
			exp, desc_count(exp, WHAT_POINT));
	}
	else if (exp == 1)
	{
		send_to_char(
			"Ваш опыт повысился всего лишь на маленькую единичку.\r\n", ch);
	}

	gain_exp(ch, exp);
	change_alignment(ch, victim);
	TopPlayer::Refresh(ch);
}

/*++
   Функция расчитывает всякие бонусы для группы при получении опыта,
 после чего вызывает функцию получения опыта для всех членов группы
 Т.к. членом группы может быть только PC, то эта функция раздаст опыт только PC

   ch - обязательно член группы, из чего следует:
            1. Это не NPC
            2. Он находится в группе лидера (или сам лидер)

   Просто для PC-последователей эта функция не вызывается

--*/
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int inroom_members, koef = 100, maxlevel;
	CHAR_DATA *k;
	struct follow_type *f;
	int leader_inroom;

	maxlevel = GET_LEVEL(ch);

	if (!(k = ch->master))
		k = ch;

	// k - подозрение на лидера группы
	leader_inroom = (AFF_FLAGGED(k, AFF_GROUP)
					 && (k->in_room == IN_ROOM(ch)));

	// Количество согрупников в комнате
	if (leader_inroom)
	{
		inroom_members = 1;
		maxlevel = GET_LEVEL(k);
	}
	else
		inroom_members = 0;

	// Вычисляем максимальный уровень в группе
	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == IN_ROOM(ch))
		{
			// просмотр членов группы в той же комнате
			// член группы => PC автоматически
			++inroom_members;
			maxlevel = MAX(maxlevel, GET_LEVEL(f->follower));
		}

	// Вычисляем, надо ли резать экспу, смотрим сначала лидера, если он рядом
	if (maxlevel - GET_LEVEL(k) > grouping[(int)GET_CLASS(k)][(int)GET_REMORT(k)] && leader_inroom)
	{
		koef -= 50 + (maxlevel - GET_LEVEL(k) - grouping[(int)GET_CLASS(k)][(int)GET_REMORT(k)]) * 2;
	}
	else	// если с лидером все ок либо он не тут, смотрим по группе
	{
		for (f = k->followers; f; f = f->next)
		{
			if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == IN_ROOM(ch))
			{
				if (maxlevel - GET_LEVEL(f->follower) >
						grouping[(int)GET_CLASS(f->follower)][(int)GET_REMORT(f->follower)])
				{
					koef -= 50 + (maxlevel - GET_LEVEL(f->follower)
						- grouping[(int)GET_CLASS(f->follower)][(int)GET_REMORT(f->follower)]) * 2;
					break;
				}
			}
		}
	}

	koef = MAX(0, koef);

	// Лидерство используется, если в комнате лидер и есть еще хоть кто-то
	// из группы из PC (последователи типа лошади или чармисов не считаются)
	if (koef >= 100 && leader_inroom && (inroom_members > 1) && calc_leadership(k))
	{
		koef += 20;
	}

	// Раздача опыта

	if (leader_inroom)
		perform_group_gain(k, victim, inroom_members, koef);

	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == IN_ROOM(ch))
			perform_group_gain(f->follower, victim, inroom_members, koef);
}

void gain_battle_exp(CHAR_DATA *ch, CHAR_DATA *victim, int dam)
{
	if (ch != victim
		&& OK_GAIN_EXP(ch, victim)
		&& GET_EXP(victim) > 0
		&& !AFF_FLAGGED(victim, AFF_CHARM)
		&& !MOB_FLAGGED(victim, MOB_ANGEL)
		&& !IS_NPC(ch)
		&& !MOB_FLAGGED(victim, MOB_NO_BATTLE_EXP))
	{
		int max_exp = MIN(max_exp_gain_pc(ch), (GET_LEVEL(victim) * GET_MAX_HIT(victim) + 4) /
			(5 * MAX(1, GET_REMORT(ch) - MAX_EXP_COEFFICIENTS_USED - 1)));
		double coeff = MIN(dam, GET_HIT(victim)) / static_cast<double>(GET_MAX_HIT(victim));
		int battle_exp = MAX(1, static_cast<int>(max_exp * coeff));
//		int battle_exp = MAX(1, (GET_LEVEL(victim) * MIN(dam, GET_HIT(victim)) + 4) /
//						 (5 * MAX(1, GET_REMORT(ch) - MAX_EXP_COEFFICIENTS_USED - 1)));
		gain_exp(ch, battle_exp);
		ch->dps_add_exp(battle_exp, true);
	}
}

// * Alterate equipment
void alterate_object(OBJ_DATA * obj, int dam, int chance)
{
	if (!obj)
		return;
	dam = number(0, dam * (material_value[GET_OBJ_MATER(obj)] + 30) /
				 MAX(1, GET_OBJ_MAX(obj) *
					 (IS_OBJ_STAT(obj, ITEM_NODROP) ? 5 :
					  IS_OBJ_STAT(obj, ITEM_BLESS) ? 15 : 10) * (GET_OBJ_SKILL(obj) == SKILL_BOWS ? 3 : 1)));

	if (dam > 0 && chance >= number(1, 100))
	{
		if (dam > 1 && obj->worn_by && GET_EQ(obj->worn_by, WEAR_SHIELD) == obj)
		{
			dam /= 2;
		}
		if ((GET_OBJ_CUR(obj) -= dam) <= 0)
		{
			if (obj->worn_by)
				act("$o рассыпал$U, не выдержав повреждений.", FALSE, obj->worn_by, obj, 0, TO_CHAR);
			else if (obj->carried_by)
				act("$o рассыпал$U, не выдержав повреждений.", FALSE, obj->carried_by, obj, 0, TO_CHAR);
			extract_obj(obj);
		}
	}
}

void alt_equip(CHAR_DATA * ch, int pos, int dam, int chance)
{
	// calculate chance if
	if (pos == NOWHERE)
	{
		pos = number(0, 100);
		if (pos < 3)
			pos = WEAR_FINGER_R + number(0, 1);
		else if (pos < 6)
			pos = WEAR_NECK_1 + number(0, 1);
		else if (pos < 20)
			pos = WEAR_BODY;
		else if (pos < 30)
			pos = WEAR_HEAD;
		else if (pos < 45)
			pos = WEAR_LEGS;
		else if (pos < 50)
			pos = WEAR_FEET;
		else if (pos < 58)
			pos = WEAR_HANDS;
		else if (pos < 66)
			pos = WEAR_ARMS;
		else if (pos < 76)
			pos = WEAR_SHIELD;
		else if (pos < 86)
			pos = WEAR_ABOUT;
		else if (pos < 90)
			pos = WEAR_WAIST;
		else if (pos < 94)
			pos = WEAR_WRIST_R + number(0, 1);
		else
			pos = WEAR_HOLD;
	}

	if (pos <= 0 || pos > WEAR_BOTHS || !GET_EQ(ch, pos) || dam < 0 || AFF_FLAGGED(ch, AFF_SHIELD))
		return; // Добавил: под "зб" не убивается стаф (Купала)
	alterate_object(GET_EQ(ch, pos), dam, chance);
}

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
	static char buf[256];
	char *cp = buf;

	for (; *str; str++)
	{
		if (*str == '#')
		{
			switch (*(++str))
			{
			case 'W':
				for (; *weapon_plural; *(cp++) = *(weapon_plural++));
				break;
			case 'w':
				for (; *weapon_singular; *(cp++) = *(weapon_singular++));
				break;
			default:
				*(cp++) = '#';
				break;
			}
		}
		else
			*(cp++) = *str;

		*cp = 0;
	}			// For

	return (buf);
}

bool check_valid_chars(CHAR_DATA *ch, CHAR_DATA *victim, const char *fname, int line)
{
	if (!ch || ch->purged() || !victim || victim->purged())
	{
		log("SYSERROR: ch = %s, victim = %s (%s:%d)",
			ch ? (ch->purged() ? "purged" : "true") : "false",
			victim ? (victim->purged() ? "purged" : "true") : "false",
			fname, line);
		return false;
	}
	return true;
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim  died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */

void char_dam_message(int dam, CHAR_DATA * ch, CHAR_DATA * victim, bool noflee)
{
	if (IN_ROOM(ch) == NOWHERE)
		return;

	switch (GET_POS(victim))
	{
	case POS_MORTALLYW:
		if (IS_POLY(victim))
			act("$n смертельно ранены и умрут, если им не помогут.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		else
			act("$n смертельно ранен$a и умрет, если $m не помогут.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		send_to_char("Вы смертельно ранены и умрете, если вам не помогут.\r\n", victim);
		break;
	case POS_INCAP:
		if (IS_POLY(victim))
			act("$n без сознания и медленно умирают. Помогите же им.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		else
			act("$n без сознания и медленно умирает. Помогите же $m.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		send_to_char("Вы без сознания и медленно умираете, брошенные без помощи.\r\n", victim);
		break;
	case POS_STUNNED:
		if (IS_POLY(victim))
			act("$n без сознания, но возможно они еще повоюют (попозже :).", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		else
			act("$n без сознания, но возможно $e еще повоюет (попозже :).", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		send_to_char("Сознание покинуло вас. В битве от вас пока проку мало.\r\n", victim);
		break;
	case POS_DEAD:
		if (IS_NPC(victim) && (MOB_FLAGGED(victim, MOB_CORPSE)))
		{
			act("$n вспыхнул$g и рассыпал$u в прах.", FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			send_to_char("Похоже вас убили и даже тела не оставили!\r\n", victim);
		}
		else
		{
			if (IS_POLY(victim))
				act("$n мертвы, их души медленно подымаются в небеса.", FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			else
				act("$n мертв$a, $s душа медленно подымается в небеса.", FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			send_to_char("Вы мертвы! Нам очень жаль...\r\n", victim);
		}
		break;
	default:		// >= POSITION SLEEPING
		if (dam > (GET_REAL_MAX_HIT(victim) / 4))
			send_to_char("Это действительно БОЛЬНО!\r\n", victim);

		if (dam > 0 && GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4))
		{
			sprintf(buf2,
					"%s Вы желаете, чтобы ваши раны не кровоточили так сильно! %s\r\n",
					CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
			send_to_char(buf2, victim);
		}
		if (ch != victim &&
				IS_NPC(victim) &&
				GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4) &&
				MOB_FLAGGED(victim, MOB_WIMPY) && !noflee && GET_POS(victim) > POS_SITTING)
			do_flee(victim, NULL, 0, 0);

		if (ch != victim &&
				!IS_NPC(victim) &&
				HERE(victim) &&
				GET_WIMP_LEV(victim) &&
				GET_HIT(victim) < GET_WIMP_LEV(victim) && !noflee && GET_POS(victim) > POS_SITTING)
		{
			send_to_char("Вы запаниковали и попытались убежать!\r\n", victim);
			do_flee(victim, NULL, 0, 0);
		}
		break;
	}
}

void test_self_hitroll(CHAR_DATA *ch)
{
	HitData hit;
	hit.weapon = RIGHT_WEAPON;
	hit.init(ch, ch);
	hit.calc_base_hr(ch);
	hit.calc_stat_hr(ch);
	hit.calc_ac(ch);

	HitData hit2;
	hit2.weapon = LEFT_WEAPON;
	hit2.init(ch, ch);
	hit2.calc_base_hr(ch);
	hit2.calc_stat_hr(ch);

	send_to_char(ch, "RIGHT_WEAPON: hitroll=%d, LEFT_WEAPON: hitroll=%d, AC=%d\r\n",
		hit.calc_thaco * -1, hit2.calc_thaco * -1, hit.victim_ac);
}

/**
 * Разный инит щитов у мобов и чаров.
 * У мобов работают все 3 щита, у чаров только 1 рандомный на текущий удар.
 */
void Damage::post_init_shields(CHAR_DATA *victim)
{
	if (IS_NPC(victim) && !IS_CHARMICE(victim))
	{
		if (AFF_FLAGGED(victim, AFF_FIRESHIELD))
			flags.set(VICTIM_FIRE_SHIELD);
		if (AFF_FLAGGED(victim, AFF_ICESHIELD))
			flags.set(VICTIM_ICE_SHIELD);
		if (AFF_FLAGGED(victim, AFF_AIRSHIELD))
			flags.set(VICTIM_AIR_SHIELD);
	}
	else
	{
		enum { FIRESHIELD, ICESHIELD, AIRSHIELD };
		std::vector<int> shields;

		if (AFF_FLAGGED(victim, AFF_FIRESHIELD))
			shields.push_back(FIRESHIELD);
		if (AFF_FLAGGED(victim, AFF_AIRSHIELD))
			shields.push_back(AIRSHIELD);
		if (AFF_FLAGGED(victim, AFF_ICESHIELD))
			shields.push_back(ICESHIELD);

		if (shields.empty())
			return;

		int shield_num = number(0, shields.size() - 1);

		if (shields[shield_num] == FIRESHIELD)
		{
			flags.set(VICTIM_FIRE_SHIELD);
		}
		else if (shields[shield_num] == AIRSHIELD)
		{
			flags.set(VICTIM_AIR_SHIELD);
		}
		else if (shields[shield_num] == ICESHIELD)
		{
			flags.set(VICTIM_ICE_SHIELD);
		}
	}
}

void Damage::post_init(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (msg_num == -1)
	{
		if (skill_num >= 0)
		{
			msg_num = skill_num + TYPE_HIT;
		}
		else if (spell_num >= 0)
		{
			msg_num = spell_num;
		}
		else if (hit_type >= 0)
		{
			msg_num = hit_type + TYPE_HIT;
		}
		else
		{
			msg_num = TYPE_HIT;
		}
	}

	if (ch_start_pos == -1)
	{
		ch_start_pos = GET_POS(ch);
	}
	if (victim_start_pos == -1)
	{
		victim_start_pos = GET_POS(victim);
	}

	post_init_shields(victim);
}

void Damage::zero_init()
{
	dam = 0;
	dam_critic = 0;
	fs_damage = 0;
	dmg_type = -1;
	skill_num = -1;
	spell_num = -1;
	hit_type = -1;
	msg_num = -1;
	ch_start_pos = -1;
	victim_start_pos = -1;
};

/*
void solo_gain(CHAR_DATA * ch, CHAR_DATA * victim)
{
  int exp;

  if (IS_NPC(ch) || !OK_GAIN_EXP(ch, victim))
     {send_to_char("Ваше деяние никто не оценил.\r\n",ch);
      return;
     }

  if (IS_NPC(ch))
     {exp  = MIN(max_exp_gain_npc, GET_EXP(victim));
      exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
     }
  else
     {exp = get_extend_exp(GET_EXP(victim), ch, victim);
      exp = MIN(max_exp_gain_pc(ch), exp);
     };

  if (!IS_NPC(ch))
     exp = MIN(max_exp_gain_pc(ch),exp);
  exp = MAX(1,exp);

  if (exp > 1)
     {sprintf(buf2, "Ваш опыт повысился на %d %s.\r\n", exp, desc_count(exp, WHAT_POINT));
      send_to_char(buf2, ch);
     }
  else
    send_to_char("Ваш опыт повысился всего лишь на маленькую единичку.\r\n", ch);

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}
*/
