/**
 \file enum_registry.h - a part of the Bylins engine.
 \brief Vedun editor: runtime enum-by-type-name registry (issue.vedun-editor Phase 2).
 \details A .scheme declares an attribute's type as e.g. `enum<EElement>` -- a string. To build a
          numbered pick-list the editor needs that enum's members at runtime, keyed by the string
          type name. NAMES_OF<E>() (meta_enum.h) supplies the (value,name) data per enum; this
          registry erases the type so the scheme layer can look it up by name.
*/

#ifndef BYLINS_SRC_ENGINE_OLC_VEDUN_ENUM_REGISTRY_H_
#define BYLINS_SRC_ENGINE_OLC_VEDUN_ENUM_REGISTRY_H_

#include "engine/structs/meta_enum.h"   // NAMES_OF<E>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace vedun {

struct EnumMember {
	std::string name;   // e.g. "kLight"
	long value;         // the underlying enum value
};

class EnumRegistry {
 public:
	static EnumRegistry &Instance();

	// Register an enum under the type name used in schemes (e.g. Register<EElement>("EElement")).
	// Pulls the members from NAMES_OF<E>() -- so E must have a NAMES_OF specialization.
	template<typename E>
	void Register(const std::string &type_name) {
		std::vector<EnumMember> members;
		for (const auto &[value, name] : NAMES_OF<E>()) {
			// Skip the conventional end-of-enum sentinel: it is not a selectable value.
			if (name == "kLast") {
				continue;
			}
			members.push_back({name, static_cast<long>(value)});
		}
		registry_[type_name] = std::move(members);
	}

	// Register an enum by an explicit list of member names, for enums parsed via inline strcmp that
	// have no NAMES_OF map (e.g. EActionTarget / EActionBase / EAlign). Values are synthetic indices --
	// the editor's pick-lists and validation key on the names, which are what land in the XML.
	void RegisterNames(const std::string &type_name, const std::vector<std::string> &names) {
		std::vector<EnumMember> members;
		long index = 0;
		for (const auto &name : names) {
			members.push_back({name, index++});
		}
		registry_[type_name] = std::move(members);
	}

	[[nodiscard]] bool Known(const std::string &type_name) const;
	// Members of a registered enum (in value order), or nullptr if the type name is unknown.
	[[nodiscard]] const std::vector<EnumMember> *Members(const std::string &type_name) const;
	// Canonical member name for a value (nullptr if unknown); value for a member name (nullopt if unknown).
	[[nodiscard]] const std::string *NameOf(const std::string &type_name, long value) const;
	[[nodiscard]] std::optional<long> ValueOf(const std::string &type_name, const std::string &member) const;

 private:
	std::map<std::string, std::vector<EnumMember>> registry_;
};

// Register the enums the editor schemes reference (idempotent). Call once before using the registry.
void RegisterEditorEnums();

} // namespace vedun

#endif // BYLINS_SRC_ENGINE_OLC_VEDUN_ENUM_REGISTRY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
