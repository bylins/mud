// -*- coding: koi8-r -*-

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef _MODIFY_H_
#define _MODIFY_H_

#include "structs/descriptor_data.h"
#include "utils/utils_string.h"

void string_add(DescriptorData *d, char *str);
void string_write(DescriptorData *d, utils::AbstractStringWriter *writer, size_t len, int mailto, void *data);
void page_string(DescriptorData *d, char *str, int keep_internal);
void page_string(DescriptorData *d, const std::string &buf);
void print_con_prompt(DescriptorData *d);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
