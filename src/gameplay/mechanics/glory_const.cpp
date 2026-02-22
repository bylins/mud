// $RCSfile$     $Date$     $Re
// vision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include "glory_const.h"

#include <list>
#include <map>
#include <string>
#include <iomanip>
#include <vector>

#include <third_party_libs/fmt/include/fmt/format.h>

#include "administration/karma.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "third_party_libs/pugixml/pugixml.h"
#include "engine/structs/structs.h"
#include "engine/ui/color.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/db.h"
#include "gameplay/core/genchar.h"
#include "engine/core/handler.h"
#include "engine/entities/char_player.h"
#include "glory_misc.h"
#include "gameplay/statistics/top.h"
#include "engine/db/global_objects.h"

extern void check_max_hp(CharData *ch);

namespace GloryConst {

enum {
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
	GLORY_MANAREG,
	GLORY_BONUSPSYS,
	GLORY_BONUSMAG,
	GLORY_TOTAL
};

struct glory_node {
	glory_node() : free_glory(0) {};
	// свободная слава
	int free_glory;
	// имя чара для топа прославленных
	//std::string name;
	long uid;
	int tmp_spent_glory;
	bool hide;
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

struct glory_olc {
	glory_olc() : olc_free_glory(0), olc_was_free_glory(0) {
		for (int i = 0; i < GLORY_TOTAL; ++i) {
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
		"Разум",
		"Запоминание",
		"Физ. урон %",
		"Сила заклинаний %"
	};

void glory_hide(CharData *ch,
				bool mode) {  //Говнокод от Стрибога не понимаю как это работает но факт, может ктонить срефакторит
	std::list<GloryNodePtr> playerGloryList;
	for (GloryListType::const_iterator it = glory_list.begin(); it != glory_list.end(); ++it) {
		playerGloryList.insert(playerGloryList.end(), it->second);
	}
	for (std::list<GloryNodePtr>::const_iterator t_it = playerGloryList.begin(); t_it != playerGloryList.end();
		 ++t_it) {
		if (ch->get_uid() == t_it->get()->uid) {
			if (mode == true) {
				sprintf(buf, "Проставляю hide славы для %s", GET_NAME(ch));
				mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
			} else {
				sprintf(buf, "Убираю hide славы для %s", GET_NAME(ch));
				mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
			}
			t_it->get()->hide = mode;
		}
	}
}

void transfer_log(const char *format, ...) {
	const auto filename = runtime_config.log_dir() + "/glory_transfer.log";

	FILE *file = fopen(filename.c_str(), "a");
	if (!file) {
		log("SYSERR: can't open %s!", filename.c_str());
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
int get_glory(long uid) {
	int glory = 0;
	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end()) {
		glory = it->second->free_glory;
	}
	return glory;
}

// * Добавление славы чару, создание новой записи при необходимости, уведомление, если чар онлайн.
void add_glory(long uid, int amount) {
	if (uid <= 0 || amount <= 0) {
		return;
	}

	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end()) {
		it->second->free_glory += amount;
	} else {
		GloryNodePtr temp_node(new glory_node);
		temp_node->free_glory = amount;
		temp_node->hide = false;
		glory_list[uid] = temp_node;
	}

	DescriptorData *d = DescriptorByUid(uid);
	if (d) {
		SendMsgToChar(d->character.get(), "%sВы заслужили %d %s постоянной славы!%s\r\n",
					  kColorGrn,
					  amount, GetDeclensionInNumber(amount, EWhat::kPoint),
					  kColorNrm);
	}
	save();
}

int stat_multi(int stat) {
	int multi = 1;
	if (stat == GLORY_HIT)
		multi = HP_FACTOR;
	if (stat == GLORY_SUCCESS)
		multi = SUCCESS_FACTOR;
	if (stat >= GLORY_WILL && stat <= GLORY_REFLEX)
		multi = SAVE_FACTOR;
	if (stat == GLORY_MIND)
		multi = RESIST_FACTOR;
	if (stat == GLORY_MANAREG)
		multi = MANAREG_FACTOR;
	if (stat == GLORY_BONUSPSYS)
		multi = BONUSPSYS_FACTOR;
	if (stat == GLORY_BONUSMAG)
		multi = BONUSMAG_FACTOR;
	return multi;
}

int calculate_glory_in_stats(GloryListType::const_iterator &i) {
	int total = 0;
	for (auto k = i->second->stats.begin(), kend = i->second->stats.end(); k != kend; ++k) {
		for (int m = 0; m < k->second; m++)
			total += m * 200 + 1000;
	}
	return total;
}

// * Распечатка 'слава информация'.
void print_glory(CharData *ch, GloryListType::iterator &it) {
	int spent = 0;
	*buf = '\0';
	for (auto i = it->second->stats.begin(), iend = it->second->stats.end(); i != iend; ++i) {
		if ((i->first >= 0) && (i->first < (int) sizeof(olc_stat_name))) {
			sprintf(buf + strlen(buf), "%-16s: +%d", olc_stat_name[i->first], i->second * stat_multi(i->first));
			if (stat_multi(i->first) > 1)
				sprintf(buf + strlen(buf), "(%d)", i->second);
			strcat(buf, "\r\n");
		} else {
			log("Glory: некорректный номер стата %d (uid: %ld)", i->first, it->first);
		}
		spent = spent + 1000 * i->second + 200 * (i->second - 1);
	}
	sprintf(buf + strlen(buf), "Свободных очков: %d. Вложено: %d\r\n", it->second->free_glory, spent);
	SendMsgToChar(buf, ch);
}

// * Показ свободной и вложенной славы у чара (glory имя).
void print_to_god(CharData *ch, CharData *god) {
	GloryListType::iterator it = glory_list.find(ch->get_uid());
	if (it == glory_list.end()) {
		SendMsgToChar(god, "У %s совсем не славы.\r\n", GET_PAD(ch, 1));
		return;
	}

	SendMsgToChar(god, "Информация об очках славы %s:\r\n", GET_PAD(ch, 1));
	print_glory(god, it);
}

int add_stat_cost(int stat, std::shared_ptr<GloryConst::glory_olc> olc) {
	if (stat < 0 || stat >= GLORY_TOTAL) {
		log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
		return 0;
	}

	int glory = (olc->stat_add[stat] * 200) + 1000;
	if (olc->stat_was[stat] - olc->stat_add[stat] > 0)
		glory -= glory * STAT_RETURN_FEE / 100;
	return glory;
}

int remove_stat_cost(int stat, std::shared_ptr<GloryConst::glory_olc> olc) {
	if (stat < 0 || stat >= GLORY_TOTAL) {
		log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
		return 0;
	}
	if (!olc->stat_add[stat]) {
		return 0;
	}

	int glory = ((olc->stat_add[stat] - 1) * 200) + 1000;
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
		"Э",
		"Z",
		"D",
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
		"Ю",
		"X",
		"F",
	};

std::string olc_print_stat(CharData *ch, int stat) {
	if (stat < 0 || stat >= GLORY_TOTAL) {
		log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
		return "";
	}

	std::string stat_add;
	if (ch->desc->glory_const->stat_add[stat] > 0) {
		stat_add = fmt::format("+{}", ch->desc->glory_const->stat_add[stat]);
	}

	return fmt::format("  {:<16} :  {}(+{:<5}){}  ({}{}{}) %4d ({}{}{})  {}(-{:<5})  | {:+}{}\r\n",
					   olc_stat_name[stat],
					   kColorBoldBlk, remove_stat_cost(stat, ch->desc->glory_const), kColorNrm,
					   kColorBoldGrn, olc_del_name[stat], kColorNrm,
					   ((ch->desc->glory_const->stat_cur[stat] + ch->desc->glory_const->stat_add[stat])
							  * stat_multi(stat)),
					   kColorBoldGrn, olc_add_name[stat], kColorNrm,
					   kColorBoldBlk, add_stat_cost(stat, ch->desc->glory_const),
					   stat_add, kColorNrm);
}

// * Распечатка олц меню.
void spend_glory_menu(CharData *ch) {
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
		<< olc_print_stat(ch, GLORY_MIND)
		<< olc_print_stat(ch, GLORY_MANAREG)
		<< olc_print_stat(ch, GLORY_BONUSPSYS)
		<< olc_print_stat(ch, GLORY_BONUSMAG);
	out << "\r\n  Свободной славы: " << ch->desc->glory_const->olc_free_glory << "\r\n\r\n";

	if (ch->desc->glory_const->olc_free_glory != ch->desc->glory_const->olc_was_free_glory) {
		out << "  " << kColorBoldGrn << "В" << kColorNrm
			<< ") Сохранить результаты\r\n";
	}
	out << "  " << kColorBoldGrn << "Я" << kColorNrm
		<< ") Выйти без сохранения\r\n"
		<< "  Ваш выбор: ";
	SendMsgToChar(out.str(), ch);
}

void olc_del_stat(CharData *ch, int stat) {
	if (stat < 0 || stat >= GLORY_TOTAL) {
		log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
		return;
	}
	if (ch->desc->glory_const->stat_add[stat] > 0) {
		ch->desc->glory_const->olc_free_glory +=
			remove_stat_cost(stat, ch->desc->glory_const);
		ch->desc->glory_const->stat_add[stat] -= 1;
	}
}

void olc_add_stat(CharData *ch, int stat) {
	int need_glory = add_stat_cost(stat, ch->desc->glory_const);
	bool ok = false;
	switch (stat) {
		case GLORY_CON:
			if (ch->desc->glory_const->olc_free_glory >= need_glory
				&& ch->desc->glory_const->stat_cur[stat]
					+ ch->desc->glory_const->stat_add[stat] < MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kCon))
				ok = true;
			else
				SendMsgToChar(ch, "Не хватает славы или превышен кап по данному параметру для вашей профессии.\r\n");
			break;
		case GLORY_STR:
			if (ch->desc->glory_const->olc_free_glory >= need_glory
				&& ch->desc->glory_const->stat_cur[stat]
					+ ch->desc->glory_const->stat_add[stat] < MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kStr))
				ok = true;
			else
				SendMsgToChar(ch, "Не хватает славы или превышен кап по данному параметру для вашей профессии.\r\n");
			break;
		case GLORY_DEX:
			if (ch->desc->glory_const->olc_free_glory >= need_glory
				&& ch->desc->glory_const->stat_cur[stat]
					+ ch->desc->glory_const->stat_add[stat] < MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kDex))
				ok = true;
			else
				SendMsgToChar(ch, "Не хватает славы или превышен кап по данному параметру для вашей профессии.\r\n");
			break;
		case GLORY_INT:
			if (ch->desc->glory_const->olc_free_glory >= need_glory
				&& ch->desc->glory_const->stat_cur[stat]
					+ ch->desc->glory_const->stat_add[stat] < MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kInt))
				ok = true;
			else
				SendMsgToChar(ch, "Не хватает славы или превышен кап по данному параметру для вашей профессии.\r\n");
			break;
		case GLORY_WIS:
			if (ch->desc->glory_const->olc_free_glory >= need_glory
				&& ch->desc->glory_const->stat_cur[stat]
					+ ch->desc->glory_const->stat_add[stat] < MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kWis))
				ok = true;
			else
				SendMsgToChar(ch, "Не хватает славы или превышен кап по данному параметру для вашей профессии.\r\n");
			break;
		case GLORY_CHA:
			if (ch->desc->glory_const->olc_free_glory >= need_glory
				&& ch->desc->glory_const->stat_cur[stat]
					+ ch->desc->glory_const->stat_add[stat] < MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kCha))
				ok = true;
			else
				SendMsgToChar(ch, "Не хватает славы или превышен кап по данному параметру для вашей профессии.\r\n");
			break;
		case GLORY_HIT:
		case GLORY_SUCCESS:
		case GLORY_WILL:
		case GLORY_STABILITY:
		case GLORY_REFLEX:
		case GLORY_MANAREG:
		case GLORY_BONUSPSYS:
		case GLORY_BONUSMAG:
			if (ch->desc->glory_const->olc_free_glory >= need_glory)
				ok = true;
			break;
		case GLORY_MIND:
			if (ch->desc->glory_const->olc_free_glory >= need_glory
				&& ch->desc->glory_const->stat_cur[stat]
					+ ch->desc->glory_const->stat_add[stat] < 75)
				ok = true;
			break;
		default: log("SYSERROR : bad stat %d (%s:%d)", stat, __FILE__, __LINE__);
	}
	if (ok) {
		ch->desc->glory_const->olc_free_glory -= need_glory;
		ch->desc->glory_const->stat_add[stat] += 1;
	}
}

