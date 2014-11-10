#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>


typedef struct barrier_t barrier_t;
struct barrier_t
{
	volatile uint32_t count;	/* barrier threshold */
	uint32_t total; /* total threads waiting barrier */
	uint32_t lock;

	uint32_t go_worker;
	uint32_t go_master;

	//pthread_spinlock_t sl;
};

/* Creates a compiler level memory barrier forcing 
 * optimizer to not re-order memory accesses across 
 * the barrier. 
 */
#define barrier() asm volatile("": : :"memory")

/* Force a read of variable */
#define atomic_read(V) (*(volatile typeof(V) *)&(V))

/*
 * "cmpxchgq  r, [m]":
 * 
 *     if (rax == [m]) {
 *         zf = 1;
 *         [m] = r;
 *     } else { 
 *         zf = 0;
 *         rax = [m];
 *     }
 *
 * Compare EDX:EAX register to 64-bit memory location. 
 * If equal, set the zero flag (ZF) to 1 and copy the 
 * ECX:EBX register to the memory location. Otherwise, 
 * copy the memory location to EDX:EAX and clear the zero flag)
 *
 * The "r" is any register, %rax (%r0) - %r16.
 * The "=a" and "a" are the %rax register.
 * Although we can return result in any register, we use "a" because it is
 * used in cmpxchgq anyway.  The result is actually in %al but not in $rax,
 * however as the code is inlined gcc can test %al as well as %rax.
 *
 * The "cc" means that flags were changed.
 */

static inline uint32_t
atomic_cmp_set(volatile uint32_t *l, uint32_t old, uint32_t set)
{
	u_char  res;

	__asm__ volatile (
 
	"lock; cmpxchgl  %3, %1;   "
	
	: "=a" (res) : "m" (*l), "a" (old), "r" (set) : "cc", "memory");

	return res;
}

/*
 * "xaddq  r, [m]":
 *
 *     temp = [m];
 *     [m] += r;
 *     r = temp;
 *
 *
 * The "+r" is any register, %rax (%r0) - %r16.
 * The "cc" means that flags were changed.
 */

static inline uint32_t
atomic_fetch_add(volatile uint32_t *value, uint32_t add)
{
    __asm__ volatile (

    "lock; xaddl  %0, %1;   "

    : "+r" (add) : "m" (*value) : "cc", "memory");

    return (*value);
}

/* Atomic 64 bit exchange 
 *
*/
static inline uint32_t xchg_32(uint32_t *ptr, uint32_t x)
{
	__asm__ __volatile__("lock;"
		   		"xchgl %0,%1"
				:"=r" ((uint32_t) x)
				:"m" (*(volatile uint32_t *)ptr), "0" (x)
				:"memory");

	return x;
}

