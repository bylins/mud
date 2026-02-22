/**
\file iosystem.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Система ввода-вывода.
\detail Сетевой ввод-вывод: получение команд от пользователей и отправка ответов сервера.
*/

#include "engine/core/iosystem.h"

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/network/descriptor_data.h"
#include "engine/network/msdp/msdp.h"
#include "engine/ui/color.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/db/global_objects.h"
#include "engine/network/msdp/msdp_constants.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/mechanics/weather.h"

#include <third_party_libs/fmt/include/fmt/format.h>


namespace iosystem {

int buf_largecount = 0;        // # of large buffers which exist
int buf_overflows = 0;        // # of overflows of output
int buf_switches = 0;        // # of switches from small to large buf
struct TextBlock *bufpool = nullptr;    // pool of large output buffers
unsigned long int number_of_bytes_read = 0;
unsigned long int number_of_bytes_written = 0;

const char str_goahead[] = {(char) IAC, (char) GA, 0};

#if defined(HAVE_ZLIB)
/*
 * MUD Client for Linux and mcclient compression support.
 * "The COMPRESS option (unofficial and completely arbitary) is
 * option 85." -- mcclient documentation as of Dec '98.
 *
 * [ Compression protocol documentation below, from Compress.cpp ]
 *
 * Server sends  IAC WILL COMPRESS
 * We reply with IAC DO COMPRESS
 *
 * Later the server sends IAC SB COMPRESS WILL SE, and immediately following
 * that, begins compressing
 *
 * Compression ends on a Z_STREAM_END, no other marker is used
 */

int mccp_start(DescriptorData *t, int ver);
int mccp_end(DescriptorData *t, int ver);

const char compress_start_v1[] = {(char) IAC, (char) SB, (char) TELOPT_COMPRESS, (char) WILL, (char) SE};
const char compress_start_v2[] = {(char) IAC, (char) SB, (char) TELOPT_COMPRESS2, (char) IAC, (char) SE};

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size);
void zlib_free(void *opaque, void *address);
#endif

ssize_t perform_socket_read(socket_t desc, char *read_point, size_t space_left);
int perform_subst(DescriptorData *t, char *orig, char *subst);
std::string MakePrompt(DescriptorData *d);
char *show_state(CharData *ch, CharData *victim);

void write_to_q(const char *txt, struct TextBlocksQueue *queue, int aliased) {
	struct TextBlock *newt;

	CREATE(newt, 1);
	newt->text = str_dup(txt);
	newt->aliased = aliased;

	// queue empty?
	if (!queue->head) {
		newt->next = nullptr;
		queue->head = queue->tail = newt;
	} else {
		queue->tail->next = newt;
		queue->tail = newt;
		newt->next = nullptr;
	}
}

int get_from_q(struct TextBlocksQueue *queue, char *dest, int *aliased) {
	struct TextBlock *tmp;

	// queue empty?
	if (!queue->head)
		return (0);

	tmp = queue->head;
	strcpy(dest, queue->head->text);
	*aliased = queue->head->aliased;
	queue->head = queue->head->next;

	free(tmp->text);
	free(tmp);

	return (1);
}

// Empty the queues before closing connection
void flush_queues(DescriptorData *d) {
	int dummy;

	if (d->large_outbuf) {
		d->large_outbuf->next = bufpool;
		bufpool = d->large_outbuf;
	}
	while (get_from_q(&d->input, buf2, &dummy));
}

