/**
\file msg_container.h - a part of the Bylins engine.
\authors Created by Claude (issue #3294).
\brief Specialized container for storing in-game player message strings.
\details Stores message strings for game entities (spells, abilities, affects, ...).
		 Built on top of info_container: a MsgSheaf is one entity's set of messages
		 keyed by a per-entity message-type enum, and MsgContainer is an InfoContainer
		 of MsgSheaf items keyed by an element-id enum (e.g. ESkill).
*/

#ifndef BYLINS_SRC_STRUCTS_MSG_CONTAINER_H_
#define BYLINS_SRC_STRUCTS_MSG_CONTAINER_H_

#include "engine/structs/info_container.h"
#include "engine/structs/meta_enum.h"
#include "utils/parse.h"
#include "utils/parser_wrapper.h"
#include "utils/random.h"
#include "utils/utils_string.h"   // issue.thing-names: utils::IsEquivalent for FindByName
#include "utils/logger.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace msg_container {

/**
 * One element's message storage. IdEnum identifies the element (e.g. ESkill);
 * MsgType is a per-element enum of message kinds (e.g. kCastStart, kCastFail).
 * A sheaf may hold several strings of the same type; GetMessage() returns a
 * random one. MsgSheaf is always enabled (it inherits BaseItem only for the id).
 */
template<typename IdEnum, typename MsgType>
class MsgSheaf : public info_container::BaseItem<IdEnum> {
 public:
	using info_container::BaseItem<IdEnum>::BaseItem;

	void AddMessage(MsgType type, std::string text) {
		messages_[type].emplace_back(std::move(text));
	}

	/**
	 * Returns a random string of the given type, or an empty string if none.
	 */
	[[nodiscard]] const std::string &GetMessage(MsgType type) const {
		static const std::string kEmpty;
		const auto it = messages_.find(type);
		if (it == messages_.end() || it->second.empty()) {
			return kEmpty;
		}
		const auto &variants = it->second;
		return variants[static_cast<std::size_t>(number(0, static_cast<int>(variants.size()) - 1))];
	}

	[[nodiscard]] bool HasMessage(MsgType type) const {
		const auto it = messages_.find(type);
		return it != messages_.end() && !it->second.empty();
	}

	// issue.thing-names: the entity's localised display name, parsed from the <name val="..."/> child
	// of the sheaf. Empty when the entity has no name in this string file. (Spells have a single name;
	// cased entities will later extend <name> with <case> children -- a sparse case map.)
	void SetName(std::string name) { name_ = std::move(name); }
	[[nodiscard]] const std::string &GetName() const { return name_; }

 private:
	std::map<MsgType, std::vector<std::string>> messages_;
	std::string name_;
};

/**
 * Builds a single MsgSheaf from one <msg_sheaf id="..."> XML node.
 * Note: parser_wrapper has no inner-tag-text support, so message text lives in
 * the "val" attribute: <message type="kCastFail" val="..." />.
 */
template<typename IdEnum, typename MsgType>
class MsgSheafBuilder : public info_container::IItemBuilder<MsgSheaf<IdEnum, MsgType>> {
 public:
	using Sheaf = MsgSheaf<IdEnum, MsgType>;
	using ItemPtr = std::shared_ptr<Sheaf>;

	ItemPtr Build(parser_wrapper::DataNode &node) override {
		const char *id_str = node.GetValue("id");
		IdEnum id;
		try {
			id = parse::ReadAsConstant<IdEnum>(id_str);
		} catch (const std::exception &) {
			err_log("MsgSheafBuilder: msg_sheaf has unknown or missing 'id' attribute ('%s').", id_str);
			return nullptr;
		}

		auto sheaf = std::make_shared<Sheaf>(id, EItemMode::kEnabled);
		for (auto &message : node.Children("message")) {
			const char *type_str = message.GetValue("type");
			try {
				const MsgType type = parse::ReadAsConstant<MsgType>(type_str);
				sheaf->AddMessage(type, message.GetValue("val"));
			} catch (const std::exception &) {
				err_log("MsgSheafBuilder: message has unknown 'type' ('%s') in msg_sheaf '%s'.", type_str, id_str);
			}
		}
		// issue.thing-names: optional <name val="..."/> -- the entity's localised display name.
		if (node.GoToChild("name")) {
			const char *name_val = node.GetValue("val");
			sheaf->SetName(name_val ? name_val : "");
			node.GoToParent();
		}
		return sheaf;
	}
};

