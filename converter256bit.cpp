#include <cstdio>
#define BASE 256
void basedigit(int n) {
	printf("%#.2x\n", n%BASE);
	if (n/BASE != 0)
		basedigit(n/BASE);
}
int main() {
	basedigit(65535);
	return 0;
}