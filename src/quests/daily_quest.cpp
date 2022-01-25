#include "quests/daily_quest.h"

#include "entities/char.h"
#include "structs/global_objects.h"

namespace DailyQuest {

DailyQuest::DailyQuest(const std::string &desk, int reward)
	: desk(desk)
	, reward(reward)
{
}

class DailyQuestLoader
{
public:
	DailyQuestLoader();

	// загрузка файла с дейликами. возвращает true в случае успеха
	bool load();

	// список загруженных квестов. валиден только когда вызов load() был успешен
	const DailyQuestMap &quest_list() const;

	// диагностическое сообщение о статусе загрузки файла
	std::string log_message() const;

	// статус последнего вызова load()
	explicit operator bool();

private:
	bool do_load();

private:
	bool m_load_status;
	DailyQuestMap m_daily_quest_list;
	std::stringstream m_log_msg;
};

DailyQuestLoader::DailyQuestLoader()
	: m_load_status(false)
{
}

const DailyQuestMap &DailyQuestLoader::quest_list() const
{
	return m_daily_quest_list;
}

std::string DailyQuestLoader::log_message() const
{
	return m_log_msg.str();
}

bool DailyQuestLoader::load()
{
	do_load();
	mudlog(std::string(m_log_msg.str()).c_str(), CMP, kLevelImmortal, SYSLOG, true);
	return m_load_status;
}

DailyQuestLoader::operator bool()
{
	return m_load_status;
}

bool DailyQuestLoader::do_load()
{
	m_load_status = false;
	m_log_msg.str(std::string());
	m_daily_quest_list.clear();

	pugi::xml_document xml_doc;
	const auto load_result = xml_doc.load_file(DQ_FILE);
	if (!load_result) {
		m_log_msg << load_result.description();
		return m_load_status;
	}
	pugi::xml_node xml_node = xml_doc.child("daily_quest");
	if (!xml_node) {
		m_log_msg << "Ошибка загрузки файла с дейликами: " << DQ_FILE;
		return m_load_status;
	}

	for (auto object = xml_node.child("quest"); object; object = object.next_sibling("quest")) {
		const std::string attr_id = object.attribute("id").as_string("");
		const std::string attr_reward = object.attribute("reward").as_string("");
		const std::string attr_desk = object.attribute("desk").as_string("");

		if (attr_id.empty() || attr_reward.empty() || attr_desk.empty()) {
			m_log_msg << "Чтений файла дейликов прервано. Найден пустой элемент.";
			return m_load_status;
		}

		int id;
		int reward;
		const std::string desk = attr_desk;
		try {
			id = std::stoi(attr_id);
			reward = std::stoi(attr_reward);
		} catch (const std::invalid_argument& ia) {
			m_log_msg << "Чтений файла дейликов прервано: найдено некорректное число";
			return m_load_status;
		} catch (const std::out_of_range& oor) {
			m_log_msg << "Чтений файла дейликов прервано: слишком большое число";
			return m_load_status;
		}

		m_daily_quest_list.try_emplace(id, desk, reward);
	}

	m_log_msg << "Файл с дейликами успешно загружен. Загружено квестов: " << m_daily_quest_list.size();
	m_load_status = true;
	return m_load_status;
}

void load_from_file(CharacterData *ch)
{
	DailyQuestLoader quest_loader;
	if (quest_loader.load()) {
		GlobalObjects::daily_quests() = quest_loader.quest_list();
	}

	if (ch) {
		send_to_char(quest_loader.log_message(), ch);
		send_to_char("\r\n", ch);

		if (!quest_loader) {
			std::stringstream log_message;
			log_message << "Текущий список квестов оставлен без изменений. Количество квестов: " << GlobalObjects::daily_quests().size() << "\r\n";
			send_to_char(log_message.str(), ch);
		}
	}
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
