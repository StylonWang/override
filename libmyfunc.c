/*
 *
 * 1. Compile this file into a library: gcc -shared -fPIC -o libmyfunc.so.1.0 libmyfunc.c
 * 2. If possible, compile your program with -fno-builtin: gcc -fno-builtin -o a.out test.c
 * 3. Run your program and see the result of substitued printf: LD_PRELOAD=./libmyfunc.so.1.0 ./a.out
 * 4. You will see every printf comes with double quotes.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int printf(const char *format, ...)
{
	va_list ap;
	int ret;
	char buf[128];

	va_start(ap, format);
	ret = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	fprintf(stdout, "\"%s\"", buf);
	return ret;
}

// printf with only one string often gets converted to puts by gcc
int puts(const char *s)
{
	return fprintf(stdout, "\"%s\n\"", s);
}
