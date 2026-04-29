#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_PROTOTYPE_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_PROTOTYPE_H_

#include <string>

namespace lua_prototype {

struct ActorContext {
	std::string name;
	long uid = 0;
	int room_vnum = -1;
};

struct CharacterContext
{
	std::string name;
	long uid = 0;
	int room_vnum = -1;
	int mob_vnum = -1;
	int level = 0;
	int hit = 0;
	int max_hit = 0;
	bool is_npc = false;
	bool charmed = false;
	bool purged = false;
	bool has_death_script = false;
	int death_trigger_count = 0;
};

struct DeathTriggerContext
{
	CharacterContext victim;
	CharacterContext actor;
	bool has_actor = false;
	int iterations = 10000;
};

std::string RunSmokeTest(const ActorContext &actor = {});
std::string RunExpression(const std::string &chunk, const ActorContext &actor);
std::string RunFile(const std::string &path, const ActorContext &actor);
std::string RunDeathTriggerCandidate(const DeathTriggerContext& ctx);
std::string RunDeathMtriggerCandidate(const DeathTriggerContext& ctx);

} // namespace lua_prototype

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_PROTOTYPE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
