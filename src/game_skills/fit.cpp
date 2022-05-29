#include "fit.h"

#include "entities/char_data.h"
#include "handler.h"

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

	if (GET_OBJ_OWNER(obj)) {
		SendMsgToChar("У этой вещи уже есть владелец.\r\n", ch);
		return;

	};

	if ((GET_OBJ_WEAR(obj) <= 1) || obj->has_flag(EObjFlag::KSetItem)) {
		SendMsgToChar("Этот предмет невозможно переделать.\r\n", ch);
		return;
	}

	switch (subcmd) {
		case kScmdDoAdapt:
			if (GET_OBJ_MATER(obj) != EObjMaterial::kMaterialUndefined
				&& GET_OBJ_MATER(obj) != EObjMaterial::kBulat
				&& GET_OBJ_MATER(obj) != EObjMaterial::kBronze
				&& GET_OBJ_MATER(obj) != EObjMaterial::kIron
				&& GET_OBJ_MATER(obj) != EObjMaterial::kSteel
				&& GET_OBJ_MATER(obj) != EObjMaterial::kForgedSteel
				&& GET_OBJ_MATER(obj) != EObjMaterial::kPreciousMetel
				&& GET_OBJ_MATER(obj) != EObjMaterial::kWood
				&& GET_OBJ_MATER(obj) != EObjMaterial::kHardWood
				&& GET_OBJ_MATER(obj) != EObjMaterial::kGlass) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_SUF_6(obj));
				SendMsgToChar(buf, ch);
				return;
			}
			break;
		case kScmdMakeOver:
			if (GET_OBJ_MATER(obj) != EObjMaterial::kBone
				&& GET_OBJ_MATER(obj) != EObjMaterial::kCloth
				&& GET_OBJ_MATER(obj) != EObjMaterial::kSkin
				&& GET_OBJ_MATER(obj) != EObjMaterial::kOrganic) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_SUF_6(obj));
				SendMsgToChar(buf, ch);
				return;
			}
			break;
		default:
			SendMsgToChar("Это какая-то ошибка...\r\n", ch);
			return;
	};
	obj->set_owner(GET_UNIQUE(vict));
	sprintf(buf, "Вы долго пыхтели и сопели, переделывая работу по десять раз.\r\n");
	sprintf(buf + strlen(buf), "Вы извели кучу времени и 10000 кун золотом.\r\n");
	sprintf(buf + strlen(buf), "В конце-концов подогнали %s точно по мерке %s.\r\n",
			GET_OBJ_PNAME(obj, 3).c_str(), GET_PAD(vict, 1));

	SendMsgToChar(buf, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
