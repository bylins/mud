/**
\authors Created by Sventovit
\date 28.02.2023.
\brief Шаблон контейнера сообщений
\details Класс, хранящий набор сообшений, идентифицируемых перечислением. Каждый элемент перечисления может
 соответствовать более чем одному сообщению. В таком случае возвращается случайное из них.
*/

#include <unordered_map>
#include <string>

#include "utils/random.h"
#include "color.h"
#include "utils/parse.h"

#ifndef BYLINS_SRC_STRUCTS_MESSAGES_DATA_H_
#define BYLINS_SRC_STRUCTS_MESSAGES_DATA_H_

template<typename IdEnum>
class MessagesData {
	using MessagesTable = std::unordered_multimap<IdEnum, std::string>;
	std::string empty_string_;
	MessagesTable messages_;

 public:
	MessagesData() = default;

	void ParseMessages(parser_wrapper::DataNode &node);
	void AddMsg(IdEnum id, const std::string_view &msg);
	void AddMsg(IdEnum id, const char *msg);
	const std::string &GetMsg(IdEnum id) const;
	const MessagesTable &Content() const;
	void Print(std::ostringstream &buffer) const;
};

template<typename IdEnum>
void MessagesData<IdEnum>::ParseMessages(parser_wrapper::DataNode &node) {
	for (auto &msg: node.Children()) {
		try {
			auto id =  parse::ReadAsConstant<IdEnum>(msg.GetValue("id"));
			auto msg_text = parse::ReadAsStr(msg.GetValue("val"));
			AddMsg(id, msg_text);
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
		}
	}
}

template<typename IdEnum>
void MessagesData<IdEnum>::AddMsg(IdEnum id, const std::string_view &msg) {
	messages_.emplace(std::make_pair(id, msg));
}

template<typename IdEnum>
void MessagesData<IdEnum>::AddMsg(IdEnum id, const char *msg) {
	messages_.emplace(std::make_pair(id, std::string(msg)));
}

template<typename IdEnum>
const std::string &MessagesData<IdEnum>::GetMsg(IdEnum id) const {
	auto messages_count = messages_.count(id);
	if (messages_count != 0) {
		auto msg_num = number(0, messages_count);
		auto range = messages_.equal_range(id);
		std::advance(range.first, msg_num);
		return range.first->second;
	} else {
		return empty_string_;
	}
}

template<typename IdEnum>
const typename MessagesData<IdEnum>::MessagesTable &MessagesData<IdEnum>::Content() const {
	return messages_;
}

template<typename IdEnum>
void MessagesData<IdEnum>::Print(std::ostringstream &buffer) const {
	buffer << " Messages:\r\n";
	for (const auto &msg: messages_) {
		buffer << "  " << KGRN << NAME_BY_ITEM<IdEnum>(msg.first) << ": " << KNRM << msg.second << "\r\n";
	}
}

#endif //BYLINS_SRC_STRUCTS_MESSAGES_DATA_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :