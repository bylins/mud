/**
 \file admin_api.h
 \brief Admin API via Unix Domain Socket
 \details Provides JSON-based API for world management via Unix socket.
          Enabled only when compiled with -DENABLE_ADMIN_API=ON
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_H_

#ifdef ENABLE_ADMIN_API

class DescriptorData;

// Admin API command handler
void admin_api_parse(DescriptorData *d, char *argument);

// Admin API input processing (separate from game process_input)
int admin_api_process_input(DescriptorData *d);

// Authentication
bool admin_api_authenticate(DescriptorData *d, const char *username, const char *password);
bool admin_api_is_authenticated(DescriptorData *d);

// API commands (require authentication)
void admin_api_list_mobs(DescriptorData *d, const char *zone_vnum);
void admin_api_get_mob(DescriptorData *d, int mob_vnum);
void admin_api_update_mob(DescriptorData *d, int mob_vnum, const char *json_data);
void admin_api_create_mob(DescriptorData *d, int zone_vnum, const char *json_data);
void admin_api_delete_mob(DescriptorData *d, int mob_vnum);

void admin_api_list_objects(DescriptorData *d, const char *zone_vnum);
void admin_api_get_object(DescriptorData *d, int obj_vnum);
void admin_api_update_object(DescriptorData *d, int obj_vnum, const char *json_data);
void admin_api_create_object(DescriptorData *d, int zone_vnum, const char *json_data);
void admin_api_delete_object(DescriptorData *d, int obj_vnum);

void admin_api_list_rooms(DescriptorData *d, const char *zone_vnum);
void admin_api_get_room(DescriptorData *d, int room_vnum);
void admin_api_update_room(DescriptorData *d, int room_vnum, const char *json_data);
void admin_api_create_room(DescriptorData *d, int zone_vnum, const char *json_data);
void admin_api_delete_room(DescriptorData *d, int room_vnum);

void admin_api_list_zones(DescriptorData *d);
void admin_api_get_zone(DescriptorData *d, int zone_vnum);
void admin_api_update_zone(DescriptorData *d, int zone_vnum, const char *json_data);

// Statistics and server info
void admin_api_get_stats(DescriptorData *d);
void admin_api_get_players(DescriptorData *d);

void admin_api_list_triggers(DescriptorData *d, const char *zone_vnum);
void admin_api_get_trigger(DescriptorData *d, int trig_vnum);
void admin_api_update_trigger(DescriptorData *d, int trig_vnum, const char *json_data);
void admin_api_create_trigger(DescriptorData *d, int zone_vnum, const char *json_data);
void admin_api_delete_trigger(DescriptorData *d, int trig_vnum);

// Utilities
void admin_api_send_json(DescriptorData *d, const char *json);
void admin_api_send_error(DescriptorData *d, const char *error_msg);

#endif // ENABLE_ADMIN_API
#endif // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
