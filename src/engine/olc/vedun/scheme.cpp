/**
 \file scheme.cpp - a part of the Bylins engine.
 \brief Vedun editor: .scheme sidecar loader (issue.vedun-editor Phase 2).
*/

#include "scheme.h"

#include "utils/parser_wrapper.h"
#include "utils/logger.h"

#include <string>

namespace vedun {

FieldType ParseFieldType(const std::string &name) {
	if (name == "int") { return FieldType::kInt; }
	if (name == "double") { return FieldType::kDouble; }
	if (name == "bool") { return FieldType::kBool; }
	if (name == "enum") { return FieldType::kEnum; }
	if (name == "flagset") { return FieldType::kFlagset; }
	if (name == "list") { return FieldType::kList; }
	if (name == "intlist") { return FieldType::kIntList; }
	if (name == "multiline") { return FieldType::kMultiline; }
	return FieldType::kString;
}

const char *FieldTypeName(FieldType type) {
	switch (type) {
		case FieldType::kInt: return "int";
		case FieldType::kDouble: return "double";
		case FieldType::kBool: return "bool";
		case FieldType::kEnum: return "enum";
		case FieldType::kFlagset: return "flagset";
		case FieldType::kList: return "list";
		case FieldType::kIntList: return "intlist";
		case FieldType::kMultiline: return "multiline";
		case FieldType::kString:
		default: return "string";
	}
}

const AttrDef *TagDef::FindAttr(const std::string &attr) const {
	for (const auto &a : attrs) {
		if (a.name == attr) {
			return &a;
		}
	}
	return nullptr;
}

const ChildDef *TagDef::FindChild(const std::string &tag) const {
	for (const auto &c : children) {
		if (c.tag == tag) {
			return &c;
		}
	}
	return nullptr;
}

const TagDef *Scheme::FindTag(const std::string &tag) const {
	const auto it = tags_.find(tag);
	return it == tags_.end() ? nullptr : &it->second;
}

bool Scheme::IsProhibited(const std::string &element_id) const {
	return prohibited_.find(element_id) != prohibited_.end();
}

void Scheme::AddTag(TagDef tag) {
	const std::string key = tag.parent.empty() ? tag.name : tag.parent + ">" + tag.name;
	tags_[key] = std::move(tag);
}

const TagDef *Scheme::FindTag(const std::string &tag, const std::string &parent) const {
	if (!parent.empty()) {
		const auto it = tags_.find(parent + ">" + tag);
		if (it != tags_.end()) {
			return &it->second;
		}
	}
	return FindTag(tag);
}

namespace {

// Read an optional numeric bound from an attribute; absent/blank -> nullopt.
std::optional<long> ReadOptionalLong(const char *value) {
	if (value == nullptr || *value == '\0') {
		return std::nullopt;
	}
	try {
		return std::stol(value);
	} catch (const std::exception &) {
		return std::nullopt;
	}
}

bool ReadBool(const char *value) {
	return value != nullptr && (*value == 'Y' || *value == 'y' || *value == '1'
		|| *value == 'T' || *value == 't');
}

AttrDef ParseAttr(parser_wrapper::DataNode &node) {
	AttrDef attr;
	attr.name = node.GetValue("name");
	attr.type = ParseFieldType(node.GetValue("type"));
	attr.enum_type = node.GetValue("enum");
	attr.readonly = ReadBool(node.GetValue("readonly"));
	attr.min = ReadOptionalLong(node.GetValue("min"));
	attr.max = ReadOptionalLong(node.GetValue("max"));
	attr.desc = node.GetValue("desc");
	return attr;
}

TagDef ParseTag(parser_wrapper::DataNode &node) {
	TagDef tag;
	tag.name = node.GetValue("name");
	tag.parent = node.GetValue("parent");
	tag.desc = node.GetValue("desc");
	for (auto &child : node.Children()) {
		const std::string kind = child.GetName();
		if (kind == "attr") {
			tag.attrs.push_back(ParseAttr(child));
		} else if (kind == "child") {
			ChildDef cd;
			cd.tag = child.GetValue("tag");
			cd.multiple = ReadBool(child.GetValue("multiple"));
			tag.children.push_back(cd);
		}
	}
	return tag;
}

} // namespace

Scheme LoadScheme(const std::filesystem::path &file) {
	Scheme scheme;
	if (!std::filesystem::exists(file)) {
		return scheme;   // no sidecar -> untyped editing, which is fine
	}
	parser_wrapper::DataNode doc(file);
	if (doc.IsEmpty()) {
		err_log("Vedun: scheme '%s' is empty or unreadable.", file.string().c_str());
		return scheme;
	}
	for (auto &node : doc.Children()) {
		const std::string kind = node.GetName();
		if (kind == "tag") {
			TagDef tag = ParseTag(node);
			if (tag.name.empty()) {
				err_log("Vedun: scheme '%s' has a <tag> with no name.", file.string().c_str());
				continue;
			}
			scheme.AddTag(std::move(tag));
		} else if (kind == "prohibited") {
			for (auto &element : node.Children()) {
				const std::string id = element.GetValue("id");
				if (!id.empty()) {
					scheme.AddProhibited(id);
				}
			}
		}
	}
	return scheme;
}

std::filesystem::path SchemePathFor(const std::filesystem::path &data_file) {
	// issue.thing-names: a "<stem>.scheme" sidecar may live next to the data file (the original
	// layout, e.g. spells.xml + spells.scheme) OR one or more levels up -- so a per-type scheme can be
	// shared across per-locale data dirs (cfg/messages/spell_msg.scheme covers cfg/messages/<lang>/
	// spell_msg.xml). Search the data file's directory first, then walk up a few parents, and return
	// the first existing match. If none exists, fall back to the same-dir path (LoadScheme then yields
	// an empty scheme, i.e. untyped editing -- unchanged behaviour).
	const std::string name = data_file.stem().string() + ".scheme";
	constexpr int kMaxSchemeSearchDepth = 4;
	std::filesystem::path dir = data_file.parent_path();
	for (int depth = 0; depth < kMaxSchemeSearchDepth; ++depth) {
		std::filesystem::path candidate = dir / name;
		std::error_code ec;
		if (std::filesystem::exists(candidate, ec)) {
			return candidate;
		}
		if (!dir.has_parent_path() || dir.parent_path() == dir) {
			break;
		}
		dir = dir.parent_path();
	}
	std::filesystem::path fallback = data_file;
	fallback.replace_extension(".scheme");
	return fallback;
}

} // namespace vedun

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
