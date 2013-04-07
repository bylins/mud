#include "dictionary.hpp"
#include "shop_ext.hpp"

// комментарий на русском в надежде починить кодировки bitbucket

Dictionary::Dictionary(DictionaryMode mode)
{
	switch(mode)
	{
		case SHOP:
			fill_shop_dictionary(dictionary_);
			break;
	}

}

int Dictionary::Size()
{
	return dictionary_.size();
}

DictionaryItemPtr DictionaryItem::GetDictionaryItem()
{
	return DictionaryItemPtr(new DictionaryItem(this->GetDictionaryName(), this->GetDictionaryTID()));
}

std::string Dictionary::GetNameByNID(unsigned nid)
{
	std::string result = std::string();
	if (dictionary_.size() > nid)
		result = dictionary_[nid]->GetDictionaryName();
	return result;
};

std::string Dictionary::GetTIDByNID(unsigned nid)
{
	std::string result = std::string();
	if (dictionary_.size() > nid)
		result = dictionary_[nid]->GetDictionaryTID();
	return result;
};

void Dictionary::AddToDictionary(DictionaryItemPtr item)
{
	this->dictionary_.push_back(item);
};

std::string Dictionary::GetNameByTID(std::string tid)
{
	std::string result="";
	return result;
};
