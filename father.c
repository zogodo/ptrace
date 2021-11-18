
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#define DR_OFFSET(num) 	((void *) (& ((struct user *) 0)->u_debugreg[num]))

int set_hwbp(pid_t pid, void *addr)
{
    unsigned long dr_7 = 0;

    if (ptrace(PTRACE_POKEUSER, pid, DR_OFFSET(0), addr) != 0) {
        perror("tracer, faile to set DR_0\n");
        return 1;
    }

    dr_7 = dr_7 | 0x01;  // set dr_0 local
    dr_7 = dr_7 | 0x02;  // set dr_0 global
    // dr_7 = dr_7 & ((~0x0f) << 16);  // type exec, and len 0
    if (ptrace(PTRACE_POKEUSER, pid, DR_OFFSET(7), (void *) dr_7) != 0) {
        perror("tracer, faile to set DR_7\n");
        return 1;
    }

    return 0;
}

void run_tracee(char *tracee_path)
{
    char *basec, *bname;
    char *argv[2] = {};

    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) != 0) {
        perror("tracee, failed setting traceme\n");
        exit(1);
    }

    if (kill(getpid(), SIGSTOP) != 0) {
        perror("tracee, failed stopping tracee");
        exit(1);
    }

    basec = strdup(tracee_path);
    bname = basename(tracee_path);
    argv[0] = bname;	
    argv[1] = NULL;

    execv(tracee_path, argv);
}

int main(int argc, char **argv)
{
    int status, ret;
    pid_t tracee_pid;
    unsigned long addr = 0x400526;
    int brk_cnt = 0;

    if (argc < 3) {
        fprintf(stderr, "tracer, no program specified!\n");
        return 1;
    }

    addr = strtol(argv[2], NULL, 16);  // addr is a hex string

    printf("tracer, tracing program: %s\n", argv[1]);

    tracee_pid = fork();
    if (tracee_pid == 0) {  // within tracee
        run_tracee(argv[1]);
        fprintf(stderr, "tracee, we shoudl never reach here!\n");
        exit(1);
    } else if (tracee_pid < 0) {
        perror("tracer, failed forking tracee process\n");
        exit(1);
    }

    // within tracer
    ret = waitpid(tracee_pid, &status, 0);
    if (ret == -1) {
        perror("tracer, failed waiting tracee");
        exit(1);
    }

    // kill tracee on exit
    if (ptrace(PTRACE_SETOPTIONS, tracee_pid,
	    NULL, (void *) PTRACE_O_TRACEEXIT) != 0) {
        perror("tracer, faile setting PTRACE_O_EXITKILL for tracee\n");
        exit(1);
    }

    // // hwbp works only after execv is executed, otherwise
    // // the debug registers will be rest after execv
    // if (set_hwbp(tracee_pid, (void *) addr) != 0) {
    //     fprintf(stderr, "faield to set harware break point at 0x%lx\n", addr);
    // }

    if (ptrace(PTRACE_CONT, tracee_pid, NULL, NULL) != 0) {
        perror("tracer, resume tracee\n");
        exit(1);
    }

    ret = waitpid(tracee_pid, &status, 0);
    if (ret == -1) {
        perror("tracer, failed waiting tracee");
        exit(1);
    }

    // ok, now we are safe to set hardware break point here
    if (set_hwbp(tracee_pid, (void *) addr) != 0) {
        fprintf(stderr, "faield to set harware break point at 0x%lx\n", addr);
    }

    while(1) {
        if (ptrace(PTRACE_CONT, tracee_pid, NULL, NULL) != 0) {
            perror("tracer, resume tracee\n");
            exit(1);
        }

        printf("break count: %d\n", ++brk_cnt);

        ret = waitpid(tracee_pid, &status, 0);
        if (ret == -1) {
            perror("tracer, failed waiting tracee");
            exit(1);
        }

        if (WIFEXITED(status)) {
            printf("tracer, tracee exited!\n");
            break;
        }
    }

    return 0;
}
