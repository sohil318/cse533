/* include rtt1 */
#include	"unprtt.h"

int rtt_d_flag = 0;		/* debug flag; can be set by caller */

/*
 * Calculate the RTO value based on current estimators:
 *		smoothed RTT plus four times the deviation
 */
//#define	RTT_RTOCALC(ptr) ((ptr)->rtt_srtt + (4.0 * (ptr)->rtt_rttvar))
#define	RTT_RTOCALC(ptr) (((ptr)->rtt_srtt >> 3) + (ptr)->rtt_rttvar)

static uint32_t
rtt_minmax(uint32_t rto)
{
	if (rto < RTT_RXTMIN)
		rto = RTT_RXTMIN;
	else if (rto > RTT_RXTMAX)
		rto = RTT_RXTMAX;
	return(rto);
}

/* to start the timer for given milliseconds */
//void
//rtt_start_timer(long int ms){
//    struct itimerval timer;
//    /* Configure the timer to expire after 'ms' msec... */
//    timer.it_value.tv_sec = ms / 1000;
//    timer.it_value.tv_usec = (ms % 1000) * 1000;
//    /* and every 0 msec after that. */
//    timer.it_interval.tv_sec = 0;
//    timer.it_interval.tv_usec = 0;
//    /* start r */
//    setitimer (ITIMER_REAL, &timer, 0);
//}

void
rtt_init(struct rtt_info *ptr)
{
	struct timeval	tv;

	Gettimeofday(&tv, NULL);
	ptr->rtt_base = tv.tv_sec;		/* # sec since 1/1/1970 at start */

	ptr->rtt_rtt    = 0;
	ptr->rtt_srtt   = 0;
	ptr->rtt_rttvar = 3000;		    
	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
		/* first RTO at (srtt + (4 * rttvar)) = 3000 milliseconds */
}
/* end rtt1 */

/*
 * Return the current timestamp.
 * Our timestamps are 32-bit integers that count milliseconds since
 * rtt_init() was called.
 */

/* include rtt_ts */
uint32_t
rtt_ts(struct rtt_info *ptr)
{
	uint32_t		ts;
	struct timeval	tv;

	Gettimeofday(&tv, NULL);
	ts = ((tv.tv_sec - ptr->rtt_base) * 1000) + (tv.tv_usec / 1000);
	return(ts);
}

void
rtt_newpack(struct rtt_info *ptr)
{
	ptr->rtt_nrexmt = 0;
}

int
rtt_start(struct rtt_info *ptr)
{
	return ptr->rtt_rto;		/* round float to int */
		/* 4return value can be used as: alarm(rtt_start(&foo)) */
}
/* end rtt_ts */

/*
 * A response was received.
 * Stop the timer and update the appropriate values in the structure
 * based on this packet's RTT.  We calculate the RTT, then update the
 * estimators of the RTT and its mean deviation.
 * This function should be called right after turning off the
 * timer with alarm(0), or right after a timeout occurs.
 */

/* include rtt_stop */
#if 0
void
rtt_stop(struct rtt_info *ptr, uint32_t ms)
{
	uint32_t delta;

	ptr->rtt_rtt = ms;		/* measured rtt in milli seconds */

	/*
	 * update our estimators of rtt and mean deviation of rtt.
	 * see jacobson's sigcomm '88 paper, appendix a, for the details.
	 * we use floating point here for simplicity.
	*/

	delta = ptr->rtt_rtt - ptr->rtt_srtt;
	ptr->rtt_srtt += delta / 8;		/* g = 1/8 */

	if (delta < 0)
		delta = -delta;				/* |delta| */

	ptr->rtt_rttvar += (delta - ptr->rtt_rttvar) / 4;	/* h = 1/4 */

	ptr->rtt_rto = rtt_minmax(rtt_rtocalc(ptr));
}
#endif
/* end rtt_stop */

void
rtt_stop(struct rtt_info *ptr, uint32_t ms)
{
	uint32_t delta;

	ptr->rtt_rtt = ms;		/* measured rtt in milli seconds */

	/*
	 * update our estimators of rtt and mean deviation of rtt.
	 * see jacobson's sigcomm '88 paper, appendix a, for the details.
	 * we use floating point here for simplicity.
	*/
	ptr->rtt_rtt -= ptr->rtt_srtt << 3;
	ptr->rtt_srtt += ptr->rtt_rtt;		/* g = 1/8 */

	if (ptr->rtt_rtt < 0)
		ptr->rtt_rtt = -ptr->rtt_rtt;				/* |delta| */
	
	ptr->rtt_rtt -= ptr->rtt_rttvar >> 2;
	
	ptr->rtt_rttvar += ptr->rtt_rtt;	/* h = 1/4 */

	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}

/*
 * A timeout has occurred.
 * Return -1 if it's time to give up, else return 0.
 */

/* includle rtt_timeout */
int
rtt_timeout(struct rtt_info *ptr)
{
	ptr->rtt_rto = ptr->rtt_rto << 1;		/* next RTO */
        printf("%d", ptr->rtt_nrexmt);	
	ptr->rtt_rto = rtt_minmax(ptr->rtt_rto);
	
	if (++ptr->rtt_nrexmt > RTT_MAXNREXMT)
		return(-1);			/* time to give up for this packet */
	return(0);
}
/* end rtt_timeout */

/*
 * Print debugging information on stderr, if the "rtt_d_flag" is nonzero.
 */

void
rtt_debug(struct rtt_info *ptr)
{
	if (rtt_d_flag == 0)
		return;

	fprintf(stderr, "rtt = %.3d, srtt = %.3d, rttvar = %.3d, rto = %.3d\n",
			ptr->rtt_rtt, ptr->rtt_srtt, ptr->rtt_rttvar, ptr->rtt_rto);
	fflush(stderr);
}