// Add a new string to a player's output queue
void write_to_output(const char *txt, DescriptorData *t) {
	// if we're in the overflow state already, ignore this new output
	if (t->bufptr == ~0ull)
		return;

	if ((ubyte) *txt == 255) {
		return;
	}

	size_t size = strlen(txt);

	// if we have enough space, just write to buffer and that's it!
	if (t->bufspace >= size) {
		strcpy(t->output + t->bufptr, txt);
		t->bufspace -= size;
		t->bufptr += size;

		return;
	}
	/*
	 * If the text is too big to fit into even a large buffer, chuck the
	 * new text and switch to the overflow state.
	 */
	if (size + t->bufptr > kLargeBufSize - 1) {
		t->bufptr = ~0ull;
		buf_overflows++;
		return;
	}
	buf_switches++;

	// if the pool has a buffer in it, grab it
	if (bufpool != nullptr) {
		t->large_outbuf = bufpool;
		bufpool = bufpool->next;
	} else        // else create a new one
	{
		CREATE(t->large_outbuf, 1);
		CREATE(t->large_outbuf->text, kLargeBufSize);
		buf_largecount++;
	}

	strcpy(t->large_outbuf->text, t->output);    // copy to big buffer
	t->output = t->large_outbuf->text;    // make big buffer primary
	strcat(t->output, txt);    // now add new text

	// set the pointer for the next write
	t->bufptr = strlen(t->output);
	// calculate how much space is left in the buffer
	t->bufspace = kLargeBufSize - 1 - t->bufptr;
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(DescriptorData *t) {
	int failed_subst;
	ssize_t bytes_read;
	size_t space_left;
	char *ptr, *read_point, *write_point, *nl_pos;
	char tmp[kMaxInputLength];

	// first, find the point where we left off reading data
	size_t buf_length = strlen(t->inbuf);
	read_point = t->inbuf + buf_length;
	space_left = kMaxRawInputLength - buf_length - 1;

	// с переходом на ивенты это необходимо для предотвращения некоторых маловероятных крешей
	if (t == nullptr) {
		log("%s", fmt::format("SYSERR: NULL descriptor in {}() at {}:{}",
							  __func__, __FILE__, __LINE__).c_str());
		return -1;
	}

	do {
		if (space_left <= 0) {
			log("WARNING: process_input: about to close connection: input overflow");
			return (-1);
		}

		bytes_read = perform_socket_read(t->descriptor, read_point, space_left);

		if (bytes_read < 0)    // Error, disconnect them.
		{
			return (-1);
		} else if (bytes_read == 0)    // Just blocking, no problems.
		{
			return (0);
		}

		// at this point, we know we got some data from the read

		read_point[bytes_read] = '\0';    // terminate the string

		// Search for an "Interpret As Command" marker.
		for (ptr = read_point; *ptr; ptr++) {
			if (ptr[0] != (char) IAC) {
				continue;
			}

			if (ptr[1] == (char) IAC) {
				// последовательность IAC IAC
				// следует заменить просто на один IAC, но
				// для раскладок kCodePageWin/kCodePageWinz это произойдет ниже.
				// Почему так сделано - не знаю, но заменять не буду.
				// II: потому что второй IAC может прочитаться в другом socket_read
				++ptr;
			} else if (ptr[1] == (char) DO) {
				switch (ptr[2]) {
					case TELOPT_COMPRESS:
#if defined HAVE_ZLIB
						mccp_start(t, 1);
#endif
						break;

					case TELOPT_COMPRESS2:
#if defined HAVE_ZLIB
						mccp_start(t, 2);
#endif
						break;

					case ::msdp::constants::TELOPT_MSDP:
						if (runtime_config.msdp_disabled()) {
							continue;
						}

						t->msdp_support(true);
						break;

					default: continue;
				}

				memmove(ptr, ptr + 3, bytes_read - (ptr - read_point) - 3 + 1);
				bytes_read -= 3;
				--ptr;
			} else if (ptr[1] == (char) DONT) {
				switch (ptr[2]) {
					case TELOPT_COMPRESS:
#if defined HAVE_ZLIB
						mccp_end(t, 1);
#endif
						break;

					case TELOPT_COMPRESS2:
#if defined HAVE_ZLIB
						mccp_end(t, 2);
#endif
						break;

					case ::msdp::constants::TELOPT_MSDP:
						if (runtime_config.msdp_disabled()) {
							continue;
						}

						t->msdp_support(false);
						break;

					default: continue;
				}

				memmove(ptr, ptr + 3, bytes_read - (ptr - read_point) - 3 + 1);
				bytes_read -= 3;
				--ptr;
			} else if (ptr[1] == char(SB)) {
				size_t sb_length = 0;
				switch (ptr[2]) {
					case ::msdp::constants::TELOPT_MSDP:
						if (!runtime_config.msdp_disabled()) {
							sb_length = msdp::handle_conversation(t, ptr, bytes_read - (ptr - read_point));
						}
						break;

					default: break;
				}

				if (0 < sb_length) {
					memmove(ptr, ptr + sb_length, bytes_read - (ptr - read_point) - sb_length + 1);
					bytes_read -= static_cast<int>(sb_length);
					--ptr;
				}
			}
		}

		// search for a newline in the data we just read
		for (ptr = read_point, nl_pos = nullptr; *ptr && !nl_pos;) {
			if (ISNEWL(*ptr))
				nl_pos = ptr;
			ptr++;
		}

		read_point += bytes_read;
		space_left -= bytes_read;

		/*
		 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
		 * causing the MUD to hang when it encounters input not terminated by a
		 * newline.  This was causing hangs at the Password: prompt, for example.
		 * I attempt to compensate by always returning after the _first_ read, instead
		 * of looping forever until a read returns -1.  This simulates non-blocking
		 * I/O because the result is we never call read unless we know from select()
		 * that data is ready (process_input is only called if select indicates that
		 * this descriptor is in the read set).  JE 2/23/95.
		 */
#if !defined(POSIX_NONBLOCK_BROKEN)
	} while (nl_pos == nullptr);
#else
	}
	while (0);

	if (nl_pos == nullptr)
		return (0);
#endif                // POSIX_NONBLOCK_BROKEN

	/*
	 * okay, at this point we have at least one newline in the string; now we
	 * can copy the formatted data to a new array for further processing.
	 */

	read_point = t->inbuf;

	while (nl_pos != nullptr) {
		int tilde = 0;
		write_point = tmp;
		space_left = kMaxInputLength - 1;

		for (ptr = read_point; (space_left > 1) && (ptr < nl_pos); ptr++) {
			// Нафиг точку с запятой - задрали уроды с тригерами (Кард)
			if (*ptr == ';'
				&&  (t->state == EConState::kPlaying
					||  t->state == EConState::kExdesc
					||  t->state == EConState::kWriteboard
					||  t->state == EConState::kWriteNote
					||  t->state == EConState::kWriteMod)) {
				// Иммам или морталам с EGodFlag::DEMIGOD разрешено использовать ";".
				if (GetRealLevel(t->character) < kLvlImmortal && !GET_GOD_FLAG(t->character, EGf::kDemigod))
					*ptr = ',';
			}
			if (*ptr == '&'
				&&  (t->state == EConState::kPlaying
					||  t->state == EConState::kExdesc
					||  t->state == EConState::kWriteboard
					||  t->state == EConState::kWriteNote
					||  t->state == EConState::kWriteMod)) {
				if (GetRealLevel(t->character) < kLvlImplementator)
					*ptr = '8';
			}
			if (*ptr == '$'
				&&  (t->state == EConState::kPlaying
					||  t->state == EConState::kExdesc
					||  t->state == EConState::kWriteNote
					||  t->state == EConState::kWriteboard
					||  t->state == EConState::kWriteMod)) {
				if (GetRealLevel(t->character) < kLvlImplementator)
					*ptr = '4';
			}
			if (*ptr == '\\'
				&&  (t->state == EConState::kPlaying
					||  t->state == EConState::kExdesc
					||  t->state == EConState::kWriteboard
					||  t->state == EConState::kWriteNote
					||  t->state == EConState::kWriteMod)) {
				if (GetRealLevel(t->character) < kLvlGreatGod)
					*ptr = '/';
			}
			if (*ptr == '\b' || *ptr == 127)    // handle backspacing or delete key
			{
				if (write_point > tmp) {
					if (*(--write_point) == '$') {
						write_point--;
						space_left += 2;
					} else
						space_left++;
				}
			} else if (isascii(*ptr) && isprint(*ptr)) {
				*(write_point++) = *ptr;
				space_left--;
				if (*ptr == '$' &&  t->state != EConState::kSedit)    // copy one character
				{
					*(write_point++) = '$';    // if it's a $, double it
					space_left--;
				}
			} else if ((ubyte) *ptr > 127) {
				switch (t->keytable) {
					default: t->keytable = 0;
						// fall through
					case 0:
					case kCodePageUTF8: *(write_point++) = *ptr;
						break;
					case kCodePageAlt: *(write_point++) = AtoK(*ptr);
						break;
					case kCodePageWin:
					case kCodePageWinz:
					case kCodePageWinzZ: *(write_point++) = WtoK(*ptr);
						if (*ptr == (char) 255 && *(ptr + 1) == (char) 255 && ptr + 1 < nl_pos)
							ptr++;
						break;
					case kCodePageWinzOld: *(write_point++) = WtoK(*ptr);
						break;
				}
				space_left--;
			}

			// Для того чтобы работали все триги в старом zMUD, заменяем все вводимые 'z' на 'я'
			// Увы, это кое-что ломает, напр. wizhelp, или "г я использую zMUD"
			if  (t->state == EConState::kPlaying ||  (t->state == EConState::kExdesc)) {
				if (t->keytable == kCodePageWinzZ || t->keytable == kCodePageWinzOld) {
					if (*(write_point - 1) == 'z') {
						*(write_point - 1) = 'я';
					}
				}
			}

		}

		*write_point = '\0';

		if (t->keytable == kCodePageUTF8) {
			int i;
			char utf8_tmp[kMaxSockBuf * 2 * 3];
			size_t len_i, len_o;

			len_i = strlen(tmp);

			for (i = 0; i < kMaxSockBuf * 2 * 3; i++) {
				utf8_tmp[i] = 0;
			}
			utf8_to_koi(tmp, utf8_tmp);
			len_o = strlen(utf8_tmp);
			strncpy(tmp, utf8_tmp, kMaxInputLength - 1);
			space_left = space_left + len_i - len_o;
		}

		if ((space_left <= 0) && (ptr < nl_pos)) {
			char buffer[kMaxInputLength + 64];

			sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
		iosystem::write_to_output(buffer, t);
		}
		if (t->snoop_by) {
		iosystem::write_to_output("<< ", t->snoop_by);
//		iosystem::write_to_output("% ", t->snoop_by); Попытаюсь сделать вменяемый вывод снупаемого трафика в отдельное окно
		iosystem::write_to_output(tmp, t->snoop_by);
		iosystem::write_to_output("\r\n", t->snoop_by);
		}
		failed_subst = 0;

		if ((tmp[0] == '~') && (tmp[1] == 0)) {
			// очистка входной очереди
			int dummy;
			tilde = 1;
			while (get_from_q(&t->input, buf2, &dummy));
		iosystem::write_to_output("Очередь очищена.\r\n", t);
			tmp[0] = 0;
		} else if (*tmp == '!' && !(*(tmp + 1)))
			// Redo last command.
			strcpy(tmp, t->last_input);
		else if (*tmp == '!' && *(tmp + 1)) {
			char *commandln = (tmp + 1);
			int starting_pos = t->history_pos,
				cnt = (t->history_pos == 0 ? kHistorySize - 1 : t->history_pos - 1);

			skip_spaces(&commandln);
			for (; cnt != starting_pos; cnt--) {
				if (t->history[cnt] && utils::IsAbbr(commandln, t->history[cnt])) {
					strcpy(tmp, t->history[cnt]);
					strcpy(t->last_input, tmp);
				iosystem::write_to_output(tmp, t);
				iosystem::write_to_output("\r\n", t);
					break;
				}
				if (cnt == 0)    // At top, loop to bottom.
					cnt = kHistorySize;
			}
		} else if (*tmp == '^') {
			if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
				strcpy(t->last_input, tmp);
		} else {
			strcpy(t->last_input, tmp);
			if (t->history[t->history_pos])
				free(t->history[t->history_pos]);    // Clear the old line.
			t->history[t->history_pos] = str_dup(tmp);    // Save the new.
			if (++t->history_pos >= kHistorySize)    // Wrap to top.
				t->history_pos = 0;
		}

		if (!failed_subst && !tilde)
			write_to_q(tmp, &t->input, 0);

		// find the end of this line
		while (ISNEWL(*nl_pos))
			nl_pos++;

		// see if there's another newline in the input buffer
		read_point = ptr = nl_pos;
		for (nl_pos = nullptr; *ptr && !nl_pos; ptr++)
			if (ISNEWL(*ptr))
				nl_pos = ptr;
	}

	// now move the rest of the buffer up to the beginning for the next pass
	write_point = t->inbuf;
	while (*read_point)
		*(write_point++) = *(read_point++);
	*write_point = '\0';

	return (1);
}

