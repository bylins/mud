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

	using StorageType = std::map<Vnum, int>;
	/*
	 * begin()/end() возвращают константные итераторы
	 * для исключения возможности модификации хранилища в обход сеттеров
	*/
	StorageType::const_iterator begin() const  {return currency_storage_.cbegin(); }
	StorageType::const_iterator end() const { return currency_storage_.cend(); }
	bool empty() const { return currency_storage_.empty(); }

public:
	// при установке значения в 0 или меньше - валюта удалится
	void SetCurrencyAmount(Vnum currency_vnum, int value);
	// value может быть отрицательным, если результат будет 0 или меньше - валюта удалится
	void ModifyCurrencyAmount(Vnum currency_vnum, int value);
	std::optional<int> GetCurrencyAmount(Vnum currency_vnum) const;

	/*
	 * Сохранение/чтение в/из строки для записи в файл.
	 * Для надежности используется text_id валюты вместо внума.
	*/
	std::optional<std::string> serialize();
	void deserealize(const std::string &data);

private:
	void Validate();
	static std::optional<std::reference_wrapper<const CurrencyInfo>> FindCurrencyByVnum(Vnum vnum);
	static std::optional<std::reference_wrapper<const CurrencyInfo>> FindCurrencyByTextId(const std::string &text_id);

private:
	const bool account_shared_;
	// ключ - vnum валюты, значение - количество
	StorageType currency_storage_;
	// кап потом вынести в конфиг каждой валюты. пока его нет, установить хоть какой-то тут
	const int currency_amount_cap_ = 1000000;
};

} //namespace currencies
