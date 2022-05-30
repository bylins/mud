#include "effect.h"

#include <sstream>

#include "color.h"

namespace effects {

void Damage::Print(std::ostringstream &buffer) const {
	buffer << " Damage: " << std::endl
		   << KGRN << "  " << dice_num
		   << "d" << dice_size
		   << "+" << dice_add << KNRM
		   << " Low skill bonus: " << KGRN << low_skill_bonus << KNRM
		   << " Hi skill bonus: " << KGRN << hi_skill_bonus << KNRM
		   << " Saving: " << KGRN << NAME_BY_ITEM<ESaving>(saving) << KNRM << std::endl;
}

void Area::Print(std::ostringstream &buffer) const {
	buffer << " Area:" << std::endl
		   << "  Cast decay: " << KGRN << cast_decay << KNRM
		   << " Level decay: " << KGRN << level_decay << KNRM
		   << " Free targets: " << KGRN << free_targets << KNRM << std::endl
		   << "  Skill divisor: " << KGRN << skill_divisor << KNRM
		   << " Targets dice: " << KGRN << targets_dice_size << KNRM
		   << " Min targets: " << KGRN << min_targets << KNRM
		   << " Max targets: " << KGRN << max_targets << KNRM << std::endl;
}

void Apply::Print(std::ostringstream &buffer) const {
	buffer << " Apply:" << std::endl;
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