/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
int perform_subst(DescriptorData *t, char *orig, char *subst) {
	char newsub[kMaxInputLength + 5];

	char *first, *second, *strpos;

	/*
	 * first is the position of the beginning of the first string (the one
	 * to be replaced
	 */
	first = subst + 1;

	// now find the second '^'
	if (!(second = strchr(first, '^'))) {
	iosystem::write_to_output("Invalid substitution.\r\n", t);
		return (1);
	}
	/* terminate "first" at the position of the '^' and make 'second' point
	 * to the beginning of the second string */
	*(second++) = '\0';

	// now, see if the contents of the first string appear in the original
	if (!(strpos = strstr(orig, first))) {
	iosystem::write_to_output("Invalid substitution.\r\n", t);
		return (1);
	}
	// now, we construct the new string for output.

	// first, everything in the original, up to the string to be replaced
	strncpy(newsub, orig, (strpos - orig));
	newsub[(strpos - orig)] = '\0';

	// now, the replacement string
	strncat(newsub, second, (kMaxInputLength - strlen(newsub) - 1));

	/* now, if there's anything left in the original after the string to
	 * replaced, copy that too. */
	if (((strpos - orig) + strlen(first)) < strlen(orig))
		strncat(newsub, strpos + strlen(first), (kMaxInputLength - strlen(newsub) - 1));

	// terminate the string in case of an overflow from strncat
	newsub[kMaxInputLength - 1] = '\0';
	strcpy(subst, newsub);

	return (0);
}

