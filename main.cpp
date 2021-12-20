#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/prctl.h>        


int rcv_sym = 0;
int num_bit = 128;
pid_t pid;

void child_dead_handler(int signo);
void confirm_handler(int signo);
void one_handler(int signo);
void zero_handler(int signo);
void daddy_dead_handler(int signo);


int main(int argc, char ** argv){
    if (argc > 2) {
        fprintf(stderr, "Incorrect input");
        exit(EXIT_FAILURE);
    }
    if (argc < 2) {
        fprintf(stderr, "Incorrect input");
        exit(EXIT_FAILURE);
    }
    pid_t ppid = getpid();

    sigset_t set;

    if (sigaddset(&set, SIGUSR1) < 0) {
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
    if (sigaddset(&set, SIGUSR2) < 0) {
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
    if (sigaddset(&set, SIGCHLD) < 0) {
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
    if (sigaddset(&set, SIGALRM) < 0) {
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
    if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

//    if (sigemptyset(&set) < 0) {
//        perror("sigemptyset");
//        exit(EXIT_FAILURE);
//    }


    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {


        if (prctl(PR_SET_PDEATHSIG, SIGKILL) < 0) {
            perror("prctl()");
            exit(EXIT_FAILURE);
        }

        if (ppid != getppid()) {
            fprintf(stderr, "Daddy DEAD!\n");
            exit(EXIT_FAILURE);
        }


        if (sigfillset(&set) < 0) {
            perror("sigfillset");
            exit(EXIT_FAILURE);
        }
        sigdelset(&set, SIGUSR1);

        struct sigaction confirm;
        confirm.sa_handler = confirm_handler;
        confirm.sa_flags = ~SA_RESETHAND;
        if (sigfillset(&confirm.sa_mask) < 0) {
            perror("sigfillset");
            exit(EXIT_FAILURE);
        }
        if (sigaction(SIGUSR1, &confirm, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        int fd_file = open(argv[1], O_RDONLY);
        if (fd_file < 0 ){
            perror("open()");
            exit(EXIT_FAILURE);
        }

        char sym = 0;
        while (1){
            int len = read(fd_file, &sym, 1);
            if (len < 0) {
                perror("read()");
                exit(EXIT_FAILURE);
            }
            if (len == 0) {
                break;
            }
            for (int i = 128; i >= 1; i /= 2){


                if (i & sym ) {
                    if (kill(ppid, SIGUSR1) < 0) {
                        perror("kill");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    if (kill(ppid, SIGUSR2) < 0) {
                        perror("kill");
                        exit(EXIT_FAILURE);
                    }
                }
/*(5*)End Child-Parent for parent's handlers*/
                sigsuspend(&set);
/*(1*)End Child-Parent for parent's PMS(in Child)*/
            }
        }
        fprintf(stderr, "I want DEI\n");
        exit(EXIT_SUCCESS);
    }

    struct sigaction childDead;
    childDead.sa_handler = child_dead_handler;
    childDead.sa_flags = SA_NOCLDWAIT | ~SA_RESETHAND;

    if (sigfillset(&childDead.sa_mask) < 0) {
        perror("sigfillset");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGCHLD, &childDead, NULL)) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct sigaction rcv_one;
    rcv_one.sa_handler = one_handler;
    rcv_one.sa_flags = ~SA_RESETHAND;
    if (sigfillset(&rcv_one.sa_mask) < 0) {
        perror("sigfillset");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR1, &rcv_one, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct sigaction rcv_zero;
    rcv_zero.sa_handler = zero_handler;
    rcv_zero.sa_flags = ~SA_RESETHAND;
    if (sigfillset(&rcv_zero.sa_mask) < 0) {
        perror("sigfillset");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR2, &rcv_zero, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sigfillset(&set);
    sigdelset(&set, SIGUSR1);
    sigdelset(&set, SIGUSR2);
    sigdelset(&set, SIGCHLD);

    while(1) {

        if(num_bit == 0){
            printf("%c", rcv_sym);
            fflush(0);
            num_bit=128;
            rcv_sym = 0;
        }


        sigsuspend(&set);

        if (kill(pid, SIGUSR1) < 0) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}


void child_dead_handler(int signo) {
    fprintf(stderr, "Child dead!\n");
    exit(EXIT_SUCCESS);
}
/*Begin child's handler*/
void confirm_handler(int signo) {
    //fprintf(stderr, "Confirm\n");
}
/*End child's handler*/

/*(1)Begin Child-Parent for pending mask signals [214-227](in Parent) + Parent's handlers*/
/*Begin Parent's handlers*/
void one_handler(int signo) {
    rcv_sym += num_bit;
    num_bit /= 2;
    //fprintf(stderr, "rcv1\n");
}

void zero_handler(int signo) {
    num_bit/=2;
    //fprintf(stderr, "rcv0\n");
}