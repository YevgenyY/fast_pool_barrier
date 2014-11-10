#include <sys/types.h>
#include <machine/atomic.h>

#include "pool_barrier.h"

#define D(format, ...) do {                                     \
        struct timeval _xxts;                                   \
        gettimeofday(&_xxts, NULL);                             \
        fprintf(stderr, "%03d.%06d %s [%d] " format "\n",       \
        (int)_xxts.tv_sec %1000, (int)_xxts.tv_usec,            \
        __FUNCTION__, __LINE__, ##__VA_ARGS__);                 \
        } while (0)
#define BARRIER_FLAG 1000
int barrier_destroy(barrier_t *b)
{
	if (b->count != 0)
		b->count--;

	while (b->count != 0)
	{
		/* Wait until everyone exits the barrier */
		usleep(1000);
	}

	D("Barrier destroyed");

	return 0;
}

int barrier_init(barrier_t *b, uint16_t total)
{
	b->count = 0;
	b->total = total;
	b->go_master = 1;
	b->go_worker = 1;

	D("Barrier created");

	return 0;
}

//D("Thread %d atomic_fetch_add returns: %p", id, atomic_fetch_add(&(b->count), 1));
//D("thread: %d, num: %lu, count: %lu", id, num, b->count);
//if (MY_IDS) D("Thread: %d leave and reset the barrier %p, count: %x", id, b,  b->count);
//if (MY_IDS) D("Thread: %d leave the barrier %p, count: %x", id, b, b->count);
#if 1 
#define MY_IDS id==7 || id==15
#define MAX_RING 1 
void barrier_wait(uint16_t id, barrier_t *b)
{
	uint32_t tries;
	uint32_t count, total;
	uint32_t *go_local;

	tries = 0;
	total = b->total;

	go_local = id < MAX_RING ? &b->go_master : &b->go_worker;

	// atomic_cmp_set returns 0 if b->count==0
	while (atomic_cmp_set(&b->lock, 0, 1) != 0)
		;

	b->count++;
	count = b->count;

	*go_local = count < total ? 0 : 1;

	b->lock = 0;

	while (1)
	{
		while (atomic_cmp_set(&b->lock, 0, 1) != 0)
			;
		if(!*go_local)
		{
			b->lock = 0;

			/* spin here */
			if(tries++ < 100000)
				continue;

			/* Wait until another threads enter the barrier */
			tries = 0;
		} else 
		/* second thread has entered the barrier */
		{
			b->go_master = 1;
			b->go_worker = 1;
			b->count = 0;

			b->lock = 0;
			return;
		}
	}
	D("A BUG!!! YOU MUST NOT BE HERE.");
}

#endif

#if 0 
void barrier_wait(uint16_t id, struct barrier_t *b) {
	uint32_t myCount = 0;
	uint32_t myTotal = 0;

	/* Get a lock on the barrier's shared data 
	 * atomic_cmp_set returns:
	 *  0 if lock == 0
	 *  1 if lock == 1
	 *  and set lock to 1
	 */
	while (atomic_cmp_set(&b->lock, 0, 1) != 0)
		;
	/* Keep a local copy of the limit (which will not change),
	 * add this thread to the number waiting, and release the lock */
	myTotal = b->total;

	b->count++;
	b->lock = 0;

	/* Now wait for other threads to reach the barrier */
	while (myCount < myTotal ) 
	{
		/* Lock the barrier data, copy the count, and release
		 * the lock. */
		while (atomic_cmp_set(&b->lock, 0, 1) != 0)
			;
		myCount = b->count;
		b->lock = 0;
	} 
}
#endif