/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t perform_socket_read(socket_t desc, char *read_point, size_t space_left) {
	ssize_t ret;

#if defined(CIRCLE_ACORN)
	ret = recv(desc, read_point, space_left, MSG_DONTWAIT);
#elif defined(CIRCLE_WINDOWS)
	ret = recv(desc, read_point, static_cast<int>(space_left), 0);
#else
	ret = read(desc, read_point, space_left);
#endif

	// Read was successful.
	if (ret > 0) {
		number_of_bytes_read += ret;
		return (ret);
	}

	// read() returned 0, meaning we got an EOF.
	if (ret == 0) {
		log("WARNING: EOF on socket read (connection broken by peer)");
		return (-1);
	}

	// * read returned a value < 0: there was an error

#if defined(CIRCLE_WINDOWS)    // Windows
	if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
		return (0);
#else

#ifdef EINTR            // Interrupted system call - various platforms
	if (errno == EINTR)
		return (0);
#endif

#ifdef EAGAIN            // POSIX
	if (errno == EAGAIN)
		return (0);
#endif

#ifdef EWOULDBLOCK        // BSD
	if (errno == EWOULDBLOCK)
		return (0);
#endif                // EWOULDBLOCK

#ifdef EDEADLK            // Macintosh
	if (errno == EDEADLK)
		return (0);
#endif

#endif                // CIRCLE_WINDOWS

	/*
	 * We don't know what happened, cut them off. This qualifies for
	 * a SYSERR because we have no idea what happened at this point.
	 */
	perror("SYSERR: perform_socket_read: about to lose connection");
	return (-1);
}

