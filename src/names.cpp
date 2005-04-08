/* ************************************************************************
*   File: names.cpp                                     Part of Bylins    *
*  Usage: saving\checked names , agreement and disagreement               *
*                                                                         *
*  17.06.2003 - made by Alez                                              *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"

// Check if name agree (name must be parsed)
int was_agree_name(DESCRIPTOR_DATA * d)
{
	FILE *fp;
	char temp[MAX_INPUT_LENGTH];
	char immname[MAX_INPUT_LENGTH];
	char mortname[6][MAX_INPUT_LENGTH];
	int immlev;
	int i;

//1. Load list 

	if (!(fp = fopen(ANAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE "' for reading");
		return (1);
	}
//2. Find name in list ...
	while (get_line(fp, temp)) {
		// Format First name/pad1/pad2/pad3/pad4/pad5/ immname immlev
		sscanf(temp, "%s %s %s %s %s %s %s %d", mortname[0],
		       mortname[1], mortname[2], mortname[3], mortname[4], mortname[5], immname, &immlev);
		if (!strcmp(mortname[0], GET_NAME(d->character))) {
			// We find char ...
			for (i = 1; i < 6; i++) {
				GET_PAD(d->character, i) = (char *) malloc(strlen(mortname[i]));
				strcpy(GET_PAD(d->character, i), mortname[i]);
			}
			// Auto-Agree char ...
			NAME_GOD(d->character) = immlev + 1000;
			NAME_ID_GOD(d->character) = get_id_by_name(immname);
			sprintf(buf, "\r\nВаше имя одобрено Богом %s!!!\r\n", immname);
			SEND_TO_Q(buf, d);
			sprintf(buf, "AUTOAGREE: %s was agreed by %s", GET_PC_NAME(d->character), immname);
			log(buf, d);

			fclose(fp);
			return (0);
		}
	}
	fclose(fp);
	return (1);
}

int was_disagree_name(DESCRIPTOR_DATA * d)
{
	FILE *fp;
	char temp[MAX_INPUT_LENGTH];
	char mortname[MAX_INPUT_LENGTH];
	char immname[MAX_INPUT_LENGTH];
	int immlev;

	if (!(fp = fopen(DNAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE "' for reading");
		return (1);
	}
	//1. Load list 
	//2. Find name in list ...
	//3. Compare names ... get next
	while (get_line(fp, temp)) {
		// Extract chars and 
		sscanf(temp, "%s %s %d", mortname, immname, &immlev);
		if (!strcmp(mortname, GET_NAME(d->character))) {
			// Char found all ok;

			sprintf(buf, "\r\nВаше имя запрещено Богом %s!!!\r\n", immname);
			SEND_TO_Q(buf, d);
			sprintf(buf, "AUTOAGREE: %s was disagreed by %s", GET_PC_NAME(d->character), immname);
			log(buf, d);

			fclose(fp);
			return (0);
		};
	}
	fclose(fp);
	return (1);
}

void rm_new_name(CHAR_DATA * d)
{
	// Delete name from new names.
	FILE *fin;
	FILE *fout;
	char temp[MAX_INPUT_LENGTH];
	char mortname[MAX_INPUT_LENGTH];

	// 1. Find name ... 
	if (!(fin = fopen(NNAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" NNAME_FILE "' for read");
		return;
	}
	if (!(fout = fopen("" NNAME_FILE ".tmp", "w"))) {
		perror("SYSERR: Unable to open '" NNAME_FILE ".tmp' for writing");
		fclose(fin);
		return;
	}
	while (get_line(fin, temp)) {
		// Get name ... 
		sscanf(temp, "%s", mortname);
		if (strcmp(mortname, GET_NAME(d))) {
			// Name un matches ... do copy ...
			fprintf(fout, "%s\n", temp);
		};
	}
	fclose(fin);
	fclose(fout);
	rename(NNAME_FILE ".tmp", NNAME_FILE);
	return;

}

void add_new_name(CHAR_DATA * d)
{
	// Add name from new names.
	FILE *fl;
	if (!(fl = fopen(NNAME_FILE, "a"))) {
		perror("SYSERR: Unable to open '" NNAME_FILE "' for writing");
		return;
	}
	// Pos to the end ...
	fprintf(fl, "%s\n", GET_NAME(d));
	fclose(fl);
	return;
}

void rm_agree_name(CHAR_DATA * d)
{
	FILE *fin;
	FILE *fout;
	char temp[MAX_INPUT_LENGTH];
	char immname[MAX_INPUT_LENGTH];
	char mortname[6][MAX_INPUT_LENGTH];
	int immlev;

	// 1. Find name ... 
	if (!(fin = fopen(ANAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE "' for read");
		return;
	}
	if (!(fout = fopen("" ANAME_FILE ".tmp", "w"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE ".tmp' for writing");
		fclose(fin);
		return;
	}
	while (get_line(fin, temp)) {
		// Get name ... 
		sscanf(temp, "%s %s %s %s %s %s %s %d", mortname[0],
		       mortname[1], mortname[2], mortname[3], mortname[4], mortname[5], immname, &immlev);
		if (strcmp(mortname[0], GET_NAME(d))) {
			// Name un matches ... do copy ...
			fprintf(fout, "%s\n", temp);
		};
	}

	fclose(fin);
	fclose(fout);
	// Rewrite from tmp
	rename(ANAME_FILE ".tmp", ANAME_FILE);
	return;
}

int process_auto_agreement(DESCRIPTOR_DATA * d)
{
	// Check for name ...
	if (!was_agree_name(d))
		return 0;
	else if (!was_disagree_name(d))
		return 1;

	// Add name for-a new names.
	rm_new_name(d->character);
	add_new_name(d->character);

	return 2;
}

void rm_disagree_name(CHAR_DATA * d)
{
	FILE *fin;
	FILE *fout;
	char temp[256];
	char immname[256];
	char mortname[256];
	int immlev;

	// 1. Find name ... 
	if (!(fin = fopen(DNAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE "' for read");
		return;
	}
	if (!(fout = fopen("" DNAME_FILE ".tmp", "w"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE ".tmp' for writing");
		fclose(fin);
		return;
	}
	while (get_line(fin, temp)) {
		// Get name ... 
		sscanf(temp, "%s %s %d", mortname, immname, &immlev);
		if (strcmp(mortname, GET_NAME(d))) {
			// Name un matches ... do copy ...
			fprintf(fout, "%s\n", temp);
		};
	}
	fclose(fin);
	fclose(fout);
	rename(DNAME_FILE ".tmp", DNAME_FILE);
	return;

}

void add_agree_name(CHAR_DATA * d, char *immname, int immlev)
{
	FILE *fl;
	if (!(fl = fopen(ANAME_FILE, "a"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE "' for writing");
		return;
	}
	// Pos to the end ...
	fprintf(fl, "%s %s %s %s %s %s %s %d\r\n", GET_NAME(d),
		GET_PAD(d, 1), GET_PAD(d, 2), GET_PAD(d, 3), GET_PAD(d, 4), GET_PAD(d, 5), immname, immlev);
	fclose(fl);
	return;
}

void add_disagree_name(CHAR_DATA * d, char *immname, int immlev)
{
	FILE *fl;
	if (!(fl = fopen(DNAME_FILE, "a"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE "' for writing");
		return;
	}
	// Pos to the end ...
	fprintf(fl, "%s %s %d\r\n", GET_NAME(d), immname, immlev);
	fclose(fl);
	return;
}

void disagree_name(CHAR_DATA * d, char *immname, int immlev)
{
	// Clean record from agreed if present ...
	rm_agree_name(d);
	rm_disagree_name(d);
	rm_new_name(d);
	// Add record to disagreed if not present ...
	add_disagree_name(d, immname, immlev);
}

void agree_name(CHAR_DATA * d, char *immname, int immlev)
{
	// Clean record from disgreed if present ...
	rm_agree_name(d);
	rm_disagree_name(d);
	rm_new_name(d);
	// Add record to agreed if not present ...
	add_agree_name(d, immname, immlev);
}
