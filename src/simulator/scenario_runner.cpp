#include "scenario_runner.h"

#include "engine/core/handler.h"
#include "engine/core/iosystem.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "engine/db/world_characters.h"
#include "engine/network/descriptor_data.h"
#include "engine/observability/event_sink.h"
#include "simulator/character_builder.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/room_data.h"
#include "engine/structs/structs.h"
#include "engine/ui/cmd/do_cast.h"
#include "engine/ui/cmd/do_equip.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/fight/fight.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/magic/spells.h"
#include "utils/utils.h"
#include "utils/logger.h"

#include <fmt/format.h>

#include <chrono>
#include <climits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace simulator {

namespace {

// Room rnum used to place participants. rnum 1 is what the in-engine `vstat`
// command uses for the same temp-spawn pattern, so we follow suit.
constexpr RoomRnum kArenaRoom = 1;

// Some real-world rooms have flags that restrict combat or magic
// (kNoMagic, kPeaceful, etc.). For a balance arena we want the cleanest
// possible environment, so we strip such flags from kArenaRoom before each
// run. The flags are restored on cleanup so we do not corrupt the loaded
// world for subsequent runs.
struct ArenaFlagSweep {
	RoomData* room = nullptr;
	bool had_no_magic = false;
	bool had_peaceful = false;

	explicit ArenaFlagSweep(RoomData* r) : room(r) {
		if (!room) {
			return;
		}
		had_no_magic = room->get_flag(ERoomFlag::kNoMagic);
		had_peaceful = room->get_flag(ERoomFlag::kPeaceful);
		if (had_no_magic) {
			room->unset_flag(ERoomFlag::kNoMagic);
		}
		if (had_peaceful) {
			room->unset_flag(ERoomFlag::kPeaceful);
		}
	}

	~ArenaFlagSweep() {
		if (!room) {
			return;
		}
		if (had_no_magic) {
			room->set_flag(ERoomFlag::kNoMagic);
		}
		if (had_peaceful) {
			room->set_flag(ERoomFlag::kPeaceful);
		}
	}
};

std::int64_t NowUnixMs();  // forward decl, defined below

// Headless DescriptorData attached to a synthetic PC so SendMsgToChar/act()
// have somewhere to write. Without it ch->desc is nullptr and combat output
// is silently dropped. Pattern follows admin_api/crud_handlers.cpp: just
// allocate the struct, point output at small_outbuf, set state to kPlaying.
// No socket fd is used (descriptor = -1); the buffered text is read out
// after each round and emitted as a 'screen_output' event so the web-UI
// replay viewer can show what each PC saw at each moment.
class HeadlessDescriptor {
public:
	void Attach(CharData* ch) {
		ch_ = ch;
		d_ = new DescriptorData();
		d_->descriptor = -1;
		d_->state = EConState::kPlaying;
		d_->output = d_->small_outbuf;
		d_->bufptr = 0;
		d_->bufspace = kSmallBufsize - 1;
		d_->keytable = 0;  // raw KOI8-R; converted to UTF-8 at flush time
		d_->small_outbuf[0] = '\0';
		ch->desc = d_;
	}

	bool IsAttached() const { return d_ != nullptr; }

	// Reads accumulated output, emits a 'screen_output' event (if non-empty
	// and at least one sink is registered), returns large_outbuf to bufpool,
	// resets the buffer for the next round.
	void FlushAndEmit(const char* role, int round_no) {
		if (!d_) return;
		const bool has_text = (d_->bufptr > 0
			&& d_->bufptr != static_cast<std::size_t>(~0ull));
		if (has_text && observability::HasAnyEventSink()) {
			std::string text(d_->output, d_->bufptr);
			observability::Event e;
			e.name = "screen_output";
			e.ts_unix_ms = NowUnixMs();
			e.attrs["round"] = static_cast<std::int64_t>(round_no);
			e.attrs["role"] = std::string(role);
			e.attrs["target_name"] = observability::EngineStringToUtf8(
				GET_NAME(ch_) ? GET_NAME(ch_) : "");
			e.attrs["text"] = observability::EngineStringToUtf8(text);
			observability::EmitToAllSinks(e);
		}
		ResetBuffer();
	}

	// Discard buffered text without emitting (used to drop spawn/setup spam
	// before the actual combat rounds start).
	void Discard() { ResetBuffer(); }

	void Detach() {
		if (!d_) return;
		if (ch_) ch_->desc = nullptr;
		// Returns large_outbuf to bufpool, clears input queue.
		iosystem::flush_queues(d_);
		delete d_;
		d_ = nullptr;
		ch_ = nullptr;
	}

	~HeadlessDescriptor() { Detach(); }

private:
	void ResetBuffer() {
		if (!d_) return;
		if (d_->large_outbuf) {
			// Returns the large buffer to bufpool and resets large_outbuf
			// to nullptr; also clears input queue but we never had any.
			iosystem::flush_queues(d_);
		}
		d_->output = d_->small_outbuf;
		d_->bufptr = 0;
		d_->bufspace = kSmallBufsize - 1;
		d_->small_outbuf[0] = '\0';
	}

