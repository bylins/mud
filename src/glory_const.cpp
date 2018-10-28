// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include "glory_const.hpp"

#include "logger.hpp"
#include "utils.h"
#include "pugixml.hpp"
#include "structs.h"
#include "screen.h"
#include "char.hpp"
#include "comm.h"
#include "db.h"
#include "genchar.h"
#include "handler.h"
#include "char_player.hpp"
#include "glory_misc.hpp"


#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <list>
#include <map>
#include <string>
#include <iomanip>
#include <vector>

extern void add_karma(CHAR_DATA * ch, const char * punish , const char * reason);
extern void check_max_hp(CHAR_DATA *ch);

namespace GloryConst
{

enum
{
	GLORY_STR = 0, // +статы G_STR..G_CHA
	GLORY_DEX,
	GLORY_INT,
	GLORY_WIS,
	GLORY_CON,
	GLORY_CHA, // -//-
	GLORY_HIT, // +хп
	GLORY_SUCCESS, //каст
	GLORY_WILL, //воля
	GLORY_STABILITY, //стойкость
	GLORY_REFLEX, //реакция
	GLORY_MIND, //разум
	GLORY_TOTAL
};

struct glory_node
{
	glory_node() : free_glory(0) {};
	// свободная слава
	int free_glory;
	// имя чара для топа прославленных
	std::string name;
	// список статов с прокинутой славой
	std::map<int /* номер стат из enum */, int /* сколько этого стата вложено*/> stats;
};

// общий список свободной и вложенной славы
typedef std::shared_ptr<glory_node> GloryNodePtr;
typedef std::map<long /* уид чара */, GloryNodePtr> GloryListType;
GloryListType glory_list;
// суммарное списанное в виде комиса кол-во славы
int total_charge = 0;
// потраченное в магазинах
int total_spent = 0;

struct glory_olc
{
	glory_olc() : olc_free_glory(0), olc_was_free_glory(0)
	{
		for (int i = 0; i < GLORY_TOTAL; ++i)
		{
			stat_cur[i] = 0;
			stat_add[i] = 0;
			stat_was[i] = 0;
		}
	};

	std::array<int, GLORY_TOTAL> stat_cur;
	std::array<int, GLORY_TOTAL> stat_add;
	std::array<int, GLORY_TOTAL> stat_was;

	int olc_free_glory;
	int olc_was_free_glory;
};

const char *olc_stat_name[] =
{
	"Сила",
	"Ловкость",
	"Ум",
	"Мудрость",
	"Телосложение",
	"Обаяние",
	"Макс.жизнь",
	"Успех.колдовства",
	"Воля",
	"Стойкость",
	"Реакция",
	"Разум"
};

void transfer_log(const char *format, ...)
{
	const char *filename = "../log/glory_transfer.log";

	FILE *file = fopen(filename, "a");
	if (!file)
	{
		log("SYSERR: can't open %s!", filename);
		return;
	}

	if (!format)
		format = "SYSERR: imm_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");

	fclose(file);
}

// * Аналог бывшего макроса GET_GLORY().
int get_glory(long uid)
{
	int glory = 0;
	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end())
	{
		glory = it->second->free_glory;
	}
	return glory;
}

// * Добавление славы чару, создание новой записи при необходимости, уведомление, если чар онлайн.
void add_glory(long uid, int amount)
{
	if (uid <= 0 || amount <= 0)
	{
		return;
	}

	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end())
	{
		it->second->free_glory += amount;
	}
	else
	{
		GloryNodePtr temp_node(new glory_node);
		temp_node->free_glory = amount;
		glory_list[uid] = temp_node;
	}

	DESCRIPTOR_DATA *d = DescByUID(uid);
	if (d)
	{
		send_to_char(d->character.get(), "%sВы заслужили %d %s постоянной славы!%s\r\n",
			CCGRN(d->character, C_NRM),
			amount, desc_count(amount, WHAT_POINT),
			CCNRM(d->character, C_NRM));
	}
	save();
}

int stat_multi(int stat)
{
	int multi = 1;
	if(stat == GLORY_HIT)
		multi = HP_FACTOR;
	if(stat == GLORY_SUCCESS)
		multi = SUCCESS_FACTOR;
	if(stat >= GLORY_WILL && stat <= GLORY_REFLEX)
		multi = SAVE_FACTOR;
	if(stat == GLORY_MIND)
		multi = RESIST_FACTOR;
	return multi;
}

