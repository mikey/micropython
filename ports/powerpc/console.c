/* Copyright 2019 Michael Neuling, IBM Corp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>

/* microwatt defs */
#define OPCODE(v)       (((v) & 0x3f) << 26)
#define __RS(RS)        (((RS) & 0x1f) << (31-10))
#define __RT(RT)        __RS(RT)

#define READ(RT)        .long (OPCODE(1) | __RT(RT) | (0 << 1))
#define POLL(RT)        .long (OPCODE(1) | __RT(RT) | (1 << 1))
#define WRITE(RS)       .long (OPCODE(1) | __RS(RS) | (2 << 1))
#define CONFIG(RS)      .long (OPCODE(1) | __RS(RS) | (3 << 1))

#  define __stringify_in_c(...)	#__VA_ARGS__
#  define stringify_in_c(...)	__stringify_in_c(__VA_ARGS__) " "

/*
 * The SIM_READ_CONSOLE callout will return -1 if there is no character to read.
 * There's no explicit poll callout so we "poll" by doing a read and stashing
 * the result until we do an actual read.
 */
int microwatt_console_poll(void)
{
	register unsigned long rc __asm("r3");

	__asm volatile (stringify_in_c(POLL(3)) :"=r"(rc));

	return rc;
}

char microwatt_console_read(void)
{
	register char c __asm("r3");

	__asm volatile (stringify_in_c(READ(3)) :"=r"(c));
	return c;
}

void microwatt_console_write(const char c)
{
	register char c1 __asm("r3") = c;

	__asm volatile (stringify_in_c(WRITE(3)) :"=r"(c1));
}

#define MICROWATT_CONFIG_IS_SIM	(0x1 << 0)

unsigned long microwatt_config(void)
{
	register char c __asm("r3");

	__asm volatile (stringify_in_c(CONFIG(3)) :"=r"(c));
	return c;
}

bool microwatt_console_active(void)
{
	return !!(microwatt_config() & MICROWATT_CONFIG_IS_SIM);
}

/* Some of this is stolen from skiboot */
#include "mambo.h"

/*
 * The SIM_READ_CONSOLE callout will return -1 if there is no character to read.
 * There's no explicit poll callout so we "poll" by doing a read and stashing
 * the result until we do an actual read.
 */
static int mambo_char = -1;

int mambo_console_poll(void)
{
	if (mambo_char < 0)
		mambo_char = callthru0(SIM_READ_CONSOLE_CODE);

	return mambo_char >= 0;
}

int mambo_console_read(unsigned char *buf, int len)
{
	int count = 0;

	while (count < len) {
		if (!mambo_console_poll())
			break;

		buf[count++] = mambo_char;
		mambo_char = -1;
	}

	return count;
}

int mambo_console_write(const char *buf, int len)
{
	callthru2(SIM_WRITE_CONSOLE_CODE, (unsigned long)buf, len);
	return len;
}

void __attribute__((noreturn)) mambo_terminate(void)
{
	/* callthru0(SIM_EXIT_CODE); */

	for (;;) ;
}
