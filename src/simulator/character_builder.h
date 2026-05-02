#ifndef BYLINS_SIMULATOR_CHARACTER_BUILDER_H
#define BYLINS_SIMULATOR_CHARACTER_BUILDER_H

#include "engine/entities/char_data.h"

namespace simulator {

class CharacterBuilder {
public:
	using character_t = CharData;
	using result_t = character_t::shared_ptr;

	void create_new();
	void create_new_with_class(const short player_class);
	void create_character_with_one_removable_affect();
	void create_character_with_two_removable_affects();
	void create_character_with_two_removable_and_two_not_removable_affects();

	void add_poison();
	void add_sleep();
	void add_detect_invis();
	void add_detect_align();
	void set_level(const int level);
	void set_class(const short player_class);
	void set_str(const int value);
	void set_dex(const int value);
	void set_con(const int value);
	void set_int(const int value);
	void set_wis(const int value);
	void set_cha(const int value);
	void set_hit(const int value);
	void set_max_hit(const int value);
	void set_name(const std::string& name);
	void set_remort(const int value);

	// Convenience preset for the headless balance simulator: creates a fresh
	// character of the given class and level with all six stats at 25 (mid of
	// the 0..90 range), and grants all class skills (at 200 / max) and
	// inborn-eligible class feats available at that level. HP is left at the
	// constructor default; callers that care should follow up with
	// set_max_hit()/set_hit().
	void make_basic_player(const short player_class, const int level);

	// Grants every class skill of m_result's current class at value 200 and
	// every class feat for which m_result meets the level/remort requirements.
	// Called by make_basic_player(); exposed separately so a caller can build
	// a character manually (set_class+set_level+stats) and then unlock skills.
	void grant_class_skills_and_feats();

	// Requires a booted world (calls PlaceCharToRoom).
	void place_in_room(const RoomRnum room);

	void make_group(CharacterBuilder& character_builder);

	result_t get() const { return m_result; }

private:
	void check_character_existance() const;

	static void check_character_existance(result_t character);

	result_t m_result;
};

}  // namespace simulator

#endif  // BYLINS_SIMULATOR_CHARACTER_BUILDER_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
