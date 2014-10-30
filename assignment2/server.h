#ifndef	__server_h
#define	__server_h

#include	"unp.h"
#include	"utils.h"
#include	<net/if.h>

/* struct for Sender Window Element	*/
typedef struct SenderWinElem  {
	packet	    pack;					/*  Data Packet			    */
	int	    isSent;					/*  Sent Flag of Data Packet	    */
} sendWinElem;

/* struct for receiver Queue		*/
typedef struct SenderQueue	{
	sendWinElem	*elem;					/*  Sender Buffer		    */
	int		winsize;				/*  Queue Size			    */
	int		slidwinsize;				/*  Sliding Window Size		    */
	int		slidwinstart;				/*  Sliding Window Start	    */
	int		sentstartidx;				/*  Oldest Packet sent to output    */
} sendQ;

#endif	/* __server_h */


