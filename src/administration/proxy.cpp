/**
\file proxy.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Система регистрации прокси-серверов.
\detail Список зарегистрированных прокси, загрузка-сохранение, система регистрации.
*/

#include "administration/proxy.h"

#include "engine/network/descriptor_data.h"
#include "engine/boot/boot_constants.h"
#include "engine/core/comm.h"
#include "engine/entities/char_data.h"
#include "utils/utils.h"
#include "utils/logger.h"

const int kMaxProxyConnect{50};

// собственно список ип и кол-ва коннектов
ProxyListType proxy_list;

void SaveProxyList() {
	std::ofstream file(PROXY_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", PROXY_FILE, __FILE__, __func__, __LINE__);
		return;
	}

	for (auto &it : proxy_list) {
		file << it.second->textIp << "  " << (it.second->textIp2.empty() ? "0"
																		 : it.second->textIp2) << "  "
			 << it.second->num << "  " << it.second->text << "\n";
	}
	file.close();
}

void RegisterSystem::LoadProxyList() {
	// если релоадим
	proxy_list.clear();

	std::ifstream file(PROXY_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", PROXY_FILE, __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer, textIp, textIp2;
	int num = 0;

	while (file) {
		file >> textIp >> textIp2 >> num;
		std::getline(file, buffer);
		utils::Trim(buffer);
		if (textIp.empty() || textIp2.empty() || buffer.empty() || num < 2 || num > kMaxProxyConnect) {
			log("Error read file: %s! IP: %s IP2: %s Num: %d Text: %s (%s %s %d)", PROXY_FILE, textIp.c_str(),
				textIp2.c_str(), num, buffer.c_str(), __FILE__, __func__, __LINE__);
			// не стоит грузить файл, если там чет не то - похерится потом при сохранении
			// хотя будет смешно, если после неудачного лоада кто-то добавит запись в онлайне Ж)
			proxy_list.clear();
			return;
		}

		ProxyIpPtr tempIp(new ProxyIp);
		tempIp->num = num;
		tempIp->text = buffer;
		tempIp->textIp = textIp;
		// 0 тут значит, что это не диапазон, чтобы не портить вид списка в маде - лучше пустую строку
		if (textIp2 != "0")
			tempIp->textIp2 = textIp2;
		unsigned long ip = TxtToIp(textIp);
		tempIp->ip2 = TxtToIp(textIp2);
		proxy_list[ip] = tempIp;
	}
	file.close();
	SaveProxyList();
}

// проверка на присутствие в списке зарегистрированных ip и кол-ва соединений
// 0 - нету, 1 - в маде уже макс. число коннектов с данного ip, 2 - все ок
int CheckProxy(DescriptorData *ch) {
	//сначала ищем в списке, зачем зря коннекты считать
	ProxyListType::const_iterator it;
	// мысль простая - есть второй ип, знач смотрим диапазон, нету - знач сверяем на равенство ип
	for (it = proxy_list.begin(); it != proxy_list.end(); ++it) {
		if (!it->second->ip2) {
			if (it->first == ch->ip)
				break;
		} else if ((it->first <= ch->ip) && (ch->ip <= it->second->ip2))
			break;
	}
	if (it == proxy_list.end())
		return 0;

	// терь можно и посчитать
	DescriptorData *i;
	int num_ip = 0;
	for (i = descriptor_list; i; i = i->next)
		if (i != ch && i->character && !IS_IMMORTAL(i->character) && (i->ip == ch->ip))
			num_ip++;

	// проверяем на кол-во коннектов
	if (it->second->num <= num_ip)
		return 1;

	return 2;
}

// Тут все, что связано с регистрацией клубов и т.п.

// конвертация строкового ip в число для сравнений
unsigned long TxtToIp(const char *text) {
	int ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0;
	unsigned long ip = 0;

	sscanf(text, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
	ip = (ip1 << 24) + (ip2 << 16) + (ip3 << 8) + ip4;
	return ip;
}

// тоже для строкового аргумента
unsigned long TxtToIp(std::string &text) {
	int ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0;
	unsigned long ip = 0;

	sscanf(text.c_str(), "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
	ip = (ip1 << 24) + (ip2 << 16) + (ip3 << 8) + ip4;
	return ip;
}

// тоже самое, только обратно для визуального отображения
std::string IpToTxt(unsigned long ip) {
	char text[32];
	sprintf(text, "%lu.%lu.%lu.%lu", ip >> 24, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
	std::string buffer = text;
	return buffer;
}

//////////////////////////////////////////////////////////////////////////////////////

/**
* Система автоматической регистрации по ранее зарегистрированному мылу.
* Флаг регистрации выставляется только на время нахождения в игре и в случае
* снятия регистрации с персонажа - данное мыло удалется из списка, т.е. при
* следующих заходах автоматически уже не зарегистрирует никого с тем же мылом,
* но если они имели свои собсвенные регистрации через команду register, то
* их эта отмена в принципе не задевает, т.е. на работу статой системы регистраций
* в данном случае никакого влияния не оказываем и флаг EPlrFlag::REGISTERED не трогаем. -- Krodo
*/
namespace RegisterSystem {

typedef std::map<std::string, std::string> EmailListType;
// список зарегистрированных мыл
EmailListType email_list;
// файл для соъхранения/лоада
const char *REGISTERED_EMAIL_FILE = LIB_PLRSTUFF"registered-email.lst";
// т.к. список потенциально может быть довольно большим, то сейвить бум только в случае изменений в add и remove
bool need_save = false;

} // namespace RegisterSystem

// * Добавления мыла в список + проставления флага EPlrFlag::REGISTERED, registered_email не выставляется
void RegisterSystem::add(CharData *ch, const char *text, const char *reason) {
	ch->SetFlag(EPlrFlag::kRegistred);
	if (!text || !reason) return;
	std::stringstream out;
	out << GET_NAME(ch) << " -> " << text << " [" << reason << "]";
	auto it = email_list.find(GET_EMAIL(ch));
	if (it == email_list.end()) {
		email_list[GET_EMAIL(ch)] = out.str();
		need_save = true;
	}
}

/**
* Удаление мыла из списка, снятие флага EPlrFlag::kRegistred и registered_email.
* В течении секунды персонаж помещается в комнату незареганных игроков, если он не один с данного ип
*/
void RegisterSystem::remove(CharData *ch) {
	ch->UnsetFlag(EPlrFlag::kRegistred);
	auto it = email_list.find(GET_EMAIL(ch));
	if (it != email_list.end()) {
		email_list.erase(it);
		if (ch->desc) {
			ch->desc->registered_email = false;
		}
		need_save = true;
	}
}

/**
* Проверка, является ли персонаж зарегистрированным каким-нить образом
* \return 0 - нет, 1 - да
*/
bool RegisterSystem::IsRegistered(CharData *ch) {
	if (ch->IsFlagged(EPlrFlag::kRegistred) || (ch->desc && ch->desc->registered_email)) {
		return true;
	}
	return false;
}

/**
* Поиск данного мыла в списке зарегистрированных
* \return 0 - не нашли, 1 - нашли
*/
bool RegisterSystem::IsRegisteredEmail(const std::string &email) {
	auto it = email_list.find(email);
	if (it != email_list.end()) {
		return true;
	}
	return false;
}

/**
* Показ информации по зарегистрированному мылу
* \return строка информации или пустая строка, если ничего не нашли
*/
std::string RegisterSystem::ShowComment(const std::string &email) {
	auto it = email_list.find(email);
	if (it != email_list.end()) {
		return it->second;
	}
	return "";
}

// * Загрузка и релоад списка мыл
void RegisterSystem::load() {
	email_list.clear();

	std::ifstream file(REGISTERED_EMAIL_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", REGISTERED_EMAIL_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	std::string email, comment;
	while (file >> email) {
		ReadEndString(file);
		std::getline(file, comment);
		email_list[email] = comment;
	}
	file.close();
}

// * Сохранение списка мыл, если оно требуется
void RegisterSystem::save() {
	if (!need_save) {
		return;
	}
	std::ofstream file(REGISTERED_EMAIL_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", REGISTERED_EMAIL_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	for (auto &it : email_list) {
		file << it.first << "\n" << it.second << "\n";
	}
	file.close();
	need_save = false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
