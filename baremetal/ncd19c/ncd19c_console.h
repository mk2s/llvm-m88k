#ifndef NCD19C_CONSOLE_H
#define NCD19C_CONSOLE_H

#define NCD_CON_SERIAL 1u
#define NCD_CON_SCREEN 2u

extern unsigned ncd_console_mask;

void ncd_uart_putc(char c);
int ncd_uart_getc(void);
void ncd_screen_putc(char c);
void ncd_console_putc(char c);
void ncd_console_puts(const char *s);

#endif
