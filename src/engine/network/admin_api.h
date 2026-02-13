/**
 \file admin_api.h
 \brief Admin API via Unix Domain Socket
 \details Provides JSON-based API for remote administration via Unix socket.
          Requires compilation with -DENABLE_ADMIN_API=ON
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_H_

#ifdef ENABLE_ADMIN_API

class DescriptorData;

// Main entry point - parse and dispatch JSON command
void admin_api_parse(DescriptorData *d, char *argument);

// Process input from socket (called by network layer)
int admin_api_process_input(DescriptorData *d);

// Authentication check
bool admin_api_is_authenticated(DescriptorData *d);

// Utility functions for sending responses
void admin_api_send_json(DescriptorData *d, const char *json);
void admin_api_send_error(DescriptorData *d, const char *error_msg);

#endif // ENABLE_ADMIN_API
#endif // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
