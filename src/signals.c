
static char rcsid[] = "@(#)$Id: signals.c,v 4.1 90/04/28 22:44:10 syd Exp $";

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
 * $Log:	signals.c,v $
 * Revision 4.1  90/04/28  22:44:10  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This set of routines traps various signals and informs the
    user of the error, leaving the program in a nice, graceful
    manner.

**/

#include "headers.h"
#include <signal.h>

#ifdef	VOIDSIG
typedef	void	sighan_type;
#else
typedef	int	sighan_type;
#endif

extern int pipe_abort;		/* set to TRUE if receive SIGPIPE */

sighan_type
quit_signal()
{
	dprint(1, (debugfile, "\n\n** Received SIGQUIT **\n\n\n\n"));
	leave();
}

sighan_type
hup_signal()
{
	dprint(1, (debugfile, "\n\n** Received SIGHUP **\n\n\n\n"));
	leave();
}

sighan_type
term_signal()
{
	dprint(1, (debugfile, "\n\n** Received SIGTERM **\n\n\n\n"));
	leave();
}

sighan_type
ill_signal()
{
	dprint(1, (debugfile, "\n\n** Received SIGILL **\n\n\n\n"));
	PutLine0(LINES, 0, "\n\nIllegal Instruction signal!\n\n");
	emergency_exit();
}

sighan_type
fpe_signal()
{
	dprint(1, (debugfile, "\n\n** Received SIGFPE **\n\n\n\n"));
	PutLine0(LINES, 0,"\n\nFloating Point Exception signal!\n\n");
	emergency_exit();
}

sighan_type
bus_signal()
{
	dprint(1, (debugfile, "\n\n** Received SIGBUS **\n\n\n\n"));
	PutLine0(LINES, 0,"\n\nBus Error signal!\n\n");
	emergency_exit();
}

sighan_type
segv_signal()
{
	dprint(1, (debugfile,"\n\n** Received SIGSEGV **\n\n\n\n"));
	PutLine0(LINES, 0,"\n\nSegment Violation signal!\n\n");
	emergency_exit();
}

sighan_type
alarm_signal()
{
	/** silently process alarm signal for timeouts... **/
#ifdef OS2
	signal(SIGALRM, SIG_ACK);
	signal(SIGALRM, alarm_signal);
#endif
#if defined(BSD) || defined(OS2)
	if (InGetPrompt)
		longjmp(GetPromptBuf, 1);
#else
	signal(SIGALRM, alarm_signal);
#endif
}

sighan_type
pipe_signal()
{
	/** silently process pipe signal... **/
	dprint(2, (debugfile, "*** received SIGPIPE ***\n\n"));

	pipe_abort = TRUE;	/* internal signal ... wheeee!  */
#ifndef OS2
	signal(SIGPIPE, SIG_ACK);
	signal(SIGPIPE, pipe_signal);
#endif
}

#ifdef SIGTSTP
int was_in_raw_state;

sighan_type
sig_user_stop()
{
	/* This is called when the user presses a ^Z to stop the
	   process within BSD
	*/
	if (signal(SIGTSTP, SIG_DFL) != SIG_DFL)
	  signal(SIGTSTP, SIG_DFL);

	was_in_raw_state = RawState();
	Raw(OFF);	/* turn it off regardless */

	printf("\n\nStopped.  Use \"fg\" to return to ELM\n\n");

	kill(0, SIGSTOP);
}

sighan_type
sig_return_from_user_stop()
{
	/** this is called when returning from a ^Z stop **/

	if (signal(SIGTSTP, sig_user_stop) == SIG_DFL)
	  signal(SIGTSTP, sig_user_stop);

	printf(
	 "\nBack in ELM. (You might need to explicitly request a redraw.)\n\n");

	if (was_in_raw_state)
	  Raw(ON);

#ifdef	BSD
	if (InGetPrompt)
		longjmp(GetPromptBuf, 1);
#endif
}
#endif
