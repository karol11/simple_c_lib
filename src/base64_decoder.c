//
// Decodes base64 string to binary form.
// 1. First decode_base64 calcullates the needed size of buffer.
// 2. Then it calls the given *allocator* with *size* and passing the *context* parameter.
// 2.1. The allocator uses the *context* as a pointer to application specific container.
// 2.2. It resizes the container and returns a pointer to it internal data buffer.
// 3. Then the decode_base64 fills the internal buffer with data.
//
// This techniqie allows caller to use any kinds of containers in a very easy way.
// All is needed - to define one allocator function for each type of used container.
// See tests for allocator examples.
//
void decode_base64(const char *src, char *(*allocator)(int size, void *context), void *context);




static int char2code(const char **s) {
	for (;;) {
		char c = *(*s)++;
		if (c >= 'A' && c <= 'Z') return c - 'A';
		if (c >= 'a' && c <= 'z') return c - 'a' + 26;
		if (c >= '0' && c <= '9') return c - '0' + 52;
		if (c == '+') return 62;
		if (c == '/') return 63;
		if (!c || c == '=') return -1;
	}
}

static int get_base64_decoded_size(const char *src) {
	int r = 0;
	for (;;) {
		if (char2code(&src) < 0 || char2code(&src) < 0) break;
		r++;
		if (char2code(&src) < 0) break;
		r++;
		if (char2code(&src) < 0) break;
		r++;
	}
	return r;
}

void decode_base64(const char *src, char *(*allocator)(int size, void *context), void *context) {
	char *dst = allocator(get_base64_decoded_size(src), context);
	for (;;) {
		int a,b;
		if ((a = char2code(&src)) < 0 || (b = char2code(&src)) < 0) break;
		*dst++ = a << 2 | b >> 4;
		if ((a = char2code(&src)) < 0) break;
		*dst++ = b << 4 | a >> 2;
		if ((b = char2code(&src)) < 0) break;
		*dst++ = a << 6 | b;
	}
}

#ifdef TESTS

#include <string.h>
#include <stdlib.h>


struct buffer{
	int size;
	char *data;
};

char *buffer_allocator(int size, void *context) {
	struct buffer *c = (struct buffer*)context;
	c->size = size;
	if (c->data)
		free(c->data);
	return c->data = (char*) malloc(size);
}

void fail(const char* msg);
#define STRINGIFY(v) _STRINGIFY(v)
#define _STRINGIFY(v) #v
#define ASSERT(C) if (!(C)) fail(STRINGIFY(C));

void decode_base64_tests()
{
	struct buffer r = {0};

	decode_base64("T	W	F	u", buffer_allocator, &r);
	ASSERT(r.size == 3 && memcmp(r.data, "Man", r.size) == 0);

	// different truncations
	decode_base64("YW55IGNhcm5hbCBwbGVhc3VyZS4=", buffer_allocator, &r);
	ASSERT(r.size == 20 && memcmp(r.data, "any carnal pleasure.", r.size) == 0);

	decode_base64("YW55IGNhcm5hbCBwbGVhc3VyZQ==", buffer_allocator, &r);
	ASSERT(r.size == 19 && memcmp(r.data, "any carnal pleasure", r.size) == 0);

	decode_base64("YW55IGNhcm5hbCBwbGVhc3Vy", buffer_allocator, &r);
	ASSERT(r.size == 18 && memcmp(r.data, "any carnal pleasur", r.size) == 0);

	decode_base64("YW55IGNhcm5hbCBwbGVhc3U=", buffer_allocator, &r);
	ASSERT(r.size == 17 && memcmp(r.data, "any carnal pleasu", r.size) == 0);

	decode_base64("YW55IGNhcm5hbCBwbGVhcw==", buffer_allocator, &r);
	ASSERT(r.size == 16 && memcmp(r.data, "any carnal pleas", r.size) == 0);

	// if incomplete =
	decode_base64("YW55IGNhcm5hbCBwbGVhcw=", buffer_allocator, &r);
	ASSERT(r.size == 16 && memcmp(r.data, "any carnal pleas", r.size) == 0);

	decode_base64(
		"TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz"
		"IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg"
		"dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"
		"dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo"
		"ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=",
		buffer_allocator, &r);
	ASSERT(r.size == 269 && memcmp(r.data,
		"Man is distinguished, not only by his reason, but by this singular passion from "
		"other animals, which is a lust of the mind, that by a perseverance of delight "
		"in the continued and indefatigable generation of knowledge, exceeds the short "
		"vehemence of any carnal pleasure.", r.size) == 0);

	free(r.data);
}

#endif //TESTS