// * Распечатка 'слава информация'.
void print_glory(CHAR_DATA *ch, GloryListType::iterator &it)
{
	*buf = '\0';
	for (std::map<int, int>::const_iterator i = it->second->stats.begin(), iend = it->second->stats.end(); i != iend; ++i)
	{
		if ((i->first >= 0) && (i->first < (int)sizeof(olc_stat_name))) {
			sprintf(buf+strlen(buf), "%-16s: +%d", olc_stat_name[i->first], i->second * stat_multi(i->first));
			if (stat_multi(i->first) > 1)
				sprintf(buf+strlen(buf), "(%d)", i->second);
			strcat(buf, "\r\n");
		}
		else
		{
			log("Glory: некорректный номер стата %d (uid: %ld)", i->first, it->first);
		}
	}
	sprintf(buf+strlen(buf), "Свободных очков : %d\r\n", it->second->free_glory);
	send_to_char(buf, ch);
	return;
}

// * Показ свободной и вложенной славы у чара (glory имя).
void print_to_god(CHAR_DATA *ch, CHAR_DATA *god)
{
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end())
	{
		send_to_char(god, "У %s совсем не славы.\r\n", GET_PAD(ch, 1));
		return;
	}

	send_to_char(god, "Информация об очках славы %s:\r\n", GET_PAD(ch, 1));
	print_glory(god, it);
	return;
}

int add_stat_cost(int stat, std::shared_ptr<GloryConst::glory_olc> olc)
{
	if (stat < 0 || stat >= GLORY_TOTAL)
	{
		log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
		return 0;
	}

	int glory = (olc->stat_add[stat] * 200) + 1000;
	if (olc->stat_was[stat] - olc->stat_add[stat] > 0)
		glory -= glory * STAT_RETURN_FEE / 100;
	return glory;
}

int remove_stat_cost(int stat, std::shared_ptr<GloryConst::glory_olc> olc)
{
	if (stat < 0 || stat >= GLORY_TOTAL)
	{
		log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
		return 0;
	}
	if (!olc->stat_add[stat])
	{
		return 0;
	}

	int glory = ((olc->stat_add[stat] -1) * 200) + 1000;
	if (olc->stat_was[stat] - olc->stat_add[stat] >= 0)
		glory -= glory * STAT_RETURN_FEE / 100;
	return glory;
}

const char *olc_del_name[] =
{
	"А",
	"Б",
	"Г",
	"Д",
	"Е",
	"Ж",
	"З",
	"И",
	"К",
	"Л",
	"М",
	"Н",
};

const char *olc_add_name[] =
{
	"О",
	"П",
	"Р",
	"С",
	"Т",
	"У",
	"Ф",
	"Х",
	"Ц",
	"Ч",
	"Ш",
	"Щ",
};

std::string olc_print_stat(CHAR_DATA *ch, int stat)
{
	if (stat < 0 || stat >= GLORY_TOTAL)
	{
		log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
		return "";
	}


	return boost::str(boost::format("  %-16s :  %s(+%5d)%s  (%s%s%s) %4d (%s%s%s)  %s(-%5d)  | %d%s\r\n")
		% olc_stat_name[stat]
		% CCINRM(ch, C_NRM)
		% remove_stat_cost(stat, ch->desc->glory_const)
		% CCNRM(ch, C_NRM)
		% CCIGRN(ch, C_NRM) % olc_del_name[stat] % CCNRM(ch, C_NRM)
		% ((ch->desc->glory_const->stat_cur[stat] + ch->desc->glory_const->stat_add[stat])*stat_multi(stat))
		% CCIGRN(ch, C_NRM) % olc_add_name[stat] % CCNRM(ch, C_NRM)
		% CCINRM(ch, C_NRM)
		% add_stat_cost(stat, ch->desc->glory_const)
		% (ch->desc->glory_const->stat_add[stat] > 0 ? std::string("+")
			+ boost::lexical_cast<std::string>(ch->desc->glory_const->stat_add[stat]) : "")
		% CCNRM(ch, C_NRM));
}

