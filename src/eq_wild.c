#include <string.h>

//#define TESTS

#ifndef __cplusplus

typedef int bool; 
#define true  1
#define false 0

#endif

//
// Searches for a substring in a string.
// Acts as LibC strstr, but allows substring to be not zero-terminated.
//
const char *strstrn(const char *text, const char *substring, size_t substring_len) {
	if (!substring_len)
		return text;
	for (;;) {
		const char *r = strchr(text, *substring);
		if (!r)
			return NULL;
		if (strncmp(r, substring, substring_len) == 0)
			return r;
		text = r + 1;
	}
}

//
// Matches the test with wildcard having '*'.
// See eq_wild_tests for usage samples.
//
bool eq_wild(const char *text, const char *wildcard) {
	const char *asterisk_pos = strchr(wildcard, '*');
	if (!asterisk_pos)
		return strcmp(text, wildcard) == 0;
	if (strncmp(text, wildcard, asterisk_pos - wildcard))
		return false;
	text += asterisk_pos - wildcard;
	wildcard = asterisk_pos + 1;
	for (;;) {
		if (*wildcard == 0)
			return true;
		asterisk_pos = strchr(wildcard, '*');
		if (!asterisk_pos) {
			const char *text_tail = text + strlen(text) - strlen(wildcard);
			return text_tail >= text && strcmp(wildcard, text_tail) == 0;
		} else {
			const char *fragment_pos = strstrn(text, wildcard, asterisk_pos - wildcard);
			if (!fragment_pos)
				return false;
			text = fragment_pos + (asterisk_pos - wildcard);
			wildcard = asterisk_pos + 1;
		}
	}
}

#ifdef TESTS

#include <stdio.h>
#include <stdlib.h>

void fail(const char* msg);
#define STRINGIFY(v) _STRINGIFY(v)
#define _STRINGIFY(v) #v
#define ASSERT(C) if (!(C)) fail(STRINGIFY(C));

void eq_wild_tests() {
	ASSERT(!eq_wild("asdf", "asd"));
	ASSERT(!eq_wild("asdf", "a"));
	ASSERT(eq_wild("asdf", "asdf"));

	ASSERT(eq_wild("asdf", "a*"));
	ASSERT(!eq_wild("asdf", "ad*"));

	ASSERT(eq_wild("asdf", "*f"));
	ASSERT(!eq_wild("asdf", "*easdf"));
	ASSERT(!eq_wild("asdf", "*adf"));

	ASSERT(eq_wild("asdf", "a*f"));
	ASSERT(!eq_wild("asdf", "an*f"));
	ASSERT(!eq_wild("asdf", "a*xf"));

	ASSERT(eq_wild("asdf", "*s*"));
	ASSERT(eq_wild("asdf", "*a*"));
	ASSERT(eq_wild("asdf", "*f*"));
	ASSERT(!eq_wild("asdf", "*x*"));

	ASSERT(eq_wild("asdf", "a*s*f"));
	ASSERT(eq_wild("asdf", "a*d*f"));
	ASSERT(!eq_wild("asdf", "as*s*f"));
	ASSERT(!eq_wild("asdf", "a*d*df"));

	ASSERT(eq_wild("just another test", "just*another*test"));
	ASSERT(!eq_wild("just some other test", "just*another*test"));
}

#endif
