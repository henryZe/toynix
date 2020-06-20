#include <lib.h>

sighandler_t signal(int signum, sighandler_t handler)
{
    return handler;
}

int kill(envid_t env, int sig)
{
    return 0;
}

int sigprocmask(enum sigmask_flag how, const sigset_t *set, sigset_t *oldset)
{
    return 0;
}

int sigwait(const sigset_t *set, int *sig)
{
    return 0;
}
