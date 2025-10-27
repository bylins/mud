/**
\file do_date.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "do_date.h"

#include "engine/entities/char_data.h"
#include "administration/shutdown_parameters.h"

void DoPageDateTime(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd) {
	time_t mytime;
	std::ostringstream out;

	if (subcmd == SCMD_DATE) {
		mytime = time(nullptr);
		out << "Текущее время сервера: " << asctime(localtime(&mytime)) << "\r\n";
	} else {
		mytime = shutdown_parameters.get_boot_time();
		out << " Up since: " << asctime(localtime(&mytime));
		out.seekp(-1, std::ios_base::end); // Удаляем \0 из конца строки.
		out << " ";
		PrintUptime(out);
	}

	SendMsgToChar(out.str(), ch);
}

void PrintUptime(std::ostringstream &out) {
	auto uptime = time(nullptr) - shutdown_parameters.get_boot_time();
	auto d = uptime / 86400;
	auto h = (uptime / 3600) % 24;
	auto m = (uptime / 60) % 60;
	auto s = uptime % 60;

	out << std::setprecision(2) << d << "д " << h << ":" << m << ":" << s << "\r\n";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
