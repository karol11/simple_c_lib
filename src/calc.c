#include <string.h>
#include <stdlib.h>
#include <math.h>

//#define TESTS

//
// Evaluates the expression given as a text string.
// expression: in-out parameter
//		- As input, it contains expression having () +-*/ ^ sin cos.
//		- On syntax error it outputs the error position inside the expression.
// out_err_msg - out parameter, returns
//		- an empty string "" if no syntax error,
//		- or text of syntax error otherwise.
// Function returns
//		- expression result
//		- or NAN on errors
// Sample:
//	  char buf[100];
//    const char *expr = gets(buf);
//    const char *err;
//    double r = calc(&expr, &err);
//    if (*err)
//		printf("error %s at %d", err, expr - buf);
//    else
//		printf("result is %lf", r);
// See calc_test for more examples.
//
double calc(const char **expression, const char **out_err_msg);




#ifndef NAN
const unsigned long long NAN_HOLD = 0x7FFFFFFFFFFFFFFFULL;
const unsigned long long PINF_HOLD = 0x7ff0000000000000ULL;
#define NAN (*(double*)&NAN_HOLD)
#define INF (*(double*)&PINF_HOLD)
#endif

static double adds(const char **expression, const char **err_msg);
 
static void skipws(const char **p) {
	while (**p && **p < ' ')
		*p++;
}

static int is(const char **p, char c)
{
	skipws(p);
	return **p == c ? ++*p, 1 : 0;
}

static int iss(const char **p, const char *token, int token_length)
{
	skipws(p);
	return strncmp(*p, token, token_length) == 0 ? *p += token_length, 1 : 0;
}

static double error(const char **err, const char *message) {
  if (!**err)
	*err = message;
  return NAN;
}

static double un(const char **p, const char **err)
{
  if (iss(p, "sin", 3))
	return sin(un(p, err));
  if (iss(p, "cos", 3))
	return cos(un(p, err));
  if (is(p, '(')) {
    double r = adds(p, err);
    return is(p, ')') ? r : error(err, "expected ')'");
  } else {
    char *next;
    double r = strtod(*p, &next);
    return next == *p ? error(err, "expected number") : (*p = next, r);
  }
}

static double powers(const char **p, const char **err)
{
  double r = un(p, err);
  while (is(p, '^'))
	r = pow(r, un(p, err));
  return r;
}

static double muls(const char **p, const char **err)
{
  double r = powers(p, err);
  for (;;) {
    if (is(p, '*'))
		r *= powers(p, err);
    else if (is(p, '/')) {
		r /= powers(p, err);
    } else
		return r;
  }
}

static double adds(const char **p, const char **err)
{
  double r = muls(p, err);
  for (;;) {
    if (is(p, '+'))
		r += muls(p, err);
    else if (is(p, '-'))
		r -= muls(p, err);
    else return r;
  }
}

double calc(const char **expression, const char **err) {
	double r;
	*err = "";
	r = adds(expression, err);
	if (**expression)
		r = error(err, "syntax error");
	return r;
}




#ifdef TESTS

#include <stdio.h>
#include <stdlib.h>

void fail(const char* msg);
#define STRINGIFY(v) _STRINGIFY(v)
#define _STRINGIFY(v) #v
#define ASSERT(C) if (!(C)) fail(STRINGIFY(C));

void calc_test_pos(const char *expr, double expected) {
	const char *err;
	const char *pos = expr;
	double r = calc(&pos, &err);
	ASSERT(fabs(r - expected) < 0.001);
}

static int is_nan(double d) { return d != d; }

void calc_test_neg(const char *expr, double expected, const char *msg, int pos) {
	const char *e = expr;
	const char *err;
	double r = calc(&e, &err);
	ASSERT((is_nan(expected) ? is_nan(r) : expected == r) && strcmp(err, msg) == 0 && pos == e - expr);
}

void calc_tests()
{
	calc_test_pos("2+3", 5);
	calc_test_pos("2*2", 4);
	calc_test_pos("2+2*2", 6);
	calc_test_pos("(2+2)*2", 8);
	calc_test_pos("3^7+1+4*-4.5", 2170);
	calc_test_pos("sin(4+1)+1", 0.0410757);

	calc_test_neg("2+2a*2", NAN, "syntax error", 3);
	calc_test_neg("abrakadabra", NAN, "expected number", 0);
	calc_test_neg("1/0", INF, "", 3);
}

#endif //TESTS
