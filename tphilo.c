/* tphilo.c
 * Ron Cotton
 *
 * CS360 - Dick Lang, Ph.D.
 * Dining philosophers problem solved using pthreads
 * using globals instead of shared memory, with no semaphores for this problem
 * instead solved with pthread_t and pthread_mutex_t.
 */

/* includes */
#define _GNU_SOURCE			/* for sys/ipc.h */
#include <sys/types.h>	/* uses semget(), semop(), wait(), getppid() */
#include <sys/ipc.h>		/* uses semget(), semop() */
#include <pthread.h>    /* for pthread()  */
#include <errno.h>			/* for perror() */
#include <string.h>			/* string of error num - strerror() */
#include <stdio.h>			/* for fprintf(), printf() */
#include <stdlib.h>			/* for exit() */
#include <stdbool.h>		/* for boolean */
#include <math.h>			  /* for random function */
#include "random_r.h"			/* for randomGaussian() */
#include <unistd.h>			/* for sleep() */
#include <assert.h>			/* unit testing */

/* defines */
#define PHILO 5
#define CHOPSTICKS 5
#define MIN_TOTAL_EAT_TIME 100
#define MEAN_EAT 9
#define STDDEV_EAT 3
#define MEAN_THINK 11
#define STDDEV_THINK 7
#define EAT true
#define THINK false
#define WAIT 1000000

/* typedef */
typedef unsigned int uint;

/* function definitions */
int checkandpickupChopsticks(uint left);
/*void pickupChopsticks(int left);*/
void dropChopsticks(uint left);
int philoToE(uint philo, uint cycles, bool toe, int *valueTotal);
int gTime(uint philo, bool toe);
void *philoThread(void *philo);
void philoWait(uint philo);

/* globals */
//int chopsticks[CHOPSTICKS];
int pEatingTime[PHILO];					/* philo total times array */
int pThinkingTime[PHILO];
int pCycles[PHILO];
pthread_mutex_t c[CHOPSTICKS]; 	/* pthread mutex structure - chopsticks */
int pName[PHILO] = {0,1,2,3,4}; /* name for each philosopher */

int main(int argc, char *argv[]) {
	pthread_t p[PHILO]; 						/* pthread per philosopher */

	srandom((long)time(NULL));
	setbuf(stdout, (char *)NULL);

	for (uint i=0; i<PHILO; ++i)
		pEatingTime[i] = pThinkingTime[i] = 0;

	for (uint i=0; i<PHILO; ++i)
		if ( pthread_mutex_init(&c[i],NULL) == -1 ) perror(strerror(errno));

  for (uint i=0; i<PHILO; ++i) {
		printf("Philosopher %d sat at the table.\n", i);
		if ( pthread_create(&p[i], NULL, philoThread, pName + i) ) perror(strerror(errno));
	} /* end for i */

	for (uint i=0; i<PHILO; ++i)
		if ( pthread_join(p[i], NULL) ) perror(strerror(errno));

	printf("Philosophers required to eat for at least %d seconds.\n", MIN_TOTAL_EAT_TIME);

	for (uint i=0; i<PHILO; ++i) {
		pthread_mutex_destroy(&c[i]);
		printf("Philosopher %d final results: Eat Time = %d, Think Time = %d, Cycles = %d\n", i, pEatingTime[i], pThinkingTime[i], pCycles[i]);
	} /* end for i */

pthread_exit(NULL);
return (0);
} /* end main() */

void *philoThread(void *philo) {
	uint i = *( (uint *) philo );
	int eatTemp = 1;									/* initially the program eats */
	int thinkingTime = 0;							/* total thinking time */
	int eatingTime = 0;								/* total eating time */
	int cycles = 0;										/* the number of cycles that the program runs */

	printf("Philosopher %d starting.\n", i);
	do {
		if (eatTemp>0) philoToE(i, ++cycles, THINK, &thinkingTime);
		eatTemp = 0;
		if (checkandpickupChopsticks(i)) {
			eatTemp = philoToE(i, cycles, EAT, &eatingTime);
			dropChopsticks(i);
		} else {
			philoWait(i);
		}
	} while (eatingTime<MIN_TOTAL_EAT_TIME);
	pEatingTime[i] = eatingTime;     	/* can read and write this without mutex */
	pThinkingTime[i] = thinkingTime;	/* because each philosoher is touching his */
	pCycles[i] = cycles;							/* own global mem */
	printf("Philosopher %d left table.\n", i);
	return (NULL);
} /* end philoThread() */

/* int checkandpickupChopsticks(uint left)
 * attempts to try and lock the left and right chopstick, if unable to, then it
 * unlocks the chopstick that it's holding (if it has one) and exits.
 */
int checkandpickupChopsticks(uint left) {
	uint right = (left+1)%PHILO;
	if (!pthread_mutex_trylock(&c[left])) {
		/* lock worked for first chopstick */
		if (!pthread_mutex_trylock(&c[right])) {
			/* 2nd lock worked for second chopstick */
			printf("Philosopher %d picked up chopsticks %d and %d.\n", left, left, right);
			return 1;
		} else {
			pthread_mutex_unlock(&c[left]); /* if unable to get right, unlock left */
		}
	}
	return 0;
} /* end of checkandpickupChopsticks() */

/* void dropChopsticks(uint left)
 * removes the mutex lock from the left and right chopstick.
 */
void dropChopsticks(uint left) {
	uint right = (left+1)%PHILO;
	printf("Philosopher %d put down chopsticks %d and %d.\n", left, left, right);
	pthread_mutex_unlock(&c[left]);
	pthread_mutex_unlock(&c[right]);
} /* end dropChopsticks() */

/* int gTime(uint philo, bool toe)
 * returns the randomGaussian_r time depending on if the second argument is
 * true (eat) or false (sleep). only called by PhiloToE
 */
int gTime(uint philo, bool toe) {
	int r = (toe)?randomGaussian_r(MEAN_EAT, STDDEV_EAT, &philo):randomGaussian_r(MEAN_THINK, STDDEV_THINK, &philo);
	return (r<0)?0:r;
} /* end gTime() */

/* int philoToE(uint philo, uint cycles, bool toe, int *valueTotal)
 * Philo Think or Eat - this gets the time and usleeps the appropriate time.
 */
int philoToE(uint philo, uint cycles, bool toe, int *valueTotal) {
	int valueTemp = gTime(philo+cycles, toe);
	printf("Philosopher %d %s for %d seconds (total %s = %d)\n", philo, (toe)?"eating":"thinking", valueTemp, (toe)?"eating":"thinking", (*valueTotal)+valueTemp);
	(*valueTotal) += valueTemp;
	usleep(valueTemp*WAIT);			/* usleep is thread safe (10^6 usec (WAIT) = 1 sec) */
	return valueTemp;
} /* end philoToE() */

/* void philoWait(uint philo)
 * Philo Wait - Waits an amount of time depending of the mean and stddev of
 * eating.
 */
void philoWait(uint philo) {
	usleep(WAIT*abs((MEAN_EAT-STDDEV_EAT)/4));
	/* usleep is thread safe (10^6 usec (WAIT) = 1 sec) */
} /* end philoWait() */
