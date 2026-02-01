// description.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "description.h"

#include "utils/logger.h"

std::vector<std::string> RoomDescription::_desc_list;
RoomDescription::reboot_map_t RoomDescription::_reboot_map;

/**
* п╢п╬п╠п╟п╡п╩п╣п╫п╦п╣ п╬п©п╦я│п╟п╫п╦я▐ п╡ п╪п╟я│я│п╦п╡ я│ п©я─п╬п╡п╣я─п╨п╬п╧ п╫п╟ я┐п╫п╦п╨п╟п╩я▄п╫п╬я│я┌я▄
* \param text - п╬п©п╦я│п╟п╫п╦п╣ п╨п╬п╪п╫п╟я┌я▀
* \return п╫п╬п╪п╣я─ п╬п©п╦я│п╟п╫п╦я▐ п╡ пЁп╩п╬п╠п╟п╩я▄п╫п╬п╪ п╪п╟я│я│п╦п╡п╣
*/
size_t RoomDescription::add_desc(const std::string &text) {
	reboot_map_t::const_iterator it = _reboot_map.find(text);
	if (it != _reboot_map.end()) {
		return it->second;
	} else {
		_desc_list.push_back(text);
		_reboot_map[text] = _desc_list.size();
		return _desc_list.size();
	}
}

const static std::string empty_string = "";

/**
* п©п╬п╦я│п╨ п╬п©п╦я│п╟п╫п╦я▐ п©п╬ п╣пЁп╬ п©п╬я─я▐п╢п╨п╬п╡п╬п╪я┐ п╫п╬п╪п╣я─я┐ п╡ п╪п╟я│я│п╦п╡п╣
* \param desc_num - п©п╬я─я▐п╢п╨п╬п╡я▀п╧ п╫п╬п╪п╣я─ п╬п©п╦я│п╟п╫п╦я▐ (descripton_num п╡ room_data)
* \return я│я┌я─п╬п╨п╟ п╬п©п╦я│п╟п╫п╦я▐ п╦п╩п╦ п©я┐я│я┌п╟я▐ я│я┌я─п╬п╨п╟ п╡ я│п╩я┐я┤п╟п╣ п╫п╣п╡п╟п╩п╦п╢п╫п╬пЁп╬ п╫п╬п╪п╣я─п╟
*/
const std::string &RoomDescription::show_desc(size_t desc_num) {
	try {
		return _desc_list.at(--desc_num);
	}
	catch (const std::out_of_range &) {
		log("SYSERROR : bad room description num '%zd' (%s %s %d)", desc_num, __FILE__, __func__, __LINE__);
		return empty_string;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
