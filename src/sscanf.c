#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

//
// Small but complying to standard 'sscanf' and 'vsscanf' implementation.
//

#define TEST

#ifdef WIN32
#define strtoll _strtoi64
#define strtoull _strtoui64
#else
	unsigned long long strtoull(char const *nptr, char **endptr, int base);
	long long strtoll(char const *nptr, char **endptr, int base);
#endif

int vsscanf(char const*buf, char const*fmt, va_list ap);

int __cdecl sscanf(char const*buf, char const *fmt, ...)
{
  va_list ap;
  int r;
  va_start(ap, fmt);
  r = vsscanf(buf, fmt, ap);
  va_end(ap);
  return r;
}

static void set_typed(va_list *ap, int dst_type, long long value) {
	switch (dst_type) {
	case 'i': *va_arg(*ap, int*) = (int) value; break;
	case 'c': *va_arg(*ap, char*) = (char) value; break;
	case 'h': *va_arg(*ap, short int*) = (short int) value; break;
	case 'l': *va_arg(*ap, long int*) = (long int) value; break;
	case 'L': *va_arg(*ap, long long*) = value; break;
	}
}

static void skip_ws(char **buf) {
	char *s = *buf;
	while (*s && *s <= ' ')
		s++;
	*buf = s;
}

#define SET_BIT(mask, i) mask[(i) >> 5] |= 1 << (i & 0x1f)
#define IS_SET(mask, i) mask[(i) >> 5] & (1 << (i & 0x1f))

int __cdecl vsscanf(char const *buf_start, char const *fmt_, va_list ap)
{
	char *buf = (char *) buf_start;
	char *fmt = (char *) fmt_; 
	int count = -1;
	bool first_match = true;
	while (*fmt) {
		if (*fmt == '%') {
			bool skip_assign = *++fmt == '*';
			long width;
			char dst_type;
			if (skip_assign)
				fmt++;
			width = strtol(fmt, &fmt, 10);
			dst_type =
				*fmt == 'L' ? ++fmt, 'L' :
				*fmt == 'h' ?
					(*++fmt == 'h' ? ++fmt, 'c' : 'h') : 
				*fmt == 'l' ? 
					(*++fmt == 'l' ? ++fmt, 'L' : 'l') :
				'i';
			switch (*fmt) {
			case 0: return count;
			case '%':
				skip_ws(&buf);
				if (*buf++ != '%')
					return count;
				break;
			case 'n':
				skip_ws(&buf);
				if (!skip_assign)
					set_typed(&ap, dst_type, buf - buf_start);
				break;
			case 'i':
			case 'd':
			case 'o':
			case 'x':
			case 'X':
			case 'p':
			case 'u':
				skip_ws(&buf);
				{
					int radix =
						*fmt == 'd' || *fmt == 'u' ? 10 :
						*fmt == 'o' ? 8 :
						*fmt == 'X' || *fmt == 'x' || *fmt == 'p' ? 16 : 0;
					char* end;
					char temp[65];
					const char* src = width >= 1 && width <=64 ?
						strncpy(temp, buf, width), temp[width]=0, temp :
						buf;
					long long v = *fmt == 'd' || *fmt == 'i' ? strtoll(src, &end, radix) : (long long)strtoull(src, &end, radix);
					end = buf + (end - src);
					if (end == buf)
						return count;
					buf = end;
					if (!skip_assign) {
						count++;
						set_typed(&ap, dst_type, v);
					}
				}
				break;
			case 'f':
			case 'e':
			case 'g':
			case 'G':
			case 'a':
#ifdef CONFIG_LIBC_FLOATINGPOINT
				skip_ws(&buf);
				{
					char* end;
					char temp[65];
					char* src = width >= 1 && width <=64 ? strncpy(temp, buf, width), temp : buf;
					double v = strtod(src, &end);
					end = buf + (end - src);
					if (end == buf)
						return count;
					buf = end;
					if (!skip_assign) {
						count++;
						if (dst_type == 'l') *va_arg(ap, double*) = v;
						else                 *va_arg(ap, float*) = (float) v;
					}
				}
				break;
#else
				return count;
#endif
			case 'c':
				if (width < 2) {
					if (!*buf)
						return count;
					if (skip_assign)
						buf++;
					else {
						*va_arg(ap, char*) = *buf++;
						count++;
					}						
				} else {
					int len = strlen(buf);
					if (len < width)
						return count;
					if (!skip_assign) {
						memcpy(va_arg(ap, char*), buf, width);
						count++;
					}
					buf += len;
				}
				break;
			case 's':
				skip_ws(&buf);
				{
					if (width == 0)
						width = 0x7fffffff;
					if (skip_assign) {
						while(*buf > ' ' && width-- > 0)
							buf++;
					} else {
						char *dst = va_arg(ap, char*);
						while(*buf > ' ' && width-- > 0)
							*dst++ = *buf++;
						*dst = 0;
						count++;
					}
				}
				break;
			case '[':
				{
					bool negate = *++fmt == '^';
					unsigned int mask[256/32];
					memset(mask, 0, sizeof(mask));
					if (negate) fmt++;
					if (*fmt == ']') fmt++, SET_BIT(mask, ']');
					while (*fmt && *fmt != ']') {
						char f = *fmt++;
						SET_BIT(mask, f);
						if (*fmt != '-')
							continue;
						if (!*++fmt)
							return count;
						if (*fmt == ']') {
							SET_BIT(mask, '-');
							break;
						}
						{
							char t = *fmt++;
							do
								++f, SET_BIT(mask, f);
							while (f < t);
						}
					}
					if (!*fmt)
						return count;
					if (negate) {
						int i = 0;
						for (; i < 256/32; i++)
							mask[i] = ~mask[i];
					}
					if (width == 0)
						width = 0x7fffffff;
					if (skip_assign) {
						while(*buf && IS_SET(mask, *buf) && width-- > 0)
							buf++;
					} else {
						char *dst = va_arg(ap, char*);
						while(*buf && IS_SET(mask, *buf) && width-- > 0)
							*dst++ = *buf++;
						*dst = 0;
						count++;
					}
				}
				break;
			}
			fmt++;
		} else if (*fmt <= ' ') {
			while (*fmt && *fmt <= ' ')
				fmt++;
			skip_ws(&buf);
		} else if (*buf++ != *fmt++)
			break;
		if (first_match) first_match = false, ++count;
	}
	return count;
};

