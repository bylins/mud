/**
 * \file world_data_source_manager.h
 * \brief Singleton manager for world data source.
 * \author Claude Code
 * \date 2026-01-28
 *
 * Provides global access to the active IWorldDataSource implementation.
 * Used by OLC system to save world data in the same format it was loaded from.
 */

#ifndef BYLINS_SRC_ENGINE_DB_WORLD_DATA_SOURCE_MANAGER_H_
#define BYLINS_SRC_ENGINE_DB_WORLD_DATA_SOURCE_MANAGER_H_

#include "world_data_source.h"

namespace world_loader
{

/**
 * \class WorldDataSourceManager
 * \brief Singleton manager for world data source access.
 *
 * Stores the active IWorldDataSource instance and provides global access.
 * The data source is set during world boot and used by OLC for saving.
 */
class WorldDataSourceManager {
public:
	/**
	 * \brief Get singleton instance.
	 * \return Reference to the singleton instance.
	 */
	static WorldDataSourceManager& Instance();

	/**
	 * \brief Set the active data source.
	 * \param data_source Unique pointer to the data source implementation.
	 *
	 * Transfers ownership to the manager. Should be called once during boot.
	 */
	void SetDataSource(std::unique_ptr<IWorldDataSource> data_source);

	/**
	 * \brief Get the active data source.
	 * \return Raw pointer to the data source, or nullptr if not set.
	 *
	 * Returns a non-owning pointer for use by OLC and other systems.
	 */
	IWorldDataSource* GetDataSource() const;

	/**
	 * \brief Check if data source is available.
	 * \return true if data source is set, false otherwise.
	 */
	bool HasDataSource() const;

	// Delete copy/move constructors and assignment operators
	WorldDataSourceManager(const WorldDataSourceManager&) = delete;
	WorldDataSourceManager& operator=(const WorldDataSourceManager&) = delete;
	WorldDataSourceManager(WorldDataSourceManager&&) = delete;
	WorldDataSourceManager& operator=(WorldDataSourceManager&&) = delete;

private:
	WorldDataSourceManager() = default;
	~WorldDataSourceManager() = default;

	std::unique_ptr<IWorldDataSource> data_source_;
};

} // namespace world_loader

#endif // BYLINS_SRC_ENGINE_DB_WORLD_DATA_SOURCE_MANAGER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
