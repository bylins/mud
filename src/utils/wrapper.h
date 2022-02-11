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

namespace parser_wrapper {

    class BaseDataNode : public std::iterator<
            std::forward_iterator_tag,  // iterator_category
            std::ptrdiff_t,             // difference
            BaseDataNode,                   // value_type
            BaseDataNode *,                 // pointer
            BaseDataNode &                  // reference
    > {
    protected:
        using DocPtr = std::shared_ptr<pugi::xml_document>;

        DocPtr xml_doc_;
        // Получить указатель на ноду непосредственно штатными средстваим нельзя.
        pugi::xml_node curren_xml_node_;

    public:
        BaseDataNode() :
                xml_doc_{std::make_shared<pugi::xml_document>()} {};

        explicit BaseDataNode(std::filesystem::path &file_name);

        BaseDataNode(const BaseDataNode &d);

        ~BaseDataNode() = default;

        BaseDataNode &operator=(const BaseDataNode &d) = default;

        explicit operator bool() const;

        bool operator==(const BaseDataNode &d) const;

        bool operator!=(const BaseDataNode &other) const;

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
        [[maybe_unused]] [[nodiscard]] bool IsNotEmpty() const { return !IsEmpty(); };

        /*
         * Значение пары с указанным ключом. Пустое значение соответствует
         * паре с ключом, равным имени текущего узла.
         */
        [[nodiscard]] const char *GetValue(const std::string &key = "") const;

        /*
         *  Переместиться к коревому узлу.
         */
        [[maybe_unused]] void GoToRadix();

        /*
         *  Переместиться к родительскому узлу, если текущий узел не корневой.
         */
        [[maybe_unused]] void GoToParent();

        /*
         *  Имеется ли дочерний узел с таким ключом.
         */
        [[maybe_unused]] bool HaveChild(const std::string &key);

        /*
         * Переместиться к дочернему узлу key.
         */
        [[maybe_unused]] void GoToChild(const std::string &key);

        /*
         * Переместиться к сестринскому узлу key, если такой узел есть.
         * Результат - сестринский узел или пусто.
         */
        [[maybe_unused]] void GoToSibling(const std::string &key);

        /*
         * Имеет предыдущий сестринский узел.
         */
        [[maybe_unused]] bool HavePrevious();

        /*
         * Переместиться к предыдущему сестринскому узлу.
         */
        [[maybe_unused]] void GoToPrevious();

        /*
         * Имеет следующий сестринский узел.
         */
        [[maybe_unused]] bool HaveNext();

        /*
         * Переместиться к следующему сестринскому узлу.
         */
        [[maybe_unused]] void GoToNext();

        /*
         * Имеет следующий одноименный сестринский узел.
         */
        //[[maybe_unused]] bool HaveNamesake();

        /*
         * Переместиться к сестринскому узлу с таким же ключом.
         * Результат - сестринский узел или пусто.
         */
        //[[maybe_unused]] void GoToNamesake();

    };

    class NamedDataNode : public BaseDataNode {
    public:
        explicit NamedDataNode(std::filesystem::path &file_name) :
                BaseDataNode(file_name) {};

        class NamedRange {
            friend class NamedDataNode;
        public:
            [[nodiscard]] auto begin() const { return *begin_; };
            [[nodiscard]] auto end() const { return *end_; };

        private:
            std::shared_ptr<NamedDataNode> begin_;
            std::shared_ptr<NamedDataNode> end_;

            [[maybe_unused]] NamedRange(NamedDataNode *node, const std::string &name);
        };

        /*
         * Список дочерних узлов с именем key для loop range based итерации,
         */
        [[nodiscard]] NamedRange Childen(const std::string &key);

        BaseDataNode &operator++();

        const BaseDataNode operator++(int);

        BaseDataNode &operator--();

        const BaseDataNode operator--(int);
    };

} //parcer_wrapper

#endif // PARSER_WRAPPER