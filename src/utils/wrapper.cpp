#include "wrapper.h"

#include <iostream>

namespace parser_wrapper {

DataNode::DataNode(const std::filesystem::path &file_name) :
	DataNode()
{
	if (auto result = xml_doc_->load_file(file_name.c_str()); !result) {
		std::ostringstream buffer;
		buffer << "..." << result.description() << std::endl << " (file: " << file_name << ")" << std::endl;
		std::cout << buffer.str(); //ABYRVALG не забыть убрать
	}
	curren_xml_node_ = xml_doc_->document_element();
}

DataNode::DataNode(const DataNode &d) :
	DataNode()
{
	xml_doc_ = d.xml_doc_;
	curren_xml_node_ = d.curren_xml_node_;
}

bool DataNode::IsEmpty() const {
	return curren_xml_node_.empty();
}

const char *DataNode::GetName() const {
	return curren_xml_node_.name();
}

const char *DataNode::GetValue(const std::string &key) const {
	if (key.empty()) {
		return curren_xml_node_.child_value();
	}
	return curren_xml_node_.attribute(key.c_str()).value();
}

void DataNode::GoToRadix() {
	curren_xml_node_ = xml_doc_->document_element();
}

void DataNode::GoToParent() {
	curren_xml_node_ = curren_xml_node_.parent();
}

bool DataNode::HaveChild(const std::string &key) {
	return curren_xml_node_.child(key.c_str());
}

void DataNode::GoToChild(const std::string &key) {
	curren_xml_node_ = curren_xml_node_.child(key.c_str());
}

void DataNode::GoToSibling(const std::string &key) {
	curren_xml_node_ = curren_xml_node_.parent().child(key.c_str());
}

bool DataNode::HavePrevious() {
	return curren_xml_node_.previous_sibling();
}

void DataNode::GoToPrevious() {
	curren_xml_node_ = curren_xml_node_.previous_sibling();
}

bool DataNode::HaveNext() {
	return curren_xml_node_.next_sibling();
}

void DataNode::GoToNext() {
	curren_xml_node_ = curren_xml_node_.next_sibling();
}

DataNode::operator bool() const {
	return !curren_xml_node_.empty();
}

bool DataNode::operator==(const DataNode &d) const {
	return curren_xml_node_ == d.curren_xml_node_;
}

bool DataNode::operator!=(const DataNode &other) const {
	return !(*this == other);
}

DataNode::reference DataNode::operator*() {
	return *this;
}

DataNode::pointer DataNode::operator->() {
	return this;
}

DataNode &DataNode::operator++() {
	curren_xml_node_ = curren_xml_node_.next_sibling();
	return *this;
}

const DataNode DataNode::operator++(int) {
	auto retval = *this;
	++*this;
	return retval;
}

DataNode &DataNode::operator--() {
	curren_xml_node_ = curren_xml_node_.previous_sibling();
	return *this;
}

const DataNode DataNode::operator--(int) {
	auto retval = *this;
	--*this;
	return retval;
}

[[nodiscard]] NodeRange<DataNode> DataNode::Children() {
	auto node = *this;
	node->curren_xml_node_ = node->curren_xml_node_.first_child();
	return NodeRange(node);
}

[[nodiscard]] NodeRange<DataNode::NameIterator> DataNode::Children(const std::string &key) {
	auto it = NameIterator(*this);
	(*it)->GoToChild(key);
	return NodeRange(it);
}

DataNode::NameIterator &DataNode::NameIterator::operator++() {
	node_->curren_xml_node_ = node_->curren_xml_node_.next_sibling(node_->curren_xml_node_.name());
	return *this;
}

const DataNode::NameIterator DataNode::NameIterator::operator++(int) {
	auto retval = *this;
	++*this;
	return retval;
}

} // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
