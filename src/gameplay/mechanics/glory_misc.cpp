// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include "glory_misc.h"
#include "glory.h"
#include "glory_const.h"
#include "gameplay/core/genchar.h"
#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "engine/entities/char_player.h"
#include "engine/ui/modify.h"
#include "engine/db/global_objects.h"

namespace GloryMisc {

class GloryLog {
 public:
	GloryLog() : type(0), num(0) {};
	// тип записи (1 - плюс слава, 2 - минус слава, 3 - убирание стата, 4 - трансфер, 5 - hide)
	int type;
	// кол-во славы при +- славы
	int num;
	// что было записано в карму
	std::string karma;
};

typedef std::shared_ptr<GloryLog> GloryLogPtr;
typedef std::multimap<time_t /* время */, GloryLogPtr> GloryLogType;
// лог манипуляций со славой
GloryLogType glory_log;

// * Загрузка лога славы.
void load_log() {
	const auto glory_file = runtime_config.log_dir() + "/glory.log";
	std::ifstream file(glory_file);
	if (!file.is_open()) {
		log("GloryLog: не удалось открыть файл на чтение: %s", glory_file.c_str());
		return;
	}

	std::string buffer;
	while (std::getline(file, buffer)) {
		std::string buffer2;
		GetOneParam(buffer, buffer2);
		time_t time = 0;
		try {
			time = std::stoi(buffer2, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
			log("GloryLog: ошибка чтения time, buffer2: %s", buffer2.c_str());
			return;
		}
		GetOneParam(buffer, buffer2);
		int type = 0;
		try {
			type = std::stoi(buffer2, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
			log("GloryLog: ошибка чтения type, buffer2: %s", buffer2.c_str());
			return;
		}
		GetOneParam(buffer, buffer2);
		int num = 0;
		try {
			num = std::stoi(buffer2, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
			log("GloryLog: ошибка чтения num, buffer2: %s", buffer2.c_str());
			return;
		}

		utils::Trim(buffer);
		GloryLogPtr temp_node(new GloryLog);
		temp_node->type = type;
		temp_node->num = num;
		temp_node->karma = buffer;
		glory_log.insert(std::make_pair(time, temp_node));
	}
}

// * Сохранение лога славы.
void save_log() {
	std::stringstream out;

	for (GloryLogType::const_iterator it = glory_log.begin(); it != glory_log.end(); ++it)
		out << it->first << " " << it->second->type << " " << it->second->num << " " << it->second->karma << "\n";

	const auto glory_file = runtime_config.log_dir() + "/glory.log";
	std::ofstream file(glory_file);
	if (!file.is_open()) {
		log("GloryLog: не удалось открыть файл на запись: %s", glory_file.c_str());
		return;
	}
	file << out.rdbuf();
	file.close();
}

// * Добавление записи в лог славы (время, тип, кол-во, строка из кармы).
void add_log(int type, int num, std::string punish, std::string reason, CharData *vict) {
	if (!vict || vict->get_name().empty()) {
		return;
	}

	GloryLogPtr temp_node(new GloryLog);
	temp_node->type = type;
	temp_node->num = num;
	std::stringstream out;
	out << GET_NAME(vict) << " : " << punish << " [" << reason << "]";
	temp_node->karma = out.str();
	glory_log.insert(std::make_pair(time(nullptr), temp_node));
	save_log();
}

/**
* Показ лога славы (show glory), отсортированного по убыванию даты, с возможностью фильтрациии.
* Фильтры: show glory число|transfer|remove|hide
*/
void show_log(CharData *ch, char const *const value) {
	if (glory_log.empty()) {
		SendMsgToChar("Пусто, слава те господи!\r\n", ch);
		return;
	}

	int type = 0;
	int num = 0;
	std::string buffer;
	if (value && *value)
		buffer = value;
	if (CompareParam(buffer, "transfer"))
		type = TRANSFER_GLORY;
	else if (CompareParam(buffer, "remove"))
		type = REMOVE_GLORY;
	else if (CompareParam(buffer, "hide"))
		type = HIDE_GLORY;
	else {
		try {
			num = std::stoi(buffer, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
			type = 0;
			num = 0;
		}
	}
	std::stringstream out;
	for (GloryLogType::reverse_iterator it = glory_log.rbegin(); it != glory_log.rend(); ++it) {
		if (type == it->second->type || (type == 0 && num == 0) || (type == 0 && num <= it->second->num)) {
			char time_buf[17];
			strftime(time_buf, sizeof(time_buf), "%H:%M %d-%m-%Y", localtime(&it->first));
			out << time_buf << " " << it->second->karma << "\r\n";
		}
	}
	page_string(ch->desc, out.str());
}

// * Суммарное кол-во стартовых статов у чара (должно совпадать с SUM_ALL_STATS)
int start_stats_count(CharData *ch) {
	int count = 0;
	for (int i = 0; i < START_STATS_TOTAL; ++i) {
		count += ch->get_start_stat(i);
	}
	return count;
}

/**
* Стартовые статы при любых условиях должны соответствовать границам ролла.
* В случае старого ролла тут это всплывет из-за нулевых статов.
*/
bool bad_start_stats(CharData *ch) {
	const auto &ch_class = MUD::Class(ch->GetClass());
	if (ch->get_start_stat(G_STR) > ch_class.GetBaseStatGenMax(EBaseStat::kStr) ||
		ch->get_start_stat(G_STR) < ch_class.GetBaseStatGenMin(EBaseStat::kStr) ||
		ch->get_start_stat(G_DEX) > ch_class.GetBaseStatGenMax(EBaseStat::kDex) ||
		ch->get_start_stat(G_DEX) < ch_class.GetBaseStatGenMin(EBaseStat::kDex) ||
		ch->get_start_stat(G_INT) > ch_class.GetBaseStatGenMax(EBaseStat::kInt) ||
		ch->get_start_stat(G_INT) < ch_class.GetBaseStatGenMin(EBaseStat::kInt) ||
		ch->get_start_stat(G_WIS) > ch_class.GetBaseStatGenMax(EBaseStat::kWis) ||
		ch->get_start_stat(G_WIS) < ch_class.GetBaseStatGenMin(EBaseStat::kWis) ||
		ch->get_start_stat(G_CON) > ch_class.GetBaseStatGenMax(EBaseStat::kCon) ||
		ch->get_start_stat(G_CON) < ch_class.GetBaseStatGenMin(EBaseStat::kCon) ||
		ch->get_start_stat(G_CHA) > ch_class.GetBaseStatGenMax(EBaseStat::kCha) ||
		ch->get_start_stat(G_CHA) < ch_class.GetBaseStatGenMin(EBaseStat::kCha) ||
		start_stats_count(ch) != kBaseStatsSum) {
		return true;
	}
	return false;
}

/**
* Считаем реальные статы с учетом мортов и влитой славы.
* \return 0 - все ок, любое другое число - все плохо
*/
int bad_real_stats(CharData *ch, int check) {
	check -= kBaseStatsSum; // стартовые статы у всех по 95
	check -= 6 * GetRealRemort(ch); // реморты
	// влитая слава
	check -= Glory::get_spend_glory(ch);
	check -= GloryConst::main_stats_count(ch);
	return check;
}

/**
* Проверка стартовых и итоговых статов.
* Если невалидные стартовые статы - чар отправляется на реролл.
* Если невалидные только итоговые статы - идет перезапись со стартовых с учетом мортов и славы.
*/
bool check_stats(CharData *ch) {
	// иммов травмировать не стоит
	if (IS_IMMORTAL(ch)) {
		return 1;
	}

	int have_stats = ch->GetInbornStr() + ch->GetInbornDex() + ch->GetInbornInt() + ch->GetInbornWis()
		+ ch->GetInbornCon() + ch->GetInbornCha();

	// чар со старым роллом статов или после попыток поправить статы в файле
	if (bad_start_stats(ch)) {
		snprintf(buf, kMaxStringLength, "\r\n%sВаши параметры за вычетом перевоплощений:\r\n"
										 "Сила: %d, Ловкость: %d, Ум: %d, Мудрость: %d, Телосложение: %d, Обаяние: %d\r\n"
										 "Просим вас заново распределить основные параметры персонажа.%s\r\n",
				 kColorBoldGrn,
				 ch->GetInbornStr() - GetRealRemort(ch),
				 ch->GetInbornDex() - GetRealRemort(ch),
				 ch->GetInbornInt() - GetRealRemort(ch),
				 ch->GetInbornWis() - GetRealRemort(ch),
				 ch->GetInbornCon() - GetRealRemort(ch),
				 ch->GetInbornCha() - GetRealRemort(ch),
				 kColorNrm);
	iosystem::write_to_output(buf, ch->desc);

		// данную фигню мы делаем для того, чтобы из ролла нельзя было случайно так просто выйти
		// сразу, не раскидав статы, а то много любителей тригов и просто нажатий не глядя
		const auto &ch_class = MUD::Class(ch->GetClass());
		ch->set_str(ch_class.GetBaseStatGenMin(EBaseStat::kStr));
		ch->set_dex(ch_class.GetBaseStatGenMin(EBaseStat::kDex));
		ch->set_int(ch_class.GetBaseStatGenMin(EBaseStat::kInt));
		ch->set_wis(ch_class.GetBaseStatGenMin(EBaseStat::kWis));
		ch->set_con(ch_class.GetBaseStatGenMin(EBaseStat::kCon));
		ch->set_cha(ch_class.GetBaseStatGenMin(EBaseStat::kCha));

		genchar_disp_menu(ch);
		ch->desc->state = EConState::kResetStats;
		return false;
	}
	// стартовые статы в поряде, но слава не сходится (снялась по времени или иммом)
	if (bad_real_stats(ch, have_stats)) {
		recalculate_stats(ch);
	}
	return true;
}

// * Пересчет статов чара на основании стартовых статов, ремортов и славы.
void recalculate_stats(CharData *ch) {
	// стартовые статы
	ch->set_str(ch->get_start_stat(G_STR));
	ch->set_dex(ch->get_start_stat(G_DEX));
	ch->set_con(ch->get_start_stat(G_CON));
	ch->set_int(ch->get_start_stat(G_INT));
	ch->set_wis(ch->get_start_stat(G_WIS));
	ch->set_cha(ch->get_start_stat(G_CHA));
	// морты
	if (GetRealRemort(ch)) {
		ch->inc_str(GetRealRemort(ch));
		ch->inc_dex(GetRealRemort(ch));
		ch->inc_con(GetRealRemort(ch));
		ch->inc_wis(GetRealRemort(ch));
		ch->inc_int(GetRealRemort(ch));
		ch->inc_cha(GetRealRemort(ch));
	}
	// влитая слава
	Glory::set_stats(ch);
	GloryConst::set_stats(ch);
}

} // namespace GloryMisc

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