/**
 * A container of MsgSheaf items keyed by element id. Reuses InfoContainer for
 * Init/Reload/operator[]/iteration. The default sheaf is the kUndefined slot
 * (XML id "kDefault" should map to IdEnum::kUndefined).
 */
template<typename IdEnum, typename MsgType>
class MsgContainer
	: public info_container::InfoContainer<IdEnum, MsgSheaf<IdEnum, MsgType>, MsgSheafBuilder<IdEnum, MsgType>> {
 public:
	using Sheaf = MsgSheaf<IdEnum, MsgType>;
	using Base = info_container::InfoContainer<IdEnum, MsgSheaf<IdEnum, MsgType>, MsgSheafBuilder<IdEnum, MsgType>>;
	using NodeRange = typename Base::NodeRange;

	/**
	 * Pre-save validation gate (the Vedun editor's SaveSession runs the loader's Validate as a
	 * dry-parse before writing). Runs the base id/structure check, then additionally rejects any
	 * <message type="..."> whose key is not a known MsgType. The lenient builder (MsgSheafBuilder)
	 * would otherwise silently drop an unknown-keyed message, which let the editor persist an
	 * arbitrary string for the message key; here a bad key fails the dry-parse so the save is
	 * refused with a clear error -- independent of the optional editor .scheme sidecar.
	 */
	[[nodiscard]] bool Validate(const NodeRange &data) const {
		if (!Base::Validate(data)) {
			return false;
		}
		for (auto sheaf : data) {
			for (auto message : sheaf.Children("message")) {
				const char *type_str = message.GetValue("type");
				try {
					(void) parse::ReadAsConstant<MsgType>(type_str);
				} catch (const std::exception &) {
					err_log("MsgContainer::Validate: message has unknown 'type' ('%s').",
							type_str ? type_str : "(missing)");
					return false;
				}
			}
		}
		return true;
	}

	/**
	 * Returns a message string for the element id and message type.
	 * Fallback: unknown id or empty message -> default sheaf -> literal error string.
	 */
	[[nodiscard]] const std::string &GetMessage(IdEnum id, MsgType type) const {
		static const std::string fallback{kFallbackErrorMsg};
		if (this->IsKnown(id)) {
			const auto &message = (*this)[id].GetMessage(type);
			if (!message.empty()) {
				return message;
			}
		} else {
			err_log("MsgContainer: unknown element id '%s', falling back to default messages.",
					NAME_BY_ITEM<IdEnum>(id).c_str());
		}

		const auto &default_message = (*this)[IdEnum::kUndefined].GetMessage(type);
		if (!default_message.empty()) {
			return default_message;
		}
		return fallback;
	}

	/**
	 * issue.thing-names: the localised display name for an element id (the <name val=...> of its
	 * sheaf), or an empty string if unknown / unnamed. No default-sheaf fallback -- a name is per
	 * entity, not shared.
	 */
	[[nodiscard]] const std::string &GetName(IdEnum id) const {
		static const std::string kEmpty;
		if (this->IsKnown(id)) {
			return (*this)[id].GetName();
		}
		return kEmpty;
	}

	/**
	 * issue.thing-names: reverse lookup -- find the element id whose name matches `name`
	 * (case/letter-equivalent, via utils::IsEquivalent), or IdEnum::kUndefined if none. This is the
	 * "search the string set, return the id" entry point; callers resolve the id back to gameplay data.
	 */
	[[nodiscard]] IdEnum FindByName(const std::string &name) const {
		for (const auto &sheaf : *this) {
			if (!sheaf.GetName().empty() && utils::IsEquivalent(name, sheaf.GetName())) {
				return sheaf.GetId();
			}
		}
		return IdEnum::kUndefined;
	}

 private:
	static constexpr const char *kFallbackErrorMsg = "ERROR: message not found";
};

} // namespace msg_container

#endif // BYLINS_SRC_STRUCTS_MSG_CONTAINER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
