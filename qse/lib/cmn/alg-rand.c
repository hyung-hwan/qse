#include <qse/cmn/alg.h>

/* Park-Miller "minimal standard" 31 bit 
 * pseudo-random number generator, implemented
 * with David G. Carta's optimisation: with
 * 32 bit math and wihtout division.
 */
qse_uint32_t qse_rand31 (qse_uint32_t seed)
{
	qse_uint32_t hi, lo;

	if (seed == 0) seed++;

	lo = 16807 * (seed & 0xFFFF);
	hi = 16807 * (seed >> 16);

	lo += (hi & 0x7FFF) << 16;
	lo += hi >> 15;

	if (lo > 0x7FFFFFFFul) lo -= 0x7FFFFFFFul;

	return lo;
}
