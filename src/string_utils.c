#include "string_utils.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char*
string_fmt(const char *fmt, ...)
{
	va_list ap, ap_copy;
	va_start(ap, fmt);

	va_copy(ap_copy, ap);
	size_t msg_len = vsnprintf(NULL, 0, fmt, ap_copy) + 1;
	va_end(ap_copy);

	char *msg = malloc(msg_len);
	if (msg) {
		va_copy(ap_copy, ap);
		vsnprintf(msg, msg_len, fmt, ap_copy);
		va_end(ap_copy);
	}
	va_end(ap);
	return msg;
}

char*
string_join(const char **strings, const char *sep)
{
	assert(strings != NULL);
	assert(sep != NULL);

	// count the number of strings to join
	size_t strings_count;
	for (strings_count = 0; strings[strings_count]; strings_count++);

	if (strings_count == 0)
		return NULL;

	// compute lengths
	size_t lengths[strings_count];
	size_t sep_len = strlen(sep);
	size_t len = 0;
	for (size_t i = 0; i < strings_count; i++) {
		if (!strings[i])
			continue;
		lengths[i] = strlen(strings[i]);
		len += lengths[i];
		if (i < strings_count - 1)
			len += sep_len;
	}

	// allocate the buffer for the resulting string
	char *result = malloc(len + 1);
	if (!result)
		return NULL;
	result[len] = 0; // NUL-terminator

	// concatenate strings
	char *dst = result;
	for (size_t i = 0; i < strings_count; i++) {
		if (!strings[i])
			continue;
		strncpy(dst, strings[i], lengths[i]);
		dst += lengths[i];
		if (i < strings_count - 1) {
			dst = strncpy(dst, sep, sep_len);
			dst += sep_len;
		}
	}

	return result;
}

char*
string_replace(const char *src, const char *pat, const char *repl)
{
	assert(src != NULL);
	assert(pat != NULL);
	assert(repl != NULL);

	size_t src_len = strlen(src);
	size_t pat_len = strlen(pat);
	size_t repl_len = strlen(repl);

	// count the number of pattern occurrences in the source string
	size_t count = 0;
	char *loc = (char*)src;
	while ((loc = strstr(loc, pat))) {
		count++;
		loc += pat_len;
	}

	// allocate a buffer large enough to contain new text
	size_t len = src_len - pat_len * count + repl_len * count;
	char *result = malloc(len + 1);
	if (!result)
		return NULL;
	result[len] = 0; // NUL-terminator

	// copy each non-overlapping string preceeding the pattern and append
	// the replacement after it for each pattern occurrence
	char *start = (char*)src;
	char *dst = result;
	loc = (char*)src;
	while ((loc = strstr(loc, pat))) {
		strncpy(dst, start, loc - start);
		dst += loc - start;
		if (repl_len > 0) {
			strncpy(dst, repl, repl_len);
			dst += repl_len;
		}
		loc += pat_len;
		start = loc;
	}
	strncpy(dst, start, src_len - (start - src));

	return result;
}

void
string_freev(char **strv, size_t count)
{
	if (strv) {
		for (size_t i = 0; i < count; i++) {
			free(strv[i]);
		}
		free(strv);
	}
}

char**
string_split(const char *s, char ch, size_t *r_count)
{
	assert(s != NULL);

	char *string = (char*)s, *eol = NULL;

	// count the number of lines
	size_t count = 1;
	while ((eol = strchr(string, ch))) {
		count++;
		string = eol + 1;
	}

	// allocate a container for string pointers
	char **strings = malloc(sizeof(char*) * count);
	if (!strings) {
		return 0;
	}
	memset(strings, 0, sizeof(char*) * count);

	// copy each string as a separate string into it
	string = (char*)s;
	for (size_t l = 0; l < count; l++) {
		// locate the end of current string
		eol = strchr(string, ch);
		if (!eol) {
			eol = strchr(string, '\0');
		}

		// compute the length and allocate memory for the string
		size_t len = eol - string;
		strings[l] = malloc(len + 1);
		if (!strings[l]) {
			goto out_of_mem;
		}

		// copy string content
		strings[l][len] = '\0';
		strncpy(strings[l], string, len);

		// advance the pointer to next string
		string = eol + 1;
	}

	if (r_count) {
		*r_count = count;
	}
	return strings;

out_of_mem:
	string_freev(strings, count);
	if (r_count) {
		*r_count = 0;
	}
	return NULL;
}

char*
string_copy(const char *str)
{
	return string_fmt("%s", str);
}
