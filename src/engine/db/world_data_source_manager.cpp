/**
 * \file world_data_source_manager.cpp
 * \brief Implementation of WorldDataSourceManager singleton.
 * \author Claude Code
 * \date 2026-01-28
 */

#include "world_data_source_manager.h"
#include <cassert>

namespace world_loader
{

WorldDataSourceManager& WorldDataSourceManager::Instance()
{
	static WorldDataSourceManager instance;
	return instance;
}

void WorldDataSourceManager::SetDataSource(std::unique_ptr<IWorldDataSource> data_source)
{
	data_source_ = std::move(data_source);
}

IWorldDataSource* WorldDataSourceManager::GetDataSource() const
{
	// Data source MUST be initialized before use
	// If this assert fails - fix initialization, don't hide the bug!
	assert(data_source_ && "WorldDataSource not initialized! Call SetDataSource() first.");
	return data_source_.get();
}

bool WorldDataSourceManager::HasDataSource() const
{
	return data_source_ != nullptr;
}

} // namespace world_loader

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
