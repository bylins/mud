/**
 \file admin_api.h
 \brief Admin API via Unix Domain Socket
 \details Provides JSON-based API for remote administration via Unix socket.
          Requires compilation with -DENABLE_ADMIN_API=ON
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_H_

#ifdef ENABLE_ADMIN_API

struct DescriptorData;

// Обработчик команд Admin API
void admin_api_parse(DescriptorData *d, char *argument);

// Обработка ввода Admin API (отдельно от игрового process_input)
int admin_api_process_input(DescriptorData *d);

// Аутентификация
bool admin_api_authenticate(DescriptorData *d, const char *username, const char *password);
bool admin_api_is_authenticated(DescriptorData *d);

// API команды (требуют аутентификации)
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

// Статистика и информация о сервере
void admin_api_get_stats(DescriptorData *d);
void admin_api_get_players(DescriptorData *d);

void admin_api_list_triggers(DescriptorData *d, const char *zone_vnum);
void admin_api_get_trigger(DescriptorData *d, int trig_vnum);
void admin_api_update_trigger(DescriptorData *d, int trig_vnum, const char *json_data);
void admin_api_create_trigger(DescriptorData *d, int zone_vnum, const char *json_data);
void admin_api_delete_trigger(DescriptorData *d, int trig_vnum);

// Утилиты
void admin_api_send_json(DescriptorData *d, const char *json);
void admin_api_send_error(DescriptorData *d, const char *error_msg);

#endif // ENABLE_ADMIN_API
#endif // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
