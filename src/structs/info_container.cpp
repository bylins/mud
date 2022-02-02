/**
\authors Created by Sventovit
\date 2.02.2022.
\brief Универсальный, елико возможно, контейнер для хранения информации об игровых сущнсотях типа скиллов и классов.
*/

#include "info_container.h"

const I &InfoContainer::operator[](const E id) const {
	try {
		return items_->at(id)->second;
	} catch (const std::out_of_range &) {
		err_log("Incorrect id (%d) passed into %s.", to_underlying(id), typeid(this).name());
		return items_->at(E::kUndefined)->second;
	}
}

bool InfoContainer::IsKnown(const E id) {
	return items_->contains(id);
}

bool InfoContainer::IsValid(const E id) {
	bool validity = IsKnown(id) && id >= ESkill::kFirst && id <= ESkill::kLast;
	if (validity) {
		validity &= IsEnabled(id);
	}
	return validity;
}

bool InfoContainer::IsEnabled(const E id) {
	return items_->at(id)->first;
}

bool InfoContainer::IsInitizalized() {
	return (items_->size() > 1);
}

void InfoContainer::Init() {}
void InfoContainer::Reload() {}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
