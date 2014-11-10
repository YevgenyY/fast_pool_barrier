/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  pool_barrier_test
 *
 *        Version:  1.0
 *        Created:  11/05/14 20:30:00
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Yevgeny (a007), a007@ixi.ru
 *        Company:  IXI Labs
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <pthread.h> 
#include <unistd.h>
#include <signal.h>
#include "pool_barrier.h"

#define D(format, ...) do {                                 \
	struct timeval _xxts;                                   \
	gettimeofday(&_xxts, NULL);                             \
	fprintf(stderr, "%03d.%06d %s [%d] " format "\n",       \
	(int)_xxts.tv_sec %1000, (int)_xxts.tv_usec,            \
	__FUNCTION__, __LINE__, ##__VA_ARGS__);                 \
} while (0)

#define MAX_THREAD 2

int barrier_destroy(barrier_t *b);
int barrier_init(barrier_t *b, uint16_t count);
int barrier_wait(uint16_t id, barrier_t *b);

struct targ {
	uint16_t id;		/* thread number in the struct targ targs[num] array */
	uint8_t used;
	barrier_t *b;
	uint32_t lock;

	pthread_t thread;	/* thread descriptor for pthread */
};

struct targ targs[MAX_THREAD];
#define BARRIER_FLAG (1UL<<31)
#define EBUSY 1

/*
 * pool_spinlock
 *
*/
static inline void pool_spinlock(uint16_t id, volatile uint32_t *lock)
{
	while(1)
	{
		if (!xchg_32(lock, EBUSY)) return;

		while(*lock) usleep(1);
	}

	return;
}

static inline void pool_unlock(uint16_t id, volatile uint32_t *lock)
{
	barrier();
	*lock = 0;
}


static void *
rx_thread(void *data)
{
	struct targ *targ = (struct targ *)data;
	barrier_t *b;
	uint32_t ret;

#if 0 
	if(targ->id)
		return;

	b = targ->b;
	b->count = 1;
	ret = 0;

	ret = atomic_cmp_set(&b->count, 0, 1);
	pthread_exit(NULL);	

#if 0
	ret =  xchg_32(&b->count, 0);
	pthread_exit(NULL);	
#endif	
	// DEBUG
	
#endif
	

	while(targ->used)
	{	
		uint32_t i;

		barrier_wait(targ->id, targ->b);
		D("Thread: %d barrier 0 passed", targ->id);

		//for (i=0; i<random(); i++);

		//if (targ->id)
		//	usleep(1000000);


		barrier_wait(targ->id, targ->b);
		D("Thread: %d barrier 1 passed", targ->id);

		//usleep(10);
	}

	pthread_exit(NULL);	
}

static void
sigint_h(__unused int sig)
{
	int i;
	for (i = 0; i < MAX_THREAD; i++) {
		/* cancel active threads. */
		printf("Cancelling thread #%d\n", i);
		//pthread_cancel(targs[i].thread);
		targs[i].used = 0;
	}

	//do_exit = 1;

	signal(SIGINT, SIG_DFL);
}

int
main(int argc, char **argv)
{
	int i;
	barrier_t *rx_barrier;

	D("Start");

	argv = argv;
	argc = argc;

	rx_barrier = malloc(sizeof(struct barrier_t));

	barrier_init(rx_barrier, 2);

	signal(SIGINT, sigint_h); 
	
	for (i=0; i < MAX_THREAD; i++)
	{
		targs[i].id = i;
		targs[i].used = 1;
		targs[i].b = rx_barrier;
		targs[i].lock = 0;

		if (pthread_create(&targs[i].thread, NULL, rx_thread, (void *)&targs[i]) == -1) {
			D("Unable to create thread %d", i);
		}
	}

	for (i=0; i < MAX_THREAD; i++)
	{
		if (pthread_join(targs[i].thread, NULL) == -1) {
			D("Unable to join thread %d", i);
		}
		D("Thread %d joined", i);
	}

	barrier_destroy(rx_barrier);

	free(rx_barrier);

	return 0;
}
