// -*- coding: koi8-r -*-

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef _MODIFY_H_
#define _MODIFY_H_

#include "structs.h"

void string_add(DESCRIPTOR_DATA * d, char *str);
void string_write(DESCRIPTOR_DATA * d, AbstractStringWriter* writer, size_t len, int mailto, void *data);
void page_string(DESCRIPTOR_DATA * d, char *str, int keep_internal);
void page_string(DESCRIPTOR_DATA * d, const std::string& buf);
void print_con_prompt(DESCRIPTOR_DATA * d);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
