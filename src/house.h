/* ************************************************************************
*   File: house.cpp                                     Part of Bylins    *
*  Usage: Handling of player houses                                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#define MAX_HOUSES	100
#define MAX_GUESTS	500
#define HOUSE_NAME_LEN  50
#define HOUSE_SNAME_LEN  4

#define HOUSE_PRIVATE	0

#define HCE_ATRIUM		0
#define	HCE_PORTAL		1

#define RANK_KNIEZE      9
#define RANK_CENTURION   7
#define RANK_VETERAN     4
#define RANK_JUNIOR      2
#define RANK_NOVICE      1
#define RANK_GUEST       0

#define POLITICS_NEUTRAL 0
#define POLITICS_WAR     1
#define POLITICS_ALLIANCE 2

struct house_control_rec {
	room_vnum vnum;		/* vnum of this house */
	room_vnum atrium;	/* vnum of atrium */
	room_vnum prison;	/* vnum клан-тюрьмы */
	char name[HOUSE_NAME_LEN];
	char sname[HOUSE_SNAME_LEN + 1];
	sh_int exit_num;	/* direction of house's exit */
	time_t built_on;	/* date this house was built */
	int mode;		/* mode of ownership */
	long owner;		/* idnum of house's owner */
	int num_of_guests;	/* how many guests in this house */
	long guests[MAX_GUESTS];	/* idnums of house's guests */
	time_t last_payment;	/* date of last house payment */
	room_vnum politics_vnum[MAX_HOUSES];	// array of corresponding house vnums
	int politics_state[MAX_HOUSES];	// array of corresponding politics state
	long unique;
	long keeper;
	long spare0;
	long spare1;
	long spare2;
	long spare3;
	long spare4;
	long spare5;
	long spare6;
	long spare7;
	room_vnum closest_rent;	/* vnum of closest rent */
};

#define HOUSE_UNIQUE(house_num) (house_control[house_num].unique)
#define HOUSE_KEEPER(house_num) (house_control[house_num].keeper)



#define TOROOM(room, dir) (world[room]->dir_option[dir] ? \
                           world[room]->dir_option[dir]->to_room : NOWHERE)

void House_listrent(CHAR_DATA * ch, room_vnum vnum);
void House_boot(void);
void House_save_all(void);
int House_can_enter(CHAR_DATA * ch, room_rnum house, int mode);
void House_crashsave(room_vnum vnum);
void House_list_guests(CHAR_DATA * ch, int i, int quiet);
void House_list_rooms(CHAR_DATA * ch, int i, int quiet);

int House_closestrent(int room);
int House_atrium(int room);
int House_vnum(int room);

int House_check_exist(long uid);

long house_zone(room_rnum rnum);
void House_set_keeper(CHAR_DATA * ch);
void House_channel(CHAR_DATA * ch, char *msg);
int House_major(CHAR_DATA * ch);
char *House_name(CHAR_DATA * ch);
char *House_rank(CHAR_DATA * ch);
char *House_sname(CHAR_DATA * ch);
void House_list(CHAR_DATA * ch);
void House_list_all(CHAR_DATA * ch);
int House_news(DESCRIPTOR_DATA * d);
void sync_char_with_clan(CHAR_DATA * ch);
int find_house(room_vnum vnum);
int House_for_uid(long uid);
int in_enemy_clanzone(CHAR_DATA * ch);
