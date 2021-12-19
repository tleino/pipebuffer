/*
 * pipebuffer - Pipeline buffering for Unix-like shells
 * Copyright (c) 2021, Tommi Leino <namhas@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/*
 * A set of reusable macros that implement a ring-buffer that does not
 * need dynamic memory allocation.
 */
#define CBUF_MAX(x) (sizeof((x)) / sizeof((x)[0]))
#define CBUF_IS_FULL(x) (x).count == CBUF_MAX((x).entry)
#define CBUF_HEAD(x) ((x).start) % CBUF_MAX((x).entry)
#define CBUF_ITER(x, y) ((x).start + (y)) % CBUF_MAX((x).entry)
#define CBUF_REV_ITER(x, y) \
        (((x).start + (x).count - (y) - 1) % CBUF_MAX((x).entry))
#define CBUF_TAIL(x) ((x).start + (x).count) % CBUF_MAX((x).entry)

/*
 * Configurable variables.
 */
enum { BLOCK=14000, BUFFER=1000 };
enum { WDELAY=5000 };

/*
 * These files will be touched when the buffer is empty, or full.
 * Modify as necessary or comment out.
 */
#define EMPTY_SIGNAL_FILE	"/var/ramdisk/empty"
#define FULL_SIGNAL_FILE	"/var/ramdisk/full"

static size_t blockno;
struct entry {
        char buf[BLOCK];
        size_t len;
        size_t blockno;
};

struct {
        struct entry entry[BUFFER];
        size_t start;
        size_t count;
} cbuf;

int main(int argc, char *argv[])
{
        char *p;
        int full, was_full;

        full = was_full = 0;
        for (;;) {
                fd_set  rfds, wfds;     /* read/write fd sets */
                int     nr, maxfd;
                ssize_t n;              /* n bytes */
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = WDELAY;

                FD_ZERO(&rfds);
                FD_ZERO(&wfds);
                FD_SET(STDIN_FILENO, &rfds);
		maxfd = STDIN_FILENO;
		if (was_full && cbuf.count >= 1) {
                        FD_SET(STDOUT_FILENO, &wfds);
                        maxfd = STDOUT_FILENO;
                }
		nr = select(maxfd+1, &rfds, NULL, NULL, &tv);
                if (nr == -1) {
                        fprintf(stderr, "error in select\n");
                        return 1;
                }
                if (CBUF_IS_FULL(cbuf))
                        full = was_full = 1;
                else
                        full = 0;
                if (FD_ISSET(STDIN_FILENO, &rfds) && !full) {
			was_full = 0;
                        p = cbuf.entry[CBUF_TAIL(cbuf)].buf;
                        cbuf.entry[CBUF_TAIL(cbuf)].blockno = \
                            blockno++;
                        n = read(STDIN_FILENO, p, BLOCK);
                        if (n <= 0) {
                                fprintf(stderr, "EOF\n");
                                return 1;
                        }
                        cbuf.entry[CBUF_TAIL(cbuf)].len = n;
#ifdef DEBUG
                        fprintf(stderr, "READ\t%zu\t%zu\n",
                                n, cbuf.entry[CBUF_TAIL(cbuf)].blockno);
#endif
                        cbuf.count++;
                }
		if (cbuf.count >= 1 && was_full == 1 &&
		    (nr == 0 || FD_ISSET(STDIN_FILENO, &rfds))) {
			if (full) {
				fprintf(stderr, "buffer full!\n");
#ifdef FULL_SIGNAL_FILE
				system("touch " FULL_SIGNAL_FILE);
#endif
			}
                        p = cbuf.entry[CBUF_HEAD(cbuf)].buf;
                        n = cbuf.entry[CBUF_HEAD(cbuf)].len;
                        n = write(STDOUT_FILENO, p, n);
#ifdef DEBUG
                        fprintf(stderr, "WRITE\t%zu\t%zu\n",
                            n, cbuf.entry[CBUF_HEAD(cbuf)].blockno);
#endif
                        cbuf.count--;
                        if (cbuf.count == 0) {
#ifdef EMPTY_SIGNAL_FILE
				system("touch " EMPTY_SIGNAL_FILE);
#endif
                                was_full = 1;
			}
                        cbuf.start++;
                }
        }
        return 0;
}
