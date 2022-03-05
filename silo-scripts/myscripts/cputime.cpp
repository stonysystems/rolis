#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define handle_error(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

#define handle_error_en(en, msg) \
    do                           \
    {                            \
        errno = en;              \
        perror(msg);             \
        exit(EXIT_FAILURE);      \
    } while (0)


static void *
thread_start_01(void *arg)
{
    printf("Subthread starting thread_start_01\n");
    for (;;) {
        continue;       
    }
    printf("Subthread ending thread_start_01\n");
}

static void *
thread_start(void *arg)
{
    printf("Subthread starting infinite loop\n");
    pthread_t thread;
    pthread_create(&thread, NULL, thread_start_01, NULL);
    for (;;) {
        usleep(10 * 1000); // wouldn't count towards cpu-time
        continue;       
    }
}

static void
pclock(char *msg, clockid_t cid)
{
    struct timespec ts;

    printf("%s", msg);
    if (clock_gettime(cid, &ts) == -1)
        handle_error("clock_gettime");
    printf("%4jd.%06ld\n", (intmax_t)ts.tv_sec, ts.tv_nsec / 1000000);
}

int main(int argc, char *argv[])
{
    pthread_t thread;
    
    int s;

    s = pthread_create(&thread, NULL, thread_start, NULL);
    if (s != 0)
        handle_error_en(s, "pthread_create");

    printf("Main thread sleeping\n");
    sleep(1);

    printf("Main thread consuming some CPU time...\n");
    for (int j = 0; j < 2000000; j++)
        getppid();

    pclock("Process total CPU time: ", CLOCK_PROCESS_CPUTIME_ID);

    clockid_t cid;
    s = pthread_getcpuclockid(pthread_self(), &cid);
    if (s != 0)
        handle_error_en(s, "pthread_getcpuclockid");
    pclock("Main thread CPU time:   ", cid);

    /* The preceding 4 lines of code could have been replaced by:
       pclock("Main thread CPU time:   ", CLOCK_THREAD_CPUTIME_ID); */

    s = pthread_getcpuclockid(thread, &cid);
    if (s != 0)
        handle_error_en(s, "pthread_getcpuclockid");
    pclock("Subthread CPU time: 1    ", cid);

    exit(EXIT_SUCCESS); /* Terminates both threads */
}
