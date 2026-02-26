#include "townportal.h"

#include "engine/ui/modify.h"
#include "engine/core/handler.h"
#include "gameplay/abilities/abilities_constants.h"
#include "gameplay/fight/pk.h"
#include "gameplay/magic/magic_rooms.h"
#include "engine/db/global_objects.h"
#include "engine/ui/table_wrapper.h"

#include <third_party_libs/fmt/include/fmt/format.h>

void GoTownportal(CharData *ch, char *argument);
void TryOpenTownportal(CharData *ch, const Runestone &stone);
void TryOpenLabelPortal(CharData *ch, char *argument);
void OpenTownportal(CharData *ch, const Runestone &stone);
void SetSkillTownportalTimer(CharData *ch);
int CalcMinRunestoneLevel(CharData *ch, const Runestone &stone);
Runestone GetLabelPortal(CharData *ch);

void DoTownportal(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kTownportal)) {
		SendMsgToChar("Прежде изучите секрет постановки врат.\r\n", ch);
		return;
	}

	if (!argument || !*argument) {
		ch->PageRunestonesToChar();
		return;
	}

	char arg2[kMaxInputLength];
	two_arguments(argument, arg, arg2);
	if (!str_cmp(arg, "забыть")) {
		auto &stone = MUD::Runestones().FindRunestone(arg2);
		ch->RemoveRunestone(stone);
		return;
	}

	GoTownportal(ch, arg);
}

void GoTownportal(CharData *ch, char *argument) {
	auto &stone = MUD::Runestones().FindRunestone(argument);
	if (stone.IsAllowed() && ch->IsRunestoneKnown(stone)) {
		TryOpenTownportal(ch, stone);
	} else {
		TryOpenLabelPortal(ch, argument);
	}
}

void TryOpenTownportal(CharData *ch, const Runestone &stone) {
	if (IsTimedBySkill(ch, ESkill::kTownportal)) {
		SendMsgToChar("У вас недостаточно сил для постановки врат.\r\n", ch);
		return;
	}

	if (MUD::Runestones().FindRunestone(GET_ROOM_VNUM(ch->in_room)).IsAllowed()) {
		SendMsgToChar("Камень рядом с вами мешает вашей магии.\r\n", ch);
		return;
	}

	if (room_spells::IsRoomAffected(world[ch->in_room], ESpell::kRuneLabel)) {
		SendMsgToChar("Начертанные на земле магические руны подавляют вашу магию!\r\n", ch);
		return;
	}

	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoMagic) && !IS_GRGOD(ch)) {
		SendMsgToChar("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", ch);
		act("Магия $n1 потерпела неудачу и развеялась по воздуху.", false, ch, nullptr, nullptr, kToRoom);
		return;
	}

	if (stone.IsDisabled()) {
		act("Лазурная пентаграмма возникла в воздухе... и сразу же растаяла.", false, ch, nullptr, nullptr, kToChar);
		act("$n сложил$g руки в молитвенном жесте, испрашивая у Богов врата...", false, ch, nullptr, nullptr, kToRoom);
		act("Лазурная пентаграмма возникла в воздухе... и сразу же растаяла.", false, ch, nullptr, nullptr, kToRoom);
		SetSkillTownportalTimer(ch);
		return;
	}

	OpenTownportal(ch, stone);
}

void TryOpenLabelPortal(CharData *ch, char *argument) {
	if (name_cmp(ch, argument)) {
		const auto &stone = GetLabelPortal(ch);
		if (stone.IsAllowed()) {
			TryOpenTownportal(ch, stone);
		} else {
			SendMsgToChar("Вы не оставляли рунных меток.\r\n", ch);
		}
		return;
	}
	SendMsgToChar("Вам неизвестны такие руны.\r\n", ch);
}

Runestone GetLabelPortal(CharData *ch) {
	auto label_room = room_spells::FindAffectedRoomByCasterID(ch->get_uid(), ESpell::kRuneLabel);
	if (label_room) {
		return {ch->get_name(), label_room->vnum, 1};
	} else {
		return {"undefined", kNowhere, 1, Runestone::State::kForbidden};
	}
}

void OpenTownportal(CharData *ch, const Runestone &stone) {
	ImproveSkill(ch, ESkill::kTownportal, 1, nullptr);
	RoomData *from_room = world[ch->in_room];
	auto to_room = GetRoomRnum(stone.GetRoomVnum());
	from_room->pkPenterUnique = 0;
	one_way_portal::ReplacePortalTimer(ch, ch->in_room, to_room, 29);
	act("Лазурная пентаграмма возникла в воздухе.", false, ch, nullptr, nullptr, kToChar);
	act("$n сложил$g руки в молитвенном жесте, испрашивая у Богов врата...", false, ch, nullptr, nullptr, kToRoom);
	act("Лазурная пентаграмма возникла в воздухе.", false, ch, nullptr, nullptr, kToRoom);
	SetSkillTownportalTimer(ch);
}

