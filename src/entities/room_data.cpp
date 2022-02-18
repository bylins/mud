// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "room_data.h"

ExitData::ExitData() : keyword(nullptr),
					   vkeyword(nullptr),
					   exit_info(0),
					   lock_complexity(0),
					   key(-1),
					   to_room_(kNowhere) {
}

ExitData::~ExitData() {
	if (keyword != nullptr)
		free(keyword);
	if (vkeyword != nullptr)
		free(vkeyword);
}

void ExitData::set_keyword(std::string const &value) {
	if (keyword != nullptr)
		free(keyword);
	keyword = nullptr;
	if (!value.empty())
		keyword = strdup(value.c_str());
}

void ExitData::set_vkeyword(std::string const &value) {
	if (vkeyword != nullptr)
		free(vkeyword);
	vkeyword = nullptr;
	if (!value.empty())
		vkeyword = strdup(value.c_str());
}

void ExitData::set_keywords(std::string const &value) {
	if (value.empty()) {
		if (keyword != nullptr)
			free(keyword);
		keyword = nullptr;
		if (vkeyword != nullptr)
			free(vkeyword);
		vkeyword = nullptr;
		return;
	}

	std::size_t i = value.find('|');
	if (i != std::string::npos) {
		set_keyword(value.substr(0, i));
		set_vkeyword(value.substr(++i));
	} else {
		set_keyword(value);
		set_vkeyword(value);
	}
}

RoomRnum ExitData::to_room() const {
	return to_room_;
}

void ExitData::to_room(const RoomRnum _) {
	to_room_ = _;
}

RoomData::RoomData() : room_vn(0),
					   zone_rn(0),
					   sector_type(0),
					   sector_state(0),
					   name(nullptr),
					   description_num(0),
					   temp_description(nullptr),
					   ex_description(nullptr),
					   light(0),
					   glight(0),
					   gdark(0),
					   func(nullptr),
					   proto_script(new ObjData::triggers_list_t()),
					   script(new Script()),
					   track(nullptr),
					   contents(nullptr),
					   people(0),
					   affected(0),
					   fires(0),
					   ices(0),
					   portal_room(0),
					   portal_time(0),
					   pkPenterUnique(0),
					   holes(0),
					   poison(0) {
	for (auto i = 0; i < kDirMaxNumber; ++i) {
		dir_option[i].reset();
	}

	memset(&weather, 0, sizeof(WeatherControl));
	memset(&base_property, 0, sizeof(RoomState));
	memset(&add_property, 0, sizeof(RoomState));
}

RoomData::~RoomData() {
	if (name != nullptr)
		free(name);
}

CharData *RoomData::first_character() const {
	CharData *first = people.empty() ? nullptr : *people.begin();

	return first;
}

void RoomData::cleanup_script() {
	script->cleanup();
}

void RoomData::set_name(std::string const &value) {
	if (name != nullptr)
		free(name);
	name = strdup(value.c_str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
