//
// Decodes a single unicode symbol from the sequence of bytes containing UTF-8 character representation.
// Also decodes utf16 surrogate pairs if they are transcoded by utf16->utf8 converter.
// Skips all ill-formed sequences.
//
// get_fn - a function returning the next byte of sequence.
// If it returns 0 or negative number, decoding stops and get_utf8 returns this value.
// get_fn_context - context to be passed to get_fn.
// Usage example: int a = get_utf8(fgetc, fopen("text", "r"));
// For more examples see utf8_tests.
//
int get_utf8(int (*get_fn)(void *context), void *get_fn_context);

//
// Encodes the single unicode character as the sequence of bytes in UTF-8 encoding.
// character - in range 0..0x10ffff
// put_fn - function to be called to store bytes.
//    If it returs <= 0, encoding sequence is terminated
// put_fn_context - cntext data to be passed to put_fn.
// Returns the result of last put_fn call, or 0 if the character out of allowed range.
// For examples see utf8_tests.
//
int put_utf8(int character, int (*put_fn)(int ch, void *context), void *put_fn_context);








int get_utf8(int (*get_fn)(void *context), void *get_fn_context)
{
	int r, n;
restart_and_reload:
	r = get_fn(get_fn_context);
restart:
	n = 2;
	if (r <= 0)
		return r;
	if ((r & 0x80) == 0)
		return r;
	if ((r & 0xe0) == 0xc0) r &= 0x1f;
	else if ((r & 0xf0) == 0xe0) n = 3, r &= 0xf;
	else if ((r & 0xf8) == 0xf0) n = 4, r &= 7;
	else
		goto restart_and_reload;
	while (--n) {
		int c = get_fn(get_fn_context);
		if ((c & 0xc0) != 0x80) {
			if (c <= 0)
				return c;
			r = c;
			goto restart;
		}
		r = r << 6 | (c & 0x3f);
	}
	while (r >= 0xD800 && r <= 0xDBFF) { // it's ill-formed surrogate from utf16->utf8
		int low_part = get_utf8(get_fn, get_fn_context);
		if (low_part < 0xDC00 || low_part > 0xDFFF)
			r = low_part; // bad utf16 sequence
		else {
			return ((r & 0x3ff) << 10 | (low_part & 0x3ff)) | 0x10000;
		}
	}
	return r;
}

int put_utf8(int v, int (*put_fn)(int ch, void *context), void *put_fn_context)
{
	if (v <= 0x7f)
		return put_fn(v, put_fn_context);
	else {
		int r;
		if (v <= 0x7ff)
			r = put_fn(v >> 6 | 0xc0, put_fn_context);
		else {
			if (v <= 0xffff)
				r = put_fn(v >> (6 + 6) | 0xe0, put_fn_context);
			else {
				if (v <= 0x10ffff)
					r = put_fn(v >> (6 + 6 + 6) | 0xf0, put_fn_context);
				else
					return 0;
				if (r > 0)
					r = put_fn(((v >> (6 + 6)) & 0x3f) | 0x80, put_fn_context);
			}
			if (r > 0)
				r = put_fn(((v >> 6) & 0x3f) | 0x80, put_fn_context);
		}
		if (r > 0)
			r = put_fn((v & 0x3f) | 0x80, put_fn_context);
		return r;
	}
}

#ifdef TESTS

#include <string.h>
#include <stdlib.h>

void fail(const char* msg);
#define STRINGIFY(v) _STRINGIFY(v)
#define _STRINGIFY(v) #v
#define ASSERT(C) if (!(C)) fail(STRINGIFY(C));

static int get_c(void *context) {
	return *(*(unsigned char**)context)++;
}
static int put_c(int v, void *context) {
	*(*(char**)context)++ = v;
	return 1;
}

static void test_both_ways(int a, char *b) {
	char buffer[20];
	char *p = buffer;
	char *pp = buffer;
	ASSERT(put_utf8(a, put_c, &p) > 0);
	*p++ = 0;
	ASSERT(memcmp(buffer, b, p - buffer) == 0);
	ASSERT(get_utf8(get_c, &pp) == a);
	ASSERT(pp + 1  == p);
}

void utf8_tests()
{
	test_both_ways(0x24, "\x24");
	test_both_ways(0xa2, "\xc2\xa2");
	test_both_ways(0x20AC, "\xe2\x82\xac");
	test_both_ways(0x10348, "\xf0\x90\x8d\x88");
}

#endif //TESTS