	DescriptorData* d_ = nullptr;
	CharData* ch_ = nullptr;
};

// Huge HP pool: both participants must survive `scenario.rounds` battle rounds
// so we can observe the full duel. Headroom away from INT_MAX avoids
// arithmetic overflow inside the engine.
constexpr int kHugeHp = INT_MAX / 4;

std::int64_t NowUnixMs() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}


// Spawn a participant and register it with the global character_list so its
// shared_ptr is owned globally for the duration of the run. Returns the raw
// pointer; ownership stays with character_list until ExtractCharFromWorld is
// called.
// Returns the max_hit override if set in the spec, -1 otherwise.
int GetMaxHitOverride(const ParticipantSpec& spec) {
	return std::visit([](auto&& s) { return s.overrides.max_hit; }, spec);
}

// Latin/English aliases for class names so YAML can stay ASCII-friendly.
// FindAvailableCharClassId compares against the Russian name from pc_*.xml
// (KOI8-R bytes); for an alias we look it up here and feed the Russian name
// into the engine resolver. Map values are KOI8-R byte sequences hardcoded
// as escape sequences to keep this file ASCII-clean.
std::string ResolveClassAlias(const std::string& input) {
	static const std::map<std::string, std::string> kAliases = {
		// kWarrior -- bogatyr (богатырь)
		{"bogatyr",       "\xC2\xCF\xC7\xC1\xD4\xD9\xD2\xD8"},
		{"warrior",       "\xC2\xCF\xC7\xC1\xD4\xD9\xD2\xD8"},
		// kAssasine -- naemnik (наемник)
		{"naemnik",       "\xCE\xC1\xC5\xCD\xCE\xC9\xCB"},
		{"assassin",      "\xCE\xC1\xC5\xCD\xCE\xC9\xCB"},
		// kCharmer -- kudesnik (кудесник)
		{"kudesnik",      "\xCB\xD5\xC4\xC5\xD3\xCE\xC9\xCB"},
		{"charmer",       "\xCB\xD5\xC4\xC5\xD3\xCE\xC9\xCB"},
		// kConjurer -- koldun (колдун)
		{"koldun",        "\xCB\xCF\xCC\xC4\xD5\xCE"},
		{"conjurer",      "\xCB\xCF\xCC\xC4\xD5\xCE"},
		// kSorcerer -- lekar (лекарь)
		{"lekar",         "\xCC\xC5\xCB\xC1\xD2\xD8"},
		{"sorcerer",      "\xCC\xC5\xCB\xC1\xD2\xD8"},
		{"healer",        "\xCC\xC5\xCB\xC1\xD2\xD8"},
		// kRanger -- ohotnik (охотник)
		{"ohotnik",       "\xCF\xC8\xCF\xD4\xCE\xC9\xCB"},
		{"ranger",        "\xCF\xC8\xCF\xD4\xCE\xC9\xCB"},
		// kMagus -- volkhv (волхв)
		{"volkhv",        "\xD7\xCF\xCC\xC8\xD7"},
		{"magus",         "\xD7\xCF\xCC\xC8\xD7"},
		// kGuard -- druzhinnik (дружинник)
		{"druzhinnik",    "\xC4\xD2\xD5\xD6\xC9\xCE\xCE\xC9\xCB"},
		{"guard",         "\xC4\xD2\xD5\xD6\xC9\xCE\xCE\xC9\xCB"},
		// kMerchant -- kupets (купец)
		{"kupets",        "\xCB\xD5\xD0\xC5\xC3"},
		{"merchant",      "\xCB\xD5\xD0\xC5\xC3"},
		// kNecromancer -- chernoknizhnik (чернокнижник)
		{"chernoknizhnik","\xDE\xC5\xD2\xCE\xCF\xCB\xCE\xC9\xD6\xCE\xC9\xCB"},
		{"necromancer",   "\xDE\xC5\xD2\xCE\xCF\xCB\xCE\xC9\xD6\xCE\xC9\xCB"},
		// kPaladine -- vityaz (витязь)
		{"vityaz",        "\xD7\xC9\xD4\xD1\xDA\xD8"},
		{"paladine",      "\xD7\xC9\xD4\xD1\xDA\xD8"},
		{"paladin",       "\xD7\xC9\xD4\xD1\xDA\xD8"},
		// kThief -- tat (тать)
		{"tat",           "\xD4\xC1\xD4\xD8"},
		{"thief",         "\xD4\xC1\xD4\xD8"},
		// kVigilant -- kuznets (кузнец)
		{"kuznets",       "\xCB\xD5\xDA\xCE\xC5\xC3"},
		{"vigilant",      "\xCB\xD5\xDA\xCE\xC5\xC3"},
		{"smith",         "\xCB\xD5\xDA\xCE\xC5\xC3"},
		// kWizard -- volshebnik (волшебник)
		{"volshebnik",    "\xD7\xCF\xCC\xDB\xC5\xC2\xCE\xC9\xCB"},
		{"wizard",        "\xD7\xCF\xCC\xDB\xC5\xC2\xCE\xC9\xCB"},
	};
	const auto it = kAliases.find(input);
	return it != kAliases.end() ? it->second : input;
}

const std::vector<PetSpec>& GetPetSpecs(const ParticipantSpec& spec) {
	return std::visit([](auto&& s) -> const std::vector<PetSpec>& {
		return s.pets;
	}, spec);
}

const std::vector<int>& GetInventory(const ParticipantSpec& spec) {
	return std::visit([](auto&& s) -> const std::vector<int>& {
		return s.inventory;
	}, spec);
}

// Equip every item from `vnums` on `ch`. Each item is created from prototype
// via world_objects.create_from_prototype_by_vnum, then auto-fitted into the
// first slot its wear-flags allow (same logic equip_start_outfit uses for
// noob outfits). Items that do not fit any slot stay in inventory.
void EquipFromVnums(CharData* ch, const std::vector<int>& vnums) {
	for (int vnum : vnums) {
		const auto obj = world_objects.create_from_prototype_by_vnum(vnum);
		if (!obj) {
			throw ScenarioRunError(fmt::format(
				"inventory item vnum {} not found in world", vnum));
		}
		PlaceObjToInventory(obj.get(), ch);
		// find_eq_pos handles armor/jewellery slots but does NOT handle
		// weapon slots (wield/both/hold/light). For weapons we replicate
		// the choice equip_start_outfit makes for noob outfits.
		int where = -1;
		if (obj->get_type() == EObjType::kWeapon) {
			if (CAN_WEAR(obj.get(), EWearFlag::kWield) && !GET_EQ(ch, EEquipPos::kWield)) {
				where = EEquipPos::kWield;
			} else if (CAN_WEAR(obj.get(), EWearFlag::kBoth) && !GET_EQ(ch, EEquipPos::kBoths)) {
				where = EEquipPos::kBoths;
			} else if (CAN_WEAR(obj.get(), EWearFlag::kHold) && !GET_EQ(ch, EEquipPos::kHold)) {
				where = EEquipPos::kHold;
			}
		} else {
			where = find_eq_pos(ch, obj.get(), nullptr);
		}
		if (where >= 0 && !GET_EQ(ch, where)) {
			RemoveObjFromChar(obj.get());
			EquipObj(ch, obj.get(), where, CharEquipFlags());
		}
	}
}

// Apply YAML stat overrides to a freshly-spawned character. Each negative
// override is left alone (engine default kept).
void ApplyStatOverrides(CharData* ch, const StatOverrides& o) {
	if (o.str >= 0) ch->set_str(o.str);
	if (o.dex >= 0) ch->set_dex(o.dex);
	if (o.con >= 0) ch->set_con(o.con);
	if (o.intel >= 0) ch->set_int(o.intel);
	if (o.wis >= 0) ch->set_wis(o.wis);
	if (o.cha >= 0) ch->set_cha(o.cha);
	if (o.max_hit >= 0) {
		ch->set_max_hit(o.max_hit);
		ch->set_hit(o.max_hit);
	}
}

// Spawn one pet (charmie) of a given vnum, attach it as a follower of `owner`,
// place into the arena room. Same recipe the engine performs at the end of
// the kCharm spell -- but we skip the cast itself, so the scenario stays
// declarative (the pet is already loyal from the start).
CharData* SpawnPet(CharData* owner, const PetSpec& spec, RoomRnum room) {
	const auto rnum = GetMobRnum(spec.vnum);
	if (rnum < 0) {
		throw ScenarioRunError(fmt::format(
			"pet vnum {} not found in world", spec.vnum));
	}
	CharData* pet = ReadMobile(rnum, kReal);
	if (!pet) {
		throw ScenarioRunError(fmt::format(
			"ReadMobile({}) returned null for pet", spec.vnum));
	}
	ApplyStatOverrides(pet, spec.overrides);
	PlaceCharToRoom(pet, room);

	Affect<EApply> af;
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = 0;
	af.type = ESpell::kCharm;
	af.duration = 999;  // long enough that no scenario will outlive it
	af.bitvector = to_underlying(EAffect::kCharmed);
	affect_to_char(pet, af);
	owner->add_follower(pet);

	return pet;
}

