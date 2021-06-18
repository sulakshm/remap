#include <sys/types.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


#define PGSIZE (4096)
const char *devfile = "/dev/testcontrol";
const unsigned WINDOW = (PGSIZE * 16);

char dest[PGSIZE];
char *global_window_ptr;


#define window(i) ((PGSIZE) * (i))

const unsigned long pattern[16] = {
	0x1111111111111111UL,
	0x2222222222222222UL,
	0x3333333333333333UL,
	0x4444444444444444UL,
	0x5555555555555555UL,
	0x6666666666666666UL,
	0x7777777777777777UL,
	0x8888888888888888UL,
	0x9999999999999999UL,
	0xaaaaaaaaaaaaaaaaUL,
	0xbbbbbbbbbbbbbbbbUL,
	0xccccccccccccccccUL,
	0xddddddddddddddddUL,
	0xeeeeeeeeeeeeeeeeUL,
	0xffffffffffffffffUL,
	0x0000000000000000UL};


int map_window(int fd)
{
	global_window_ptr = mmap(NULL, WINDOW, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (global_window_ptr == MAP_FAILED) {
		printf("mmap failed.. error %d", errno);
		return -1;
	}

	return 0;
}

int fillpage(void *b, int i)
{
	unsigned long p = pattern[i];
	unsigned long *d = b;

	int iter = PGSIZE/sizeof(unsigned long);
	int x;

	for (x=0; x<iter; x++) {
		d[x] = p;
	}

	return 0;
}

int checkpage(int i)
{
	unsigned long *s = (unsigned long*)((char*)global_window_ptr + window(i));
	unsigned long *d = (unsigned long*) dest;

	fillpage(d, i);

	assert(memcmp(s, d, PGSIZE) == 0);

	return 0;
}


int main(int argc, char *argv[])
{
	int i;
	int fd = open(devfile, O_RDWR);
	assert (fd != 0);

	printf("%s open success\n", devfile);

	assert(map_window(fd) == 0);
	printf("%s map success\n", devfile);


	for (i=0; i<16; i++) {
		int rc = checkpage(i);
		if (rc != 0) {
			printf("%d checkpage failed %d\n", i, rc);
		} else {
			printf("%d checkpage pass\n", i);
		}
	}


	munmap(global_window_ptr, WINDOW);
	close(fd);

	return 0;
}