int olc_real_stat(CharData *ch, int stat) {
	return ch->desc->glory_const->stat_cur[stat]
		+ ch->desc->glory_const->stat_add[stat];
}

bool parse_spend_glory_menu(CharData *ch, char *arg) {
	switch (LOWER(*arg)) {
		case 'а': olc_del_stat(ch, GLORY_STR);
			break;
		case 'б': olc_del_stat(ch, GLORY_DEX);
			break;
		case 'г': olc_del_stat(ch, GLORY_INT);
			break;
		case 'д': olc_del_stat(ch, GLORY_WIS);
			break;
		case 'е': olc_del_stat(ch, GLORY_CON);
			break;
		case 'ж': olc_del_stat(ch, GLORY_CHA);
			break;
		case 'з': olc_del_stat(ch, GLORY_HIT);
			break;
		case 'и': olc_del_stat(ch, GLORY_SUCCESS);
			break;
		case 'к': olc_del_stat(ch, GLORY_WILL);
			break;
		case 'л': olc_del_stat(ch, GLORY_STABILITY);
			break;
		case 'м': olc_del_stat(ch, GLORY_REFLEX);
			break;
		case 'н': olc_del_stat(ch, GLORY_MIND);
			break;
		case 'э': olc_del_stat(ch, GLORY_MANAREG);
			break;
		case 'x': olc_add_stat(ch, GLORY_BONUSPSYS);
			break;
		case 'z': olc_del_stat(ch, GLORY_BONUSPSYS);
			break;
		case 'f': olc_add_stat(ch, GLORY_BONUSMAG);
			break;
		case 'd': olc_del_stat(ch, GLORY_BONUSMAG);
			break;
		case 'о': olc_add_stat(ch, GLORY_STR);
			break;
		case 'п': olc_add_stat(ch, GLORY_DEX);
			break;
		case 'р': olc_add_stat(ch, GLORY_INT);
			break;
		case 'с': olc_add_stat(ch, GLORY_WIS);
			break;
		case 'т': olc_add_stat(ch, GLORY_CON);
			break;
		case 'у': olc_add_stat(ch, GLORY_CHA);
			break;
		case 'ф': olc_add_stat(ch, GLORY_HIT);
			break;
		case 'х': olc_add_stat(ch, GLORY_SUCCESS);
			break;
		case 'ц': olc_add_stat(ch, GLORY_WILL);
			break;
		case 'ч': olc_add_stat(ch, GLORY_STABILITY);
			break;
		case 'ш': olc_add_stat(ch, GLORY_REFLEX);
			break;
		case 'щ': olc_add_stat(ch, GLORY_MIND);
			break;
		case 'ю': olc_add_stat(ch, GLORY_MANAREG);
			break;
		case 'в': {
			// получившиеся статы
			ch->set_str(olc_real_stat(ch, GLORY_STR));
			ch->set_dex(olc_real_stat(ch, GLORY_DEX));
			ch->set_int(olc_real_stat(ch, GLORY_INT));
			ch->set_wis(olc_real_stat(ch, GLORY_WIS));
			ch->set_con(olc_real_stat(ch, GLORY_CON));
			ch->set_cha(olc_real_stat(ch, GLORY_CHA));
			// обновление записи в списке славы
			GloryListType::const_iterator it = glory_list.find(ch->get_uid());
			if (glory_list.end() == it) {
				log("SYSERROR : нет записи чара при выходе из олц постоянной славы name=%s (%s:%d)",
					ch->get_name().c_str(), __FILE__, __LINE__);
				SendMsgToChar("Ошибка сохранения, сообщите Богам!\r\n", ch);
				ch->desc->glory_const.reset();
				ch->desc->state = EConState::kPlaying;
				return 1;
			}
			// слава перед редактированием (для расчета комиса)
			int was_glory = it->second->free_glory + calculate_glory_in_stats(it);
			// обновление вложенных статов
			it->second->free_glory = ch->desc->glory_const->olc_free_glory;
			it->second->stats.clear();
			for (int i = 0; i < GLORY_TOTAL; ++i) {
				if (ch->desc->glory_const->stat_add[i] > 0) {
					it->second->stats[i] = ch->desc->glory_const->stat_add[i];
				}
			}
			// расчет снятого комиса
			int now_glory = it->second->free_glory + calculate_glory_in_stats(it);
			if (was_glory < now_glory) {
				log("SYSERROR : прибавка постоянной славы после редактирования в олц (%d -> %d) name=%s (%s:%d)",
					was_glory, now_glory, ch->get_name().c_str(), __FILE__, __LINE__);
			} else {
				total_charge += was_glory - now_glory;
			}
			// выход из олц с обновлением хп
			ch->desc->glory_const.reset();
			ch->desc->state = EConState::kPlaying;
			check_max_hp(ch);
			SendMsgToChar("Ваши изменения сохранены.\r\n", ch);
			ch->setGloryRespecTime(time(nullptr));
			ch->save_char();
			save();
			return 1;
		}
		case 'я': ch->desc->glory_const.reset();
			ch->desc->state = EConState::kPlaying;
			SendMsgToChar("Редактирование прервано.\r\n", ch);
			return 1;
		default: break;
	}
	return 0;
}

