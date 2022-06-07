#include "color.h"
#include "handler.h"
#include "entities/player_races.h"
#include "structs/global_objects.h"

int feat_slot_lvl(int remort, int slot_for_remort, int slot) {
	int result = 0;
	for (result = 1; result < kLvlImmortal; result++) {
		if (result * (5 + remort / slot_for_remort) / kLastFeatSlotLvl == slot) {
			break;
		}
	}
	/*
	  ВНИМАНИЕ: формула содрана с CalcFeatLvl (feats.h)!
	  (1+GetRealLevel(ch)*(5+GET_REAL_REMORT(ch)/MUD::Classes(ch->get_class()).GetRemortsNumForFeatSlot()/kLastFeatSlotLvl)
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
	char msg[kMaxStringLength];
	bool sfound;

	//Найдем максимальный слот, который вобще может потребоваться данному персонажу на текущем морте
	max_slot = CalcFeatSlotsAmountPerRemort(ch);
	char **names = new char *[max_slot];

	for (int k = 0; k < max_slot; k++) {
		names[k] = new char[kMaxStringLength];
	}

	if (all_feats) {
		sprintf(names[0], "\r\nКруг 1  (1  уровень):\r\n");
	} else {
		*names[0] = '\0';
	}
	for (i = 1; i < max_slot; i++) {
		if (all_feats) {
			// на каком уровне будет слот i?
			j = feat_slot_lvl(GET_REMORT(ch), MUD::Class(ch->GetClass()).GetRemortsNumForFeatSlot(), i);
			sprintf(names[i], "\r\nКруг %-2d (%-2d уровень):\r\n", i + 1, j);
		} else {
			*names[i] = '\0';
		}
	}

	sprintf(buf2, "\r\nВрожденные способности :\r\n");
	j = 0;
	if (all_feats) {
		if (clr(vict, C_NRM)) // реж цвет >= обычный
			SendMsgToChar(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						  " &gЗеленым цветом и пометкой [И] выделены уже изученные способности.\r\n&n"
						  " Пометкой [Д] выделены доступные для изучения способности.\r\n"
						  " &rКрасным цветом и пометкой [Н] выделены способности, недоступные вам в настоящий момент.&n\r\n\r\n",
						  vict);
		else
			SendMsgToChar(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						  " Пометкой [И] выделены уже изученные способности.\r\n"
						  " Пометкой [Д] выделены доступные для изучения способности.\r\n"
						  " Пометкой [Н] выделены способности, недоступные вам в настоящий момент.\r\n\r\n", vict);
		for (const auto &feat : MUD::Class(ch->GetClass()).feats) {
			if (feat.IsUnavailable() &&
				!PlayerRace::FeatureCheck((int) GET_KIN(ch), (int) GET_RACE(ch), to_underlying(feat.GetId()))) {
				continue;
			}
			if (clr(vict, C_NRM)) {
				sprintf(buf, "        %s%s %-30s%s\r\n",
						ch->HaveFeat(feat.GetId()) ? KGRN :
						CanGetFeat(ch, feat.GetId()) ? KNRM : KRED,
						ch->HaveFeat(feat.GetId()) ? "[И]" :
						CanGetFeat(ch, feat.GetId()) ? "[Д]" : "[Н]",
						MUD::Feat(feat.GetId()).GetCName(), KNRM);
			} else {
				sprintf(buf, "    %s %-30s\r\n",
						ch->HaveFeat(feat.GetId()) ? "[И]" :
						CanGetFeat(ch, feat.GetId()) ? "[Д]" : "[Н]",
						MUD::Feat(feat.GetId()).GetCName());
			}

			if (feat.IsInborn() ||
				PlayerRace::FeatureCheck((int) GET_KIN(ch), (int) GET_RACE(ch), to_underlying(feat.GetId()))) {
				strcat(buf2, buf);
				j++;
			} else if (feat.GetSlot() < max_slot) {
				strcat(names[feat.GetSlot()], buf);
			}
		}
		sprintf(buf1, "--------------------------------------");
		for (i = 0; i < max_slot; i++) {
			if (strlen(buf1) >= kMaxStringLength - 60) {
				strcat(buf1, "***ПЕРЕПОЛНЕНИЕ***\r\n");
				break;
			}
			sprintf(buf1 + strlen(buf1), "%s", names[i]);
		}

		SendMsgToChar(buf1, vict);
//		page_string(ch->desc, buf, 1);
		if (j)
			SendMsgToChar(buf2, vict);

		for (int k = 0; k < max_slot; k++)
			delete[] names[k];

		delete[] names;

		return;
	}

// ======================================================

	sprintf(buf1, "Вы обладаете следующими способностями :\r\n");

	for (const auto &feat : MUD::Class(ch->GetClass()).feats) {
		if (strlen(buf2) >= kMaxStringLength - 60) {
			strcat(buf2, "***ПЕРЕПОЛНЕНИЕ***\r\n");
			break;
		}
		if (ch->HaveFeat(feat.GetId())) {
			if (MUD::Feat(feat.GetId()).IsInvalid()) {
				ch->UnsetFeat(feat.GetId());
				continue;
			}

			switch (feat.GetId()) {
				case EFeat::kBerserker:
				case EFeat::kLightWalk:
				case EFeat::kSpellCapabler:
				case EFeat::kRelocate:
				case EFeat::kShadowThrower:
					if (IsTimedByFeat(ch, feat.GetId())) {
						sprintf(buf, "[%3d] ", IsTimedByFeat(ch, feat.GetId()));
					} else {
						sprintf(buf, "[-!-] ");
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
					if (PRF_FLAGGED(ch, GetPrfWithFeatNumber(feat.GetId()))) {
						sprintf(buf, "[-%s*%s-] ", CCIGRN(vict, C_NRM), CCNRM(vict, C_NRM));
					} else {
						sprintf(buf, "[-:-] ");
					}
					break;
				default: sprintf(buf, "      ");
			}
			if (CanUseFeat(ch, feat.GetId())) {
				sprintf(buf + strlen(buf), "%s%s%s\r\n",
						CCIYEL(vict, C_NRM), MUD::Feat(feat.GetId()).GetCName(), CCNRM(vict, C_NRM));
			} else if (clr(vict, C_NRM)) {
				sprintf(buf + strlen(buf), "%s\r\n", MUD::Feat(feat.GetId()).GetCName());
			} else {
				sprintf(buf, "[-Н-] %s\r\n", MUD::Feat(feat.GetId()).GetCName());
			}
			if (feat.IsInborn() ||
				PlayerRace::FeatureCheck((int) GET_KIN(ch), (int) GET_RACE(ch), to_underlying(feat.GetId()))) {
				sprintf(buf2 + strlen(buf2), "    ");
				strcat(buf2, buf);
				j++;
			} else {
				slot = feat.GetSlot();
				sfound = false;
				while (slot < max_slot) {
					if (*names[slot] == '\0') {
						sprintf(names[slot], " %s%-2d%s) ",
								CCGRN(vict, C_NRM), slot + 1, CCNRM(vict, C_NRM));
						strcat(names[slot], buf);
						sfound = true;
						break;
					} else {
						slot++;
					}
				}
				if (!sfound) {
					// Если способность не врожденная и под нее нет слота - удаляем
					//	чтобы можно было менять слоты на лету и чтобы не читерили
					sprintf(msg, "WARNING: Unset out of slots feature '%s' for character '%s'!",
							MUD::Feat(feat.GetId()).GetCName(), GET_NAME(ch));
					mudlog(msg, BRF, kLvlImplementator, SYSLOG, true);
					ch->UnsetFeat(feat.GetId());
				}
			}
		}
	}

	auto max_slot_per_lvl = CalcMaxFeatSlotPerLvl(ch);
	for (i = 0; i < max_slot; i++) {
		if (*names[i] == '\0')
			sprintf(names[i], " %s%-2d%s)       %s[пусто]%s\r\n",
					CCGRN(vict, C_NRM), i + 1, CCNRM(vict, C_NRM), CCIWHT(vict, C_NRM), CCNRM(vict, C_NRM));
		if (i >= max_slot_per_lvl)
			break;
		sprintf(buf1 + strlen(buf1), "%s", names[i]);
	}
	SendMsgToChar(buf1, vict);

	if (j)
		SendMsgToChar(buf2, vict);

	for (int k = 0; k < max_slot; k++)
		delete[] names[k];

	delete[] names;
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