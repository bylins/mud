#include "rune_stones.h"

#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"
#include "gameplay/abilities/abilities_constants.h"
#include "engine/ui/modify.h"
#include "engine/ui/table_wrapper.h"
#include "engine/core/handler.h"

#include <fmt/format.h>

// issue.runestones: extracted from townportal.cpp. The minimum character level at which a stone can be
// read/memorised (remort lowers it). Used only by ViewRunestone.
static int CalcMinRunestoneLevel(CharData *ch, const Runestone &stone) {
	if (stone.IsAllowed()) {
		return std::max(1, stone.GetMinCharLevel() - GetRealRemort(ch) / 2);
	}

	return kLvlImplementator + 1;
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
		out << " В настоящий момент вам неизвестны рунные камни.\r\n";
	} else {
		out << " Вам известны следующие рунные камни:\r\n";
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
		out << fmt::format("\r\n Сейчас вы помните {} рунных камней.\r\n", size());
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

// issue.runestones: the player-facing add/remove (with messages) used to be CharData::AddRunestone /
// CharData::RemoveRunestone; moved here as they are runestone logic, not character logic.
void CharacterRunestoneRoster::AddRunestone(CharData *ch, const Runestone &stone) {
	if (IsFull(ch)) {
		SendMsgToChar
			("В вашей памяти не осталось места для новых рунных камней. Сперва забудьте какой-нибудь.\r\n", ch);
		return;
	}

	if (AddRunestone(stone)) {
		auto msg = fmt::format(
			"Вы осмотрели надпись и крепко запомнили начертанное огненными рунами слово '&R{}&n'.\r\n",
			stone.GetName());
		SendMsgToChar(msg, ch);
	} else {
		SendMsgToChar("Руны всё время странно искажаются и вам не удаётся их запомнить.\r\n", ch);
	}
	DeleteIrrelevant(ch);
}

void CharacterRunestoneRoster::RemoveRunestone(CharData *ch, const Runestone &stone) {
	if (RemoveRunestone(stone)) {
		auto msg = fmt::format("Вы полностью забыли, как выглядит рунный камень '&R{}&n'.\r\n", stone.GetName());
		SendMsgToChar(msg, ch);
	} else {
		SendMsgToChar("Чтобы забыть что-нибудь ненужное, следует сперва изучить что-нибудь ненужное...", ch);
	}
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
