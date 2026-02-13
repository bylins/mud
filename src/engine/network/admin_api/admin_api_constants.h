/**
 \file admin_api_constants.h
 \brief Constants and enums for Admin API
 \authors Bylins team
 \date 2026-02-13

 This file contains all constants, limits, and enum classes used by the Admin API.
 Part of the Admin API refactoring to eliminate magic numbers and improve maintainability.
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_ADMIN_API_CONSTANTS_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_ADMIN_API_CONSTANTS_H_

#include <cstddef>
#include <string_view>

namespace admin_api {

// ============================================================================
// Buffer Size Limits
// ============================================================================

/// Maximum size for large buffer accumulation (1 MB)
constexpr size_t kMaxLargeBufferSize = 1048576;

/// Maximum chunks allowed for chunked messages
constexpr int kMaxChunks = 4;

// Note: Command enum and string conversion functions were removed as unused.
// CommandRegistry uses direct stringБ├▓handler mapping via std::unordered_map.

}  // namespace admin_api

#endif  // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_ADMIN_API_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
