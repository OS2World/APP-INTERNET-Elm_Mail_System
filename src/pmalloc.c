
static char rcsid[] = "@(#)$Id: pmalloc.c,v 4.1 90/04/28 22:43:43 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1 $   $State: Exp $
 *
 * 			Copyright (c) 1986, 1987 Dave Taylor
 * 			Copyright (c) 1988, 1989, 1990 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log:	pmalloc.c,v $
 * Revision 4.1  90/04/28  22:43:43  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This routine contains a cheap permanent version of the malloc call to
    speed up the initial allocation of the weedout headers and the uuname
    data.

      This routine is originally from Jim Davis of HP Labs, with some
    mods by me.
**/

#include <stdio.h>
#include "defs.h"

/*VARARGS0*/

char *pmalloc(size)
int size;
{
	/** return the address of a specified block **/

	static char *our_block = NULL;
	static int   free_mem  = 0;

	char   *return_value, *malloc();

	/** if bigger than our threshold, just do the real thing! **/

	if (size > PMALLOC_THRESHOLD)
	   return(malloc(size));

	/** if bigger than available space, get more, tossing what's left **/

	if (size > free_mem) {
	  if ((our_block = malloc(PMALLOC_BUFFER_SIZE)) == NULL) {
	    fprintf(stderr, "\n\r\n\rCouldn't malloc %d bytes!!\n\r\n\r",
		    PMALLOC_BUFFER_SIZE);
	    leave();
          }
	  our_block += 4;  /* just for safety, don't give back true address */
	  free_mem = PMALLOC_BUFFER_SIZE-4;
	}

	return_value  = our_block;	/* get the memory */
	size = ((size+3)/4)*4;		/* Go to quad byte boundary */
	our_block += size;		/* use it up      */
	free_mem  -= size;		/*  and decrement */

	return( (char *) return_value);
}