bool write_to_descriptor_with_options(DescriptorData *t, const char *buffer, size_t buffer_size, int &written) {
#if defined(HAVE_ZLIB)
	Bytef compressed[kSmallBufsize];

	if (t->deflate)    // Complex case, compression, write it out.
	{
		written = 0;

		// First we set up our input data.
		t->deflate->avail_in = static_cast<uInt>(buffer_size);
		t->deflate->next_in = (Bytef *) (buffer);
		t->deflate->next_out = compressed;
		t->deflate->avail_out = kSmallBufsize;

		int counter = 0;
		do {
			++counter;
			int df, prevsize = kSmallBufsize - t->deflate->avail_out;

			// If there is input or the output has reset from being previously full, run compression again.
			if (t->deflate->avail_in
				|| t->deflate->avail_out == kSmallBufsize) {
				if ((df = deflate(t->deflate, Z_SYNC_FLUSH)) != Z_OK) {
					log("SYSERR: process_output: deflate() returned %d.", df);
				}
			}

			// There should always be something new to write out.
			written = write_to_descriptor(t->descriptor, (char *) compressed + prevsize,
										  kSmallBufsize - t->deflate->avail_out - prevsize);

			// Wrap the buffer when we've run out of buffer space for the output.
			if (t->deflate->avail_out == 0) {
				t->deflate->avail_out = kSmallBufsize;
				t->deflate->next_out = compressed;
			}

			// Oops. This shouldn't happen, I hope. -gg 2/19/99
			if (written <= 0) {
				return false;
			}

			// Need to loop while we still have input or when the output buffer was previously full.
		} while (t->deflate->avail_out == kSmallBufsize || t->deflate->avail_in);
	} else {
		written = write_to_descriptor(t->descriptor, buffer, buffer_size);
	}
#else
	written = write_to_descriptor(t->descriptor, buffer, buffer_size);
#endif

	return true;
}

/*
 * Send all the output that we've accumulated for a player out to
 * the player's descriptor.
 */

// \TODO Никакой чардаты тут быть не должно. Нужно сделать флаги/режимы для аккаунта или дескриптора
int process_output(DescriptorData *t) {
	char i[kMaxSockBuf * 2], o[kMaxSockBuf * 2 * 3], *pi, *po;
	int written = 0, offset, result;

	// с переходом на ивенты это необходимо для предотвращения некоторых маловероятных крешей
	if (t == nullptr) {
		log("%s", fmt::format("SYSERR: NULL descriptor in {}() at {}:{}",
							  __func__, __FILE__, __LINE__).c_str());
		return -1;
	}

#ifdef ENABLE_ADMIN_API
	// Admin API uses raw JSON output, skip formatting
	if (t->admin_api_mode) {
		if (!t->output || !*t->output)
			return 0;

		int result = write_to_descriptor(t->descriptor, t->output, strlen(t->output));
		t->bufptr = 0;
		*t->output = '\0';
		return (result >= 0 ? 1 : -1);
	}
#endif

	// Отправляю данные снуперам
	// handle snooping: prepend "% " and send to snooper
	if (t->output && t->snoop_by) {
	iosystem::write_to_output("% ", t->snoop_by);
	iosystem::write_to_output(t->output, t->snoop_by);
	iosystem::write_to_output("%%", t->snoop_by);
	}

	pi = i;
	po = o;
	// we may need this \r\n for later -- see below
	strcpy(i, "\r\n");

	// now, append the 'real' output
	strcpy(i + 2, t->output);

	// if we're in the overflow state, notify the user
	if (t->bufptr == ~0ull) {
		strcat(i, "***ПЕРЕПОЛНЕНИЕ***\r\n");
	}

	// add the extra CRLF if the person isn't in compact mode
	if  (t->state == EConState::kPlaying && t->character && !t->character->IsNpc()
		&& !t->character->IsFlagged(EPrf::kCompact)) {
		strcat(i, "\r\n");
	} else if  (t->state == EConState::kPlaying && t->character && !t->character->IsNpc()
		&& t->character->IsFlagged(EPrf::kCompact)) {
		// added by WorM (Видолюб)
		//фикс сжатого режима добавляет в конец строки \r\n если его там нету, чтобы промпт был всегда на след. строке
		for (size_t c = strlen(i) - 1; c > 0; c--) {
			if (*(i + c) == '\n' || *(i + c) == '\r')
				break;
			else if (*(i + c) != ';' && *(i + c) != '\033' && *(i + c) != 'm' && !(*(i + c) >= '0' && *(i + c) <= '9')
				&&
					*(i + c) != '[' && *(i + c) != '&' && *(i + c) != 'n' && *(i + c) != 'R' && *(i + c) != 'Y'
				&& *(i + c) != 'Q' && *(i + c) != 'q') {
				strcat(i, "\r\n");
				break;
			}
		}
	}// end by WorM

	if  (t->state == EConState::kPlaying && t->character)
		t->msdp_report_changed_vars();

	// add a prompt
	strncat(i, MakePrompt(t).c_str(), kMaxPromptLength);

	// easy color
	int pos;
	if ((t->character) && (pos = proc_color(i))) {
		sprintf(buf,
				"SYSERR: %s pos:%d player:%s in proc_color!",
				(pos < 0 ? (pos == -1 ? "NULL buffer" : "zero length buffer") : "go out of buffer"),
				pos,
				GET_NAME(t->character));
		mudlog(buf, BRF, kLvlGod, SYSLOG, true);
	}

	t->string_to_client_encoding(pi, po);

	if (t->has_prompt)
		offset = 0;
	else
		offset = 2;

	if (t->character && t->character->IsFlagged(EPrf::kGoAhead))
		strncat(o, str_goahead, kMaxPromptLength);

	if (!write_to_descriptor_with_options(t, o + offset, strlen(o + offset), result)) {
		return -1;
	}

	written = result >= 0 ? result : -result;

	/*
	 * if we were using a large buffer, put the large buffer on the buffer pool
	 * and switch back to the small one
	 */
	if (t->large_outbuf) {
		t->large_outbuf->next = bufpool;
		bufpool = t->large_outbuf;
		t->large_outbuf = nullptr;
		t->output = t->small_outbuf;
	}
	// reset total bufspace back to that of a small buffer
	t->bufspace = kSmallBufsize - 1;
	t->bufptr = 0;
	*(t->output) = '\0';

	// Error, cut off.
	if (result == 0)
		return (-1);

	// Normal case, wrote ok.
	if (result > 0)
		return (1);

	/*
	 * We blocked, restore the unwritten output. IsKnown
	 * bug in that the snooping immortal will see it twice
	 * but details...
	 */
iosystem::write_to_output(o + written + offset, t);
	return (0);
}

