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

DataNode DataNode::NewDocument() {
	DataNode d;
	// Курсор на сам документ (pugi::xml_document : xml_node), чтобы первый AddChild создал корень.
	d.impl_->curren_xml_node = *d.impl_->xml_doc;
	return d;
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

std::vector<std::pair<std::string, std::string>> DataNode::Attributes() const {
	std::vector<std::pair<std::string, std::string>> out;
	for (auto attr = impl_->curren_xml_node.first_attribute(); attr; attr = attr.next_attribute()) {
		out.emplace_back(attr.name(), attr.value());
	}
	return out;
}

bool DataNode::SetValue(const std::string &key, const std::string &value) {
	auto node = impl_->curren_xml_node;
	// Пустой ключ = текст самого узла (симметрично GetValue("")): пишем inner-text.
	// pugi::xml_node::text().set() создаёт/обновляет PCDATA-потомка, не трогая атрибуты
	// и прочих потомков. Существующие файлы пишутся с непустым ключом (имя атрибута),
	// так что их запись не меняется.
	if (key.empty()) {
		return node.text().set(value.c_str());
	}
	auto attr = node.attribute(key.c_str());
	if (!attr) {
		attr = node.append_attribute(key.c_str());
	}
	return attr.set_value(value.c_str());
}

bool DataNode::RemoveValue(const std::string &key) {
	return impl_->curren_xml_node.remove_attribute(key.c_str());
}

bool DataNode::Save(const std::filesystem::path &file) const {
	// pugixml's default parser does not retain the <?xml ...?> declaration, so a plain
	// re-save (e.g. a Vedun edit) would re-emit a bare <?xml version="1.0"?> and drop
	// encoding="koi8-r" -- the project convention for every cfg file. Restore it here so
	// editing a cfg file through Vedun no longer silently strips the encoding declaration.
	auto &doc = *impl_->xml_doc;
	pugi::xml_node decl = doc.first_child();
	if (decl.type() != pugi::node_declaration) {
		decl = doc.prepend_child(pugi::node_declaration);
	}
	if (!decl.attribute("version")) {
		decl.append_attribute("version");
	}
	decl.attribute("version").set_value("1.0");
	if (!decl.attribute("encoding")) {
		decl.append_attribute("encoding");
	}
	decl.attribute("encoding").set_value("koi8-r");
	return doc.save_file(file.string().c_str());
}

std::string DataNode::ToXmlString() const {
	std::ostringstream os;
	impl_->curren_xml_node.print(os, "  ", pugi::format_default);
	return os.str();
}

DataNode DataNode::AddChild(const std::string &name) {
	auto node = impl_->curren_xml_node.append_child(name.c_str());
	DataNode child(*this);                 // copy shares the same xml_doc (shared_ptr)
	child.impl_->curren_xml_node = node;
	return child;
}

bool DataNode::RemoveChild(const DataNode &child) {
	return impl_->curren_xml_node.remove_child(child.impl_->curren_xml_node);
}

bool DataNode::MoveChildUp(const DataNode &child) {
	auto node = child.impl_->curren_xml_node;
	auto prev = node.previous_sibling();
	if (!prev) {
		return false;
	}
	return !impl_->curren_xml_node.insert_move_before(node, prev).empty();
}

bool DataNode::MoveChildDown(const DataNode &child) {
	auto node = child.impl_->curren_xml_node;
	auto next = node.next_sibling();
	if (!next) {
		return false;
	}
	return !impl_->curren_xml_node.insert_move_after(node, next).empty();
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
	node.impl_->filter_name.clear();   // a no-arg Children() iterates ALL children, regardless of any
	                                   // filter inherited from a node copied out of a Children(key) range.
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
