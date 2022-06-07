#include "do_skills.h"

#include "color.h"
#include "handler.h"
#include "structs/global_objects.h"

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

	sprintf(buf, "Вы владеете следующими умениями:\r\n");
	strcpy(buf2, buf);
	std::list<std::string> skills_names;

	for (const auto &skill : MUD::Skills()) {
		if (ch->GetSkill(skill.GetId())) {
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
					sprintf(buf, "[-%d-] ", (kHoursPerDay - IsTimedBySkill(ch, skill_id)) / kHoursPerWarcry);
					break;
				case ESkill::kTurnUndead: {
					auto bonus = CanUseFeat(ch, EFeat::kExorcist) ? -2 : 0;
					bonus = std::max(1, kHoursPerTurnUndead + bonus);
					sprintf(buf, "[-%d-] ", (kHoursPerDay - IsTimedBySkill(ch, skill_id)) / bonus);
					break;
				}
				case ESkill::kFirstAid:
				case ESkill::kHangovering:
				case ESkill::kIdentify:
				case ESkill::kDisguise:
				case ESkill::kCourage:
				case ESkill::kJinx:
				case ESkill::kTownportal:
				case ESkill::kStrangle:
				case ESkill::kStun:
				case ESkill::kRepair:
					if (IsTimedBySkill(ch, skill_id))
						sprintf(buf, "[%3d] ", IsTimedBySkill(ch, skill_id));
					else
						sprintf(buf, "[-!-] ");
					break;
				default: sprintf(buf, "      ");
			}

			sprintf(buf + strlen(buf), "%-23s %s (%d)%s \r\n",
					skill.GetName(),
					how_good(ch->GetSkill(skill_id), CalcSkillHardCap(ch, skill_id)),
					CalcSkillMinCap(ch, skill_id),
					CCNRM(ch, C_NRM));

			skills_names.emplace_back(buf);

			i++;
		}
	}

	if (!i) {
		if (nullptr == filter) {
			sprintf(buf2 + strlen(buf2), "Нет умений.\r\n");
		} else {
			sprintf(buf2 + strlen(buf2), "Нет умений, удовлетворяющих фильтру.\r\n");
		}
	} else {
		// output set of skills
		size_t buf2_length = strlen(buf2);
		for (const auto &skill_name : skills_names) {
			// why 60?
			if (buf2_length + skill_name.length() >= kMaxStringLength - 60) {
				strcat(buf2, "***ПЕРЕПОЛНЕНИЕ***\r\n");
				break;
			}

			strncat(buf2 + buf2_length, skill_name.c_str(), skill_name.length());
			buf2_length += skill_name.length();
		}
	}
	SendMsgToChar(buf2, vict);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
