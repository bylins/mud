local actor_name = actor and actor.name or "unknown"
local actor_uid = actor and actor.uid or -1
local actor_room = actor and actor.room_vnum or -1

return string.format(
	"hello from lua: actor=%s uid=%d room=%d sum=%d",
	actor_name,
	actor_uid,
	actor_room,
	cpp_add(20, 22)
)