/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

#if defined(CIRCLE_WINDOWS)

ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
	ssize_t result;

	/* Windows signature: int send(SOCKET s, const char* buf, int len, int flags);
	** ... -=>
	**/
#if defined WIN32
	int l = static_cast<int>(length);
#else
	size_t l = length;
#endif
	/* >=- ... */
	result = static_cast<decltype(result)>(send(desc, txt, l, 0));

	if (result > 0)
	{
		// Write was successful
		number_of_bytes_written += result;
		return (result);
	}

	if (result == 0)
	{
		// This should never happen!
		log("SYSERR: Huh??  write() returned 0???  Please report this!");
		return (-1);
	}

	// result < 0: An error was encountered.

	// Transient error?
	if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
		return (0);

	// Must be a fatal error.
	return (-1);
}

#else

#if defined(CIRCLE_ACORN)
#define write	socketwrite
#endif

#if defined(__APPLE__) || defined(__MACH__) || defined(__CYGWIN__)
#include <sys/socket.h>
# ifndef MSG_NOSIGNAL
#   define MSG_NOSIGNAL SO_NOSIGPIPE
# endif
#endif

// perform_socket_write for all Non-Windows platforms
ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length) {
	ssize_t result;

	result = send(desc, txt, length, MSG_NOSIGNAL);

	if (result > 0) {
		// Write was successful.
		number_of_bytes_written += result;
		return (result);
	}

	if (result == 0) {
		// This should never happen!
		log("SYSERR: Huh??  write() returned 0???  Please report this!");
		return (-1);
	}

	/*
	 * result < 0, so an error was encountered - is it transient?
	 * Unfortunately, different systems use different constants to
	 * indicate this.
	 */

#ifdef EAGAIN            // POSIX
	if (errno == EAGAIN)
		return (0);
#endif

#ifdef EWOULDBLOCK        // BSD
	if (errno == EWOULDBLOCK)
		return (0);
#endif

#ifdef EDEADLK            // Macintosh
	if (errno == EDEADLK)
		return (0);
#endif

	// Looks like the error was fatal.  Too bad.
	return (-1);
}

#endif                // CIRCLE_WINDOWS

/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level send() until all
 * the text has been delivered to the OS, or until an error is
 * encountered. 'written' is updated to add how many bytes were sent
 * over the socket successfully prior to the return. It is not zero'd.
 *
 * Returns:
 *  +  All is well and good.
 *  0  A fatal or unexpected error was encountered.
 *  -  The socket write would block.
 */
int write_to_descriptor(socket_t desc, const char *txt, size_t total) {
	ssize_t bytes_written, total_written = 0;

	if (total == 0) {
		log("write_to_descriptor: write nothing?!");
		return 0;
	}

	while (total > 0) {
		bytes_written = perform_socket_write(desc, txt, total);

		if (bytes_written < 0) {
			// Fatal error.  Disconnect the player_data.
			perror("SYSERR: write_to_descriptor");
			return (0);
		} else if (bytes_written == 0) {
			/*
			 * Temporary failure -- socket buffer full. For now, we'll just
			 * cut off the player, but eventually we'll stuff the unsent
			 * text into a buffer and retry to write later.  JE 30 June 98.
			 * Implemented the anti-cut-off code he wanted. GG 13 Jan 99.
			 */
			log("WARNING: write_to_descriptor: socket write would block.");
			return (-total_written);
		} else {
			txt += bytes_written;
			total -= bytes_written;
			total_written += bytes_written;
		}
	}

	return (total_written);
}

#if defined(HAVE_ZLIB)

