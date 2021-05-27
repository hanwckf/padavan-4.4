
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "eloop.h"


struct eloop_sock {
	int sock;
	void *eloop_data;
	void *user_data;
	void (*handler)(int sock, void *eloop_ctx, void *sock_ctx);
};

struct eloop_timeout {
	struct timeval time;
	void *eloop_data;
	void *user_data;
	void (*handler)(void *eloop_ctx, void *sock_ctx);
	struct eloop_timeout *next;
};

struct eloop_signal {
	int sig;
	void *user_data;
	void (*handler)(int sig, void *eloop_ctx, void *signal_ctx);
};

struct eloop_data {
	void *user_data;

	int max_sock, reader_count;
	struct eloop_sock *readers;

	struct eloop_timeout *timeout;

	int signal_count;
	struct eloop_signal *signals;

	int terminate;
    int reload;
};

static struct eloop_data eloop;


void eloop_init(void *user_data)
{
	memset(&eloop, 0, sizeof(eloop));
	eloop.user_data = user_data;
}


int eloop_register_read_sock(int sock, void (*handler)(int sock, void *eloop_ctx, void *sock_ctx),
                             void *eloop_data, void *user_data)
{
	struct eloop_sock *tmp;

	tmp = (struct eloop_sock *)	realloc(eloop.readers, (eloop.reader_count + 1) * sizeof(struct eloop_sock));
	if (tmp == NULL)
		return -1;

	tmp[eloop.reader_count].sock = sock;
	tmp[eloop.reader_count].eloop_data = eloop_data;
	tmp[eloop.reader_count].user_data = user_data;
	tmp[eloop.reader_count].handler = handler;
	eloop.reader_count++;
	eloop.readers = tmp;

	if (sock > eloop.max_sock)
		eloop.max_sock = sock;

	return 0;
}

int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   void (*handler)(void *eloop_ctx, void *timeout_ctx),
			   void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *tmp, *prev;

	timeout = (struct eloop_timeout *) malloc(sizeof(*timeout));
	if (timeout == NULL)
		return -1;

	gettimeofday(&timeout->time, NULL);
	timeout->time.tv_sec += secs;
	timeout->time.tv_usec += usecs;

	while (timeout->time.tv_usec >= 1000000)
    {
		timeout->time.tv_sec++;
		timeout->time.tv_usec -= 1000000;
	}
	timeout->eloop_data = eloop_data;
	timeout->user_data = user_data;
	timeout->handler = handler;
	timeout->next = NULL;

	if (eloop.timeout == NULL) {
		eloop.timeout = timeout;
		return 0;
	}

	prev = NULL;
	tmp = eloop.timeout;
	while (tmp != NULL)
    {
		if (timercmp(&timeout->time, &tmp->time, <))
			break;
		prev = tmp;
		tmp = tmp->next;
	}

	if (prev == NULL)
    {
		timeout->next = eloop.timeout;
		eloop.timeout = timeout;
	}
    else
    {
		timeout->next = prev->next;
		prev->next = timeout;
	}

	return 0;
}

int eloop_cancel_timeout(void (*handler)(void *eloop_ctx, void *sock_ctx),
			 void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *prev, *next;
	int removed = 0;

	prev = NULL;
	timeout = eloop.timeout;
	while (timeout != NULL)
    {
		next = timeout->next;

		if (timeout->handler == handler && (timeout->eloop_data == eloop_data || eloop_data == ELOOP_ALL_CTX) 
            && (timeout->user_data == user_data || user_data == ELOOP_ALL_CTX))
        {
			if (prev == NULL)
				eloop.timeout = next;
			else
				prev->next = next;
			free(timeout);
			removed++;
		} else
			prev = timeout;

		timeout = next;
	}

	return removed;
}

static void eloop_handle_signal(int sig)
{
	int i;
	for (i = 0; i < eloop.signal_count; i++)
    {
		if (eloop.signals[i].sig == sig)
        {
			eloop.signals[i].handler(eloop.signals[i].sig, eloop.user_data, eloop.signals[i].user_data);
			break;
		}
	}
}

int eloop_register_signal(int sig, void (*handler)(int sig, void *eloop_ctx, void *signal_ctx), void *user_data)
{
	struct eloop_signal *tmp;

	tmp = (struct eloop_signal *) realloc(eloop.signals, (eloop.signal_count + 1) *	sizeof(struct eloop_signal));
	if (tmp == NULL)
		return -1;

	tmp[eloop.signal_count].sig = sig;
	tmp[eloop.signal_count].user_data = user_data;
	tmp[eloop.signal_count].handler = handler;
	eloop.signal_count++;
	eloop.signals = tmp;
	signal(sig, eloop_handle_signal);

	return 0;
}

void eloop_run(void)
{
	fd_set rfds;
	int i, res;
	struct timeval tv, now;

	while (!eloop.terminate && (eloop.timeout || eloop.reader_count > 0))
    {
		if (eloop.timeout)
        {
            gettimeofday(&now, NULL);
			if (timercmp(&now, &eloop.timeout->time, >=))
				tv.tv_sec = tv.tv_usec = 0;
			else
				timersub(&eloop.timeout->time, &now, &tv);
		}

		FD_ZERO(&rfds);
		for (i = 0; i < eloop.reader_count; i++)
			FD_SET(eloop.readers[i].sock, &rfds);
		res = select(eloop.max_sock + 1, &rfds, NULL, NULL,
			     eloop.timeout ? &tv : NULL);
		if (res < 0 && errno != EINTR)
        {
			perror("select");
			return ;
		}

		/* check if some registered timeouts have occurred */
		if (eloop.timeout)
        {
			struct eloop_timeout *tmp;

			gettimeofday(&now, NULL);
			if (timercmp(&now, &eloop.timeout->time, >=))
            {
				tmp = eloop.timeout;
				eloop.timeout = eloop.timeout->next;
				tmp->handler(tmp->eloop_data, tmp->user_data);
				free(tmp);
			}

		}  
        
		if (res <= 0)
			continue;

		for (i = 0; i < eloop.reader_count; i++)
        {
			if (FD_ISSET(eloop.readers[i].sock, &rfds))
            {
				eloop.readers[i].handler(eloop.readers[i].sock,	eloop.readers[i].eloop_data, eloop.readers[i].user_data);
			}
		}
	}
}

void eloop_terminate(void)
{
	eloop.terminate = 1;
}

void eloop_reload(void)
{
	eloop.reload = 1;
}

void eloop_destroy(void)
{
	struct eloop_timeout *timeout, *prev;

	timeout = eloop.timeout;
	while (timeout != NULL)
    {
		prev = timeout;
		timeout = timeout->next;
		free(prev);
	}
	free(eloop.readers);
	free(eloop.signals);
}

int eloop_terminated(void)
{
	return eloop.terminate;
}
