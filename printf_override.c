/*
 *
 * 1. Compile this file into a library: gcc -shared -fPIC -o liboveride.so.1.0 printf_override.c
 * 2. If possible, compile your program with -fno-builtin: gcc -fno-builtin -o a.out test.c
 * 3. Run your program and see the result of replaced printf: LD_PRELOAD=./liboveride.so.1.0 ./a.out
 *
 * Set environment variable "ALLOW_PRINTF" to "no" to block printf messages.
 * Set environment variable "ALLOW_PRINTF" to "syslog" to redirect to syslog.
 * Otherwise, all printf messages are not blocked.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <syslog.h>
#include <string.h>
#include <string.h>

typedef enum {
	PT_NONE = 0,		// whether to allow printf is not yet determined
	PT_NORMAL = 1,		// allow printf to stdout
	PT_NULL = 2,		// block all printf messages
	PT_SYSLOG = 3,		// direct printf to syslog
} PrintState_t;

#define ENV_NAME "ALLOW_PRINTF"

// mutex to control printf message from interlaced with each other.
static pthread_mutex_t g_printf_lock = PTHREAD_MUTEX_INITIALIZER;
static PrintState_t g_print_state = PT_NONE;
static int g_syslog_opened = 0;

// NOTE: Please do hold g_printf_lock before calling this function!!!
//
// make sure openlog() is called only once
static void open_syslog(void)
{
	if (g_syslog_opened) return;

	openlog("", LOG_NDELAY, LOG_USER);
	g_syslog_opened = 1;
}

// NOTE: Please do hold g_printf_lock before calling this function!!!
static void get_printf_state(void)
{
	char *environ_value = getenv(ENV_NAME);

	if(NULL==environ_value) {
		// no environment variable
		g_print_state = PT_NORMAL;
	}
	else if(0==strncmp(environ_value, "no", strlen(environ_value))) {
		g_print_state = PT_NULL;
	}
	else if(0==strncmp(environ_value, "syslog", strlen(environ_value))) {
		g_print_state = PT_SYSLOG;

		open_syslog();
	}
	else {
		g_print_state = PT_NORMAL;
	}
}

// NOTE: Please do hold g_printf_lock before calling this function!!!
//
static int is_printf_allowed(void) 
{
	// not yet initialized, check environment variable and set g_print_state
	if (PT_NONE==g_print_state) {
		get_printf_state();
	}
	
	if(PT_NORMAL==g_print_state) {
		return 1; // printf allowed
	}

	return 0;
}

static int is_printf_to_syslog(void) 
{
	// not yet initialized, check environment variable and set g_print_state
	if (PT_NONE==g_print_state) {
		get_printf_state();
	}
	
	if(PT_SYSLOG==g_print_state) {
		return 1; // printf is redirected to syslog
	}

	return 0;
}

int printf(const char *format, ...)
{
	int ret = 0;

	pthread_mutex_lock(&g_printf_lock);

	if( is_printf_allowed() ) {
		char buf[128]; // sorry, max 128 bytes
		va_list ap;

		va_start(ap, format);
		ret = vsnprintf(buf, sizeof(buf), format, ap);
		va_end(ap);

		fprintf(stdout, "%s", buf);
	}
	else if( is_printf_to_syslog() ) {
		char buf[128]; // sorry, max 128 bytes
		va_list ap;

		va_start(ap, format);
		ret = vsnprintf(buf, sizeof(buf), format, ap);
		va_end(ap);

		syslog(LOG_DEBUG, "%s", buf);
	}

	pthread_mutex_unlock(&g_printf_lock);

	return ret;
}

// printf with only one string often gets converted to puts by gcc
int puts(const char *s)
{
	int ret = 0;

	if(!s) return 0;

	pthread_mutex_lock(&g_printf_lock);

	if( is_printf_allowed() ) {
		ret = fprintf(stdout, "%s\n", s);
	}
	else if( is_printf_to_syslog() ) {
		ret = strlen(s);
		syslog(LOG_DEBUG, "%s", s);
	}

	pthread_mutex_unlock(&g_printf_lock);

	return ret;
}

