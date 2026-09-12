#include "do_skills.h"
#include <fmt/format.h>

#include "engine/ui/color.h"
#include "engine/entities/char_data.h"
#include "gameplay/abilities/timed_abilities.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/weather.h"

void DoSkills(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}

	if (argument) {
		// trim argument left
		while ('\0' != *argument && a_isspace(*argument)) {
			++argument;
		}

		if (*argument) {
			// trim argument right
			size_t length = strlen(argument);
			while (0 < length && a_isspace(argument[length - 1])) {
				argument[--length] = '\0';
			}

			if (0 == length) {
				argument = nullptr;
			}
		}
	}

	DisplaySkills(ch, ch, argument);
}

void DisplaySkills(CharData *ch, CharData *vict, const char *filter/* = nullptr*/) {
	int i = 0;
	std::string line;

	std::string out = "Вы владеете следующими умениями:\r\n";
	std::list<std::string> skills_names;

	for (const auto &skill : MUD::Skills()) {
		if (GetSkill(ch, skill.GetId())) {
			if (skill.IsInvalid()) {
				continue;
			}
			// filter out skill that does not correspond to filter condition
			if (filter && nullptr == strstr(skill.GetName(), filter)) {
				continue;
			}
			auto skill_id = skill.GetId();
			switch (skill_id) {
				case ESkill::kWarcry:
					line = fmt::format("[-{}-] ", (kHoursPerDay - IsTimedBySkill(ch, skill_id)) / kHoursPerWarcry);
					break;
				case ESkill::kTurnUndead: {
					auto bonus = CanUseFeat(ch, EFeat::kExorcist) ? -2 : 0;
					bonus = std::max(1, kHoursPerTurnUndead + bonus);
					line = fmt::format("[-{}-] ", (kHoursPerDay - IsTimedBySkill(ch, skill_id)) / bonus);
					break;
				}
				case ESkill::kFirstAid:
				case ESkill::kHangovering:
				case ESkill::kIdentify:
				case ESkill::kDisguise:
				case ESkill::kCourage:
				case ESkill::kJinx:
				case ESkill::kTownportal:
				case ESkill::kStun:
				case ESkill::kRepair:
					if (IsTimedBySkill(ch, skill_id) > 0)
						line = fmt::format("[{:3}] ", IsTimedBySkill(ch, skill_id));
					else
						line = "[-!-] ";
					break;
				default: line = "      ";
			}

			// Ширина колонки - в символах, а не в байтах (issue #3681).
			line += fmt::format("{:<23} {} ({}){} \r\n",
					skill.GetName(),
					how_good(GetSkill(ch, skill_id), CalcSkillHardCap(ch, skill_id)),
					GetTrainedSkill(ch, skill_id) == 0 ? GetEquippedSkill(ch, skill_id) : 
					std::min(CalcSkillMinCap(ch, skill_id) + GetEquippedSkill(ch, skill_id), MUD::Skill(skill_id).cap),
					kColorNrm);
			skills_names.emplace_back(line);
			i++;
		}
	}

	if (!i) {
		out += (nullptr == filter) ? "Нет умений.\r\n" : "Нет умений, удовлетворяющих фильтру.\r\n";
	} else {
		// Сторож переполнения снят вместе с буфером: список умений растёт сам.
		for (const auto &skill_name : skills_names) {
			out += skill_name;
		}
	}
	SendMsgToChar(out, vict);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
