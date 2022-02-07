#include<stdio.h>
#include<semaphore.h>
#include<fcntl.h>
#include<stdlib.h>
#include<time.h>
#include<errno.h>
#include "pppd.h"
#include "syncppp.h"

int syncppp(int nproc)
{
    int flags;
    int value;
    sem_t *block; 
    sem_t *count;
    struct timespec ts;

    if (nproc <= 1) {
        error("syncppp: number of pppd should be larger than 1");
        return -1;
    }

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        error("clock_gettime error");
        return -1;
    }
    ts.tv_sec += SYNCPPP_TIMEOUT;


    flags = O_RDWR | O_CREAT;
    block = sem_open(SEM_BLOCK_NAME, flags, 0644, 0);
    count = sem_open(SEM_COUNT_NAME, flags, 0644, 0);
    if (block == SEM_FAILED || count == SEM_FAILED) {
        error("syncppp: sem_open failed");
        return -1;
    }

    if (sem_post(count) < 0) {
        error("syncppp: sem_post failed");
        return -1;
    }
    if (sem_getvalue(count, &value) < 0) {
        error("syncppp: sem_getvalue failed");
        return -1;
    }
    info("%d pppd have arrived, waiting for the left %d", value, nproc-value);
    if (value >= nproc) {
        while (nproc-1 > 0) {
            if (sem_post(block) < 0) {
                error("syncppp: sem_post failed");
                return -1;
            }
            nproc--;
        }
    } else {
        if (sem_timedwait(block, &ts) < 0) {
            if (errno == ETIMEDOUT) {
                error("syncppp: sem_timewait time out");
            } else {
                error("syncppp: sem_timewait error");
            }
            return -1;
        }

    }

    sem_close(count);
    sem_close(block);

    sem_unlink(SEM_COUNT_NAME);
    sem_unlink(SEM_BLOCK_NAME);

    return 0;
}

