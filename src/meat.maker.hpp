#ifndef ___MEAT_MAKER_HPP__
#define ___MEAT_MAKER_HPP__

#include "structs.h"

#include <unordered_map>

class MeatMapping : private std::unordered_map<obj_vnum, obj_vnum>
{
public:
	using meat_mapping_t = std::pair<obj_vnum, obj_vnum>;
	using raw_mapping_t = std::vector<meat_mapping_t>;

	const static obj_vnum ARTEFACT_KEY = 324;
	const static raw_mapping_t RAW_MAPPING;
	
	MeatMapping();

	using base_t = std::unordered_map<obj_vnum, obj_vnum>;
	using base_t::size;

	bool has(const key_type from) const { return end() != find(from); }
	mapped_type get(const key_type from) const { return at(from); }
	key_type random_key() const;
	key_type get_artefact_key() const { return ARTEFACT_KEY; }

private:
	void build_randomly_returnable_keys_index();
	std::vector<obj_vnum> m_randomly_returnable_keys;
};

extern MeatMapping meat_mapping;

#endif // ___MEAT_MAKER_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
