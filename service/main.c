/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * Fluent Bit Dashboard Web Service
 * ================================
 *
 * This web service consumes the Fluent Bit statistics reported over
 * the Unix socket located at /tmp/fluentbit.sock .
 *
 * This code is based on Duda API 1 which is the stable but legacy
 * version, for hence it will only runs on Linux based systems.
 *
 * The goal is to use Duda API 2 which is under development, that
 * version runs on top of the new Monkey v1.6 stack and supports
 * different operating systems such as OSX, BSD and Linux.
 *
 * --
 * Eduardo Silva <eduardo@treasure-data.com>
 */

#include <stdio.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <string.h>

#include "webservice.h"

DUDA_REGISTER("Fluent Bit", "Dashboard");

#define FLB_SOCK    "/tmp/fluentbit.sock"

pthread_mutex_t st_mutex = PTHREAD_MUTEX_INITIALIZER;

struct flb_stats {
    int len;
    char *buf;
};

struct flb_stats cur_stats;
duda_global_t stats_report;

/* Connects to Fluent Bit socket and returns a file descriptor */
int fluentbit_connect(char *path)
{
    int fd;
    int ret;
    struct sockaddr_un addr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);

    ret = connect(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if (ret == -1) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}

int create_event(int efd, int fd, int init_mode, unsigned int behavior)
{
    int ret;
    struct epoll_event event = {0, {0}};

    event.data.fd = fd;
    event.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    if (behavior == MK_EPOLL_EDGE_TRIGGERED) {
        event.events |= EPOLLET;
    }

    switch (init_mode) {
    case MK_EPOLL_READ:
        event.events |= EPOLLIN;
        break;
    case MK_EPOLL_WRITE:
        event.events |= EPOLLOUT;
        break;
    case MK_EPOLL_RW:
        event.events |= EPOLLIN | EPOLLOUT;
        break;
    case MK_EPOLL_SLEEP:
        event.events = 0;
        break;
    }

    /* Add to epoll queue */
    ret = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
    if (mk_unlikely(ret < 0 && errno != EEXIST)) {
        perror("epoll_ctl");
        return ret;
    }

    return ret;
}

int handle_stats(int fd)
{
    ssize_t ret;
    char buf[32768]; /* 32KB buffer should be enough */

    ret = read(fd, buf, sizeof(buf));
    if (ret <= 0) {
        perror("read");
        return -1;
    }

    pthread_mutex_lock(&st_mutex);
    if (cur_stats.buf) {
        monkey->mem_free(cur_stats.buf);
    }

    /* Lock and assign a new buffer to the stats */
    cur_stats.buf = monkey->mem_alloc(ret + 1);
    memcpy(cur_stats.buf, &buf, ret);
    cur_stats.buf[ret] = '\0';
    cur_stats.len = ret;
    pthread_mutex_unlock(&st_mutex);

    return 0;
}

/* Worker to consume Fluent Bit statistics */
void *worker_stats_reader(void *data)
{
    int i;
    int fd;
    int ret;
    int evl;
    int n_events = 8;
    int n_fds;
    struct epoll_event *events = NULL;

    while (1) {
        /* Connect to the Fluent Bit server */
        fd = fluentbit_connect(FLB_SOCK);
        if (fd == -1) {
            /* If it fails, always try to reconnect */
            goto again;
        }

        /* Create the epoll(7) instance */
        evl = epoll_create(64);
        if (evl == -1) {
            perror("epoll_create");
            goto again;
        }

        /* Register the socket into the event loop */
        ret = create_event(evl, fd, MK_EPOLL_READ, MK_EPOLL_LEVEL_TRIGGERED);
        if (ret == -1) {
            goto again;
        }

        /* Allocate an array to get the events reported */
        events = monkey->mem_alloc(sizeof(struct epoll_event) * n_events);
        if (!events) {
            goto again;
        }

        /*
         * A while() inside a while() is very dirty, once we move to Duda API 2
         * we will use the right interfaces.
         */
        while (1) {
            n_fds = epoll_wait(evl, events, n_events, -1);
            for (i = 0; i < n_fds; i++) {
                /* If we have some data available (READ) */
                if (events[i].events & EPOLLIN) {
                    if (handle_stats(events[i].data.fd) == -1) {
                        goto again;
                    }
                }
                else {
                    /* Anything different than EPOLLIN is an error, just break */
                    goto again;
                }
            }
        }

    again:
        /*
         * Release resources and always delay, don't burn the CPU if
         * there are exceptions.
         */
        if (fd) {
            close(fd);
        }
        if (evl > 0) {
            close(evl);
        }
        if (events) {
            monkey->mem_free(events);
        }
        sleep(1);
    }
}

void cb_stats(duda_request_t *dr)
{
    response->http_status(dr, 200);

    pthread_mutex_lock(&st_mutex);
    if (cur_stats.buf) {
        response->printf(dr, "%s", cur_stats.buf);
    }
    pthread_mutex_unlock(&st_mutex);
    response->end(dr, NULL);
}

int duda_main()
{
    /* Reset stats */
    cur_stats.len = 0;
    cur_stats.buf = NULL;

    /* Spawn a worker that consumes the data from the Fluent Bit socket */
    worker->spawn(worker_stats_reader, NULL);

    map->static_add("/stats", "cb_stats");
    return 0;
}