// * Распечатка олц меню.
void spend_glory_menu(CHAR_DATA *ch)
{
	std::ostringstream out;
	out << "\r\n                         -      +\r\n";

	out << olc_print_stat(ch, GLORY_STR)
		<< olc_print_stat(ch, GLORY_DEX)
		<< olc_print_stat(ch, GLORY_INT)
		<< olc_print_stat(ch, GLORY_WIS)
		<< olc_print_stat(ch, GLORY_CON)
		<< olc_print_stat(ch, GLORY_CHA)
		<< olc_print_stat(ch, GLORY_HIT)
		<< olc_print_stat(ch, GLORY_SUCCESS)
		<< olc_print_stat(ch, GLORY_WILL)
		<< olc_print_stat(ch, GLORY_STABILITY)
		<< olc_print_stat(ch, GLORY_REFLEX)
		<< olc_print_stat(ch, GLORY_MIND);

	out << "\r\n  Свободной славы: " << ch->desc->glory_const->olc_free_glory << "\r\n\r\n";

	if (ch->desc->glory_const->olc_free_glory != ch->desc->glory_const->olc_was_free_glory)
	{
		out << "  " << CCIGRN(ch, C_SPR) << "В" << CCNRM(ch, C_SPR)
			<< ") Сохранить результаты\r\n";
	}
	out << "  " << CCIGRN(ch, C_SPR) << "Я" << CCNRM(ch, C_SPR)
		<< ") Выйти без сохранения\r\n"
		<< "  Ваш выбор: ";
	send_to_char(out.str(), ch);
}

void olc_del_stat(CHAR_DATA *ch, int stat)
{
	if (stat < 0 || stat >= GLORY_TOTAL)
	{
		log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
		return;
	}
	if (ch->desc->glory_const->stat_add[stat] > 0)
	{
		ch->desc->glory_const->olc_free_glory +=
			remove_stat_cost(stat, ch->desc->glory_const);
		ch->desc->glory_const->stat_add[stat] -= 1;
	}
}

void olc_add_stat(CHAR_DATA *ch, int stat)
{
	int need_glory = add_stat_cost(stat, ch->desc->glory_const);
	bool ok = false;
	switch (stat)
	{
		case GLORY_CON:
		case GLORY_STR:
		case GLORY_DEX:
		case GLORY_INT:
			if (ch->desc->glory_const->olc_free_glory >= need_glory)
				ok = true;
			break;
		case GLORY_WIS:
		case GLORY_CHA:
			if (ch->desc->glory_const->olc_free_glory >= need_glory
				&& ch->desc->glory_const->stat_cur[stat]
					+ ch->desc->glory_const->stat_add[stat] < 50)
				ok = true;
			break;
		case GLORY_HIT:
		case GLORY_SUCCESS:
		case GLORY_WILL:
		case GLORY_STABILITY:
		case GLORY_REFLEX:
			if (ch->desc->glory_const->olc_free_glory >= need_glory)
				ok = true;
			break;
		case GLORY_MIND:
			if (ch->desc->glory_const->olc_free_glory >= need_glory
				&& ch->desc->glory_const->stat_cur[stat]
					+ ch->desc->glory_const->stat_add[stat] < 75)
				ok = true;
			break;
		default:
			log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
	}
	if (ok)
	{
			ch->desc->glory_const->olc_free_glory -= need_glory;
			ch->desc->glory_const->stat_add[stat] += 1;
	}
}

int olc_real_stat(CHAR_DATA *ch, int stat)
{
	return ch->desc->glory_const->stat_cur[stat]
		+ ch->desc->glory_const->stat_add[stat];
}

int calculate_glory_in_stats(GloryListType::const_iterator &i)
{
	int total = 0;
	for (std::map<int, int>::const_iterator k = i->second->stats.begin(),
		kend = i->second->stats.end(); k != kend; ++k)
	{
		for (int m = 0; m < k->second; m++)
			total += m * 200 + 1000;
	}
	return total;
}

