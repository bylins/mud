/**
\file do_proxy.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Команда регистрации прокси. Должна использовать административную механику из proxy.
*/

#include "administration/proxy.h"

#include "engine/entities/char_data.h"
#include "engine/ui/modify.h"

#include <third_party_libs/fmt/include/fmt/format.h>

void do_proxy(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	if (CompareParam(buffer2, "list") || CompareParam(buffer2, "список")) {
		std::ostringstream out;
		out << "Формат списка: IP | IP2 | Максимум соединений | Комментарий\r\n";

		for (const auto &it : proxy_list) {
			out << fmt::format(" {:<15}   {:<15}   {:<2}   {}\r\n",
							   it.second->textIp, it.second->textIp2, it.second->num, it.second->text);
		}
		page_string(ch->desc, out.str());

	} else if (CompareParam(buffer2, "add") || CompareParam(buffer2, "добавить")) {
		GetOneParam(buffer, buffer2);
		if (buffer2.empty()) {
			SendMsgToChar("Укажите регистрируемый IP адрес или параметр mask|маска.\r\n", ch);
			return;
		}
		// случай добавления диапазона ип
		std::string textIp, textIp2;
		if (CompareParam(buffer2, "mask") || CompareParam(buffer2, "маска")) {
			GetOneParam(buffer, buffer2);
			if (buffer2.empty()) {
				SendMsgToChar("Укажите начало диапазона.\r\n", ch);
				return;
			}
			textIp = buffer2;
			// должен быть второй ип
			GetOneParam(buffer, buffer2);
			if (buffer2.empty()) {
				SendMsgToChar("Укажите конец диапазона.\r\n", ch);
				return;
			}
			textIp2 = buffer2;
		} else {// если это не диапазон и не пустая строка - значит это простой ип
			textIp = buffer2;
		}

		GetOneParam(buffer, buffer2);
		if (buffer2.empty()) {
			SendMsgToChar("Укажите максимальное кол-во коннектов.\r\n", ch);
			return;
		}
		int num = atoi(buffer2.c_str());
		if (num < 2 || num > kMaxProxyConnect) {
			SendMsgToChar(ch, "Некорректное число коннектов (2-%d).\r\n", kMaxProxyConnect);
			return;
		}

		utils::TrimIf(buffer, " \'");
		if (buffer.empty()) {
			SendMsgToChar("Укажите причину регистрации.\r\n", ch);
			return;
		}
		char timeBuf[11];
		time_t now = time(nullptr);
		strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y", localtime(&now));

		ProxyIpPtr tempIp(new ProxyIp);
		tempIp->text = buffer + " [" + GET_NAME(ch) + " " + timeBuf + "]";
		tempIp->num = num;
		tempIp->textIp = textIp;
		tempIp->textIp2 = textIp2;
		unsigned long ip = TxtToIp(textIp);
		unsigned long ip2 = TxtToIp(textIp2);
		if (ip2 && ((ip2 - ip) > 65535)) {
			SendMsgToChar("Слишком широкий диапазон. (Максимум x.x.0.0. до x.x.255.255).\r\n", ch);
			return;
		}
		tempIp->ip2 = ip2;
		proxy_list[ip] = tempIp;
		SaveProxyList();
		SendMsgToChar("Запись добавлена.\r\n", ch);

	} else if (CompareParam(buffer2, "remove") || CompareParam(buffer2, "удалить")) {
		GetOneParam(buffer, buffer2);
		if (buffer2.empty()) {
			SendMsgToChar("Укажите удаляемый IP адрес или начало диапазона.\r\n", ch);
			return;
		}
		unsigned long ip = TxtToIp(buffer2);
		auto it = proxy_list.find(ip);
		if (it == proxy_list.end()) {
			SendMsgToChar("Данный IP/диапазон и так не зарегистрирован.\r\n", ch);
			return;
		}
		proxy_list.erase(it);
		SaveProxyList();
		SendMsgToChar("Запись удалена.\r\n", ch);

	} else
		SendMsgToChar("Формат: proxy <list|список>\r\n"
					  "        proxy <add|добавить> ip <количество коннектов> комментарий\r\n"
					  "        proxy <add|добавить> <mask|маска> <начало диапазона> <конец диапазона> <количество коннектов> комментарий\r\n"
					  "        proxy <remove|удалить> ip\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
