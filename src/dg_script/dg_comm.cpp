/**************************************************************************
*  File: dg_comm.cpp                                     Part of Bylins   *
*  Usage:  dg_script comm functions.                                      *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#include "obj.hpp"
#include "dg_scripts.h"
#include "comm.h"
#include "handler.h"
#include "char_obj_utils.inl"

extern DESCRIPTOR_DATA *descriptor_list;

// same as any_one_arg except that it stops at punctuation 
char *any_one_name(char *argument, char *first_arg)
{
	char *arg;

	// Find first non blank 
	while (a_isspace(*argument))
		argument++;

	// Find length of first word 
	/*
	 * Тут нужно что-то делать!!!
	 * Библиотечная функция ispunct() неправильно работает для русского языка
	 * (по крайней мере у меня). Пока закоментировал.
	 */
	for (arg = first_arg; *argument && !a_isspace(*argument)	/*&&
									   (!ispunct(*argument) || *argument == '#' || *argument == '-') */ ;
			arg++, argument++)
		*arg = LOWER(*argument);
	*arg = '\0';

	return argument;
}


void sub_write_to_char(CHAR_DATA * ch, char *tokens[], void *otokens[], char type[])
{
	char sb[MAX_STRING_LENGTH];
	int i;

	strcpy(sb, "");

	for (i = 0; tokens[i + 1]; i++)
	{
		strcat(sb, tokens[i]);
		switch (type[i])
		{
		case '~':
			if (!otokens[i])
				strcat(sb, "Кто-то");
			else if ((CHAR_DATA *) otokens[i] == ch)
				strcat(sb, "Вы");
			else
				strcat(sb, PERS((CHAR_DATA *) otokens[i], ch, 0));
			break;

		case '@':
			if (!otokens[i])
				strcat(sb, "чей-то");
			else if ((CHAR_DATA *) otokens[i] == ch)
				strcat(sb, "ваш");
			else
			{
				strcat(sb, PERS((CHAR_DATA *) otokens[i], ch, 1));
			}
			break;

		case '^':
			if (!otokens[i] || !CAN_SEE(ch, (CHAR_DATA *) otokens[i]))
				strcat(sb, "чей-то");
			else if (otokens[i] == ch)
				strcat(sb, "ваш");
			else
				strcat(sb, HSHR((CHAR_DATA *) otokens[i]));
			break;

		case '}':
			if (!otokens[i] || !CAN_SEE(ch, (CHAR_DATA *) otokens[i]))
				strcat(sb, "Он");
			else if (otokens[i] == ch)
				strcat(sb, "Вы");
			else
				strcat(sb, HSSH((CHAR_DATA *) otokens[i]));
			break;

		case '*':
			if (!otokens[i] || !CAN_SEE(ch, (CHAR_DATA *) otokens[i]))
				strcat(sb, "ему");
			else if (otokens[i] == ch)
				strcat(sb, "вам");
			else
				strcat(sb, HMHR((CHAR_DATA *) otokens[i]));
			break;

		case '`':
			if (!otokens[i])
				strcat(sb, "что-то");
			else
				strcat(sb, OBJS(((OBJ_DATA *) otokens[i]), ch));
			break;
		}
	}

	strcat(sb, tokens[i]);
	strcat(sb, "\n\r");
	sb[0] = UPPER(sb[0]);	// Библиотечный вызов toupper() глючит для русского
	send_to_char(sb, ch);
}


void sub_write(char *arg, CHAR_DATA * ch, byte find_invis, int targets)
{
	char str[MAX_INPUT_LENGTH * 2];
	char type[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
	char *tokens[MAX_INPUT_LENGTH], *s, *p;
	void *otokens[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int i, tmp;
	int to_sleeping = 0;	// mainly for windows compiles 

	if (!arg)
		return;

	tokens[0] = str;

	for (i = 0, p = arg, s = str; *p;)
	{
		switch (*p)
		{
		case '~':
		case '@':
		case '^':
		case '}':
		case '*':
			// get CHAR_DATA and move to next token 
			type[i] = *p;
			*s = '\0';
			p = any_one_name(++p, name);
			otokens[i] = find_invis ? get_char(name) : get_char_room_vis(ch, name);
			tokens[++i] = ++s;
			break;

		case '`':
			// get OBJ_DATA, move to next token 
			type[i] = *p;
			*s = '\0';
			p = any_one_name(++p, name);
			otokens[i] = find_invis ? (obj = get_obj(name)) :
						 ((obj = get_obj_in_list_vis(ch, name, world[ch->in_room]->contents)) ? obj :
						  (obj = get_object_in_equip_vis(ch, name, ch->equipment, &tmp)) ? obj :
						  (obj = get_obj_in_list_vis(ch, name, ch->carrying)));
			otokens[i] = obj;
			tokens[++i] = ++s;
			break;

		case '\\':
			p++;
			*s++ = *p++;
			break;

		default:
			*s++ = *p++;
		}
	}

	*s = '\0';
	tokens[++i] = NULL;

	if (IS_SET(targets, TO_CHAR) && SENDOK(ch))
	{
		sub_write_to_char(ch, tokens, otokens, type);
	}

	if (IS_SET(targets, TO_ROOM))
	{
		for (const auto to : world[ch->in_room]->people)
		{
			if (to != ch
				&& SENDOK(to))
			{
				sub_write_to_char(to, tokens, otokens, type);
			}
		}
	}
}

void send_to_zone(char *messg, int zone_rnum)
{
	DESCRIPTOR_DATA *i;

	if (!messg || !*messg)
		return;

	for (i = descriptor_list; i; i = i->next)
		if (!i->connected && i->character && AWAKE(i->character) &&
				(IN_ROOM(i->character) != NOWHERE) && (world[IN_ROOM(i->character)]->zone == zone_rnum))
			SEND_TO_Q(messg, i);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