bool parse_spend_glory_menu(CHAR_DATA *ch, char *arg)
{
	switch (LOWER(*arg))
	{
		case 'а':
			olc_del_stat(ch, GLORY_STR);
			break;
		case 'б':
			olc_del_stat(ch, GLORY_DEX);
			break;
		case 'г':
			olc_del_stat(ch, GLORY_INT);
			break;
		case 'д':
			olc_del_stat(ch, GLORY_WIS);
			break;
		case 'е':
			olc_del_stat(ch, GLORY_CON);
			break;
		case 'ж':
			olc_del_stat(ch, GLORY_CHA);
			break;
		case 'з':
			olc_del_stat(ch, GLORY_HIT);
			break;
		case 'и':
			olc_del_stat(ch, GLORY_SUCCESS);
			break;
		case 'к':
			olc_del_stat(ch, GLORY_WILL);
			break;
		case 'л':
			olc_del_stat(ch, GLORY_STABILITY);
			break;
		case 'м':
			olc_del_stat(ch, GLORY_REFLEX);
			break;
		case 'н':
			olc_del_stat(ch, GLORY_MIND);
			break;
		case 'о':
			olc_add_stat(ch, GLORY_STR);
			break;
		case 'п':
			olc_add_stat(ch, GLORY_DEX);
			break;
		case 'р':
			olc_add_stat(ch, GLORY_INT);
			break;
		case 'с':
			olc_add_stat(ch, GLORY_WIS);
			break;
		case 'т':
			olc_add_stat(ch, GLORY_CON);
			break;
		case 'у':
			olc_add_stat(ch, GLORY_CHA);
			break;
		case 'ф':
			olc_add_stat(ch, GLORY_HIT);
			break;
		case 'х':
			olc_add_stat(ch, GLORY_SUCCESS);
			break;
		case 'ц':
			olc_add_stat(ch, GLORY_WILL);
			break;
		case 'ч':
			olc_add_stat(ch, GLORY_STABILITY);
			break;
		case 'ш':
			olc_add_stat(ch, GLORY_REFLEX);
			break;
		case 'щ':
			olc_add_stat(ch, GLORY_MIND);
			break;
		case 'в':
		{
			// получившиеся статы
			ch->set_str(olc_real_stat(ch, GLORY_STR));
			ch->set_dex(olc_real_stat(ch, GLORY_DEX));
			ch->set_int(olc_real_stat(ch, GLORY_INT));
			ch->set_wis(olc_real_stat(ch, GLORY_WIS));
			ch->set_con(olc_real_stat(ch, GLORY_CON));
			ch->set_cha(olc_real_stat(ch, GLORY_CHA));
			// обновление записи в списке славы
			GloryListType::const_iterator it = glory_list.find(GET_UNIQUE(ch));
			if (glory_list.end() == it)
			{
				log("SYSERROR : нет записи чара при выходе из олц постоянной славы name=%s (%s:%d)",
					ch->get_name().c_str(), __FILE__, __LINE__);
				send_to_char("Ошибка сохранения, сообщите Богам!\r\n", ch);
				ch->desc->glory_const.reset();
				STATE(ch->desc) = CON_PLAYING;
				return 1;
			}
			// слава перед редактированием (для расчета комиса)
			int was_glory = it->second->free_glory + calculate_glory_in_stats(it);
			// обновление вложенных статов
			it->second->free_glory = ch->desc->glory_const->olc_free_glory;
			it->second->stats.clear();
			for (int i = 0; i < GLORY_TOTAL; ++i)
			{
				if (ch->desc->glory_const->stat_add[i] > 0)
				{
					it->second->stats[i] = ch->desc->glory_const->stat_add[i];
				}
			}
			// расчет снятого комиса
			int now_glory = it->second->free_glory + calculate_glory_in_stats(it);
			if (was_glory < now_glory)
			{
				log("SYSERROR : прибавка постоянной славы после редактирования в олц (%d -> %d) name=%s (%s:%d)",
					was_glory, now_glory, ch->get_name().c_str(), __FILE__, __LINE__);
			}
			else
			{
				total_charge += was_glory - now_glory;
			}
			// выход из олц с обновлением хп
			ch->desc->glory_const.reset();
			STATE(ch->desc) = CON_PLAYING;
			check_max_hp(ch);
			send_to_char("Ваши изменения сохранены.\r\n", ch);
			ch->save_char();
			save();
			return 1;
		}
		case 'я':
			ch->desc->glory_const.reset();
			STATE(ch->desc) = CON_PLAYING;
			send_to_char("Редактирование прервано.\r\n", ch);
			return 1;
		default:
			break;
	}
	return 0;
}

const char *GLORY_CONST_FORMAT =
	"Формат: слава2\r\n"
	"        слава2 информация\r\n"
	"        слава2 перевести <имя> <кол-во>\r\n";

