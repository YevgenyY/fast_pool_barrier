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

	while (b->count != 0)
	{
		/* Wait until everyone exits the barrier */
		usleep(1000);
	}

	pthread_spin_destroy(&b->sl);

	//D("Barrier destroyed");

	return 0;
}

int barrier_init(barrier_t *b, uint16_t total)
{
	/* Init spinlocks */
	if(pthread_spin_init(&b->sl, PTHREAD_PROCESS_PRIVATE))
	{
		perror("spinlock_init failed");
		exit(-1);
	}

	b->count = 0;
	b->total = total;

	D("Barrier created");

	return 0;
}

#define MY_IDS id==0 || id==1
int barrier_wait(uint16_t id, barrier_t *b)
{
	uint16_t ret;
	uint64_t tries;

	tries = 0;

	if (MY_IDS) D("!!!Thread: %d enter the barrier, count: %lu", id, b->count);

	while(1)
	{
		uint64_t num;
	    atomic_fetch_add(&b->count, 1);

		num = atomic_read(b->count);

		D("thread: %d, num: %lu, count: %lu", id, num, b->count);

		if (b->count < b->total)
		{
			while (atomic_read(b->count) == num)
			{
				/* spin here */
				if(tries++ < 1000000)
					continue;

				//if (MY_IDS) D("!!!Thread: %d make usleep, count: %lu", id, b->count);

				/* Wait until another threads enter the barrier */
				usleep(1);
				tries = 0;
			}

			if (MY_IDS) D("!!!Thread: %d leave the barrier, count: %lu", id, b->count);
			ret = 0;
			break;
		}

		if(b->count == b->total)
		{
			barrier();
			b->count = 0;
			barrier();

			if (MY_IDS) D("!!!Thread: %d leave and reset the barrier, count: %lu", id, b->count);
			break;
		}
	}
	return ret;
}

