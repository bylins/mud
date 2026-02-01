/**
 * \file world_data_source_manager.cpp
 * \brief Implementation of WorldDataSourceManager singleton.
 * \author Claude Code
 * \date 2026-01-28
 */

#include "world_data_source_manager.h"
#include "null_world_data_source.h"

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
	if (data_source_) {
		return data_source_.get();
	}

	// Return static NullDataSource as fallback
	static NullWorldDataSource null_source;
	return &null_source;
}

bool WorldDataSourceManager::HasDataSource() const
{
	return data_source_ != nullptr;
}

} // namespace world_loader

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
