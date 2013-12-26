
/* $Id: curses.h,v 4.1 90/04/28 22:42:05 syd Exp $ */

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
 * $Log:	curses.h,v $
 * Revision 4.1  90/04/28  22:42:05  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

     /*** Include file for seperate compilation.  ***/

#define OFF		0
#define ON 		1

int  InitScreen(),      /* This must be called before anything else!! */

     ClearScreen(), 	 CleartoEOLN(),

     MoveCursor(),

     StartBold(),        EndBold(),
     StartUnderline(),   EndUnderline(),
     StartHalfbright(),  EndHalfbright(),
     StartInverse(),     EndInverse(),

     transmit_functions(),

     Raw(),              RawState(),
     ReadCh();

char *return_value_of();
