#include <stdio.h>
#include <stdlib.h>

void calc_tests();
void eq_wild_tests();
void sscanf_tests();
void decode_base64_tests();

void fail(const char *msg) {
	printf("fail: %s\n", msg);
	exit(-1);
}

int main() {
	decode_base64_tests();
	calc_tests();
	eq_wild_tests();
	sscanf_tests(); 
	printf("ok\n");
}
