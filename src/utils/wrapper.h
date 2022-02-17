/*
 \authors Created by Sventovit
 \date 04.06.2021.
 \brief Обертка над парсером файлов.
 \details Модуль-обертка над используемым парсером файлов. Желательно нигде не использовать напрямую сам парсер,
 а только обертку, добавляя в интерфейс нужные функции по необходимости. Иначе при желании или потребности сменить
 парсер придется переписывать весь код, работающий с ним. При использовании обертки достаточно будет переработать
 только ее.

 Идея следующая. Все данные представляются в виде DataNode, для внешнего пользователя являющейся итератором над
 древовидной структорой пар key-value. Ключ - строка, значением может быть только С-строка или DataNode, которая,
 в свою очередь, содержит коллекцию, возможно пустую, пар. В итоге получается дерево узлов, по которому можно
 перемещаться вверх и вниз или скопировать любой узел, чтобы работать с ним отдельно. Содержимое тэга xml (если
 внутри - парсер именно xml), доступно по пустому ключу. Но в целом, в формате файла лучше использовать знаения
 атрибутов, а не содержимое тэгов,
 */

#ifndef PARSER_WRAPPER
#define PARSER_WRAPPER

#include <filesystem>
#include <memory>

#include "pugixml.h"
#include "structs/iterators.h"

namespace parser_wrapper {

class DataNode : public std::iterator<
	std::forward_iterator_tag,  // iterator_category
	std::ptrdiff_t,             // difference
	DataNode,                   // value_type
	DataNode *,                 // pointer
	DataNode &                  // reference
> {
 protected:
	using DocPtr = std::shared_ptr<pugi::xml_document>;

	DocPtr xml_doc_;
	// Получить указатель на ноду непосредственно штатными средстваим нельзя.
	pugi::xml_node curren_xml_node_;

 public:
	DataNode() :
		xml_doc_{std::make_shared<pugi::xml_document>()},
		curren_xml_node_{pugi::xml_node()} {};

	explicit DataNode(const std::filesystem::path &file_name);

	DataNode(const DataNode &d);

	~DataNode() = default;

	class NameIterator {
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using value_type        = DataNode;
		using pointer           = DataNode*;
		using reference         = DataNode&;

		std::shared_ptr<DataNode> node_;
	 public:
		NameIterator() :
			node_{std::make_shared<DataNode>()} {};

		explicit NameIterator(DataNode &node) :
			node_{std::make_shared<DataNode>(node)}
		{};

		NameIterator &operator++();

		const NameIterator operator++(int);

		bool operator==(const NameIterator &it) const { return node_->curren_xml_node_ == it.node_->curren_xml_node_; };

		bool operator!=(const NameIterator &other) const { return !(*this == other); };

		reference operator*() const { return *node_; }

		pointer operator->() { return node_.get(); }

	};

	DataNode &operator=(const DataNode &d) = default;

	explicit operator bool() const;

	DataNode &operator++();

	const DataNode operator++(int);

	DataNode &operator--();

	const DataNode operator--(int);

	bool operator==(const DataNode &d) const;

	bool operator!=(const DataNode &other) const;

	reference operator*();

	pointer operator->();

	/*
	 *  Имя текущего узла.
	 */
	[[nodiscard]] const char *GetName() const;

	/*
	 *  Проверка, что текущий узел пуст.
	 */
	[[nodiscard]] bool IsEmpty() const;

	/*
	 *  Проверка, что текущий узел не пуст.
	 */
	[[nodiscard]] bool IsNotEmpty() const { return !IsEmpty(); };

	/*
	 * Значение пары с указанным ключом. Пустое значение соответствует
	 * паре с ключом, равным имени текущего узла.
	 */
	[[nodiscard]] const char *GetValue(const std::string &key = "") const;

	/*
	 *  Переместиться к коревому узлу.
	 */
	void GoToRadix();

	/*
	 *  Переместиться к родительскому узлу, если текущий узел не корневой.
	 */
	void GoToParent();

	/*
	 *  Имеется ли дочерний узел с таким ключом.
	 */
	bool HaveChild(const std::string &key);

	/*
	 * Переместиться к дочернему узлу key.
	 */
	void GoToChild(const std::string &key);

	/*
	 * Переместиться к сестринскому узлу key, если такой узел есть.
	 * Результат - сестринский узел или пусто.
	 */
	void GoToSibling(const std::string &key);

	/*
	 * Имеет предыдущий сестринский узел.
	 */
	bool HavePrevious();

	/*
	 * Переместиться к предыдущему сестринскому узлу.
	 */
	void GoToPrevious();

	/*
	 * Имеет следующий сестринский узел.
	 */
	bool HaveNext();

	/*
	 * Переместиться к следующему сестринскому узлу.
	 */
	void GoToNext();

	/*
	 * Диапазон всех дочерних узлов.
	 */
	[[nodiscard]] iterators::Range<DataNode> Children();

	/*
	 * Диапазон дочерних узлов с именем key,
	 */
	[[nodiscard]] iterators::Range<NameIterator> Children(const std::string &key);

};

} //parcer_wrapper

#endif // PARSER_WRAPPER

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