void do_spend_glory(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	// До исправления баги
	//send_to_char("Временно отключено...\r\n", ch);
	//return;
	if (glory_list.end() == it || IS_IMMORTAL(ch))
	{
		send_to_char("Вам это не нужно...\r\n", ch);
		return;
	}

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	if (buffer2.empty())
	{
		if (it->second->free_glory < 1000 && it->second->stats.empty())
		{
			send_to_char("У вас недостаточно очков славы для использования этой команды.\r\n", ch);
			return;
		}

		std::shared_ptr<glory_olc> tmp_glory_olc(new glory_olc);
		tmp_glory_olc->stat_cur[GLORY_STR] = ch->get_inborn_str();
		tmp_glory_olc->stat_cur[GLORY_DEX] = ch->get_inborn_dex();
		tmp_glory_olc->stat_cur[GLORY_INT] = ch->get_inborn_int();
		tmp_glory_olc->stat_cur[GLORY_WIS] = ch->get_inborn_wis();
		tmp_glory_olc->stat_cur[GLORY_CON] = ch->get_inborn_con();
		tmp_glory_olc->stat_cur[GLORY_CHA] = ch->get_inborn_cha();
		tmp_glory_olc->stat_cur[GLORY_HIT] = it->second->stats[GLORY_HIT];
		tmp_glory_olc->stat_cur[GLORY_SUCCESS] = it->second->stats[GLORY_SUCCESS];
		tmp_glory_olc->stat_cur[GLORY_WILL] = it->second->stats[GLORY_WILL];
		tmp_glory_olc->stat_cur[GLORY_STABILITY] = it->second->stats[GLORY_STABILITY];
		tmp_glory_olc->stat_cur[GLORY_REFLEX] = it->second->stats[GLORY_REFLEX];
		tmp_glory_olc->stat_cur[GLORY_MIND] = it->second->stats[GLORY_MIND];

		for (std::map<int, int>::const_iterator i = it->second->stats.begin(), iend = it->second->stats.end(); i != iend; ++i)
		{
			if (i->first < GLORY_TOTAL && i->first >= 0)
			{
				tmp_glory_olc->stat_cur[i->first] -= i->second;
				tmp_glory_olc->stat_add[i->first] = tmp_glory_olc->stat_was[i->first] = i->second;
			}
			else
			{
				log("Glory: некорректный номер стата %d (uid: %ld)", i->first, it->first);
			}
		}
		tmp_glory_olc->olc_free_glory = tmp_glory_olc->olc_was_free_glory = it->second->free_glory;

		ch->desc->glory_const = tmp_glory_olc;
		STATE(ch->desc) = CON_GLORY_CONST;
		spend_glory_menu(ch);
	}
	else if (CompareParam(buffer2, "информация"))
	{
		send_to_char("Информация о вложенных вами очках славы:\r\n", ch);
		print_glory(ch, it);
	}
	else if (CompareParam(buffer2, "перевести"))
	{
		if (it->second->free_glory < MIN_TRANSFER_AMOUNT)
		{
			send_to_char(ch,
				"У вас недостаточно свободных очков постоянной славы (минимум %d).\r\n",
				MIN_TRANSFER_AMOUNT);
			return;
		}

		std::string name;
		GetOneParam(buffer, name);
		// buffer = кол-во
		boost::trim(buffer);

		Player p_vict;
		CHAR_DATA *vict = &p_vict;
		if (load_char(name.c_str(), vict) < 0)
		{
			send_to_char(ch, "%s - некорректное имя персонажа.\r\n", name.c_str());
			return;
		}
		if (str_cmp(GET_EMAIL(ch), GET_EMAIL(vict)))
		{
			send_to_char(ch, "Персонажи имеют разные email адреса.\r\n");
			return;
		}

		int amount = 0;
		try
		{
			amount = std::stoi(buffer);
		}
		catch(...)
		{
			send_to_char(ch, "%s - некорректное количество для перевода.\r\n", buffer.c_str());
			send_to_char(GLORY_CONST_FORMAT, ch);
			return;
		}

		if (amount < MIN_TRANSFER_AMOUNT || amount > it->second->free_glory)
		{
			send_to_char(ch,
				"%d - некорректное количество для перевода.\r\n"
				"Вы можете перевести от %d до %d постоянной славы.\r\n",
				amount, MIN_TRANSFER_AMOUNT, it->second->free_glory);
			return;
		}

		int tax = MAX(MIN_TRANSFER_TAX, amount / 100 * TRANSFER_FEE);
		int total_amount = amount - tax;

		remove_glory(GET_UNIQUE(ch), amount);
		add_glory(GET_UNIQUE(vict), total_amount);

		snprintf(buf, MAX_STRING_LENGTH,
			"Transfer %d const glory from %s", total_amount, GET_NAME(ch));
		add_karma(vict, buf, "командой");

		snprintf(buf, MAX_STRING_LENGTH, "Transfer %d const glory to %s", amount, GET_NAME(vict));
		add_karma(ch, buf, "командой");

		total_charge += tax;
		transfer_log("%s -> %s transfered %d (%d tax)", GET_NAME(ch), GET_NAME(vict), total_amount, tax);

		ch->save_char();
		vict->save_char();
		save();

		send_to_char(ch, "%s переведено %d постоянной славы (%d комиссии).\r\n",
			GET_PAD(vict, 2), total_amount, tax);

		// TODO: ну если в глори-лог или карму, то надо стоимость/налог
		// на трансфер ставить, чтобы не заспамили.
		// а без отдельных логов потом фик поймешь откуда на чаре слава
	}
	else
	{
		send_to_char(GLORY_CONST_FORMAT, ch);
	}
}

