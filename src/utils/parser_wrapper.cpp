#include "parser_wrapper.h"

#include "utils/logger.h"

#include "third_party_libs/pugixml/pugixml.h"

#include <sstream>

namespace parser_wrapper {

struct DataNode::Impl {
	std::shared_ptr<pugi::xml_document> xml_doc{std::make_shared<pugi::xml_document>()};
	pugi::xml_node curren_xml_node{};
	std::string filter_name{};
};

DataNode::DataNode() :
	impl_{std::make_unique<Impl>()} {}

DataNode::DataNode(const std::filesystem::path &file_name) :
	DataNode()
{
	if (auto result = impl_->xml_doc->load_file(file_name.c_str()); !result) {
		std::ostringstream buffer;
		buffer << "..." << result.description() << "\r\n" << " (file: " << file_name << ")" << "\r\n";
		err_log("%s", buffer.str().c_str());
	}
	impl_->curren_xml_node = impl_->xml_doc->document_element();
}

DataNode::DataNode(const DataNode &d) :
	impl_{std::make_unique<Impl>(*d.impl_)} {}

DataNode::DataNode(DataNode &&d) noexcept = default;

DataNode::~DataNode() = default;

DataNode &DataNode::operator=(const DataNode &d) {
	if (this != &d) {
		*impl_ = *d.impl_;
	}
	return *this;
}

DataNode &DataNode::operator=(DataNode &&d) noexcept = default;

bool DataNode::IsEmpty() const {
	return impl_->curren_xml_node.empty();
}

const char *DataNode::GetName() const {
	return impl_->curren_xml_node.name();
}

const char *DataNode::GetValue(const std::string &key) const {
	if (key.empty()) {
		return impl_->curren_xml_node.child_value();
	}
	return impl_->curren_xml_node.attribute(key.c_str()).value();
}

void DataNode::GoToRadix() {
	impl_->curren_xml_node = impl_->xml_doc->document_element();
}

void DataNode::GoToParent() {
	impl_->curren_xml_node = impl_->curren_xml_node.parent();
}

bool DataNode::HaveChild(const std::string &key) {
	return impl_->curren_xml_node.child(key.c_str());
}

bool DataNode::GoToChild(const std::string &key) {
	if (impl_->curren_xml_node.child(key.c_str())) {
		impl_->curren_xml_node = impl_->curren_xml_node.child(key.c_str());
		return true;
	}
	return false;
}

bool DataNode::GoToSibling(const std::string &key) {
	if (impl_->curren_xml_node.parent().child(key.c_str())) {
		impl_->curren_xml_node = impl_->curren_xml_node.parent().child(key.c_str());
		return true;
	}
	return false;
}

bool DataNode::HavePrevious() {
	return impl_->curren_xml_node.previous_sibling();
}

void DataNode::GoToPrevious() {
	impl_->curren_xml_node = impl_->curren_xml_node.previous_sibling();
}

bool DataNode::HaveNext() {
	return impl_->curren_xml_node.next_sibling();
}

void DataNode::GoToNext() {
	impl_->curren_xml_node = impl_->curren_xml_node.next_sibling();
}

DataNode::operator bool() const {
	return !impl_->curren_xml_node.empty();
}

bool DataNode::operator==(const DataNode &d) const {
	return impl_->curren_xml_node == d.impl_->curren_xml_node;
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
	if (impl_->filter_name.empty()) {
		impl_->curren_xml_node = impl_->curren_xml_node.next_sibling();
	} else {
		impl_->curren_xml_node = impl_->curren_xml_node.next_sibling(impl_->filter_name.c_str());
	}
	return *this;
}

const DataNode DataNode::operator++(int) {
	auto retval = *this;
	++*this;
	return retval;
}

DataNode &DataNode::operator--() {
	impl_->curren_xml_node = impl_->curren_xml_node.previous_sibling();
	return *this;
}

const DataNode DataNode::operator--(int) {
	auto retval = *this;
	--*this;
	return retval;
}

[[nodiscard]] iterators::Range<DataNode> DataNode::Children() {
	auto node = *this;
	node.impl_->curren_xml_node = node.impl_->curren_xml_node.first_child();
	return iterators::Range(node);
}

[[nodiscard]] iterators::Range<DataNode> DataNode::Children(const std::string &key) {
	auto node = *this;
	node.impl_->filter_name = key;
	node.impl_->curren_xml_node = node.impl_->curren_xml_node.child(key.c_str());
	return iterators::Range(node);
}

} // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
