#include "engine/ui/color.h"
#include <fmt/format.h>
#include "gameplay/core/remort.h"
#include "engine/entities/char_data.h"
#include "gameplay/abilities/timed_abilities.h"
#include "gameplay/mechanics/player_races.h"
#include "engine/db/global_objects.h"

#include <string>
#include <vector>

int feat_slot_lvl(int remort, int slot_for_remort, int slot) {
	int result = 0;
	for (result = 1; result < kLvlImmortal; result++) {
		if (result * (5 + remort / slot_for_remort) / kLastFeatSlotLvl == slot) {
			break;
		}
	}
	/*
	  ВНИМАНИЕ: формула содрана с CalcFeatLvl (feats.h)!
	  (1+GetRealLevel(ch)*(5+remort::GetRealRemort(ch)/MUD::Classes(ch->get_class()).GetRemortsNumForFeatSlot()/kLastFeatSlotLvl)
	  сделано это потому, что "обратная" формула, использованная ранее в list_feats,
	  выдавала неверные результаты ввиду нюансов округления
	  */
	return result;
}

/*
1. Определяем есть ли способность, если нет - новый цикл
2. Если есть
2.2 Врожденная : таймер(отступ)+имя -> буфер врожденных
2.3 Неврожденная: таймер(отступ)+имя -> 3
3. Если слот <= макс слота и свободен -> номер слота+таймер+имя в массив имен под номер слота
3.2. Если слот занят, увеличиваеим номер слота на 1 и переход на 3 пункт
3.3 Иначе удаляем способность.
Примечание: удаление реализовано с целью сделать возможнным изменение слота в процессе игры.
Лишние способности у персонажей удалятся автоматически при использовании команды "способности".
*/
void DisplayFeats(CharData *ch, CharData *vict, bool all_feats) {
	int i = 0, j = 0, slot, max_slot = 0;
	bool sfound;

	//Найдем максимальный слот, который вобще может потребоваться данному персонажу на текущем морте
	max_slot = CalcFeatSlotsAmountPerRemort(ch);
	std::vector<std::string> names(max_slot);

	if (all_feats) {
		names[0] = "\r\nКруг 1  (1  уровень):\r\n";
		for (i = 1; i < max_slot; i++) {
			// на каком уровне будет слот i?
			j = feat_slot_lvl(ch->get_remort(), MUD::Class(ch->GetClass()).GetRemortsNumForFeatSlot(), i);
			names[i] = fmt::format("\r\nКруг {:<2} ({:<2} уровень):\r\n", i + 1, j);
		}
	}

	std::string inborn = "\r\nВрожденные способности :\r\n";
	j = 0;
	if (all_feats) {
		if (!ch->IsFlagged(EPrf::kBlindMode)) {
			SendMsgToChar(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						  " &gЗеленым цветом и пометкой [И] выделены уже изученные способности.\r\n&n"
						  " Пометкой [Д] выделены доступные для изучения способности.\r\n"
						  " &rКрасным цветом и пометкой [Н] выделены способности, недоступные вам в настоящий момент.&n\r\n\r\n",
						  vict);
		} else {
			SendMsgToChar(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						  " Пометкой [И] выделены уже изученные способности.\r\n"
						  " Пометкой [Д] выделены доступные для изучения способности.\r\n"
						  " Пометкой [Н] выделены способности, недоступные вам в настоящий момент.\r\n\r\n", vict);
		}
		for (const auto &feat : MUD::Class(ch->GetClass()).feats) {
			if (feat.IsUnavailable() &&
				!MUD::PcRaces()[GET_RACE(ch)].HasFeature(feat.GetId())) {
				continue;
			}
			const char *mark = ch->HaveFeat(feat.GetId()) ? "[И]"
							   : CanGetFeat(ch, feat.GetId()) ? "[Д]" : "[Н]";
			std::string line;
			if (!ch->IsFlagged(EPrf::kBlindMode)) {
				const char *color = ch->HaveFeat(feat.GetId()) ? "&g"
									: CanGetFeat(ch, feat.GetId()) ? "&n" : "&r";
				line = fmt::format("        {}{} {:<30}&n\r\n", color, mark, MUD::Feat(feat.GetId()).GetCName());
			} else {
				line = fmt::format("    {} {:<30}\r\n", mark, MUD::Feat(feat.GetId()).GetCName());
			}

			if (feat.IsInborn() ||
				MUD::PcRaces()[GET_RACE(ch)].HasFeature(feat.GetId())) {
				inborn += line;
				j++;
			} else if (feat.GetSlot() < max_slot) {
				names[feat.GetSlot()] += line;
			}
		}

		std::string out = "--------------------------------------";
		for (i = 0; i < max_slot; i++) {
			out += names[i];
		}
		SendMsgToChar(out, vict);
		if (j) {
			SendMsgToChar(inborn, vict);
		}

		return;
	}

// ======================================================

	std::string out = "Вы обладаете следующими способностями :\r\n";

	for (const auto &feat : MUD::Class(ch->GetClass()).feats) {
		if (ch->HaveFeat(feat.GetId())) {
			if (MUD::Feat(feat.GetId()).IsInvalid()) {
				ch->UnsetFeat(feat.GetId());
				continue;
			}

			std::string line;
			switch (feat.GetId()) {
				case EFeat::kBerserker:
				case EFeat::kLightWalk:
				case EFeat::kSpellCapabler:
				case EFeat::kRelocate:
				case EFeat::kShadowThrower:
					if (IsTimedByFeat(ch, feat.GetId())) {
						line = fmt::format("[{:3}] ", IsTimedByFeat(ch, feat.GetId()));
					} else {
						line = "[-!-] ";
					}
					break;
				case EFeat::kPowerAttack:
				case EFeat::kGreatPowerAttack:
				case EFeat::kAimingAttack:
				case EFeat::kGreatAimingAttack:
				case EFeat::kScirmisher:
				case EFeat::kDoubleThrower:
				case EFeat::kTripleThrower:
				case EFeat::kSerratedBlade:
					if (ch->IsFlagged(GetPrfWithFeatNumber(feat.GetId()))) {
						line = "[-&G*&n-] ";
					} else {
						line = "[-:-] ";
					}
					break;
				default: line = "      ";
			}
			if (CanUseFeat(ch, feat.GetId())) {
				line += fmt::format("&Y{}&n\r\n", MUD::Feat(feat.GetId()).GetCName());
			} else if (!ch->IsFlagged(EPrf::kBlindMode)) {
				line += fmt::format("{}\r\n", MUD::Feat(feat.GetId()).GetCName());
			} else {
				line = fmt::format("[-Н-] {}\r\n", MUD::Feat(feat.GetId()).GetCName());
			}
			if (feat.IsInborn() ||
				MUD::PcRaces()[GET_RACE(ch)].HasFeature(feat.GetId())) {
				inborn += "    ";
				inborn += line;
				j++;
			} else {
				slot = feat.GetSlot();
				sfound = false;
				while (slot < max_slot) {
					if (names[slot].empty()) {
						names[slot] = fmt::format(" &g{:<2}&n) ", slot + 1) + line;
						sfound = true;
						break;
					} else {
						slot++;
					}
				}
				if (!sfound) {
					// Если способность не врожденная и под нее нет слота - удаляем
					//	чтобы можно было менять слоты на лету и чтобы не читерили
					mudlog(fmt::format("WARNING: Unset out of slots feature '{}' for character '{}'!",
									   MUD::Feat(feat.GetId()).GetCName(), GET_NAME(ch)),
						   BRF, kLvlImplementator, SYSLOG, true);
					ch->UnsetFeat(feat.GetId());
				}
			}
		}
	}

	auto max_slot_per_lvl = CalcMaxFeatSlotPerLvl(ch);
	for (i = 0; i < max_slot; i++) {
		if (names[i].empty()) {
			names[i] = fmt::format(" &g{:<2}&n)       &K[пусто]&n\r\n", i + 1);
		}
		if (i >= max_slot_per_lvl)
			break;
		out += names[i];
	}
	SendMsgToChar(out, vict);

	if (j) {
		SendMsgToChar(inborn, vict);
	}
	const auto &race_features = MUD::PcRaces()[GET_RACE(ch)].GetFeatures();
	if (race_features.size() > 0) {
		SendMsgToChar(vict,  "Родовые способности :\r\n");
		for (const EFeat feat_id : race_features) {
			SendMsgToChar(vict, "          %s\r\n", MUD::Feat(feat_id).GetCName());
		}
	}
}

void DoFeatures(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}
	skip_spaces(&argument);
	if (utils::IsAbbr(argument, "все") || utils::IsAbbr(argument, "all")) {
		DisplayFeats(ch, ch, true);
	} else {
		DisplayFeats(ch, ch, false);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :