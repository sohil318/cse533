#ifndef	__client_h
#define	__client_h

#include	"unp.h"
#include	"utils.h"
#include	<net/if.h>

/* struct for Receiver Window Element	*/
typedef struct RecvWinElem  {
	msg	    packet;					/*  DATA Packet 		    */
	int	    seqnum;					/*  Sequence Number of Packet	    */
	int	    retranx;					/*  Retransmission Count	    */
	int	    isValid;					/*  Check if Packet is present	    */
} recvWinElem;

/* struct for receiver Queue		*/
typedef struct ReceiverQueue	{
	recvWinElem	*buffer;				/*  Receiver Buffer		    */
	int		winsize;				/*  Queue Size			    */
	int		advwinsize;				/*  Advertising Window Size	    */
	int		advwinstart;				/*  Advertising Window Start	    */
	int		readpacketidx;				/*  Oldest Packet to read to output */
} recvQ;

#endif	/* __client_h */
