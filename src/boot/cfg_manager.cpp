/*
 \authors Created by Sventovit
 \date 14.02.2022.
 \brief Менеджер файлов конфигурации.
 \details Менеджер файлов конфигурации должен хранить информацию об именах и местоположении файлов конфигурации,
 порядке их загрузки и управлять процессом загрузки данных из файлов по контейнерам.
 */

#include "cfg_manager.h"

#include "game_classes/classes_info.h"
#include "game_skills/skills_info.h"

namespace cfg_manager {

CfgManager::CfgManager() {
/*	loaders_.emplace("abilities", "cfg/abilities.xml");
	loaders_.emplace("mobraces", "cfg/mob_races.xml");*/
	loaders_.emplace("classes", LoaderInfo("cfg/classes/pc_classes.xml",
										   std::make_unique<classes::ClassesLoader>(classes::ClassesLoader())));
	loaders_.emplace("skills", LoaderInfo("cfg/skills.xml",
										  std::make_unique<SkillsLoader>(SkillsLoader())));
}

void CfgManager::ReloadCfg(const std::string &id) {
	if (!loaders_.contains(id)) {
		err_log("Неверный параметр перезагрузки файла конфигурации (%s)", id.c_str());
		return;
	}
	const auto &loader_info = loaders_.at(id);
	auto data = parser_wrapper::DataNode(loader_info.file);
	loader_info.loader->Reload(data);
}

void CfgManager::LoadCfg(const std::string &id) {
	if (!loaders_.contains(id)) {
		err_log("Неверный параметр загрузки файла конфигурации (%s)", id.c_str());
		return;
	}
	const auto &loader_info = loaders_.at(id);
	auto data = parser_wrapper::DataNode(loader_info.file);
	loader_info.loader->Load(data);
}

} // namespace cfg manager

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