void SetSkillTownportalTimer(CharData *ch) {
	if (!IS_IMMORTAL(ch)) {
		TimedSkill timed;
		timed.skill = ESkill::kTownportal;
		// timed.time - это unsigned char, поэтому при уходе в минус будет вынос на 255 и ниже
		int modif = ch->GetSkill(ESkill::kTownportal) / 7 + number(1, 5);
		timed.time = MAX(1, 25 - modif);
		ImposeTimedSkill(ch, &timed);
	}
}

int CalcMinRunestoneLevel(CharData *ch, const Runestone &stone) {
	if (stone.IsAllowed()) {
		return std::max(1, stone.GetMinCharLevel() - GetRealRemort(ch) / 2);
	}

	return kLvlImplementator + 1;
}

namespace one_way_portal {

void ReplacePortalTimer(CharData *ch, RoomRnum from_room, RoomRnum to_room, int time) {
//	sprintf(buf, "Ставим портал из %d в %d", world[from_room]->vnum, world[to_room]->vnum);
//	mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);

	Affect<room_spells::ERoomApply> af;
	af.type = ESpell::kPortalTimer;
	af.bitvector = 0;
	af.duration = time; //раз в 2 секунды
	af.modifier = to_room;
	af.battleflag = 0;
	af.location = room_spells::ERoomApply::kNone;
	af.caster_id = ch ? ch->get_uid() : 0;
	af.must_handled = false;
	af.apply_time = 0;
//	room_spells::AffectRoomJoinReplace(world[from_room], af);
	room_spells::affect_to_room(world[from_room], af);
	room_spells::AddRoomToAffected(world[from_room]);
	af.modifier = from_room;
	af.bitvector = room_spells::ERoomAffect::kNoPortalExit;
	room_spells::affect_to_room(world[to_room], af);
//	room_spells::AffectRoomJoinReplace(world[to_room], af);
	room_spells::AddRoomToAffected(world[to_room]);
}

} // namespace OneWayPortal

inline void DecayPortalMessage(const RoomRnum room_num) {
	act("Пентаграмма медленно растаяла.", false, world[room_num]->first_character(), nullptr, nullptr, kToRoom);
	act("Пентаграмма медленно растаяла.", false, world[room_num]->first_character(), nullptr, nullptr, kToChar);
}

void Runestone::SetEnabled(bool enabled) {
	if (state_ != State::kForbidden) {
		state_ = (enabled ? State::kEnabled : State::kDisabled);
	}
}

// ============================================ RUNESTONE ROSTER ================================================

RunestoneRoster::RunestoneRoster() {
	incorrect_stone_ = Runestone("undefined", kNowhere, kMaxPlayerLevel + 1, Runestone::State::kForbidden);
}

void RunestoneRoster::LoadRunestones() {
	FILE *portal_f;
	char nm[300], nm2[300], *wrd;
	int rnm = 0, i, level = 0;

	clear();
	if (!(portal_f = fopen(LIB_MISC "portals.lst", "r"))) {
		log("Cannot open portals.lst");
		return;
	}

	while (get_line(portal_f, nm)) {
		if (!nm[0] || nm[0] == ';')
			continue;
		sscanf(nm, "%d %d %s", &rnm, &level, nm2);
		if (GetRoomRnum(rnm) == kNowhere || nm2[0] == '\0') {
			log("Invalid runestone entry detected");
			continue;
		}
		wrd = nm2;
		for (i = 0; !(i == 10 || wrd[i] == ' ' || wrd[i] == '\0'); i++);
		wrd[i] = '\0';

		emplace_back(wrd, rnm, level);
	}
	fclose(portal_f);
}

Runestone &RunestoneRoster::FindRunestone(RoomVnum vnum) {
	auto predicate = [vnum](const Runestone &p) { return p.GetRoomVnum() == vnum; };
	auto it = std::find_if(begin(), end(), predicate);
	if (it != end()) {
		return *it;
	}

	return incorrect_stone_;
}

Runestone &RunestoneRoster::FindRunestone(std::string_view name) {
	auto predicate = [name](const Runestone &p) { return p.GetName() == name; };
	auto it = std::find_if(begin(), end(), predicate);
	if (it != end()) {
		return *it;
	}

	return incorrect_stone_;
}

bool RunestoneRoster::ViewRunestone(CharData *ch, int where_bits) {
	if (ch->GetSkill(ESkill::kTownportal) == 0) {
		return false;
	}
	auto &stone = FindRunestone(GET_ROOM_VNUM(ch->in_room));
	if (stone.IsAllowed() && IS_SET(where_bits, EFind::kObjRoom)) {
		if (stone.IsDisabled()) {
			SendMsgToChar("Камень грубо расколот на несколько частей и прочитать надпись невозможно.\r\n", ch);
			return true;
		} else if (ch->IsRunestoneKnown(stone)) {
			auto msg = fmt::format("На камне огненными рунами начертано слово '&R{}&n'.\r\n", stone.GetName());
			SendMsgToChar(msg, ch);
			return true;
		} else if (GetRealLevel(ch) < CalcMinRunestoneLevel(ch, stone)) {
			SendMsgToChar("Здесь что-то написано огненными рунами.\r\n", ch);
			SendMsgToChar("Но вы еще недостаточно искусны, чтобы разобрать слово.\r\n", ch);
			return true;
		} else {
			ch->AddRunestone(stone);
			return true;
		}
	}
	return false;
}