CharData* SpawnParticipant(const ParticipantSpec& spec) {
	return std::visit([](auto&& s) -> CharData* {
		using T = std::decay_t<decltype(s)>;
		if constexpr (std::is_same_v<T, PlayerSpec>) {
			const auto resolved_name = ResolveClassAlias(s.class_name);
			const auto cls = FindAvailableCharClassId(resolved_name);
			if (cls == ECharClass::kUndefined) {
				throw ScenarioRunError(fmt::format(
					"unknown player class: '{}'", s.class_name));
			}
			CharacterBuilder b;
			b.make_basic_player(static_cast<short>(cls), s.level);
			b.set_remort(s.remort);
			// grant_class_skills_and_feats читает уровень И реморт PC --
			// зовём его после set_remort, иначе высоко-ремортные фиты,
			// скиллы и спеллы выпали бы, и реморт из YAML ничего не
			// поменял бы в бою.
			b.grant_class_skills_and_feats();
			auto sp = b.get();
			character_list.push_front(sp);
			ApplyStatOverrides(sp.get(), s.overrides);
			return sp.get();
		} else if constexpr (std::is_same_v<T, MobSpec>) {
			const auto rnum = GetMobRnum(s.vnum);
			if (rnum < 0) {
				throw ScenarioRunError(fmt::format(
					"mob vnum {} not found in world", s.vnum));
			}
			CharData* mob = ReadMobile(rnum, kReal);
			if (!mob) {
				throw ScenarioRunError(fmt::format(
					"ReadMobile({}) returned null", s.vnum));
			}
			ApplyStatOverrides(mob, s.overrides);
			return mob;
		}
		return static_cast<CharData*>(nullptr);
	}, spec);
}

void AddParticipantAttrs(observability::Event& e, const char* role, const ParticipantSpec& spec) {
	std::visit([&e, role](auto&& s) {
		using T = std::decay_t<decltype(s)>;
		if constexpr (std::is_same_v<T, PlayerSpec>) {
			e.attrs[std::string(role) + "_type"] = std::string("player");
			e.attrs[std::string(role) + "_class"] = observability::EngineStringToUtf8(s.class_name);
			e.attrs[std::string(role) + "_level"] = static_cast<std::int64_t>(s.level);
		} else if constexpr (std::is_same_v<T, MobSpec>) {
			e.attrs[std::string(role) + "_type"] = std::string("mob");
			e.attrs[std::string(role) + "_vnum"] = static_cast<std::int64_t>(s.vnum);
		}
	}, spec);
}

// Periodic snapshot of a participant's state for replay-mode reconstruction.
// Emits hp/move/position; the full per-affect timeline is already covered by
// affect_added/removed events from src/gameplay/affects/affect_data.cpp.
void EmitCharState(const char* role,
		const CharData* ch, int round_no) {
	if (!observability::HasAnyEventSink()) return;
	observability::Event e;
	e.name = "char_state";
	e.ts_unix_ms = NowUnixMs();
	e.attrs["round"] = static_cast<std::int64_t>(round_no);
	e.attrs["role"] = std::string(role);
	e.attrs["target_name"] = observability::EngineStringToUtf8(
		GET_NAME(ch) ? GET_NAME(ch) : "");
	// Identity: PC vs NPC + display fields. У PC имя задано simulator'ом
	// ('attacker' / 'victim'), уровень и класс полезны для контекста.
	// У моба важен vnum + short_descr (полное имя из прототипа).
	e.attrs["is_npc"] = ch->IsNpc();
	if (ch->IsNpc()) {
		e.attrs["vnum"] = static_cast<std::int64_t>(GET_MOB_VNUM(ch));
		e.attrs["short_descr"] = observability::EngineStringToUtf8(
			ch->get_npc_name());
	} else {
		e.attrs["vnum"] = static_cast<std::int64_t>(-1);
		e.attrs["short_descr"] = std::string();
	}
	e.attrs["level"] = static_cast<std::int64_t>(ch->GetLevel());
	// Реморт PC -- часть "уровня прокачки", влияет на выданные скиллы
	// и фиты. Веб-UI показывает рядом с уровнем.
	e.attrs["remort"] = static_cast<std::int64_t>(ch->IsNpc() ? 0 : ch->get_remort());
	e.attrs["class_name"] = observability::EngineStringToUtf8(
		ch->IsNpc() ? std::string() : MUD::Class(ch->GetClass()).GetName());
	e.attrs["hp"] = static_cast<std::int64_t>(ch->get_hit());
	e.attrs["max_hp"] = static_cast<std::int64_t>(ch->get_max_hit());
	e.attrs["move"] = static_cast<std::int64_t>(ch->get_move());
	e.attrs["max_move"] = static_cast<std::int64_t>(ch->get_max_move());
	e.attrs["position"] = static_cast<std::int64_t>(ch->GetPosition());
	e.attrs["in_room"] = static_cast<std::int64_t>(ch->in_room);
	// Эффективные статы (база + апплаи от шмота / аффектов), как их видит
	// боевой код через GetRealStr/Dex/... Веб-UI рисует их в state-панели,
	// иначе апплаи со шмота (+1 str у штанов воина) не были бы видны.
	e.attrs["str"] = static_cast<std::int64_t>(GetRealStr(ch));
	e.attrs["dex"] = static_cast<std::int64_t>(GetRealDex(ch));
	e.attrs["con"] = static_cast<std::int64_t>(GetRealCon(ch));
	e.attrs["int"] = static_cast<std::int64_t>(GetRealInt(ch));
	e.attrs["wis"] = static_cast<std::int64_t>(GetRealWis(ch));
	e.attrs["cha"] = static_cast<std::int64_t>(GetRealCha(ch));
	// Базовые тоже эмитим -- иногда полезно видеть, что override из YAML
	// применился до апплаев.
	e.attrs["str_base"] = static_cast<std::int64_t>(ch->get_str());
	e.attrs["dex_base"] = static_cast<std::int64_t>(ch->get_dex());
	e.attrs["con_base"] = static_cast<std::int64_t>(ch->get_con());
	e.attrs["int_base"] = static_cast<std::int64_t>(ch->get_int());
	e.attrs["wis_base"] = static_cast<std::int64_t>(ch->get_wis());
	e.attrs["cha_base"] = static_cast<std::int64_t>(ch->get_cha());
	// Прибавки (hitroll/damroll) от шмота, важно для штанов воина (+2/+2).
	e.attrs["hitroll"] = static_cast<std::int64_t>(ch->add_abils.hr_add);
	e.attrs["damroll"] = static_cast<std::int64_t>(ch->add_abils.dr_add);
	// Доп. прибавки от шмота: HP, AC, мораль, регены.
	e.attrs["hit_add"]    = static_cast<std::int64_t>(ch->add_abils.hit_add);
	e.attrs["ac_add"]     = static_cast<std::int64_t>(ch->add_abils.ac_add);
	e.attrs["armour_add"] = static_cast<std::int64_t>(ch->add_abils.armour);
	e.attrs["morale_add"] = static_cast<std::int64_t>(ch->add_abils.morale);
	e.attrs["initiative_add"] = static_cast<std::int64_t>(ch->add_abils.initiative_add);
	// Доп. поля для верификации применения feat-апплаев (kPowerMagic +50%
	// percent_spellpower_add для колдуна и т.п.). Полезно когда сравниваешь
	// dpr классов и подозреваешь, что какой-то inborn feat не применился.
	e.attrs["spellpower_add_pct"] = static_cast<std::int64_t>(
		ch->add_abils.percent_spellpower_add);
	e.attrs["physdam_add_pct"] = static_cast<std::int64_t>(
		ch->add_abils.percent_physdam_add);
	// Список выданных способностей -- веб-UI показывает их в state-панели
	// чтобы было видно, чем класс отличается на этом уровне.
	std::string feats_list;
	for (const auto& feat : MUD::Feats()) {
		if (ch->HaveFeat(feat.GetId())) {
			if (!feats_list.empty()) feats_list += "|";
			feats_list += feat.GetName();
		}
	}
	e.attrs["feats_list"] = observability::EngineStringToUtf8(feats_list);
	// Список активных аффектов с их типом (spell name) и оставшейся
	// длительностью; '|'-разделитель -- web-UI парсит и рисует chip'ами.
	std::string aff_list;
	int aff_count = 0;
	for (const auto& a : ch->affected) {
		++aff_count;
		if (!aff_list.empty()) aff_list += "|";
		aff_list += MUD::Spell(a->type).GetCName();
		aff_list += fmt::format(" ({}t)", a->duration);
	}
	e.attrs["affects_count"] = static_cast<std::int64_t>(aff_count);
	e.attrs["affects_list"] = observability::EngineStringToUtf8(aff_list);
	// Список одетых предметов: 'slot:vnum:name', разделители '|'. Веб-UI
	// рендерит их в state-панели чтобы было видно, чем именно бьётся /
	// защищается персонаж.
	static const char* kSlotNames[EEquipPos::kNumEquipPos] = {
		"light", "fingerR", "fingerL", "neck", "chest", "body", "head",
		"legs", "feet", "hands", "arms", "shield", "shoulders", "waist",
		"wristR", "wristL", "wield", "hold", "both", "quiver",
	};
	std::string equip_list;
	for (int slot = 0; slot < EEquipPos::kNumEquipPos; ++slot) {
		const auto* obj = ch->equipment[slot];
		if (!obj) continue;
		if (!equip_list.empty()) equip_list += "|";
		equip_list += fmt::format("{}:{}:{}",
			kSlotNames[slot],
			obj->get_vnum(),
			obj->get_short_description());
	}
	e.attrs["equip_list"] = observability::EngineStringToUtf8(equip_list);
	e.attrs["aff_silence"] = AFF_FLAGGED(ch, EAffect::kSilence) ? true : false;
	e.attrs["aff_charmed"] = AFF_FLAGGED(ch, EAffect::kCharmed) ? true : false;
	e.attrs["aff_sleep"] = AFF_FLAGGED(ch, EAffect::kSleep) ? true : false;
	// Affect-flags from items / passive sources do NOT show up in
	// ch->affected[] (which only holds spell-Affect entries), but they're
	// in AFF_FLAGS(ch). Walk a fixed list of "interesting" flags so the
	// state-panel picks up "ускорение" / "каменные руки" / etc. that come
	// from worn equipment.
	struct AffEntry { EAffect flag; const char* name; };
	static const AffEntry kAffectFlags[] = {
		{EAffect::kHaste,           "ускорение"},
		{EAffect::kStoneHands,      "каменные руки"},
		{EAffect::kWaterBreath,     "дыхание под водой"},
		{EAffect::kFly,             "полёт"},
		{EAffect::kBless,           "доблесть"},
		{EAffect::kBlink,           "мерцание"},
		{EAffect::kSanctuary,       "святилище"},
		{EAffect::kAirShield,       "воздушный щит"},
		{EAffect::kFireShield,      "огненный щит"},
		{EAffect::kIceShield,       "ледяной щит"},
		{EAffect::kMagicGlass,      "зеркало магии"},
		{EAffect::kPrismaticAura,   "призматическая аура"},
		{EAffect::kHolyLight,       "святой свет"},
		{EAffect::kHolyDark,        "святая тьма"},
		{EAffect::kDetectAlign,     "видеть наклонности"},
		{EAffect::kDetectInvisible, "видеть невидимость"},
		{EAffect::kDetectMagic,     "видеть магию"},
		{EAffect::kDetectLife,      "видеть живых"},
		{EAffect::kInfravision,     "инфразрение"},
		{EAffect::kWaterWalk,       "хождение по воде"},
		{EAffect::kInvisible,       "невидимость"},
		{EAffect::kSneak,           "подкрадывание"},
		{EAffect::kHide,            "укрытие"},
		{EAffect::kCloudOfArrows,   "облако стрел"},
		{EAffect::kAirAura,         "воздушная аура"},
		{EAffect::kFireAura,        "огненная аура"},
		{EAffect::kIceAura,         "ледяная аура"},
		{EAffect::kEarthAura,       "земляная аура"},
		{EAffect::kCommander,       "полководец"},
		{EAffect::kFrenzy,          "берсерк"},
		{EAffect::kCombatLuck,      "удача в бою"},
		{EAffect::kGodsShield,      "божий щит"},
		{EAffect::kBerserk,         "ярость"},
		{EAffect::kBrokenChains,    "разорванные цепи"},
		{EAffect::kVampirism,       "вампиризм"},
		{EAffect::kCurse,           "проклятие"},
		{EAffect::kPoisoned,        "яд"},
		{EAffect::kBlind,           "слепота"},
	};
	std::string flags_list;
	for (const auto& f : kAffectFlags) {
		if (AFF_FLAGGED(ch, f.flag)) {
			if (!flags_list.empty()) flags_list += "|";
			flags_list += f.name;
		}
	}
	e.attrs["flags_list"] = observability::EngineStringToUtf8(flags_list);
	observability::EmitToAllSinks(e);
}

}  // namespace