int mccp_start(DescriptorData *t, int ver) {
	int derr;

	if (t->deflate) {
		return 1;    // компрессия уже включена
	}

	// Set up zlib structures.
	CREATE(t->deflate, 1);
	t->deflate->zalloc = zlib_alloc;
	t->deflate->zfree = zlib_free;
	t->deflate->opaque = nullptr;

	// Initialize.
	if ((derr = deflateInit(t->deflate, Z_DEFAULT_COMPRESSION)) != 0) {
		log("SYSERR: deflateEnd returned %d.", derr);
		free(t->deflate);
		t->deflate = nullptr;
		return 0;
	}

	if (ver != 2) {
		iosystem::write_to_descriptor(t->descriptor, compress_start_v1, sizeof(compress_start_v1));
	} else {
		iosystem::write_to_descriptor(t->descriptor, compress_start_v2, sizeof(compress_start_v2));
	}

	t->mccp_version = ver;
	return 1;
}

int mccp_end(DescriptorData *t, int ver) {
	int derr;
	int prevsize, pending;
	unsigned char tmp[1];

	if (t->deflate == nullptr)
		return 1;

	if (t->mccp_version != ver)
		return 0;

	t->deflate->avail_in = 0;
	t->deflate->next_in = tmp;
	prevsize = kSmallBufsize - t->deflate->avail_out;

	log("SYSERR: about to deflate Z_FINISH.");

	if ((derr = deflate(t->deflate, Z_FINISH)) != Z_STREAM_END) {
		log("SYSERR: deflate returned %d upon Z_FINISH. (in: %d, out: %d)",
			derr, t->deflate->avail_in, t->deflate->avail_out);
		return 0;
	}

	pending = kSmallBufsize - t->deflate->avail_out - prevsize;

	if (!iosystem::write_to_descriptor(t->descriptor, t->small_outbuf + prevsize, pending))
		return 0;

	if ((derr = deflateEnd(t->deflate)) != Z_OK)
		log("SYSERR: deflateEnd returned %d. (in: %d, out: %d)", derr,
			t->deflate->avail_in, t->deflate->avail_out);

	free(t->deflate);
	t->deflate = nullptr;

	return 1;
}
#endif

int toggle_compression([[maybe_unused]] DescriptorData *t) {
#if defined(HAVE_ZLIB)
	if (t->mccp_version == 0)
		return 0;
	if (t->deflate == nullptr) {
		return mccp_start(t, t->mccp_version) ? 1 : 0;
	} else {
		return mccp_end(t, t->mccp_version) ? 0 : 1;
	}
#endif
	return 0;
}

#if defined(HAVE_ZLIB)

// Compression stuff.

void *zlib_alloc(void * /*opaque*/, unsigned int items, unsigned int size) {
	return calloc(items, size);
}

void zlib_free(void * /*opaque*/, void *address) {
	free(address);
}

#endif