const char *GLORY_CONST_FORMAT =
	"Формат: слава2 изменить\r\n"
	"        слава2 информация\r\n"
	"        слава2 перевести <имя> <кол-во>\r\n";

void do_spend_glory(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	GloryListType::iterator it = glory_list.find(ch->get_uid());
	if (glory_list.end() == it || IS_IMMORTAL(ch)) {
		SendMsgToChar("Вам это не нужно...\r\n", ch);
		return;
	}

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	if (CompareParam(buffer2, "информация")) {
		SendMsgToChar("Информация о вложенных вами очках славы:\r\n", ch);
		print_glory(ch, it);
		return;
	}
	if (CompareParam(buffer2, "перевести")) {
		std::string name;
		GetOneParam(buffer, name);
		// buffer = кол-во
		utils::Trim(buffer);

		Player p_vict;
		CharData *vict = &p_vict;
		if (LoadPlayerCharacter(name.c_str(), vict, ELoadCharFlags::kFindId) < 0) {
			SendMsgToChar(ch, "%s - некорректное имя персонажа.\r\n", name.c_str());
			return;
		}
		if (str_cmp(GET_EMAIL(ch), GET_EMAIL(vict))) {
			SendMsgToChar(ch, "Персонажи имеют разные email адреса.\r\n");
			return;
		}
		if (ch->getGloryRespecTime() != 0 && (time(0) - ch->getGloryRespecTime() < 86400)) {
			SendMsgToChar("Операцию со славой можно делать только раз в сутки.\r\n", ch);
			return;
		}

		int amount = 0;
		try {
			amount = std::stoi(buffer);
		}
		catch (...) {
			SendMsgToChar(ch, "%s - некорректное количество для перевода.\r\n", buffer.c_str());
			SendMsgToChar(GLORY_CONST_FORMAT, ch);
			return;
		}

		if (amount < MIN_TRANSFER_TAX || amount > it->second->free_glory) {
			SendMsgToChar(ch,
						  "%d - некорректное количество для перевода.\r\n"
						  "Вы можете перевести от %d до %d постоянной славы.\r\n",
						  amount, MIN_TRANSFER_TAX, it->second->free_glory);
			return;
		}

		int tax = int(amount / 100. * TRANSFER_FEE);
		int total_amount = amount - tax;

		remove_glory(ch->get_uid(), amount);
		add_glory(vict->get_uid(), total_amount);

		snprintf(buf, kMaxStringLength,
				 "Transfer %d const glory from %s", total_amount, GET_NAME(ch));
		AddKarma(vict, buf, "командой");

		snprintf(buf, kMaxStringLength, "Transfer %d const glory to %s", amount, GET_NAME(vict));
		AddKarma(ch, buf, "командой");

		total_charge += tax;
		transfer_log("%s -> %s transfered %d (%d tax)", GET_NAME(ch), GET_NAME(vict), total_amount, tax);

		ch->save_char();
		vict->save_char();
		save();

		SendMsgToChar(ch, "%s переведено %d постоянной славы (%d комиссии).\r\n",
					  GET_PAD(vict, 2), total_amount, tax);
		ch->setGloryRespecTime(time(nullptr));
		return;
	}
	if (CompareParam(buffer2, "изменить")) {
		if (it->second->free_glory < 1000 && it->second->stats.empty()) {
			SendMsgToChar("У вас недостаточно очков славы для использования этой команды.\r\n", ch);
			return;
		}
		if (ch->getGloryRespecTime() != 0 && (time(0) - ch->getGloryRespecTime() < 86400)) {
			SendMsgToChar("Операцию со славой можно делать только раз в сутки.\r\n", ch);
			return;
		}
		std::shared_ptr<glory_olc> tmp_glory_olc(new glory_olc);
		tmp_glory_olc->stat_cur[GLORY_STR] = ch->GetInbornStr();
		tmp_glory_olc->stat_cur[GLORY_DEX] = ch->GetInbornDex();
		tmp_glory_olc->stat_cur[GLORY_INT] = ch->GetInbornInt();
		tmp_glory_olc->stat_cur[GLORY_WIS] = ch->GetInbornWis();
		tmp_glory_olc->stat_cur[GLORY_CON] = ch->GetInbornCon();
		tmp_glory_olc->stat_cur[GLORY_CHA] = ch->GetInbornCha();
		tmp_glory_olc->stat_cur[GLORY_HIT] = it->second->stats[GLORY_HIT];
		tmp_glory_olc->stat_cur[GLORY_SUCCESS] = it->second->stats[GLORY_SUCCESS];
		tmp_glory_olc->stat_cur[GLORY_WILL] = it->second->stats[GLORY_WILL];
		tmp_glory_olc->stat_cur[GLORY_STABILITY] = it->second->stats[GLORY_STABILITY];
		tmp_glory_olc->stat_cur[GLORY_REFLEX] = it->second->stats[GLORY_REFLEX];
		tmp_glory_olc->stat_cur[GLORY_MIND] = it->second->stats[GLORY_MIND];
		tmp_glory_olc->stat_cur[GLORY_MANAREG] = it->second->stats[GLORY_MANAREG];
		tmp_glory_olc->stat_cur[GLORY_BONUSPSYS] = it->second->stats[GLORY_BONUSPSYS];  
		tmp_glory_olc->stat_cur[GLORY_BONUSMAG] = it->second->stats[GLORY_BONUSMAG];

		for (std::map<int, int>::const_iterator i = it->second->stats.begin(), iend = it->second->stats.end();
			 i != iend; ++i) {
			if (i->first < GLORY_TOTAL && i->first >= 0) {
				tmp_glory_olc->stat_cur[i->first] -= i->second;
				tmp_glory_olc->stat_add[i->first] = tmp_glory_olc->stat_was[i->first] = i->second;
			} else {
				log("Glory: некорректный номер стата %d (uid: %ld)", i->first, it->first);
			}
		}
		tmp_glory_olc->olc_free_glory = tmp_glory_olc->olc_was_free_glory = it->second->free_glory;

		ch->desc->glory_const = tmp_glory_olc;
		ch->desc->state = EConState::kGloryConst;
		spend_glory_menu(ch);
	} else {
		SendMsgToChar(GLORY_CONST_FORMAT, ch);
	}
}

