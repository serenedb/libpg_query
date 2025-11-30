/*-------------------------------------------------------------------------
 *
 * timestamp.c
 *	  Functions for the built-in SQL types "timestamp" and "interval".
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/timestamp.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <sys/time.h>

#include "access/xact.h"
#include "catalog/pg_type.h"
#include "common/int.h"
#include "funcapi.h"
#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "nodes/supportnodes.h"
#include "optimizer/optimizer.h"
#include "parser/scansup.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/float.h"
#include "utils/numeric.h"
#include "utils/sortsupport.h"

/*
 * gcc's -ffast-math switch breaks routines that expect exact results from
 * expressions like timeval / SECS_PER_HOUR, where timeval is double.
 */
#ifdef __FAST_MATH__
#error -ffast-math is known to break this code
#endif

#define SAMESIGN(a,b)	(((a) < 0) == ((b) < 0))

/* interval2itm()
 * Convert an Interval to a pg_itm structure.
 * Note: overflow is not possible, because the pg_itm fields are
 * wide enough for all possible conversion results.
 */
void
interval2itm(Interval span, struct pg_itm *itm)
{
	TimeOffset	time;
	TimeOffset	tfrac;

	itm->tm_year = span.month / MONTHS_PER_YEAR;
	itm->tm_mon = span.month % MONTHS_PER_YEAR;
	itm->tm_mday = span.day;
	time = span.time;

	tfrac = time / USECS_PER_HOUR;
	time -= tfrac * USECS_PER_HOUR;
	itm->tm_hour = tfrac;
	tfrac = time / USECS_PER_MINUTE;
	time -= tfrac * USECS_PER_MINUTE;
	itm->tm_min = (int) tfrac;
	tfrac = time / USECS_PER_SEC;
	time -= tfrac * USECS_PER_SEC;
	itm->tm_sec = (int) tfrac;
	itm->tm_usec = (int) time;
}

/* itm2interval()
 * Convert a pg_itm structure to an Interval.
 * Returns 0 if OK, -1 on overflow.
 *
 * This is for use in computations expected to produce finite results.  Any
 * inputs that lead to infinite results are treated as overflows.
 */
int
itm2interval(struct pg_itm *itm, Interval *span)
{
	int64		total_months = (int64) itm->tm_year * MONTHS_PER_YEAR + itm->tm_mon;

	if (total_months > INT_MAX || total_months < INT_MIN)
		return -1;
	span->month = (int32) total_months;
	span->day = itm->tm_mday;
	if (pg_mul_s64_overflow(itm->tm_hour, USECS_PER_HOUR,
							&span->time))
		return -1;
	/* tm_min, tm_sec are 32 bits, so intermediate products can't overflow */
	if (pg_add_s64_overflow(span->time, itm->tm_min * USECS_PER_MINUTE,
							&span->time))
		return -1;
	if (pg_add_s64_overflow(span->time, itm->tm_sec * USECS_PER_SEC,
							&span->time))
		return -1;
	if (pg_add_s64_overflow(span->time, itm->tm_usec,
							&span->time))
		return -1;
	if (INTERVAL_NOT_FINITE(span))
		return -1;
	return 0;
}

/* itmin2interval()
 * Convert a pg_itm_in structure to an Interval.
 * Returns 0 if OK, -1 on overflow.
 *
 * Note: if the result is infinite, it is not treated as an overflow.  This
 * avoids any dump/reload hazards from pre-17 databases that do not support
 * infinite intervals, but do allow finite intervals with all fields set to
 * INT_MIN/INT_MAX (outside the documented range).  Such intervals will be
 * silently converted to +/-infinity.  This may not be ideal, but seems
 * preferable to failure, and ought to be pretty unlikely in practice.
 */
int
itmin2interval(struct pg_itm_in *itm_in, Interval *span)
{
	int64		total_months = (int64) itm_in->tm_year * MONTHS_PER_YEAR + itm_in->tm_mon;

	if (total_months > INT_MAX || total_months < INT_MIN)
		return -1;
	span->month = (int32) total_months;
	span->day = itm_in->tm_mday;
	span->time = itm_in->tm_usec;
	return 0;
}