/**
* Удаление славы у чара (свободной), ниже нуля быть не может.
* \return 0 - ничего не списано, число > 0 - сколько реально списали
*/
int remove_glory(long uid, int amount)
{
	if (uid <= 0 || amount <= 0)
	{
		return 0;
	}

	int real_removed = amount;

	GloryListType::iterator i = glory_list.find(uid);
	if (glory_list.end() != i)
	{
		// ниже нуля ес-сно не отнимаем
		if (i->second->free_glory >= amount)
		{
			i->second->free_glory -= amount;
		}
		else
		{
			real_removed = i->second->free_glory;
			i->second->free_glory = 0;
		}
		// смысла пустую запись оставлять до ребута нет
		if (!i->second->free_glory && i->second->stats.empty())
		{
			glory_list.erase(i);
		}
		save();
	}
	else
	{
		real_removed = 0;
	}

	return real_removed;
}

bool reset_glory(CHAR_DATA *ch)
{
	GloryListType::iterator i = glory_list.find(GET_UNIQUE(ch));
	if (glory_list.end() != i)
	{
		glory_list.erase(i);
		save();
		check_max_hp(ch);
		GloryMisc::recalculate_stats(ch);
		ch->save_char();
		return true;
	}
	return false;
}

void do_glory(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (!*argument)
	{
		send_to_char("Формат команды : \r\n"
			"   glory <имя> (информация по указанному персонажу)\r\n"
			"   glory <имя> +|-<кол-во славы> причина\r\n"
			"   glory <имя> reset причина (обнуление свободной и вложенной славы)\r\n", ch);
		return;
	}

	enum {SHOW_GLORY, ADD_GLORY, SUB_GLORY, RESET_GLORY};

	char num[MAX_INPUT_LENGTH];
	int mode = 0;

	char *reason = two_arguments(argument, arg, num);
	skip_spaces(&reason);

	if (!*num)
	{
		mode = SHOW_GLORY;
	}
	else if (*num == '+')
	{
		mode = ADD_GLORY;
	}
	else if (*num == '-')
	{
		mode = SUB_GLORY;
	}
	else if (is_abbrev(num, "reset"))
	{
		mode = RESET_GLORY;
	}
	// точки убираем, чтобы карма всегда писалась
	skip_dots(&reason);

	if (mode != SHOW_GLORY && (!reason || !*reason))
	{
		send_to_char("Укажите причину изменения славы?\r\n", ch);
		return;
	}

	CHAR_DATA *vict = get_player_vis(ch, arg, FIND_CHAR_WORLD);
	if (vict && vict->desc && STATE(vict->desc) == CON_GLORY_CONST)
	{
		send_to_char("Персонаж в данный момент редактирует свою славу.\r\n", ch);
		return;
	}
	Player t_vict; // TODO: мутно
	if (!vict)
	{
		if (load_char(arg, &t_vict) < 0)
		{
			send_to_char("Такого персонажа не существует.\r\n", ch);
			return;
		}
		vict = &t_vict;
	}

	switch (mode)
	{
		case ADD_GLORY:
		{
			int amount = atoi((num + 1));
			add_glory(GET_UNIQUE(vict), amount);
			send_to_char(ch, "%s добавлено %d у.е. постоянной славы (Всего: %d у.е.).\r\n",
				GET_PAD(vict, 2), amount, get_glory(GET_UNIQUE(vict)));
			// запись в карму, логи
			sprintf(buf, "(GC) %s sets +%d const glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
			mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);
			sprintf(buf, "Change const glory +%d by %s", amount, GET_NAME(ch));
			add_karma(vict, buf, reason);
			GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
			break;
		}
		case SUB_GLORY:
		{
			int amount = remove_glory(GET_UNIQUE(vict), atoi((num + 1)));
			if (amount <= 0)
			{
				send_to_char(ch, "У %s нет свободной постоянной славы.\r\n", GET_PAD(vict, 1));
				break;
			}
			send_to_char(ch, "У %s вычтено %d у.е. постоянной славы (Всего: %d у.е.).\r\n",
				GET_PAD(vict, 1), amount, get_glory(GET_UNIQUE(vict)));
			// запись в карму, логи
			sprintf(buf, "(GC) %s sets -%d const glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
			mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);
			sprintf(buf, "Change const glory -%d by %s", amount, GET_NAME(ch));
			add_karma(vict, buf, reason);
			GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
			break;
		}
		case RESET_GLORY:
		{
			if (reset_glory(vict))
			{
				send_to_char(ch, "%s - очищена запись постоянной славы.\r\n", vict->get_name().c_str());
				// запись в карму, логи
				sprintf(buf, "(GC) %s reset const glory to %s.", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
				imm_log("%s", buf);
				sprintf(buf, "Reset stats and const glory by %s", GET_NAME(ch));
				add_karma(vict, buf, reason);
				GloryMisc::add_log(mode, 0, std::string(buf), std::string(reason), vict);
			}
			else
			{
				send_to_char(ch, "%s - запись постоянной славы и так пустая.\r\n", vict->get_name().c_str());
			}
			break;
		}
		default:
			GloryConst::print_to_god(vict, ch);
	}
	vict->save_char();
}

void save()
{
	pugi::xml_document doc;
	doc.append_attribute("encoding") = "koi8-r";
	doc.append_child().set_name("glory_list");
	pugi::xml_node char_list = doc.child("glory_list");

	char_list.append_attribute("version") = cur_ver;
	for (GloryListType::const_iterator i = glory_list.begin(), iend = glory_list.end(); i != iend; ++i)
	{
		pugi::xml_node char_node = char_list.append_child();
		char_node.set_name("char");
		char_node.append_attribute("uid") = (int)i->first;
		char_node.append_attribute("glory") = i->second->free_glory;

		for (std::map<int, int>::const_iterator k = i->second->stats.begin(),
			kend = i->second->stats.end(); k != kend; ++k)
		{
			if (k->second > 0)
			{
				pugi::xml_node stat = char_node.append_child();
				stat.set_name("stat");
				stat.append_attribute("num") = k->first;
				stat.append_attribute("amount") = k->second;
			}
		}
		if ((char_node.begin() == char_node.end()) && (i->second->free_glory == 0))
		{
			char_list.remove_child(char_node);
		}
	}

	pugi::xml_node charge_node = char_list.append_child();
	charge_node.set_name("total_charge");
	charge_node.append_attribute("amount") = total_charge;

	pugi::xml_node spent_node = char_list.append_child();
	spent_node.set_name("total_spent");
	spent_node.append_attribute("amount") = total_spent;

	doc.save_file(LIB_PLRSTUFF"glory_const.xml");
}

void load()
{
	int ver=0;
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_PLRSTUFF"glory_const.xml");
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "SYSERR: error reading glory_const.xml: %s", result.description());
		perror(buf);
		return;
	}
	pugi::xml_node char_list = doc.child("glory_list");
	if(char_list.attribute("version")) {
		ver = std::stoi(char_list.attribute("version").value(), nullptr, 10);
		if (ver > cur_ver)
		{
			snprintf(buf, MAX_STRING_LENGTH, "SYSERR: error reading glory_const.xml: unsupported version: %d, current version: %d", ver, cur_ver);
			perror(buf);
			return;
		}
	}
	for (pugi::xml_node node = char_list.child("char"); node; node = node.next_sibling("char"))
	{
		const auto uid_str = node.attribute("uid").value();
		long uid = 0;
		try
		{
			uid = std::stol(uid_str, nullptr, 10);
		}
		catch (const std::invalid_argument&)
		{
			log("SYSERR: UID [%s] не является верным десятичным числом.", uid_str);
			continue;
		}

		std::string name = GetNameByUnique(uid);
		if (name.empty())
		{
			log("GloryConst: UID %ld - персонажа не существует.", uid);
			continue;
		}

		if (glory_list.find(uid) != glory_list.end())
		{
			log("SYSERROR : дубликат записи uid=%ld, name=%s (%s:%d)",
				uid, name.c_str(), __FILE__, __LINE__);
			continue;
		}

		GloryNodePtr tmp_node(new glory_node);

		long free_glory = std::stoi(node.attribute("glory").value(), nullptr, 10);
		tmp_node->free_glory = free_glory;

		for (pugi::xml_node stat = node.child("stat"); stat; stat = stat.next_sibling("stat"))
		{
			int divider = 1;
			int stat_num = std::stoi(stat.attribute("num").value(), nullptr, 10);
			if (ver == 0)
			{
				if(stat_num == GLORY_HIT)
					divider = 50;
				if(stat_num == GLORY_SUCCESS)
					divider = 10;
				if(stat_num >= GLORY_WILL && stat_num <= GLORY_REFLEX)
					divider = 15;
				if(stat_num == GLORY_MIND)
					divider = 7;
			}
			int stat_amount = std::stoi(stat.attribute("amount").value(), nullptr, 10)/divider;
			if (stat_num >= GLORY_TOTAL && stat_num < 0)
			{
				log("SYSERROR : невалидный номер влитого стата num=%d, name=%s (%s:%d)",
					stat_num, name.c_str(), __FILE__, __LINE__);
				continue;
			}
			if (tmp_node->stats.find(stat_num) != tmp_node->stats.end())
			{
				log("SYSERROR : дубликат влитого стата num=%d, name=%s (%s:%d)",
					stat_num, name.c_str(), __FILE__, __LINE__);
				continue;
			}
			tmp_node->stats[stat_num] = stat_amount;
		}
		glory_list[uid] = tmp_node;
    }
    pugi::xml_node charge_node = char_list.child("total_charge");
    if (charge_node)
    {
		total_charge = std::stoi(charge_node.attribute("amount").value(), nullptr, 10);
    }
    pugi::xml_node spent_node = char_list.child("total_spent");
    if (spent_node)
    {
		total_spent = std::stoi(spent_node.attribute("amount").value(), nullptr, 10);
    }
    if (ver < cur_ver)//автоматом обновляем xml
    	save();
}

