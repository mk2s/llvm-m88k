#include "ncd19c_console.h"

/* SCC2681 DUART channel B (the AUX connector), set up by the ROM at 9600 8N1. */
#define UART_SRB  ((volatile unsigned char *)0x01000024)
#define UART_THRB ((volatile unsigned char *)0x0100002C)
#define UART_RHRB ((volatile unsigned char *)0x0100002C)
#define SR_RXRDY 0x01
#define SR_TXRDY 0x04

/* ROM out1Char: character in r2, drawn at the monitor's text cursor. It is
   called through a wrapper in crt0.S, because it clobbers callee-saved
   registers. */
void ncd_rom_out1char(int c);

unsigned ncd_console_mask = NCD_CON_SERIAL | NCD_CON_SCREEN;

static void uart_raw(char c) {
  while (!(*UART_SRB & SR_TXRDY))
    ;
  *UART_THRB = (unsigned char)c;
}

void ncd_uart_putc(char c) {
  if (c == '\n')
    uart_raw('\r');
  uart_raw(c);
}

int ncd_uart_getc(void) {
  while (!(*UART_SRB & SR_RXRDY))
    ;
  return *UART_RHRB;
}

void ncd_screen_putc(char c) {
  if (c == '\r')
    return;
  ncd_rom_out1char((unsigned char)c);
}

void ncd_console_putc(char c) {
  if (ncd_console_mask & NCD_CON_SERIAL)
    ncd_uart_putc(c);
  if (ncd_console_mask & NCD_CON_SCREEN)
    ncd_screen_putc(c);
}

void ncd_console_puts(const char *s) {
  while (*s)
    ncd_console_putc(*s++);
}
