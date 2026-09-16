#include "fit.h"
#include "utils/grammar/gender.h"

#include "engine/entities/char_data.h"
#include "engine/core/target_resolver.h"
#include "utils/utils_string.h"

#include <fmt/format.h>

void DoFit(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjData *obj;
	CharData *vict;

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

	std::string remains;
	const std::string obj_name = utils::ExtractFirstArgumentLower(argument, remains);

	if (obj_name.empty()) {
		SendMsgToChar("Что вы хотите переделать?\r\n", ch);
		return;
	};

	if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying))) {
		SendMsgToChar(fmt::format("У вас нет '{}'.\r\n", obj_name), ch);
		return;
	};

	vict = target_resolver::FindCharInRoom(ch, utils::ExtractFirstArgumentLower(remains));
	if (!vict) {
		SendMsgToChar("Под кого вы хотите переделать эту вещь?\r\n Нет такого создания в округе!\r\n", ch);
		return;
	};

	if (obj->get_owner()) {
		SendMsgToChar("У этой вещи уже есть владелец.\r\n", ch);
		return;

	};

	if ((obj->get_wear_flags() <= 1) || obj->has_flag(EObjFlag::kSetItem)) {
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
				SendMsgToChar(fmt::format("К сожалению {} сделан{} из неподходящего материала.\r\n",
										  obj->get_PName(grammar::ECase::kNom),
										  grammar::ObjSexEnding((obj)->get_sex(), 6)), ch);
				return;
			}
			break;
		case kScmdMakeOver:
			if (obj->get_material() != EObjMaterial::kBone
				&& obj->get_material() != EObjMaterial::kCloth
				&& obj->get_material() != EObjMaterial::kSkin
				&& obj->get_material() != EObjMaterial::kOrganic) {
				SendMsgToChar(fmt::format("К сожалению {} сделан{} из неподходящего материала.\r\n",
										  obj->get_PName(grammar::ECase::kNom),
										  grammar::ObjSexEnding((obj)->get_sex(), 6)), ch);
				return;
			}
			break;
		default:
			SendMsgToChar("Это какая-то ошибка...\r\n", ch);
			return;
	};
	obj->set_owner(vict->get_uid());
	SendMsgToChar(fmt::format("Вы долго пыхтели и сопели, переделывая работу по десять раз.\r\n"
							  "Вы извели кучу времени и 10000 кун золотом.\r\n"
							  "В конце-концов подогнали {} точно по мерке {}.\r\n",
							  obj->get_PName(grammar::ECase::kAcc), GET_PAD(vict, 1)), ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
