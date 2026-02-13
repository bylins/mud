/**
 \file command_registry.h
 \brief Command dispatch registry for Admin API
 \authors Bylins team
 \date 2026-02-13

 Provides a hash table-based command registry to replace the 24-item else-if chain
 in admin_api_parse(). Improves dispatch performance from O(n) to O(1) and makes
 it easier to add new commands.
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_COMMAND_REGISTRY_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_COMMAND_REGISTRY_H_

#include "admin_api_constants.h"
#include "json_helpers.h"

#include <functional>
#include <unordered_map>
#include <string_view>

// Forward declarations
struct DescriptorData;

namespace admin_api {

// ============================================================================
// Command Handler Type
// ============================================================================

/**
 * \brief Command handler function signature
 * \param d Descriptor for the connection
 * \param request JSON request object
 *
 * Handler is responsible for:
 * - Extracting parameters from request (vnum, zone, etc.)
 * - Calling appropriate CRUD handler
 * - Sending response (success or error)
 */
using CommandHandler = std::function<void(DescriptorData*, const nlohmann::json&)>;

// ============================================================================
// Command Registry
// ============================================================================

/**
 * \brief Command dispatch registry
 *
 * Replaces the 24-item else-if chain with O(1) hash table lookup.
 * Commands are registered at initialization and dispatched based on
 * command string from JSON request.
 */
class CommandRegistry
{
public:
	/**
	 * \brief Get the global registry instance
	 * \return Reference to singleton registry
	 */
	static CommandRegistry& Instance();

	/**
	 * \brief Register a command handler
	 * \param command Command string (e.g., "get_mob")
	 * \param handler Handler function
	 */
	void Register(std::string_view command, CommandHandler handler);

	/**
	 * \brief Dispatch a command
	 * \param command Command string
	 * \param d Descriptor for the connection
	 * \param request JSON request object
	 * \return true if command was found and dispatched, false otherwise
	 */
	bool Dispatch(std::string_view command, DescriptorData* d, const nlohmann::json& request);

	/**
	 * \brief Check if command is registered
	 * \param command Command string
	 * \return true if command is registered
	 */
	bool IsRegistered(std::string_view command) const;

	/**
	 * \brief Get number of registered commands
	 * \return Command count
	 */
	size_t Count() const;

private:
	CommandRegistry() = default;

	// Command name Б├▓ handler function
	std::unordered_map<std::string, CommandHandler> handlers_;
};

// ============================================================================
// Registration Helpers
// ============================================================================

/**
 * \brief Initialize and register all Admin API commands
 *
 * Called once at server startup to populate the command registry.
 * Registers all 24 commands (auth, get_mob, update_mob, etc.)
 */
void InitializeCommandRegistry();

}  // namespace admin_api

#endif  // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_COMMAND_REGISTRY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
