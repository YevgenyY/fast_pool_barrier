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
	uint64_t count;	/* barrier threshold */
	uint64_t total; /* total threads waiting barrier */
	pthread_spinlock_t sl;
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

static inline uint64_t
atomic_cmp_set(uint64_t *lock, uint64_t old, uint64_t set)
{
	u_char  res;

	__asm__ volatile (
 
	"    lock;"
	"    cmpxchgq  %3, %1;   "
	"    sete      %0;       "
	
	: "=a" (res) : "m" (*lock), "a" (old), "r" (set) : "cc", "memory");

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

static inline uint64_t
atomic_fetch_add(uint64_t *value, uint64_t add)
{
    __asm__ volatile (

	"    lock;"
    "    xaddq  %0, %1;   "

    : "+r" (add) : "m" (*value) : "cc", "memory");

    return add;
}

/* Atomic 64 bit exchange 
 *
*/
static inline uint64_t xchg_64(void *ptr, uint64_t x)
{
	__asm__ __volatile__("lock;"
		   		"xchgq %0,%1"
				:"=r" ((uint64_t) x)
				:"m" (*(volatile uint64_t *)ptr), "0" (x)
				:"memory");

	return x;
}
