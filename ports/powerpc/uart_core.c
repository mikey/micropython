/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019, Michael Neuling, IBM Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <unistd.h>
#include <stdbool.h>
#include "py/mpconfig.h"

extern int microwatt_console_poll(void);
extern char microwatt_console_read(void);
extern void microwatt_console_write(const char c);
extern bool microwatt_console_active(void);

/*
 * Core UART functions to implement for a port
 */

#if MICROPY_MIN_USE_STM32_MCU
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
} periph_uart_t;
#define USART1 ((periph_uart_t*)0x40011000)
#endif

static int potato_console;
static int qemu_console;
static uint64_t potato_uart_base;
static uint64_t qemu_uart_base;

#define POTATO_CONSOLE_TX		0x00
#define POTATO_CONSOLE_RX		0x08
#define POTATO_CONSOLE_STATUS		0x10
#define   POTATO_CONSOLE_STATUS_RX_EMPTY		0x01
#define   POTATO_CONSOLE_STATUS_TX_EMPTY		0x02
#define   POTATO_CONSOLE_STATUS_RX_FULL			0x04
#define   POTATO_CONSOLE_STATUS_TX_FULL			0x08
#define POTATO_CONSOLE_CLOCK_DIV	0x18
#define POTATO_CONSOLE_IRQ_EN		0x20

static uint64_t potato_uart_reg_read(int offset)
{
	uint64_t addr;
	uint64_t val;

	addr = potato_uart_base + offset;

	val = *(volatile uint64_t *)addr;

	return val;
}

static void potato_uart_reg_write(int offset, uint64_t val)
{
	uint64_t addr;

	addr = potato_uart_base + offset;

	*(volatile uint64_t *)addr = val;
}

static int potato_uart_rx_empty(void)
{
	uint64_t val;

	val = potato_uart_reg_read(POTATO_CONSOLE_STATUS);

	if (val & POTATO_CONSOLE_STATUS_RX_EMPTY)
		return 1;

	return 0;
}

static int potato_uart_tx_full(void)
{
	uint64_t val;

	val = potato_uart_reg_read(POTATO_CONSOLE_STATUS);

	if (val & POTATO_CONSOLE_STATUS_TX_FULL)
		return 1;

	return 0;
}

static char potato_uart_read(void)
{
	uint64_t val;

	val = potato_uart_reg_read(POTATO_CONSOLE_RX);

	return (char)(val & 0x000000ff);
}

static void potato_uart_write(char c)
{
	uint64_t val;

	val = c;

	potato_uart_reg_write(POTATO_CONSOLE_TX, val);
}

static unsigned long potato_uart_divisor(unsigned long proc_freq, unsigned long uart_freq)
{
	return proc_freq / (uart_freq * 16) - 1;
}

#define PROC_FREQ 50000000
#define UART_FREQ 115200
#define UART_BASE 0xc0002000
#define QEMU_UART_BASE 0x60300d00103f8

/* Taken from skiboot */
#define REG_RBR		0
#define REG_THR		0
#define REG_DLL		0
#define REG_IER		1
#define REG_DLM		1
#define REG_FCR		2
#define REG_IIR		2
#define REG_LCR		3
#define REG_MCR		4
#define REG_LSR		5
#define REG_MSR		6
#define REG_SCR		7

#define LSR_DR		0x01  /* Data ready */
#define LSR_OE		0x02  /* Overrun */
#define LSR_PE		0x04  /* Parity error */
#define LSR_FE		0x08  /* Framing error */
#define LSR_BI		0x10  /* Break */
#define LSR_THRE	0x20  /* Xmit holding register empty */
#define LSR_TEMT	0x40  /* Xmitter empty */
#define LSR_ERR		0x80  /* Error */

#define LCR_DLAB 	0x80  /* DLL access */

#define IER_RX		0x01
#define IER_THRE	0x02
#define IER_ALL		0x0f

static void qemu_uart_reg_write(uint64_t offset, uint8_t val)
{
	uint64_t addr;

	addr = qemu_uart_base + offset;

	*(volatile uint8_t *)addr = val;
}

static uint8_t qemu_uart_reg_read(uint64_t offset)
{
	uint64_t addr;
	uint8_t val;

	addr = qemu_uart_base + offset;

	val = *(volatile uint8_t *)addr;

	return val;
}

static int qemu_uart_tx_full(void)
{
	return !(qemu_uart_reg_read(REG_LSR) & LSR_THRE);
}

static int qemu_uart_rx_empty(void)
{
	return !(qemu_uart_reg_read(REG_LSR) & LSR_DR);
}

static char qemu_uart_read(void)
{
	return qemu_uart_reg_read(REG_THR);
}

static void qemu_uart_write(char c)
{
	qemu_uart_reg_write(REG_RBR, c);
}

void uart_init_ppc(int qemu)
{
	qemu_console = qemu;

	if (!qemu_console  && !microwatt_console_active()) {
		potato_console = 1;

		potato_uart_base = UART_BASE;
		potato_uart_reg_write(POTATO_CONSOLE_CLOCK_DIV, potato_uart_divisor(PROC_FREQ, UART_FREQ));
	} else {
		qemu_uart_base = QEMU_UART_BASE;
	}
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
#if MICROPY_MIN_USE_STDOUT
    int r = read(0, &c, 1);
    (void)r;
#elif MICROPY_MIN_USE_POWERPC
    if (qemu_console) {
	    while (qemu_uart_rx_empty()) ;
	    c = qemu_uart_read();
    } else if (potato_console) {
	    while (potato_uart_rx_empty()) ;
	    c = potato_uart_read();
    } else {
	    while (!microwatt_console_poll()) ;
	    c = microwatt_console_read();
    }
#endif
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
#if MICROPY_MIN_USE_STDOUT
    int r = write(1, str, len);
    (void)r;
#elif MICROPY_MIN_USE_POWERPC
    if (qemu_console) {
	int i;
	for (i = 0; i < len; i++) {
		while(qemu_uart_tx_full());
		qemu_uart_write(str[i]);
	}
    } else if (potato_console) {
	    int i;
	    for (i = 0; i < len; i++) {
		    while (potato_uart_tx_full());
		    potato_uart_write(str[i]);
	    }
    } else {
	    int i;
	    for (i = 0; i < len; i++)
		    microwatt_console_write(str[i]);
    }
#endif
}
