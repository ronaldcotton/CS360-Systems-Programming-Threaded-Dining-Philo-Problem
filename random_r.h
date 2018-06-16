
/* successive calls to randomGaussian produce integer return values */
/* having a gaussian distribution with the given mean and standard  */
/* deviation.  Return values may be negative.                       */
/* This routine is re-entrant and thread safe, but requires the     */
/* caller to pass a pointer to an unsigned integer to be used as    */
/* state for the pseudo-random sequence.                            */

#ifndef RANDOM_R_H
#define RANDOM_R_H

int randomGaussian_r(int mean, int stddev, unsigned int* state);

#endif /* RANDOM_H */