void set_stats(CHAR_DATA *ch)
{
	GloryListType::iterator i = glory_list.find(GET_UNIQUE(ch));
	if (glory_list.end() == i)
	{
		return;
	}

	for (std::map<int, int>::const_iterator k = i->second->stats.begin(),
		kend = i->second->stats.end(); k != kend; ++k)
	{
		switch(k->first)
		{
			case G_STR:
				ch->inc_str(k->second);
				break;
			case G_DEX:
				ch->inc_dex(k->second);
				break;
			case G_INT:
				ch->inc_int(k->second);
				break;
			case G_WIS:
				ch->inc_wis(k->second);
				break;
			case G_CON:
				ch->inc_con(k->second);
				break;
			case G_CHA:
				ch->inc_cha(k->second);
				break;
			default:
				log("Glory: некорректный номер стата %d (uid: %d)", k->first, GET_UNIQUE(ch));
		}
	}
}

// * Количество вложенных статов (только из числа 6 основных).
int main_stats_count(CHAR_DATA *ch)
{
	GloryListType::iterator i = glory_list.find(GET_UNIQUE(ch));
	if (glory_list.end() == i)
	{
		return 0;
	}

	int count = 0;
	for (std::map<int, int>::const_iterator k = i->second->stats.begin(),
		kend = i->second->stats.end(); k != kend; ++k)
	{
		switch(k->first)
		{
			case G_STR:
			case G_DEX:
			case G_INT:
			case G_WIS:
			case G_CON:
			case G_CHA:
				count += k->second;
				break;
		}
	}
	return count;
}

