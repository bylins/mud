#include "fit.h"

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"

void DoFit(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjData *obj;
	CharData *vict;
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];

	if (GetRealLevel(ch) < kLvlImmortal) {
		SendMsgToChar("Вы не можете этого.", ch);
		return;
	};

	if ((subcmd == kScmdDoAdapt) && !CanUseFeat(ch, EFeat::kToFitItem)) {
		SendMsgToChar("Вы не умеете этого.", ch);
		return;
	};
	if ((subcmd == kScmdMakeOver) && !CanUseFeat(ch, EFeat::kToFitClotches)) {
		SendMsgToChar("Вы не умеете этого.", ch);
		return;
	};

	argument = one_argument(argument, arg1);

	if (!*arg1) {
		SendMsgToChar("Что вы хотите переделать?\r\n", ch);
		return;
	};

	if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
		sprintf(buf, "У вас нет \'%s\'.\r\n", arg1);
		SendMsgToChar(buf, ch);
		return;
	};

	argument = one_argument(argument, arg2);
	if (!(vict = get_char_vis(ch, arg2, EFind::kCharInRoom))) {
		SendMsgToChar("Под кого вы хотите переделать эту вещь?\r\n Нет такого создания в округе!\r\n", ch);
		return;
	};

	if (obj->get_owner()) {
		SendMsgToChar("У этой вещи уже есть владелец.\r\n", ch);
		return;

	};

	if ((obj->get_wear_flags() <= 1) || obj->has_flag(EObjFlag::KSetItem)) {
		SendMsgToChar("Этот предмет невозможно переделать.\r\n", ch);
		return;
	}

	switch (subcmd) {
		case kScmdDoAdapt:
			if (obj->get_material() != EObjMaterial::kMaterialUndefined
				&& obj->get_material() != EObjMaterial::kBulat
				&& obj->get_material() != EObjMaterial::kBronze
				&& obj->get_material() != EObjMaterial::kIron
				&& obj->get_material() != EObjMaterial::kSteel
				&& obj->get_material() != EObjMaterial::kForgedSteel
				&& obj->get_material() != EObjMaterial::kPreciousMetel
				&& obj->get_material() != EObjMaterial::kWood
				&& obj->get_material() != EObjMaterial::kHardWood
				&& obj->get_material() != EObjMaterial::kGlass) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_6(obj));
				SendMsgToChar(buf, ch);
				return;
			}
			break;
		case kScmdMakeOver:
			if (obj->get_material() != EObjMaterial::kBone
				&& obj->get_material() != EObjMaterial::kCloth
				&& obj->get_material() != EObjMaterial::kSkin
				&& obj->get_material() != EObjMaterial::kOrganic) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_6(obj));
				SendMsgToChar(buf, ch);
				return;
			}
			break;
		default:
			SendMsgToChar("Это какая-то ошибка...\r\n", ch);
			return;
	};
	obj->set_owner(GET_UID(vict));
	sprintf(buf, "Вы долго пыхтели и сопели, переделывая работу по десять раз.\r\n");
	sprintf(buf + strlen(buf), "Вы извели кучу времени и 10000 кун золотом.\r\n");
	sprintf(buf + strlen(buf), "В конце-концов подогнали %s точно по мерке %s.\r\n",
			obj->get_PName(ECase::kAcc).c_str(), GET_PAD(vict, 1));

	SendMsgToChar(buf, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