void RunestoneRoster::ShowRunestone(CharData *ch) {
	if (ch->GetSkill(ESkill::kTownportal)) {
		const auto &stone = FindRunestone(GET_ROOM_VNUM(ch->in_room));
		if (stone.IsAllowed()) {
			if (stone.IsEnabled()) {
				SendMsgToChar("Рунный камень с изображением пентаграммы немного выступает из земли.\r\n", ch);
			} else {
				SendMsgToChar("Рунный камень с изображением пентаграммы немного выступает из земли... расколот.\r\n",
							  ch);
			}
		}
	}
}

std::vector<RoomVnum> RunestoneRoster::GetVnumRoster() {
	std::vector<RoomVnum> vnums;
	vnums.reserve(size());
	for (const auto &it : *this) {
		if (it.IsAllowed()) {
			vnums.push_back(it.GetRoomVnum());
		}
	}
	return vnums;
}

std::vector<std::string_view> RunestoneRoster::GetNameRoster() {
	std::vector<std::string_view> names;
	names.reserve(size());
	for (const auto &it : *this) {
		if (it.IsAllowed()) {
			names.push_back(it.GetName());
		}
	}
	return names;
}

// ======================================== CHARACTER RUNESTONE ROSTER ==============================================

void CharacterRunestoneRoster::Serialize(std::ostringstream &out) {
	for (const auto it : *this) {
		out << fmt::format("Prtl: {}\n", it);
	}
}

void CharacterRunestoneRoster::PageToChar(CharData *ch) {
	std::ostringstream out;
	if (empty()) {
		out << " В настоящий момент вам неизвестны рунные метки.\r\n";
	} else {
		out << " Вам известны следующие рунные метки:\r\n";
		table_wrapper::Table table;
		const int columns_num{4};
		int count = 1;
		for (const auto it : *this) {
			auto &portal = MUD::Runestones().FindRunestone(it);
			if (portal.IsAllowed()) {
				table << portal.GetName();
				if (count % columns_num == 0) {
					table << table_wrapper::kEndRow;
				}
				++count;
			}
		}
		table_wrapper::DecorateSimpleTable(ch, table);
		table_wrapper::PrintTableToStream(out, table);
		out << fmt::format("\r\n Сейчас вы помните {} рунных меток.\r\n", size());
	}
	out << fmt::format(" Максимально возможно {}.\r\n", CalcLimit(ch));

	page_string(ch->desc, out.str());
}

bool CharacterRunestoneRoster::RemoveRunestone(const Runestone &stone) {
	auto predicate = [stone](const RoomVnum p) { return p == stone.GetRoomVnum(); };
	return std::erase_if(*this, predicate);
}

bool CharacterRunestoneRoster::AddRunestone(const Runestone &stone) {
	if (stone.IsAllowed()) {
		const auto room_vnum = stone.GetRoomVnum();
		std::erase_if(*this, [room_vnum](RoomVnum p) { return p == room_vnum; });
		push_back(room_vnum);
		return true;
	}
	return false;
}

bool CharacterRunestoneRoster::IsFull(CharData *ch) {
	return CalcLimit(ch) <= Count();
}

bool CharacterRunestoneRoster::Contains(const Runestone &stone) {
	const auto it = std::find(begin(), end(), stone.GetRoomVnum());
	return (it != end());
}

void CharacterRunestoneRoster::DeleteIrrelevant(CharData *ch) {
	auto &runestones = MUD::Runestones();
	auto predicate =
		[&runestones](const RoomVnum vnum) { return runestones.FindRunestone(vnum).IsForbidden(); };
	std::erase_if(*this, predicate);
	if (IsOverfilled(ch)) {
		ShrinkToLimit(ch);
	}
}

bool CharacterRunestoneRoster::IsOverfilled(CharData *ch) {
	return (size() > CalcLimit(ch));
}

void CharacterRunestoneRoster::ShrinkToLimit(CharData *ch) {
	resize(CalcLimit(ch));
}

std::size_t CharacterRunestoneRoster::CalcLimit(CharData *ch) {
	const auto skill = ch->GetSkill(ESkill::kTownportal);
	auto low_skill = std::min(skill, abilities::kNoviceSkillThreshold);
	auto hi_skill = std::max(0, skill - abilities::kNoviceSkillThreshold);
	return (1 + low_skill/9 + hi_skill/5);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
