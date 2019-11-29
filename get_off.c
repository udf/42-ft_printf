#include <stdio.h>
#include <unistd.h>
int main() {
	printf("%ld", (void *)&vdprintf - (void *)&write);
	return 0;
}
