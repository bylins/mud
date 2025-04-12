#pragma once

#include "engine/structs/structs.h"

#include <map>
#include <string>
#include <optional>

namespace currencies {

class CurrencyInfo;

// класс для хранения альтернативных валют в аккаунте или инвентаре персонажа
class CurrencyContainer
{
public:
	// account_shared==true - контейнер для аккаунта
	// account_shared==false - контейнер для персонажа
	CurrencyContainer(bool account_shared);

public:
	std::optional<std::string> serialize();
	void deserealize(const std::string &data);

private:
	void Validate();
	static std::optional<std::reference_wrapper<const CurrencyInfo>> FindCurrencyByVnum(Vnum vnum);
	static std::optional<std::reference_wrapper<const CurrencyInfo>> FindCurrencyByTextId(const std::string &text_id);

private:
	const bool account_shared_;
	// ключ - vnum валюты, значение - количество
	std::map<int, int> currency_storage_;
};

} //namespace currencies
