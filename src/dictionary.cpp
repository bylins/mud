//#include "dictionary.h"

//#include <memory>
#include "game_economics/shop_ext.h"

Dictionary::Dictionary(DictionaryMode mode) {
	switch (mode) {
		case SHOP: fill_shop_dictionary(dictionary_);
			break;
	}
}

size_t Dictionary::Size() {
	return dictionary_.size();
}

DictionaryItemPtr DictionaryItem::GetDictionaryItem() {
	return std::make_shared<DictionaryItem>(this->GetDictionaryName(), this->GetDictionaryTID());
}

std::string Dictionary::GetNameByNID(size_t nid) {
	std::string result = std::string();
	if (dictionary_.size() > nid) {
		result = dictionary_[nid]->GetDictionaryName();
	}
	return result;
};

std::string Dictionary::GetTIDByNID(size_t nid) {
	std::string result;
	if (dictionary_.size() > nid) {
		result = dictionary_[nid]->GetDictionaryTID();
	}
	return result;
};

void Dictionary::AddToDictionary(const DictionaryItemPtr& item) {
	this->dictionary_.push_back(item);
};

std::string Dictionary::GetNameByTID(const std::string&/* tid*/) {
	std::string result;
	return result;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
