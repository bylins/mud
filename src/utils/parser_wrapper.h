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
#include <string>
#include <utility>
#include <vector>

#include "engine/structs/iterators.h"

namespace parser_wrapper {

class DataNode {
 public:
	using value_type        = DataNode;
	using pointer           = DataNode*;
	using reference         = DataNode&;

	DataNode();

	explicit DataNode(const std::filesystem::path &file_name);

	DataNode(const DataNode &d);

	DataNode(DataNode &&d) noexcept;

	~DataNode();

	DataNode &operator=(const DataNode &d);

	DataNode &operator=(DataNode &&d) noexcept;

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
	 * Переместиться к дочернему узлу key, если такой узел есть.
	 */
	bool GoToChild(const std::string &key);

	/*
	 * Переместиться к сестринскому узлу key, если такой узел есть.
	 */
	bool GoToSibling(const std::string &key);

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
	[[nodiscard]] iterators::Range<DataNode> Children(const std::string &key);

	/*
	 * issue.vedun-editor: enumerate this node's attributes as (name, value) pairs, in document
	 * order. The editor reflects an arbitrary node's attributes into form fields.
	 */
	[[nodiscard]] std::vector<std::pair<std::string, std::string>> Attributes() const;

	/*
	 * issue.vedun-editor: set an attribute's value on the current node (creating the attribute
	 * if absent). Mutates the shared document. Returns false on failure.
	 */
	bool SetValue(const std::string &key, const std::string &value);

	/*
	 * issue.vedun-editor: append an empty child element with the given tag name to the current
	 * node (mutates the shared document). Returns the new child, sharing this node's document.
	 */
	DataNode AddChild(const std::string &name);

	/*
	 * issue.vedun-editor: remove the given child element from the current node (mutates the
	 * shared document). Returns false if it is not a child of this node.
	 */
	bool RemoveChild(const DataNode &child);

	/*
	 * issue.vedun-editor: reorder a child element among its siblings (mutates the shared
	 * document). Returns false if it is already at the start/end. The editor's move menu uses
	 * these; combined with AddChild they cover inserting at an arbitrary position.
	 */
	bool MoveChildUp(const DataNode &child);
	bool MoveChildDown(const DataNode &child);

	/*
	 * issue.vedun-editor: serialize the whole document to a file (pugixml). Returns false on
	 * failure. The editor writes to a temp file then renames for an atomic commit.
	 */
	[[nodiscard]] bool Save(const std::filesystem::path &file) const;

 private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} //parcer_wrapper

#endif // PARSER_WRAPPER

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