#undef IS_SET
#undef SET_BIT



#ifdef TEST


#include <assert.h>
#include <stdio.h>
#include <math.h>

#define STRINGIFY(v) _STRINGIFY(v)
#define _STRINGIFY(v) #v

#define ASSERT_S(expr) if (!(expr)) { printf("failed at %s\n", STRINGIFY(expr)); }

void sscanf_test()
{
	int r;
	int i, j, k, l;
	unsigned m, n;
	void *p;
	char s[256];
	char c;
	union {
		int i;
		long l;
		unsigned u;
		unsigned long ul;
		short s;
		unsigned short us;
		signed char c;
		unsigned char uc;
		long long ll;
		unsigned long long ull;
		unsigned char b[8];
	} u;
#ifdef CONFIG_LIBC_FLOATINGPOINT
	float fl;
	union {
		float f;
		double d;
		unsigned char b[sizeof(double)];
	} f;
#endif

	i=0xcc;
	r=sscanf("124", "%d", &i);
	ASSERT_S(r == 1 && i == 124);

	i=0xcc;
	r=sscanf("-124", "%d", &i);
	ASSERT_S(r == 1 && i == -124);
	
	i=0xcc;
	r=sscanf("+124", "%d", &i);
	ASSERT_S(r == 1 && i  == 124);

	i=0xcc;
	r=sscanf("+124", "%d", &i);
	ASSERT_S(r == 1 && i  == 124);

	i=0xcc;
	r=sscanf("0", "%d", &i);
	ASSERT_S(r == 1 && i == 0);

	i=0xcc;
	r=sscanf("-0", "%d", &i);
	ASSERT_S(r == 1 && i == 0);

	i=0xcc;
	r=sscanf("+0", "%d", &i);
	ASSERT_S(r == 1 && i == 0);

	i=0xcc;
	r=sscanf("010", "%d", &i);
	ASSERT_S(r == 1 && i == 10);

	i=0xcc;
	r=sscanf("-010", "%d", &i);
	ASSERT_S(r == 1 && i == -10);

	i=0xcc;
	r=sscanf(" 1", "%d", &i);
	ASSERT_S(r == 1 && i == 1);

	n=0xcc;
	r=sscanf("0", "%u", &n);
	ASSERT_S(r == 1 && n == 0);

	n=0xcc;
	r=sscanf("010", "%u", &n);
	ASSERT_S(r == 1 && n == 10);

	n=0xcc;
	r=sscanf("2147483640", "%u", &n);
	ASSERT_S(r == 1 && n == 2147483640);

	n=0xcc;
	r=sscanf(" 1", "%u", &n);
	ASSERT_S(r == 1 && n == 1);

	n=0xcc;
	r=sscanf("12345678", "%4u", &n);
	ASSERT_S(r == 1 && n == 1234);

	i=0xcc;
	r=sscanf("42", "%i", &i);
	ASSERT_S(r == 1 && i == 42);

	i=0xcc;
	r=sscanf("-42", "%i", &i);
	ASSERT_S(r == 1 && i == -42);

	i=0xcc;
	r=sscanf("+42", "%i", &i);
	ASSERT_S(r == 1 && i == +42);

	i=0xcc;
	r=sscanf("010", "%i", &i);
	ASSERT_S(r == 1 && i == 8);

	i=0xcc;
	r=sscanf("+010", "%i", &i);
	ASSERT_S(r == 1 && i == +8);

	i=0xcc;
	r=sscanf("-010", "%i", &i);
	ASSERT_S(r == 1 && i == -8);

	i=0xcc;
	r=sscanf("0x1f", "%i", &i);
	ASSERT_S(r == 1 && i == 31);

	i=0xcc;
	r=sscanf("+0x1f", "%i", &i);
	ASSERT_S(r == 1 && i == +31);

	i=0xcc;
	r=sscanf("-0x1f", "%i", &i);
	ASSERT_S(r == 1 && i == -31);

	i=0xcc;
	r=sscanf("0", "%i", &i);
	ASSERT_S(r == 1 && i == 0);

	i=0xcc;
	r=sscanf("+0", "%i", &i);
	ASSERT_S(r == 1 && i == 0);

	i=0xcc;
	r=sscanf("-0", "%i", &i);
	ASSERT_S(r == 1 && i == 0);

	i=0xcc;
	r=sscanf(" 0", "%i", &i);
	ASSERT_S(r == 1 && i == 0);

	n=0xcc;
	r=sscanf("%42", "%%%u", &n);
	ASSERT_S(r == 1 && n == 42);

	n=0xcc;
	r=sscanf("0", "%o", &n);
	ASSERT_S(r == 1 && n == 0);

	n=0xcc;
	r=sscanf("10", "%o", &n);
	ASSERT_S(r == 1 && n == 8);

	n=0xcc;
	r=sscanf("17777777777", "%o", &n);
	ASSERT_S(r == 1 && n == 017777777777);

	n=0xcc;
	r=sscanf("0", "%x", &n);
	ASSERT_S(r == 1 && n == 0);

	n=0xcc;
	r=sscanf("1", "%X", &n);
	ASSERT_S(r == 1 && n == 1);

	n=0xcc;
	r=sscanf("1f", "%x", &n);
	ASSERT_S(r == 1 && n == 31);

	n=0xcc;
	r=sscanf("7fffffff", "%x", &n);
	ASSERT_S(r == 1 && n == 0x7fffffff);

	memset(s, 0xcc, sizeof(s));
	r=sscanf(" test 42", "%s", s);
	ASSERT_S(r == 1 && strcmp(s, "test") == 0);

	memset(s, 0xcc, sizeof(s));
	r=sscanf(" testtest", "%5s", s);
	ASSERT_S(r == 1 && strcmp(s, "testt") == 0);

	n=0xcc;
	r=sscanf("12 42", "%*u%u", &n);
	ASSERT_S(r == 1 && n == 42);

	m=0xcc;
	i=0xcc;
	r=sscanf(" 42", "%u%n", &m, &i);
	ASSERT_S(r == 1 && m == 42 && i == 3);

	m=0xcc;
	n=0x5a;
	r=sscanf("12", "%u %n", &m, &n);
	ASSERT_S(r == 1 && m == 12 && n == 2);

	memset(s, 0, sizeof(s));
	r=sscanf(" 1234", "%c", s);
	ASSERT_S(r == 1 && *s == ' ');

	memset(s, 0, sizeof(s));
	r=sscanf(" 1234", "%3c", s);
	ASSERT_S(r == 1 && memcmp(s, " 12", 3) == 0);

	memset(s, 0, sizeof(s));
	r=sscanf(" 1234", " %2c", s);
	ASSERT_S(r == 1 && memcmp(s, "12", 2) == 0);

	p=(void*)0xCCCCCCCC;
	printf("%p\n", p);
	r=sscanf(" 0x12345678", "%p", &p);
	ASSERT_S(r == 1 && p == (void*)0x12345678);

	memset(s, 0, sizeof(s));
	i = n = c= j = m = 0;
	r=sscanf("12 test 45 c 67 xx", "%i%s %u %c%d %*s%n", &i, s, &n, &c, &j, &m);
	ASSERT_S(r == 5 && i == 12 && !strcmp(s, "test") && n == 45 && c == 'c' && j == 67 && m == 18);

	memset(s, 0, sizeof(s));
	r=sscanf("12345", "%[321]", s);
	ASSERT_S(r == 1 && strcmp("123", s) == 0);

	memset(s, 0, sizeof(s));
	r=sscanf("12345", "%[1-3]", s);
	ASSERT_S(r == 1 && strcmp("123", s) == 0);

	memset(s, 0, sizeof(s));
	r=sscanf("56781234", "%[^1-4]", s);
	ASSERT_S(r == 1 && strcmp("5678", s) == 0);

	memset(s, 0, sizeof(s));
	r=sscanf("23-4", "%[-2-3]", s);
	ASSERT_S(r == 1 && strcmp("23-", s) == 0);

	memset(s, 0, sizeof(s));
	r=sscanf("23-4", "%[2-3-]", s);
	ASSERT_S(r == 1 && strcmp("23-", s) == 0);

	memset(s, 0, sizeof(s));
	r=sscanf("[]xx", "%[][]", s);
	ASSERT_S(r == 1 && strcmp("[]", s) == 0);

	memset(s, 0, sizeof(s));
	r=sscanf("xyz]x", "%[^]]", s);
	ASSERT_S(r == 1 && strcmp("xyz", s) == 0);

	memset(s, 0, sizeof(s)), n=0;
	r=sscanf("12345", "%[1-3]4%u", s, &n);
	ASSERT_S(r == 2 && strcmp("123", s) == 0 && n == 5);

	memset(u.b, 0xaa, sizeof(u));
	r=sscanf("12345678", "%lx", &u.ul);
	ASSERT_S(r == 1 && u.ul == 0x12345678 && memchr(&u, 0xaa, sizeof(u)));

	memset(u.b, 0xaa, sizeof(u));
	r=sscanf("12345678", "%hx", &u.us);
	ASSERT_S(r == 1 && u.us == 0x5678 && memchr(&u.l, 0xaa, sizeof(u.l)));

	memset(u.b, 0xaa, sizeof(u));
	r=sscanf("12345678", "%hhx", &u.uc);
	ASSERT_S(r == 1 && u.uc == 0x78 && memchr(&u.s, 0xaa, sizeof(u.s)));

	memset(u.b, 0xaa, sizeof(u));
	r=sscanf("12345678", "%llx", &u.ull);
	ASSERT_S(r == 1 && u.ull == 0x12345678 && !memchr(&u.ll, 0xaa, sizeof(u.ll)));

	r=sscanf("9223372036854775807", "%lld", &u.ll);
	ASSERT_S(r == 1 && u.ll == 9223372036854775807LL);

	r=sscanf("18446744073709551615", "%llu", &u.ull);
	ASSERT_S(r == 1 && u.ull == 18446744073709551615ULL);

	r=sscanf("-9223372036854775807", "%lld", &u.ll);
	ASSERT_S(r == 1 && u.ll == -9223372036854775807LL);

#ifdef CONFIG_LIBC_FLOATINGPOINT
	memset(&f, 0xaa, sizeof(f));
	r=sscanf("-12.345", "%f", &f.f);
	ASSERT_S(r == 1 && fabsf(f.f + 12.345) < 0.000001 && memchr(&f, 0xaa, sizeof(f)));

	memset(&f, 0xaa, sizeof(f));
	r=sscanf("0.1234", "%le", &f.d);
	ASSERT_S(r == 1 && fabs(f.d - 0.1234) < 0.00000001 && !memchr(&f, 0xaa, sizeof(f)));

	memset(&f, 0xaa, sizeof(f));
	r=sscanf("5.24e3", "%f", &f.f);
	ASSERT_S(r == 1 && fabsf(f.f - 5240) < 0.001 && memchr(&f, 0xaa, sizeof(f)));

	memset(&f, 0xaa, sizeof(f)), n=0;
	r=sscanf("123.4567.89", "%6f%f%n", &f.f, &fl, &n);
	ASSERT_S(r == 2 && fabsf(f.f - 123.45) < 0.001 && fabsf(fl - 67.89) < 0.001 && n == 11);
#endif

	r=sscanf("", "%u", &n);
	ASSERT_S(r == EOF);

	r=sscanf("12", "%u%u", &m, &n);
	ASSERT_S(r == 1);

	r=sscanf(" ", "%u", &n);
	ASSERT_S(r == EOF);

	r=sscanf("a12", "ab%u", &n);
	ASSERT_S(r == 0);

	n=0;
	r=sscanf("12345", "%-3u", &n);
	ASSERT_S(r == 1 && n == 12345);

	m=0xaa;
	n=0xee;
	r=sscanf("6543", "%u,%n", &m, &n);
	ASSERT_S(r == 1);
	ASSERT_S(n == 0xee);

	m=0xaa;
	n=0xee;
	r=sscanf(" 100.2 AAA, 11/12\n", " %*[^,], %d/%d", &m, &n);
	ASSERT_S(r == 2 && m == 11 && n == 12);

	m=0xaa;
	n=0xee;
	r=sscanf(" 100.2 XXX, 11/12\n", " %*s%*s %*d/%d", &m);
	ASSERT_S(r == 1 && m == 12);
}
#undef ASSERT_S
#undef STRINGIFY
#undef _STRINGIFY

#endif //TEST