void RunScenario(const Scenario& scenario) {
	if (scenario.rounds <= 0) {
		throw ScenarioRunError("scenario.rounds must be positive");
	}

	// Strip room-level combat/magic restrictions for the arena; restored on
	// scope exit.
	ArenaFlagSweep arena_flags(world[kArenaRoom]);

	// Spawn once. The duel runs continuously; we observe each battle round.
	CharData* attacker = SpawnParticipant(scenario.attacker);
	CharData* victim = SpawnParticipant(scenario.victim);
	// Display names for synthetic PCs. Without this GET_NAME() returns ""
	// and the JSONL events log "<empty> -> костяная гончая: 5 (удар)".
	if (std::holds_alternative<PlayerSpec>(scenario.attacker)) {
		attacker->set_name("attacker");
	}
	if (std::holds_alternative<PlayerSpec>(scenario.victim)) {
		victim->set_name("victim");
	}
	PlaceCharToRoom(attacker, kArenaRoom);
	PlaceCharToRoom(victim, kArenaRoom);
	// affect_total() рано возвращается, если in_room == kNowhere. Поскольку
	// CharacterBuilder.SetFeat вызывает affect_total ДО того, как PC попал
	// в комнату, feat-апплаи (kPowerMagic +50% spellpower и т.п.) не
	// пересчитывались. Перевызываем после PlaceCharToRoom.
	affect_total(attacker);
	affect_total(victim);
	// Inventory / equipment from the YAML scenario. Run after the first
	// affect_total so item-applied bonuses come on top of feat applies,
	// and re-run affect_total so equipped items' applies actually take
	// effect.
	EquipFromVnums(attacker, GetInventory(scenario.attacker));
	EquipFromVnums(victim, GetInventory(scenario.victim));
	affect_total(attacker);
	affect_total(victim);
	// affect_total for a synthetic PC drops position to ~kSit (3); without
	// kStand the engine refuses to cast or properly fight. Force kStand
	// AFTER the very last affect_total call (otherwise it gets reset).
	attacker->SetPosition(EPosition::kStand);
	victim->SetPosition(EPosition::kStand);

	// Pre-applied affects from the scenario. Через CallMagic вместо CastSpell:
	// CastSpell проверяет, что кастер знает заклинание (есть в class spells),
	// помнит его, не лежит в коме и т. д. Для предзаклинаний из YAML это
	// мешает -- пользователь хочет, чтобы дружинник вышел в бой с "защитой
	// богов" (заклинание витязя), а не споткнулся на "вы не знаете такого
	// заклинания". CallMagic -- внутренний слой ниже CastSpell, он напрямую
	// применяет эффект на цели как если бы заклинание уже было успешно
	// вычитано.
	auto apply_affects = [](CharData* ch, const std::vector<AffectSpec>& list) {
		for (const auto& aff : list) {
			std::string spell_name = aff.spell_name;
			const auto sid = FixNameAndFindSpellId(spell_name);
			if (sid == ESpell::kUndefined) {
				throw ScenarioRunError(fmt::format(
					"unknown affect spell: '{}'", aff.spell_name));
			}
			CallMagic(ch, ch, nullptr, nullptr, sid, GetRealLevel(ch));
		}
	};
	std::visit([&](auto&& s) { apply_affects(attacker, s.affects); }, scenario.attacker);
	std::visit([&](auto&& s) { apply_affects(victim, s.affects); }, scenario.victim);
	// Casting may have dropped position again -- re-stand.
	attacker->SetPosition(EPosition::kStand);
	victim->SetPosition(EPosition::kStand);

	// Pets / charmies / raised undead, declared as part of a participant in
	// the YAML scenario. We do NOT cast kCharm to subdue them -- they join
	// already loyal, with an EAffect::kCharmed marker and the master pointer
	// set to the owner. This matches typical balance scenarios for kudesnik
	// (charmer) and necromancer where the player walks in with their pets.
	std::vector<CharData*> attacker_pets;
	for (const auto& pet : GetPetSpecs(scenario.attacker)) {
		auto* p = SpawnPet(attacker, pet, kArenaRoom);
		affect_total(p);
		attacker_pets.push_back(p);
	}
	std::vector<CharData*> victim_pets;
	for (const auto& pet : GetPetSpecs(scenario.victim)) {
		auto* p = SpawnPet(victim, pet, kArenaRoom);
		affect_total(p);
		victim_pets.push_back(p);
	}

	// By default both get massive HP so the duel survives all rounds. If a
	// participant has an explicit max_hit override in the YAML scenario, that
	// override wins (it was already applied inside SpawnParticipant and we
	// must not overwrite it here).
	if (GetMaxHitOverride(scenario.attacker) < 0) {
		attacker->set_max_hit(kHugeHp);
		attacker->set_hit(kHugeHp);
	}
	if (GetMaxHitOverride(scenario.victim) < 0) {
		victim->set_max_hit(kHugeHp);
		victim->set_hit(kHugeHp);
	}

	const auto* cast = std::get_if<CastAction>(&scenario.action);
	// In both melee and cast scenarios we engage once at the start. Cast spells
	// like kCallLighting need kTarFightVict to resolve a target; SetFighting
	// also makes get_char_vis() find the mob reliably even with multi-word
	// keywords like "kostyanaya gonchaya".
	SetFighting(attacker, victim);
	for (auto* pet : attacker_pets) {
		SetFighting(pet, victim);
	}
	for (auto* pet : victim_pets) {
		SetFighting(pet, attacker);
	}

	// Attach a headless descriptor to each PC participant so we can capture
	// the exact text the player would have seen at a telnet client (act,
	// SendMsgToChar, etc.). Mobs do not have descriptors -- skip them.
	HeadlessDescriptor attacker_desc, victim_desc;
	if (std::holds_alternative<PlayerSpec>(scenario.attacker)) {
		attacker_desc.Attach(attacker);
	}
	if (std::holds_alternative<PlayerSpec>(scenario.victim)) {
		victim_desc.Attach(victim);
	}
	// Drop everything that landed in the buffer during spawn/setup -- room
	// description on entry, "you stop following X", etc. We only care about
	// the combat timeline.
	attacker_desc.Discard();
	victim_desc.Discard();

	// Initial snapshot before the first round, so replay can reconstruct
	// starting state.
	EmitCharState("attacker", attacker, -1);
	EmitCharState("victim", victim, -1);
	for (std::size_t i = 0; i < attacker_pets.size(); ++i) {
		EmitCharState(fmt::format("attacker_pet_{}", i).c_str(),
			attacker_pets[i], -1);
	}
	for (std::size_t i = 0; i < victim_pets.size(); ++i) {
		EmitCharState(fmt::format("victim_pet_{}", i).c_str(),
			victim_pets[i], -1);
	}

	int prev_hp = victim->get_hit();

	// Per-round event: tick one battle round (kBattleRound pulses == one
	// perform_violence step), measure HP delta, emit. For 'cast' scenarios
	// we additionally invoke DoCast at the start of each round (waiting out
	// kGlobalCooldown if it is still active) and record cast_attempt events.
	for (int r = 0; r < scenario.rounds; ++r) {
		if (cast) {
			// Wait out any leftover global cooldown AND wait_state (cast time
			// for the previous spell). Cap the wait so a misconfigured spell
			// can't hang the run.
			constexpr long long kMaxWaitPulses = kBattleRound * 4;
			long long waited = 0;
			while ((attacker->HasCooldown(ESkill::kGlobalCooldown) ||
					attacker->get_wait() > 0) &&
					waited < kMaxWaitPulses) {
				MUD::heartbeat()(0);
				++waited;
				if (attacker->in_room == kNowhere || victim->in_room == kNowhere) {
					break;
				}
			}
			if (attacker->in_room != kNowhere && victim->in_room != kNowhere) {
				// We bypass DoCast and call CastSpell directly: DoCast routes
				// to ch->SetCast() (deferred cast) when there is an enemy,
				// which makes the magic damage land later through the
				// mem_queue heartbeat step and not through Damage::Process
				// (or at level 0). For simulator we want immediate, observable
				// magic damage on every casting round.
				std::string spell_name = cast->spell_name;
				const auto sid = FixNameAndFindSpellId(spell_name);
				if (sid != ESpell::kUndefined) {
					CastSpell(attacker, victim, nullptr, nullptr, sid, sid);
					if (GET_SPELL_MEM(attacker, sid) > 0) {
						GET_SPELL_MEM(attacker, sid)--;
					}
					// DoCast выставил бы wait_state на kBattleRound при
					// нормальном пути; для прямого CastSpell (минуем DoCast,
					// см. комментарий выше) делаем то же руками, иначе
					// spell-каст в каждом раунде = переоценённый dpr.
					SetWaitState(attacker, kBattleRound);
				}
			}
		}
		for (long long p = 0; p < kBattleRound; ++p) {
			MUD::heartbeat()(0);
			if (attacker->in_room == kNowhere || victim->in_room == kNowhere) {
				break;  // somebody died or got extracted
			}
		}
		// check_idling() in game_limits.cpp вытащит синтетического PC в
		// kStrangeRoom после idle_void пульсов, потому что у него нет
		// input'а -- ch->char_specials.timer только растёт. На длинных
		// сценариях (сотни раундов) PC внезапно "пропадал в пустоте этого
		// мира". Сбрасываем idle-таймер каждый раунд для всех participants.
		auto reset_idle = [](CharData* c) {
			if (c) c->char_specials.timer = 0;
		};
		reset_idle(attacker);
		reset_idle(victim);
		for (auto* p : attacker_pets) reset_idle(p);
		for (auto* p : victim_pets) reset_idle(p);
		const bool alive = attacker->in_room != kNowhere && victim->in_room != kNowhere;
		const int hp_now = alive ? victim->get_hit() : 0;
		const int damage = alive ? (prev_hp - hp_now) : prev_hp;

		observability::Event e;
		e.name = "round";
		e.ts_unix_ms = NowUnixMs();
		e.attrs["round"] = static_cast<std::int64_t>(r);
		AddParticipantAttrs(e, "attacker", scenario.attacker);
		AddParticipantAttrs(e, "victim", scenario.victim);
		e.attrs["hp_before"] = static_cast<std::int64_t>(prev_hp);
		e.attrs["hp_after"] = static_cast<std::int64_t>(hp_now);
		e.attrs["damage_observed"] = static_cast<std::int64_t>(damage);
		e.attrs["victim_alive"] = alive;
		observability::EmitToAllSinks(e);

		// Per-round snapshot for both participants and their pets. Replay
		// tooling consumes these to reconstruct any participant's state at
		// any round without having to fold all damage/affect events.
		if (alive) {
			EmitCharState("attacker", attacker, r);
			EmitCharState("victim", victim, r);
			for (std::size_t i = 0; i < attacker_pets.size(); ++i) {
				if (attacker_pets[i]->in_room != kNowhere) {
					EmitCharState(fmt::format("attacker_pet_{}", i).c_str(),
						attacker_pets[i], r);
				}
			}
			for (std::size_t i = 0; i < victim_pets.size(); ++i) {
				if (victim_pets[i]->in_room != kNowhere) {
					EmitCharState(fmt::format("victim_pet_{}", i).c_str(),
						victim_pets[i], r);
				}
			}
		}

		// Drain per-PC text accumulated during this pulse_violence -- one
		// 'screen_output' event per PC per round, with whatever the engine
		// printed via SendMsgToChar/act() this round.
		attacker_desc.FlushAndEmit("attacker", r);
		victim_desc.FlushAndEmit("victim", r);

		if (!alive) {
			break;
		}
		prev_hp = hp_now;
	}

	// Detach descriptors before ExtractCharFromWorld -- once the char is
	// extracted, ch->desc would dangle if we still held the link.
	attacker_desc.Detach();
	victim_desc.Detach();

	// Cleanup pets first so their follower lists do not point to extracted
	// owners; then the owners themselves.
	for (auto* pet : attacker_pets) {
		if (pet->in_room != kNowhere) {
			ExtractCharFromWorld(pet, false);
		}
	}
	for (auto* pet : victim_pets) {
		if (pet->in_room != kNowhere) {
			ExtractCharFromWorld(pet, false);
		}
	}
	if (attacker->in_room != kNowhere) {
		ExtractCharFromWorld(attacker, false);
	}
	if (victim->in_room != kNowhere) {
		ExtractCharFromWorld(victim, false);
	}
	observability::FlushAllSinks();
}

}  // namespace simulator

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
