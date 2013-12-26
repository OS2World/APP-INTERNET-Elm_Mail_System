
static char rcsid[] ="@(#)$Id: buffer.c,v 4.1 90/04/28 22:41:54 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1 $   $State: Exp $
 *
 * 			Copyright (c) 1986, 1987 Dave Taylor
 * 			Copyright (c) 1988, 1989, 1990 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein - elm@DSI.COM
 *			dsinc!elm
 *
 *******************************************************************************
 * $Log:	buffer.c,v $
 * Revision 4.1  90/04/28  22:41:54  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

#include <stdio.h>

char  _vbuf[5*BUFSIZ];		/* space for file buffering */