// \TODO Вообще, этому скорей место в UI. Надо подумать, как перенести.
std::string MakePrompt(DescriptorData *d) {
	const auto& ch = d->character;
	auto out = fmt::memory_buffer();
	out.reserve(kMaxPromptLength);
	if (d->showstr_count) {
		fmt::format_to(std::back_inserter(out),
				  "\rЛистать : <RETURN>, Q<К>онец, R<П>овтор, B<Н>азад, или номер страницы ({}/{}).",
				  d->showstr_page, d->showstr_count);
	} else if (d->writer) {
		fmt::format_to(std::back_inserter(out), "] ");
	} else if (d->state == EConState::kPlaying && !ch->IsNpc()) {
		if (GET_INVIS_LEV(ch)) {
			fmt::format_to(std::back_inserter(out), "i{} ", GET_INVIS_LEV(ch));
		}

		if (ch->IsFlagged(EPrf::kDispHp)) {
			fmt::format_to(std::back_inserter(out), "{}{}H{} ",
					  GetWarmValueColor(ch->get_hit(), ch->get_real_max_hit()), ch->get_hit(), kColorNrm);
		}

		if (ch->IsFlagged(EPrf::kDispMove)) {
			fmt::format_to(std::back_inserter(out), "{}{}M{} ",
					  GetWarmValueColor(ch->get_move(), ch->get_real_max_move()), ch->get_move(), kColorNrm);
		}

		if (ch->IsFlagged(EPrf::kDispMana) && IS_MANA_CASTER(ch)) {
			int current_mana = 100 * ch->mem_queue.stored;
			fmt::format_to(std::back_inserter(out), "{}э{}{} ",
					  GetColdValueColor(current_mana, GET_MAX_MANA((ch).get())), ch->mem_queue.stored, kColorNrm);
		}

		if (ch->IsFlagged(EPrf::kDispExp)) {
			if (IS_IMMORTAL(ch)) {
				fmt::format_to(std::back_inserter(out), "??? ");
			} else {
				fmt::format_to(std::back_inserter(out), "{}o ",
						  GetExpUntilNextLvl(ch.get(), GetRealLevel(ch) + 1) - ch->get_exp());
			}
		}

		if (ch->IsFlagged(EPrf::kDispMana) && !IS_MANA_CASTER(ch)) {
			if (!ch->mem_queue.Empty()) {
				auto mana_gain = CalcManaGain(ch.get());
				if (mana_gain) {
					int sec_hp = std::max(0, 1 + ch->mem_queue.total - ch->mem_queue.stored);
					sec_hp = sec_hp*60/mana_gain;
					int ch_hp = sec_hp/60;
					sec_hp %= 60;
					fmt::format_to(std::back_inserter(out), "Зауч:{}:{:02} ", ch_hp, sec_hp);
				} else {
					fmt::format_to(std::back_inserter(out), "Зауч:- ");
				}
			} else {
				fmt::format_to(std::back_inserter(out), "Зауч:0 ");
			}
		}

		if (ch->IsFlagged(EPrf::kDispCooldowns)) {
			fmt::format_to(std::back_inserter(out), "{}:{} ",
					  MUD::Skill(ESkill::kGlobalCooldown).GetAbbr(),
					  ch->getSkillCooldownInPulses(ESkill::kGlobalCooldown));

			for (const auto &skill : MUD::Skills()) {
				if (skill.IsAvailable()) {
					int cooldown = ch->getSkillCooldownInPulses(skill.GetId());
					if (cooldown > 0) {
						fmt::format_to(std::back_inserter(out), "{}:{} ", skill.GetAbbr(), cooldown);
					}
				}
			}
		}

		if (ch->IsFlagged(EPrf::kDispTimed)) {
			for (auto timed = ch->timed; timed; timed = timed->next) {
				if (timed->skill != ESkill::kWarcry && timed->skill != ESkill::kTurnUndead) {
					fmt::format_to(std::back_inserter(out), "{}:{} ",
							  MUD::Skill(timed->skill).GetAbbr(), +timed->time);
				}
			}
			if (ch->GetSkill(ESkill::kWarcry)) {
				int wc_count = (kHoursPerDay - IsTimedBySkill(ch.get(), ESkill::kWarcry)) / kHoursPerWarcry;
				fmt::format_to(std::back_inserter(out), "{}:{} ",
						  MUD::Skill(ESkill::kWarcry).GetAbbr(), wc_count);
			}
			if (ch->GetSkill(ESkill::kTurnUndead)) {
				auto bonus =
					std::max(1, kHoursPerTurnUndead + (CanUseFeat(ch.get(), EFeat::kExorcist) ? -2 : 0));
				fmt::format_to(std::back_inserter(out), "{}:{} ",
						  MUD::Skill(ESkill::kTurnUndead).GetAbbr(),
						  (kHoursPerDay - IsTimedBySkill(ch.get(), ESkill::kTurnUndead)) / bonus);
			}
		}

		if (!ch->GetEnemy() || ch->in_room != ch->GetEnemy()->in_room) {
			if (ch->IsFlagged(EPrf::kDispLvl)) {
				fmt::format_to(std::back_inserter(out), "{}L ", GetRealLevel(ch));
			}

			if (ch->IsFlagged(EPrf::kDispMoney)) {
				fmt::format_to(std::back_inserter(out), "{}G ", ch->get_gold());
			}

			if (ch->IsFlagged(EPrf::kDispExits)) {
				static const char *dirs[] = {"С", "В", "Ю", "З", "^", "v"};
				fmt::format_to(std::back_inserter(out), "Вых:");
				if (!AFF_FLAGGED(ch, EAffect::kBlind)) {
					for (auto dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
						if (EXIT(ch, dir) && EXIT(ch, dir)->to_room() != kNowhere &&
							!EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kHidden)) {
							if (EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kClosed))
								fmt::format_to(std::back_inserter(out), "({})", dirs[dir]);
							else
								fmt::format_to(std::back_inserter(out), "{}", dirs[dir]);
						}
					}
				}
			}
		} else {
			if (ch->IsFlagged(EPrf::kDispFight)) {
				fmt::format_to(std::back_inserter(out), "{}", show_state(ch.get(), ch.get()));
			}
			if (ch->GetEnemy()->GetEnemy() && ch->GetEnemy()->GetEnemy() != ch.get()) {
				fmt::format_to(std::back_inserter(out), "{}",
						  show_state(ch.get(), ch->GetEnemy()->GetEnemy()));
			}
			fmt::format_to(std::back_inserter(out), "{}", show_state(ch.get(), ch->GetEnemy()));
		};
		fmt::format_to(std::back_inserter(out), "> ");
	} else if (d->state == EConState::kPlaying && ch->IsNpc()) {
		fmt::format_to(std::back_inserter(out), "{{}}-> ", ch->get_name());
	}

	return to_string(out);
}

char *show_state(CharData *ch, CharData *victim) {
	static const char *WORD_STATE[12] = {"Смертельно ранен",
										 "О.тяжело ранен",
										 "О.тяжело ранен",
										 "Тяжело ранен",
										 "Тяжело ранен",
										 "Ранен",
										 "Ранен",
										 "Ранен",
										 "Легко ранен",
										 "Легко ранен",
										 "Слегка ранен",
										 "Невредим"
	};

	const int ch_hp = posi_value(victim->get_hit(), victim->get_real_max_hit()) + 1;
	sprintf(buf, "%s&q[%s:%s%s]%s&Q ",
			GetWarmValueColor(victim->get_hit(), victim->get_real_max_hit()),
			PERS(victim, ch, 0), WORD_STATE[ch_hp], GET_CH_SUF_6(victim), kColorNrm);
	return buf;
}

} // namespace iosystem

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
