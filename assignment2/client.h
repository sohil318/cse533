#ifndef	__client_h
#define	__client_h

#include	"unp.h"
#include	"utils.h"
#include	<net/if.h>

/* struct for Receiver Window Element	*/
typedef struct RecvWinElem  {
	packet	    pack;					/*  Data Packet			    */
	int	    isValid;					/*  Check if Packet is present	    */
} recvWinElem;

/* struct for receiver Queue		*/
typedef struct ReceiverQueue	{
	recvWinElem	*elem;					/*  Receiver Buffer		    */
	int		winsize;				/*  Queue Size			    */
	int		adwinsize;				/*  Advertising Window Size	    */
	int		adwinstart;				/*  Advertising Window Start	    */
	int		readpacketidx;				/*  Oldest Packet to read to output */
} recvQ;

#endif	/* __client_h */
