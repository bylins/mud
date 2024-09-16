/**
\file iosystem.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Система ввода-вывода.
\detail Код, связанный с основным вводом-выводом команд пользователей и ответов сервера.
*/

#ifndef BYLINS_SRC_ENGINE_CORE_IOSYSTEM_H_
#define BYLINS_SRC_ENGINE_CORE_IOSYSTEM_H_

#include "utils/utils_string.h"
#include "engine/network/telnet.h"
#include "engine/core/sysdep.h"

struct DescriptorData;

namespace iosystem {

struct TextBlock {
  char *text = nullptr;
  int aliased = 0;
  struct TextBlock *next = nullptr;
};

struct TextBlocksQueue {
  struct TextBlock *head = nullptr;
  struct TextBlock *tail = nullptr;
};

extern int buf_switches;
extern int buf_largecount;
extern int buf_overflows;
extern unsigned long int number_of_bytes_read;
extern unsigned long int number_of_bytes_written;

void write_to_q(const char *txt, struct TextBlocksQueue *queue, int aliased);
int get_from_q(struct TextBlocksQueue *queue, char *dest, int *aliased);
void flush_queues(DescriptorData *d);
int write_to_descriptor(socket_t desc, const char *txt, size_t total);
bool write_to_descriptor_with_options(DescriptorData *t, const char *buffer, size_t byffer_size, int &written);
void write_to_output(const char *txt, DescriptorData *d);
void string_add(DescriptorData *d, char *str);
int toggle_compression(DescriptorData *d);
int process_input(DescriptorData *t);
int process_output(DescriptorData *t);

// Telnet options
#define TELOPT_COMPRESS     85
#define TELOPT_COMPRESS2    86

#if defined(HAVE_ZLIB)
const char compress_will[] = {(char) IAC, (char) WILL, (char) TELOPT_COMPRESS2,
							  (char) IAC, (char) WILL, (char) TELOPT_COMPRESS};
#endif

} // namespace iosystem

#endif //BYLINS_SRC_ENGINE_CORE_IOSYSTEM_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
