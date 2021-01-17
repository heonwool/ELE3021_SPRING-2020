#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
	int child = fork();

	if(child == 0) {
		for(int i = 0; i < 50; i++) {
			printf(1, "Child\n");
			yield();
		}
	} else {
		for(int i = 0; i < 50; i++) {
			printf(1, "Parent\n");
			yield();
		}
	}
	exit();
}
