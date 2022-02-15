/*
 \authors Created by Sventovit
 \date 14.02.2022.
 \brief Менеджер файлов конфигурации.
 \details Менеджер файлов конфигурации должен хранить информацию об именах и местоположении файлов конфигурации,
 порядке их загрузки и управлять процессом загрузки данных из файлов по контейнерам.
 */

#include "cfg_manager.h"

#include "skills_info.h"

namespace cfg_manager {

CfgManager::CfgManager() {
/*	loaders_.emplace("pcclasses", "lib/cfg/classes/pc_classes.xml");
	loaders_.emplace("abilities", "lib/cfg/abilities.xml");
	loaders_.emplace("mobraces", "lib/cfg/mob_races.xml");*/
	loaders_.emplace("skills", LoaderInfo("lib/cfg/skills.xml", std::make_unique<SkillsLoader>(SkillsLoader())));
}

void CfgManager::BootBaseCfgs() {
	BootSIngleFile("skills");
}

void CfgManager::ReloadCfgFile(const std::string &id) {
	if (!loaders_.contains(id)) {
		return;
	}
	BootSIngleFile(id);
}

void CfgManager::BootSIngleFile(const std::string &id) {
	const auto &loader_info = loaders_.at(id);
	auto data = parser_wrapper::DataNode(loader_info.file);
	loader_info.loader->Load(data);
}

} // namespace cfg manager

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