/**
* Удаление славы у чара (свободной), ниже нуля быть не может.
* \return 0 - ничего не списано, число > 0 - сколько реально списали
*/
int remove_glory(long uid, int amount) {
	if (uid <= 0 || amount <= 0) {
		return 0;
	}

	int real_removed = amount;

	GloryListType::iterator i = glory_list.find(uid);
	if (glory_list.end() != i) {
		// ниже нуля ес-сно не отнимаем
		if (i->second->free_glory >= amount) {
			i->second->free_glory -= amount;
		} else {
			real_removed = i->second->free_glory;
			i->second->free_glory = 0;
		}
		// смысла пустую запись оставлять до ребута нет
		if (!i->second->free_glory && i->second->stats.empty()) {
			glory_list.erase(i);
		}
		save();
	} else {
		real_removed = 0;
	}

	return real_removed;
}

bool reset_glory(CharData *ch) {
	GloryListType::iterator i = glory_list.find(ch->get_uid());
	if (glory_list.end() != i) {
		glory_list.erase(i);
		save();
		check_max_hp(ch);
		GloryMisc::recalculate_stats(ch);
		ch->save_char();
		return true;
	}
	return false;
}

void do_glory(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!*argument) {
		SendMsgToChar("Формат команды : \r\n"
					 "   glory <имя> (информация по указанному персонажу)\r\n"
					 "   glory <имя> +|-<кол-во славы> причина\r\n"
					 "   glory <имя> reset причина (обнуление свободной и вложенной славы)\r\n", ch);
		return;
	}

	enum { SHOW_GLORY, ADD_GLORY, SUB_GLORY, RESET_GLORY };

	char num[kMaxInputLength];
	int mode = 0;

	char *reason = two_arguments(argument, arg, num);
	skip_spaces(&reason);

	if (!*num) {
		mode = SHOW_GLORY;
	} else if (*num == '+') {
		mode = ADD_GLORY;
	} else if (*num == '-') {
		mode = SUB_GLORY;
	} else if (utils::IsAbbr(num, "reset")) {
		mode = RESET_GLORY;
	}
	// точки убираем, чтобы карма всегда писалась
	skip_dots(&reason);

	if (mode != SHOW_GLORY && (!reason || !*reason)) {
		SendMsgToChar("Укажите причину изменения славы?\r\n", ch);
		return;
	}

	CharData *vict = get_player_vis(ch, arg, EFind::kCharInWorld);
	if (vict && vict->desc && vict->desc->state == EConState::kGloryConst) {
		SendMsgToChar("Персонаж в данный момент редактирует свою славу.\r\n", ch);
		return;
	}
	Player t_vict; // TODO: мутно
	if (!vict) {
		if (LoadPlayerCharacter(arg, &t_vict, ELoadCharFlags::kFindId) < 0) {
			SendMsgToChar("Такого персонажа не существует.\r\n", ch);
			return;
		}
		vict = &t_vict;
	}

	switch (mode) {
		case ADD_GLORY: {
			int amount = atoi((num + 1));
			add_glory(vict->get_uid(), amount);
			SendMsgToChar(ch, "%s добавлено %d у.е. постоянной славы (Всего: %d у.е.).\r\n",
						  GET_PAD(vict, 2), amount, get_glory(vict->get_uid()));
			// запись в карму, логи
			sprintf(buf, "(GC) %s sets +%d const glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
			mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
			imm_log("%s", buf);
			sprintf(buf, "Change const glory +%d by %s", amount, GET_NAME(ch));
			AddKarma(vict, buf, reason);
			GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
			break;
		}
		case SUB_GLORY: {
			int amount = remove_glory(vict->get_uid(), atoi((num + 1)));
			if (amount <= 0) {
				SendMsgToChar(ch, "У %s нет свободной постоянной славы.\r\n", GET_PAD(vict, 1));
				break;
			}
			SendMsgToChar(ch, "У %s вычтено %d у.е. постоянной славы (Всего: %d у.е.).\r\n",
						  GET_PAD(vict, 1), amount, get_glory(vict->get_uid()));
			// запись в карму, логи
			sprintf(buf, "(GC) %s sets -%d const glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
			mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
			imm_log("%s", buf);
			sprintf(buf, "Change const glory -%d by %s", amount, GET_NAME(ch));
			AddKarma(vict, buf, reason);
			GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
			break;
		}
		case RESET_GLORY: {
			if (reset_glory(vict)) {
				SendMsgToChar(ch, "%s - очищена запись постоянной славы.\r\n", vict->get_name().c_str());
				// запись в карму, логи
				sprintf(buf, "(GC) %s reset const glory to %s.", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);
				sprintf(buf, "Reset stats and const glory by %s", GET_NAME(ch));
				AddKarma(vict, buf, reason);
				GloryMisc::add_log(mode, 0, std::string(buf), std::string(reason), vict);
			} else {
				SendMsgToChar(ch, "%s - запись постоянной славы и так пустая.\r\n", vict->get_name().c_str());
			}
			break;
		}
		default: GloryConst::print_to_god(vict, ch);
	}
	vict->save_char();
}

void save() {
	pugi::xml_document doc;
	doc.append_attribute("encoding") = "koi8-r";
	doc.append_child().set_name("glory_list");
	pugi::xml_node char_list = doc.child("glory_list");

	char_list.append_attribute("version") = cur_ver;
	for (GloryListType::const_iterator i = glory_list.begin(), iend = glory_list.end(); i != iend; ++i) {
		pugi::xml_node char_node = char_list.append_child();
		char_node.set_name("char");
		char_node.append_attribute("uid") = (int) i->first;
		char_node.append_attribute("name") = GetNameByUnique(i->second->uid, false).c_str();
		char_node.append_attribute("glory") = i->second->free_glory;
		char_node.append_attribute("hide") = i->second->hide;

		for (std::map<int, int>::const_iterator k = i->second->stats.begin(),
				 kend = i->second->stats.end(); k != kend; ++k) {
			if (k->second > 0) {
				pugi::xml_node stat = char_node.append_child();
				stat.set_name("stat");
				stat.append_attribute("num") = k->first;
				stat.append_attribute("amount") = k->second;
			}
		}
		if ((char_node.begin() == char_node.end()) && (i->second->free_glory == 0)) {
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

void load() {
	int ver = 0;
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_PLRSTUFF"glory_const.xml");
	if (!result) {
		snprintf(buf, kMaxStringLength, "SYSERR: error reading glory_const.xml: %s", result.description());
		perror(buf);
		return;
	}
	pugi::xml_node char_list = doc.child("glory_list");
	if (char_list.attribute("version")) {
		ver = std::stoi(char_list.attribute("version").value(), nullptr, 10);
		if (ver > cur_ver) {
			snprintf(buf,
					 kMaxStringLength,
					 "SYSERR: error reading glory_const.xml: unsupported version: %d, current version: %d",
					 ver,
					 cur_ver);
			perror(buf);
			return;
		}
	}
	for (pugi::xml_node node = char_list.child("char"); node; node = node.next_sibling("char")) {
		const auto uid_str = node.attribute("uid").value();
		long uid = 0;
		try {
			uid = std::stol(uid_str, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
			log("SYSERR: UID [%s] не является верным десятичным числом.", uid_str);
			continue;
		}

		std::string name = GetNameByUnique(uid);
		if (name.empty()) {
			log("GloryConst: UID %ld - персонажа не существует.", uid);
			continue;
		}

		if (glory_list.find(uid) != glory_list.end()) {
			log("SYSERROR : дубликат записи uid=%ld, name=%s (%s:%d)",
				uid, name.c_str(), __FILE__, __LINE__);
			continue;
		}

		GloryNodePtr tmp_node(new glory_node);

		long free_glory = std::stoi(node.attribute("glory").value(), nullptr, 10);
		tmp_node->free_glory = free_glory;
		tmp_node->uid = uid;
		//tmp_node->name = name;
		tmp_node->hide = node.attribute("hide").as_bool();

		for (pugi::xml_node stat = node.child("stat"); stat; stat = stat.next_sibling("stat")) {
			int divider = 1;
			int stat_num = std::stoi(stat.attribute("num").value(), nullptr, 10);
			if (ver == 0) {
				if (stat_num == GLORY_HIT)
					divider = 50;
				if (stat_num == GLORY_SUCCESS)
					divider = 10;
				if (stat_num >= GLORY_WILL && stat_num <= GLORY_REFLEX)
					divider = 15;
				if (stat_num == GLORY_MIND)
					divider = 7;
			}
			int stat_amount = std::stoi(stat.attribute("amount").value(), nullptr, 10) / divider;
			if (stat_num >= GLORY_TOTAL && stat_num < 0) {
				log("SYSERROR : невалидный номер влитого стата num=%d, name=%s (%s:%d)",
					stat_num, name.c_str(), __FILE__, __LINE__);
				continue;
			}
			if (tmp_node->stats.find(stat_num) != tmp_node->stats.end()) {
				log("SYSERROR : дубликат влитого стата num=%d, name=%s (%s:%d)",
					stat_num, name.c_str(), __FILE__, __LINE__);
				continue;
			}
			tmp_node->stats[stat_num] = stat_amount;

		}
		glory_list[uid] = tmp_node;
	}
	pugi::xml_node charge_node = char_list.child("total_charge");
	if (charge_node) {
		total_charge = std::stoi(charge_node.attribute("amount").value(), nullptr, 10);
	}
	pugi::xml_node spent_node = char_list.child("total_spent");
	if (spent_node) {
		total_spent = std::stoi(spent_node.attribute("amount").value(), nullptr, 10);
	}
	if (ver < cur_ver)//автоматом обновляем xml
		save();
}

void set_stats(CharData *ch) {
	GloryListType::iterator i = glory_list.find(ch->get_uid());
	if (glory_list.end() == i) {
		return;
	}

	for (std::map<int, int>::const_iterator k = i->second->stats.begin(),
			 kend = i->second->stats.end(); k != kend; ++k) {
		switch (k->first) {
			case G_STR: ch->inc_str(k->second);
				break;
			case G_DEX: ch->inc_dex(k->second);
				break;
			case G_INT: ch->inc_int(k->second);
				break;
			case G_WIS: ch->inc_wis(k->second);
				break;
			case G_CON: ch->inc_con(k->second);
				break;
			case G_CHA: ch->inc_cha(k->second);
				break;
			default: log("Glory: некорректный номер стата %d (uid: %ld)", k->first, ch->get_uid());
		}
	}
}

// * Количество вложенных статов (только из числа 6 основных).
int main_stats_count(CharData *ch) {
	GloryListType::iterator i = glory_list.find(ch->get_uid());
	if (glory_list.end() == i) {
		return 0;
	}

	int count = 0;
	for (std::map<int, int>::const_iterator k = i->second->stats.begin(),
			 kend = i->second->stats.end(); k != kend; ++k) {
		switch (k->first) {
			case G_STR:
			case G_DEX:
			case G_INT:
			case G_WIS:
			case G_CON:
			case G_CHA: count += k->second;
				break;
		}
	}
	return count;
}

// * Вывод инфы в show stats.
void show_stats(CharData *ch) {
	int free_glory = 0, spend_glory = 0;
	for (GloryListType::const_iterator i = glory_list.begin(), iend = glory_list.end(); i != iend; ++i) {
		free_glory += i->second->free_glory;
		spend_glory += calculate_glory_in_stats(i);
	}
	SendMsgToChar(ch,
				  "  Слава2: вложено %d, свободно %d, всего %d, комиссии %d\r\n"
				  "  Всего потрачено славы в магазинах: %d\r\n",
				  spend_glory, free_glory, free_glory + spend_glory, total_charge, total_spent);
}

void add_total_spent(int amount) {
	if (amount > 0) {
		total_spent += amount;
	}
}

void apply_modifiers(CharData *ch) {
	auto it = glory_list.find(ch->get_uid());
	if (it == glory_list.end()) {
		return;
	}

	for (std::map<int, int>::const_iterator i = it->second->stats.begin(); i != it->second->stats.end(); ++i) {
		auto location{EApply::kNone};
		bool add = true;
		switch (i->first) {
			case GLORY_HIT: location = EApply::kHp;
				break;
			case GLORY_SUCCESS: location = EApply::kCastSuccess;
				break;
			case GLORY_WILL: location = EApply::kSavingWill;
				add = false;
				break;
			case GLORY_STABILITY: location = EApply::kSavingStability;
				add = false;
				break;
			case GLORY_REFLEX: location = EApply::kSavingReflex;
				add = false;
				break;
			case GLORY_MIND: location = EApply::kResistMind;
				break;
			case GLORY_MANAREG: location = EApply::kManaRegen;
				break;
			case GLORY_BONUSPSYS: location = EApply::kPhysicDamagePercent;
				break;
			case GLORY_BONUSMAG: location = EApply::kMagicDamagePercent;
				break;
			default: break;
		}
		if (location) {
			affect_modify(ch, location, i->second * stat_multi(i->first), static_cast<EAffect>(0), add);
		}
	}
}

void PrintGloryChart(CharData *ch) {
	std::stringstream out;
	std::map<int, GloryNodePtr> temp_list;
	std::stringstream hide;

	bool print_hide = false;
	if (IS_IMMORTAL(ch)) {
		print_hide = true;
		hide << "\r\nПерсонажи, исключенные из списка: ";
	}
	for (GloryListType::const_iterator it = glory_list.begin(); it != glory_list.end(); ++it) {
		it->second->tmp_spent_glory = calculate_glory_in_stats(it);
	}

	std::list<GloryNodePtr> playerGloryList;
	for (GloryListType::const_iterator it = glory_list.begin(); it != glory_list.end(); ++it) {
		playerGloryList.insert(playerGloryList.end(), it->second);
	}

	playerGloryList.sort([](const GloryNodePtr &a, const GloryNodePtr &b) -> bool {
		return (a->free_glory + a->tmp_spent_glory) > (b->free_glory + b->tmp_spent_glory);
	});

	out << kColorWht << "Лучшие прославленные:\r\n" << kColorNrm;

	int i = 0;

	for (std::list<GloryNodePtr>::const_iterator t_it = playerGloryList.begin();
		 t_it != playerGloryList.end() && i < kPlayerChartSize; ++t_it, ++i) {

		std::string name = GetNameByUnique(t_it->get()->uid);
		name[0] = UPPER(name[0]);
		if (name.length() == 0) {
			name = "*скрыто*";
		}
		if (t_it->get()->hide) {
			name += "(скрыт)";
		}
		if (!IsActiveUser(t_it->get()->uid)) {
			name += " (в отпуске)";
		}

		if (!t_it->get()->hide  /*&& IsActiveUser( t_it->get()->uid ) */) {
			out << fmt::format("\t{:<25} {:<2}\r\n", name, (t_it->get()->free_glory + t_it->get()->tmp_spent_glory));
		} else {
			if (print_hide) {
				hide << "\r\n" << "\t" << name << " (доступно:" << t_it->get()->free_glory << ", вложено:"
					 << t_it->get()->tmp_spent_glory << ")";
			}
			--i;
		}
	}

	SendMsgToChar(out.str().c_str(), ch);

	if (print_hide) {
		hide << "\r\n";
		SendMsgToChar(hide.str().c_str(), ch);
	}
}

} // namespace GloryConst

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
