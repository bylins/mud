/**
\file do_loadstat.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/config.h"
#include "utils/utils_time.h"

#include <fstream>

void DoLoadstat(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	const std::string profiler_path = runtime_config.log_dir() + "/" + LOAD_LOG_FILE;
	std::ifstream istream(profiler_path, std::ifstream::in);
	int length;

	if (!istream.is_open()) {
		SendMsgToChar("Can't open file", ch);
		log("ERROR: Can't open file %s", profiler_path.c_str());
		return;
	}

	istream.seekg(0, std::ifstream::end);
	length = istream.tellg();
	istream.seekg(0, std::ifstream::beg);
	std::string content(std::min(length, kMaxStringLength - 1), '\0');
	istream.read(content.data(), static_cast<std::streamsize>(content.size()));
	content.resize(istream.gcount());
	SendMsgToChar(content, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
