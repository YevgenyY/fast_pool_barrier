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
	pthread_spin_lock(&b->sl);

	if (b->total != 0)
		b->total--;

	pthread_spin_unlock(&b->sl);

	while (b->total != 0)
	{
		/* Wait until everyone exits the barrier */
		usleep(1000);
	}

	pthread_spin_destroy(&b->sl);

	//D("Barrier destroyed");

	return 0;
}

int barrier_init(barrier_t *b, uint16_t count)
{
	/* Init spinlocks */
	if(pthread_spin_init(&b->sl, PTHREAD_PROCESS_PRIVATE))
	{
		perror("spinlock_init failed");
		exit(-1);
	}

	b->count = count;
	b->total = 0;

	D("Barrier created");

	return 0;
}

#define MY_IDS id==0 || id==1
int barrier_wait(uint16_t id, barrier_t *b)
{
	uint64_t tries;

	tries = 0;

	if (MY_IDS) D("!!!Thread: %d enter the barrier, total: %lu", id, b->total);
	//pthread_spin_lock(&b->sl);
	//b->total++; 
	//pthread_spin_unlock(&b->sl);
	atomic_fetch_add(&b->total, 1);


	if (b->total == b->count)
	{
		if (MY_IDS) D("!!!Thread: %d leave the barrier, total: %lu", id, b->total);
		return 0;
	}
	else
	{
		while (b->total < b->count)
		{
			if (tries++ < 1000000) // make 1000 spins
				continue;

			/* Wait until enough threads enter the barrier */
			if (MY_IDS) D("!!!Thread: %d make usleep, total: %lu", id, b->total);
			usleep(1);
			tries = 0;
		}
		if (MY_IDS) D("!!!Thread: %d leave and reset the barrier, total: %lu", id, b->total);

		/* reset counter and leave barrier */
		//pthread_spin_lock(&b->sl);
		//b->total = 0;
		//pthread_spin_unlock(&b->sl);
		//atomic_cmp_set(b->total, 2, 0);
		xchg_64(&b->total, 0);

		return 0;
	}
}

