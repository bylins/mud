#include "wrapper.h"

#include <iostream>

namespace parser_wrapper {

    BaseDataNode::BaseDataNode(std::filesystem::path &file_name) :
            BaseDataNode()
    {
        if (auto result = xml_doc_->load_file(file_name.c_str()); !result) {
            std::ostringstream buffer;
            buffer << "..." << result.description() << std::endl << " (file: " << file_name << ")" << std::endl;
            std::cout << buffer.str(); //ABYRVALG не забыть убрать
        }
        curren_xml_node_ = xml_doc_->document_element();
    }

    BaseDataNode::BaseDataNode(const BaseDataNode &d) :
            BaseDataNode()
    {
        xml_doc_ = d.xml_doc_;
        curren_xml_node_ = d.curren_xml_node_;
    }

    bool BaseDataNode::IsEmpty() const {
        return curren_xml_node_.empty();
    }

    const char *BaseDataNode::GetName() const {
        return curren_xml_node_.name();
    }

    const char *BaseDataNode::GetValue(const std::string &key) const {
        if (key.empty()) {
            return curren_xml_node_.child_value();
        }
        return curren_xml_node_.attribute(key.c_str()).value();
    }

    [[maybe_unused]] void BaseDataNode::GoToRadix() {
        curren_xml_node_ = xml_doc_->document_element();
    }

    [[maybe_unused]] void BaseDataNode::GoToParent() {
        curren_xml_node_ = curren_xml_node_.parent();
    }

    [[maybe_unused]] bool BaseDataNode::HaveChild(const std::string &key) {
        return curren_xml_node_.child(key.c_str());
    }

    [[maybe_unused]] void BaseDataNode::GoToChild(const std::string &key) {
        curren_xml_node_ = curren_xml_node_.child(key.c_str());
    }

    [[maybe_unused]] void BaseDataNode::GoToSibling(const std::string &key) {
        curren_xml_node_ = curren_xml_node_.parent().child(key.c_str());
    }

    [[maybe_unused]] bool BaseDataNode::HavePrevious() {
        return curren_xml_node_.previous_sibling();
    }

    [[maybe_unused]] void BaseDataNode::GoToPrevious() {
        curren_xml_node_ = curren_xml_node_.previous_sibling();
    }

    [[maybe_unused]] bool BaseDataNode::HaveNext() {
        return curren_xml_node_.next_sibling();
    }

    [[maybe_unused]] void BaseDataNode::GoToNext() {
        curren_xml_node_ = curren_xml_node_.next_sibling();
    }

/*    [[maybe_unused]] bool DataNode::HaveNamesake() {
        return curren_xml_node_.next_sibling(curren_xml_node_.name());
    }

    [[maybe_unused]] void DataNode::GoToNamesake() {
        curren_xml_node_ = curren_xml_node_.next_sibling(curren_xml_node_.name());
    }*/

    BaseDataNode::operator bool() const {
        return !curren_xml_node_.empty();
    }

    bool BaseDataNode::operator==(const BaseDataNode &d) const {
        return curren_xml_node_ == d.curren_xml_node_;
    }

    bool BaseDataNode::operator!=(const BaseDataNode &other) const {
        return !(*this == other);
    }

    BaseDataNode::reference BaseDataNode::operator*() {
        return *this;
    }

    BaseDataNode::pointer BaseDataNode::operator->() {
        return this;
    }

    [[nodiscard]] NamedDataNode::NamedRange NamedDataNode::Childen(const std::string &key) {
        return {this, key};
    }

    BaseDataNode &NamedDataNode::operator++() {
        curren_xml_node_ = curren_xml_node_.next_sibling(curren_xml_node_.name());
        return *this;
    }

    const BaseDataNode NamedDataNode::operator++(int) {
        auto retval = *this;
        ++*this;
        return retval;
    }

    BaseDataNode &NamedDataNode::operator--() {
        curren_xml_node_ = curren_xml_node_.previous_sibling(curren_xml_node_.name());
        return *this;
    }

    const BaseDataNode NamedDataNode::operator--(int) {
        auto retval = *this;
        --*this;
        return retval;
    }

    [[maybe_unused]] NamedDataNode::NamedRange::NamedRange(NamedDataNode *node, const std::string &name) :
            begin_{std::make_shared<NamedDataNode>(*node)},
            end_{std::make_shared<NamedDataNode>(*node)} {
        begin_->curren_xml_node_ = begin_->curren_xml_node_.child(name.c_str());
        end_->curren_xml_node_ = pugi::xml_node();
    }

} // namespace