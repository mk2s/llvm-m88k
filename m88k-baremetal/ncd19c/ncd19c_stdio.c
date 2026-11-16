/* picolibc (tinystdio) glue for the NCD 19c: stdin/stdout/stderr on the console. */
#include <stdio.h>
#include "ncd19c_console.h"

static int ncd_stdio_putc(char c, FILE *file)
{
	(void)file;
	ncd_console_putc(c);
	return (unsigned char)c;
}

static int ncd_stdio_getc(FILE *file)
{
	(void)file;
	return ncd_uart_getc();
}

static FILE __stdio = FDEV_SETUP_STREAM(ncd_stdio_putc, ncd_stdio_getc, NULL,
					_FDEV_SETUP_RW);

FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);
