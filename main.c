#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>   /* For constants ORIG_EAX etc */

int main()
{
    pid_t child = fork();
    if(child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl("/bin/ls", "ls", NULL);
    } else {
        wait(NULL);

        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, child, NULL, &regs);
        printf("The child made a system call %ld\n", regs.orig_rax);

        struct user* user_space = (struct user*)0;
        long original_rax = ptrace(PTRACE_PEEKUSER, child, &user_space->regs.orig_rax, NULL);

        // long original_rax = ptrace(PTRACE_PEEKUSER, child, 8 * ORIG_RAX, NULL);
        // printf("The child made a system call %ld\n", original_rax);
        // ptrace(PTRACE_CONT, child, NULL, NULL);
    }
    return 0;
}

