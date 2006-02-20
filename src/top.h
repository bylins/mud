/* ************************************************************************
*   File: top.h                                         Part of Bylins    *
*  Usage: header file for tops handling                                   *
*                                                                         *
*                                                                         *
*  $Author$                                                          *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#define MAX_REMORT_TOP_SIZE		5

#define TOP_ALL				NUM_CLASSES
#define TOP_CLANS			(NUM_CLASSES + 1)

struct max_remort_top_element {
	char name[MAX_NAME_LENGTH+1];
	int remort;
	long exp;
};

struct top_show_struct {
	const char *cmd;
	const byte mode;
};

void upd_p_max_remort_top(CHAR_DATA * ch);
void load_max_remort_top(void);
