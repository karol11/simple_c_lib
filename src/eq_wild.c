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

void eq_wild_test(const char *t, const char *w, bool expected) {
	if (eq_wild(t, w) != expected) {
		printf("expected: eq_wild(%s, %s) == %s", t, w, expected ? "true" : "false");
		exit(-1);
	}
}

void eq_wild_tests() {
	eq_wild_test("asdf", "asd", false);
	eq_wild_test("asdf", "a", false);
	eq_wild_test("asdf", "asdf", true);

	eq_wild_test("asdf", "a*", true);
	eq_wild_test("asdf", "ad*", false);

	eq_wild_test("asdf", "*f", true);
	eq_wild_test("asdf", "*easdf", false);
	eq_wild_test("asdf", "*adf", false);

	eq_wild_test("asdf", "a*f", true);
	eq_wild_test("asdf", "an*f", false);
	eq_wild_test("asdf", "a*xf", false);

	eq_wild_test("asdf", "*s*", true);
	eq_wild_test("asdf", "*a*", true);
	eq_wild_test("asdf", "*f*", true);
	eq_wild_test("asdf", "*x*", false);

	eq_wild_test("asdf", "a*s*f", true);
	eq_wild_test("asdf", "a*d*f", true);
	eq_wild_test("asdf", "as*s*f", false);
	eq_wild_test("asdf", "a*d*df", false);

	eq_wild_test("just another test", "just*another*test", true);
	eq_wild_test("just some other test", "just*another*test", false);
}

#endif