// * Вывод инфы в show stats.
void show_stats(CHAR_DATA *ch)
{
	int free_glory = 0, spend_glory = 0;
	for (GloryListType::const_iterator i = glory_list.begin(), iend = glory_list.end();  i != iend; ++i)
	{
		free_glory += i->second->free_glory;
		spend_glory += calculate_glory_in_stats(i);
	}
	send_to_char(ch,
		"  Слава2: вложено %d, свободно %d, всего %d, комиссии %d\r\n"
		"  Всего потрачено славы в магазинах: %d\r\n",
		spend_glory, free_glory, free_glory + spend_glory, total_charge, total_spent);
}

void add_total_spent(int amount)
{
	if (amount > 0)
	{
		total_spent += amount;
	}
}

void apply_modifiers(CHAR_DATA *ch)
{
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it==glory_list.end())
		return;

	for (std::map<int, int>::const_iterator i = it->second->stats.begin(); i != it->second->stats.end(); ++i)
	{
		int location = 0;
		bool add = true;
		switch (i->first)
		{
			case GLORY_HIT:
				location = APPLY_HIT_GLORY;
				break;
			case GLORY_SUCCESS:
				location = APPLY_CAST_SUCCESS;
				break;
			case GLORY_WILL:
				location = APPLY_SAVING_WILL;
				add = false;
				break;
			case GLORY_STABILITY:
				location = APPLY_SAVING_STABILITY;
				add = false;
				break;
			case GLORY_REFLEX:
				location = APPLY_SAVING_REFLEX;
				add = false;
				break;
			case GLORY_MIND:
				location = APPLY_RESIST_MIND;
				break;
			default:
				break;
		}
		// TODO: убрать наверно надо эти костыли блин, но аппли у нас не позволяет навесить что либо больше чем влазит в signed byte т.е. +127
// не совсем понял что тут делается, но тупо увеличил аппли до инта, кто мешал это сделать раньше? А то 10 лет багрепортят что аффекты после 127 глючат.
		if (location)
		{
			const int K = location != APPLY_HIT_GLORY ? stat_multi(i->first) : 1;
			affect_modify(ch, location, i->second * K, static_cast<EAffectFlag>(0), add);
		}
	}
}

} // namespace GloryConst

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
