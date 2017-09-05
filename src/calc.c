#include <string.h>
#include <stdlib.h>
#include <math.h>

//#define TESTS

//
// Evaluates expression given as a text string.
// expression: in-out parameter
//		- As input contains expression having () +-*/ ^ sin cos.
//		- On syntax error returns the position of this error inside the expression.
// out_err_msg - out parameter, returns
//		- an empty string if no syntax error,
//		- or text of syntax error otherwise.
// Function returns
//		- NAN or +-INF on calculation or syntax errors
//		- expression result otherwise.
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
  if (!*err)
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
	*err = "";
	double r = adds(expression, err);
	if (**expression)
		r = error(err, "syntax error");
	return r;
}



#ifdef TESTS

#include <stdio.h>
#include <stdlib.h>

void calc_test_pos(const char *expr, double expected) {
	const char *err;
	const char *pos = expr;
	double r = calc(&pos, &err);
	if (abs(r - expected) < 0.001) {
		printf("expected: calc(%s) == %lf, but result is %lf and error is '%s' at (%d) '%s'", expr, expected, r, err, pos - expr, pos);
		exit(-1);
	}
}

void calc_test_neg(const char *expr, double expected, const char *msg, int pos) {
	const char *e = expr;
	const char *err;
	double r = calc(&e, &err);
	if (abs(r - expected) < 0.001 || strcmp(err, msg) != 0 || pos != e - expr) {
		printf("expected error '%s' at (%d) %s with result %lf, but calc returns == %lf, err '%s', at (%d) '%s' ",
			msg, pos, expr + pos, expected, r, err, e - expr, e);
		exit(-1);
	}
}

void calc_test()
{
	calc_test_pos("2+3", 5);
	calc_test_pos("2*2", 4);
	calc_test_pos("2+2*2", 6);
	calc_test_pos("(2+2)*2", 8);
	calc_test_pos("3^7+1+4*-4.5", 2170);
	calc_test_pos("sin(4+1)+1", 1.087156);

	calc_test_neg("2+2a*2", NAN, "syntax error", 3);
	calc_test_neg("abrakadabra", NAN, "syntax error", 0);
	calc_test_neg("1/0", INF, "", 3);
}

#endif //TESTS
