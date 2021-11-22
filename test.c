#include <elf.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#define AARCH64_DEBUG_ARCH(x) (((x) >> 8) & 0xff)
#define AARCH64_DEBUG_NUM_SLOTS(x) ((x)&0xff)
#define AARCH64_HWP_MAX_NUM 16
#define AARCH64_HBP_MAX_NUM 16

/*通过 aarch64_linux_get_debug_reg_capacity来获取
 hw watchpoint(aarch64_num_wp_regs) 
 hw breakpoint(aarch64_num_bp_regs)
 在我们平台上这两个值分别是
 aarch64_num_wp_regs = 4
 aarch64_num_bp_regs = 8 
 最新的gdb 7.12中支持的debug arch更全面，我们目前这个值是0x6
/* Macro for the expected version of the ARMv8-A debug architecture.  */
#define AARCH64_DEBUG_ARCH_V8 0x6
#define AARCH64_DEBUG_ARCH_V8_1 0x7
#define AARCH64_DEBUG_ARCH_V8_2 0x8

/* Get the hardware debug register capacity information from the
   process represented by TID.  */

void aarch64_linux_get_debug_reg_capacity(int tid)
{
    struct iovec iov;
    struct user_hwdebug_state dreg_state;
    int aarch64_num_wp_regs;
    int aarch64_num_bp_regs;

    iov.iov_base = &dreg_state;
    iov.iov_len = sizeof(dreg_state);
    printf("aaa\n");

    /* Get hardware watchpoint register info.  */
    if (ptrace(PTRACE_GETREGSET, tid, NT_ARM_HW_WATCH, &iov) == 0 && AARCH64_DEBUG_ARCH(dreg_state.dbg_info) == AARCH64_DEBUG_ARCH_V8)
    {
        aarch64_num_wp_regs = AARCH64_DEBUG_NUM_SLOTS(dreg_state.dbg_info);
        if (aarch64_num_wp_regs > AARCH64_HWP_MAX_NUM)
        {
            printf("Unexpected number of hardware watchpoint registers"
                   " reported by ptrace, got %d, expected %d.\n",
                   aarch64_num_wp_regs, AARCH64_HWP_MAX_NUM);
            aarch64_num_wp_regs = AARCH64_HWP_MAX_NUM;
        }
        printf("aaa %d\n", aarch64_num_wp_regs);
    }
    else
    {
        printf(("Unable to determine the number of hardware watchpoints"
                " available.\n"));
        aarch64_num_wp_regs = 0;
    }
    printf("bbb\n");

    /* Get hardware breakpoint register info.  */
    if (ptrace(PTRACE_GETREGSET, tid, NT_ARM_HW_BREAK, &iov) == 0 && AARCH64_DEBUG_ARCH(dreg_state.dbg_info) == AARCH64_DEBUG_ARCH_V8)
    {
        aarch64_num_bp_regs = AARCH64_DEBUG_NUM_SLOTS(dreg_state.dbg_info);
        if (aarch64_num_bp_regs > AARCH64_HBP_MAX_NUM)
        {
            printf(("Unexpected number of hardware breakpoint registers"
                    " reported by ptrace, got %d, expected %d.\n"),
                   aarch64_num_bp_regs, AARCH64_HBP_MAX_NUM);
            aarch64_num_bp_regs = AARCH64_HBP_MAX_NUM;
        }
        printf("aaa %d\n", aarch64_num_bp_regs);
    }
    else
    {
        printf(("Unable to determine the number of hardware breakpoints"
                " available.\n"));
        aarch64_num_bp_regs = 0;
    }
    printf("ccc\n");
}

int main()
{
    int status;

    // int tid = getpid();
    int tid = fork();
    if (tid == 0)
    {
        printf("111\n");
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) != 0)
        {
            perror("tracee, failed setting traceme\n");
            exit(1);
        }

        if (kill(getpid(), SIGSTOP) != 0)
        {
            perror("tracee, failed stopping tracee");
            exit(1);
        }
        while (1)
        {
            printf(".");
            sleep(1);
        }
    }
    else
    {
        printf("222\n");
        int ret = waitpid(tid, &status, 0);
        printf("333\n");
        if (ret == -1)
        {
            perror("tracer, failed waiting tracee");
            exit(1);
        }
        printf("444\n");

        // kill tracee on exit
        if (ptrace(PTRACE_SETOPTIONS, tid,
                   NULL, (void *)PTRACE_O_TRACEEXIT) != 0)
        {
            perror("tracer, faile setting PTRACE_O_EXITKILL for tracee\n");
            exit(1);
        }
        printf("555\n");
        aarch64_linux_get_debug_reg_capacity(tid);
    }

    return 0;
}
