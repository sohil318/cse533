#ifndef	__server_h
#define	__server_h

#include	"unp.h"
#include	"utils.h"
#include	<net/if.h>

/* struct for Sender Window Element	*/
typedef struct SenderWinElem  {
	msg	    packet;					/*  DATA Packet 		    */
	int	    seqnum;					/*  Sequence Number of Packet	    */
	int	    retranx;					/*  Retransmission Count	    */
	int	    isPresent;					/*  Sent Flag of Data Packet	    */
} sendWinElem;

/* struct for receiver Queue		*/
typedef struct SenderQueue	{
	sendWinElem	*buffer;				/*  Sender Buffer		    */
	int		winsize;				/*  Queue Size			    */
	int		cwinsize;				/*  Sliding Window Size		    */
	int		slidwinstart;				/*  Sliding Window Start	    */
	int		slidwinend;				/*  Oldest Packet sent to output    */
} sendQ;

#endif	/* __server_h */


