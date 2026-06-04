/**
 \file scheme.h - a part of the Bylins engine.
 \brief Vedun editor: the optional .scheme sidecar (issue.vedun-editor Phase 2).
 \details A data file (e.g. lib/cfg/spells.xml) may have a same-name `.scheme` sidecar describing its
          per-tag grammar: each tag's attribute TYPES (so the editor can show enum pick-lists, ranges,
          help) and which child tags it may contain, plus a list of elements that must not be edited.
          The scheme is XML (parsed via DataNode). It is OPTIONAL: with no scheme every field is a
          plain string box (still validated on save by the loader); the scheme only adds affordances.

          Format:
            <scheme>
              <prohibited><element id="kUndefined"/> ...</prohibited>
              <tag name="spell" desc="...">
                <attr name="id" type="enum" enum="ESpell" readonly="Y" desc="..."/>
                <attr name="danger" type="int" min="0" max="100" desc="..."/>
                <child tag="misc"/> ...
              </tag>
              ...
            </scheme>
*/

#ifndef BYLINS_SRC_ENGINE_OLC_VEDUN_SCHEME_H_
#define BYLINS_SRC_ENGINE_OLC_VEDUN_SCHEME_H_

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace vedun {

enum class FieldType {
	kString,     // free text (the default for un-declared fields)
	kInt,
	kDouble,
	kBool,
	kEnum,       // a single enum token; enum_type names the enum (e.g. "EElement")
	kFlagset,    // pipe-separated enum tokens (e.g. "kMagDamage|kMagAffects")
	kList,       // a list of enum tokens (ordered) -- reserved for later phases
	kMultiline,  // multi-line text
};

[[nodiscard]] FieldType ParseFieldType(const std::string &name);
[[nodiscard]] const char *FieldTypeName(FieldType type);

struct AttrDef {
	std::string name;
	FieldType type{FieldType::kString};
	std::string enum_type;          // for kEnum/kFlagset/kList: the enum name resolved via EnumRegistry
	bool readonly{false};
	std::optional<long> min;        // numeric bounds (kInt/kDouble), optional
	std::optional<long> max;
	std::string desc;
};

struct TagDef {
	std::string name;
	std::string parent;   // optional: scopes this def to a parent tag (tag names repeat across contexts)
	std::string desc;
	std::vector<AttrDef> attrs;
	std::vector<std::string> children;   // allowed child tag names
	[[nodiscard]] const AttrDef *FindAttr(const std::string &attr) const;
};

class Scheme {
 public:
	[[nodiscard]] bool Empty() const { return tags_.empty() && prohibited_.empty(); }
	[[nodiscard]] const TagDef *FindTag(const std::string &tag) const;
	// Context-aware lookup: prefers a def scoped to `parent`, else the global def.
	[[nodiscard]] const TagDef *FindTag(const std::string &tag, const std::string &parent) const;
	[[nodiscard]] bool IsProhibited(const std::string &element_id) const;
	[[nodiscard]] const std::map<std::string, TagDef> &Tags() const { return tags_; }

	void AddTag(TagDef tag);
	void AddProhibited(const std::string &element_id) { prohibited_.insert(element_id); }

 private:
	std::map<std::string, TagDef> tags_;
	std::set<std::string> prohibited_;
};

// Load a .scheme XML file. An absent/unreadable file yields an empty Scheme (the editor still works,
// untyped). A malformed scheme logs and yields whatever parsed.
[[nodiscard]] Scheme LoadScheme(const std::filesystem::path &file);

// The scheme path for a data file: same directory + same stem + ".scheme".
[[nodiscard]] std::filesystem::path SchemePathFor(const std::filesystem::path &data_file);

} // namespace vedun

#endif // BYLINS_SRC_ENGINE_OLC_VEDUN_SCHEME_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
