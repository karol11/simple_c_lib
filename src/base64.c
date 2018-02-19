//
// Decodes a base64 string to a binary form.
// Params:
//   src - an asciiz source string
//   allocator - a user-defined function handling ouput data alocation
//   context - a pointer to be passed to the allocator.
//
// 1. First the decode_base64 function calculates the size of the decoded data.
// 2. Then it calls the given allocator with the calculated size and the context parameter.
// 2.1. The allocator uses the context as a pointer to application-specific container.
// 2.2. It resizes the container and returns a pointer to internal data inside the container.
// 3. Then the decode_base64 fills the internal buffer with data.
//
// This techniqie allows the caller to use any kinds of containers in a very easy way.
// All is needed - to define one allocator function for each container type.
// See tests for allocator examples.
//
void decode_base64(
    const char *src,
	char *(*allocator)(int size, void *context),
	void *context);

void encode_base64(
	const unsigned char *src,
	int src_size,
	char *(*allocator)(int size, void *context),
	void *context);





static int code2char(unsigned int c) {
	c &= 0x3f;
	return
		c < 52 ?  (c < 26 ? 'A' : 'a' - 26) + c :
		c < 62 ?  '0' + c - 52 :
		c == 62 ? '+' : '/';
}

void encode_base64(const unsigned char *src, int src_size, char *(*allocator)(int size, void *context), void *context) {
	char *dst = allocator((src_size + 2) / 3 * 4, context);
	if (!dst)
		return;
	for (; src_size >= 3; src_size -= 3, dst += 4, src += 3) {
		unsigned int a = src[0];
		unsigned int b = src[1];
		unsigned int c = src[2];
		dst[0] = code2char(a >> 2);
		dst[1] = code2char(a << 4 | b >> 4);
		dst[2] = code2char(b << 2 | c >> 6);
		dst[3] = code2char(c);
	}
	if (src_size != 0) {
		unsigned int a = *src;
		dst[0] = code2char(a >> 2);
		if (src_size == 1) {
			dst[1] = code2char(a << 4);
			dst[2] = '=';
		} else {
			unsigned int b = src[1];
			dst[1] = code2char(a << 4 | b >> 4);
			dst[2] = code2char(b << 2);
		}
		dst[3] = '=';
	}
}

static int char2code(const char **s) {
	for (;;) {
		char c = *(*s)++;
		if (c < 'A') {
			if (c >= '0') {
				if (c <= '9')
					return c - '0' + 52;
			} else {
				if (c == '+') return 62;
				if (c == '/') return 63;
				if (!c || c == '=') return -1;				
			}
		} else {
			if (c < 'a') {
				if (c <= 'Z')
					return c - 'A';
			} else {
				if (c <= 'z')
					return c - 'a' + 26;
			}
		}
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
	if (!dst)
		return;
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

static void check_two_way(const char *encoded, const char *raw) {
	struct buffer buf = {0};
	int raw_size = strlen(raw);

	encode_base64(raw, raw_size, buffer_allocator, &buf);
	ASSERT(buf.size == strlen(encoded) && memcmp(buf.data, encoded, buf.size) == 0);

	decode_base64(encoded, buffer_allocator, &buf);
	ASSERT(buf.size == raw_size && memcmp(buf.data, raw, buf.size) == 0);

	free(buf.data);
}

void decode_base64_tests()
{
	struct buffer r = {0};

	decode_base64("T	W	F	u", buffer_allocator, &r);
	ASSERT(r.size == 3 && memcmp(r.data, "Man", r.size) == 0);

	encode_base64("Man", 3, buffer_allocator, &r);
	ASSERT(r.size == 4 && memcmp(r.data, "TWFu", r.size) == 0);

	// different truncations
	check_two_way("YW55IGNhcm5hbCBwbGVhc3VyZS4=", "any carnal pleasure.");
	check_two_way("YW55IGNhcm5hbCBwbGVhc3VyZQ==", "any carnal pleasure");
	check_two_way("YW55IGNhcm5hbCBwbGVhc3Vy",     "any carnal pleasur");
	check_two_way("YW55IGNhcm5hbCBwbGVhc3U=",     "any carnal pleasu");
	check_two_way("YW55IGNhcm5hbCBwbGVhcw==",     "any carnal pleas");

	check_two_way("/8AMqg==", "\xff\xc0\x0c\xaa");

	// if incomplete =
	decode_base64("YW55IGNhcm5hbCBwbGVhcw=", buffer_allocator, &r);
	ASSERT(r.size == 16 && memcmp(r.data, "any carnal pleas", r.size) == 0);

	check_two_way(
		"TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz"
		"IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg"
		"dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"
		"dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo"
		"ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4="
		,
		"Man is distinguished, not only by his reason, but by this singular passion from "
		"other animals, which is a lust of the mind, that by a perseverance of delight "
		"in the continued and indefatigable generation of knowledge, exceeds the short "
		"vehemence of any carnal pleasure.");

	free(r.data);
}

#endif //TESTS
