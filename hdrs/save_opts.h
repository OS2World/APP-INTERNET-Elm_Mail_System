
/* @(#)$Id: save_opts.h,v 4.1 90/04/28 22:42:12 syd Exp $ */

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
 * $Log:	save_opts.h,v $
 * Revision 4.1  90/04/28  22:42:12  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Some crazy includes for the save-opts part of the Elm program!

**/

#define ALTERNATIVES		0
#define ALWAYSDELETE		1
#define ALWAYSKEEP		2
#define ALWAYSSTORE		3
#define ARROW			4
#define ASK			5
#define ASKCC			6
#define ATTRIBUTION             7
#define AUTOCOPY                8
#define BOUNCEBACK              9
#define CALENDAR                10
#define COPY                    11
#define EDITOR                  12
#define ESCAPECHAR              13
#define FORCENAME               14
#define FORMS                   15
#define FULLNAME                16
#define KEEPEMPTY               17
#define KEYPAD                  18
#define LOCALSIGNATURE          19
#define MAILDIR                 20
#define MENU                    21
#define MOVEPAGE                22
#define NAMES                   23
#define NOHEADER                24
#define PAGER                   25
#define POINTNEW                26
#define PREFIX                  27
#define PRINT                   28
#define PROMPTAFTER             29
#define RECEIVEDMAIL            30
#define REMOTESIGNATURE         31
#define RESOLVE                 32
#define SAVENAME                33
#define SENTMAIL                34
#define SHELL                   35
#define SIGDASHES               36
#define SIGNATURE               37
#define SOFTKEYS                38
#define SORTBY                  39
#define TIMEOUT                 40
#define TITLES                  41
#define TMPDIR                  42
#define USERLEVEL               43
#define WARNINGS                44
#define WEED                    45
#define WEEDOUT                 46

#define NUMBER_OF_SAVEABLE_OPTIONS	WEEDOUT+1

struct save_info_recs {
	char 	name[NLEN]; 	/* name of instruction */
	long 	offset;		/* offset into elmrc-info file */
	} save_info[NUMBER_OF_SAVEABLE_OPTIONS] =
{
 { "alternatives", -1L },
 { "alwaysdelete", -1L },
 { "alwayskeep", -1L },
 { "alwaysstore", -1L },
 { "arrow", -1L},
 { "ask", -1L },
 { "askcc", -1L },
 { "attribution", -1L },
 { "autocopy", -1L },
 { "bounceback", -1L },
 { "calendar", -1L },
 { "copy", -1L },
 { "editor", -1L },
 { "escape", -1L },
 { "forcename", -1L },
 { "forms", -1L },
 { "fullname", -1L },
 { "keepempty", -1L },
 { "keypad", -1L },
 { "localsignature", -1L },
 { "maildir", -1L },
 { "menu", -1L },
 { "movepage", -1L },
 { "names", -1L },
 { "noheader", -1L },
 { "pager", -1L },
 { "pointnew", -1L},
 { "prefix", -1L },
 { "print", -1L },
 { "promptafter", -1L },
 { "receivedmail", -1L },
 { "remotesignature",-1L},
 { "resolve", -1L },
 { "savename", -1L },
 { "sentmail", -1L },
 { "shell", -1L },
 { "sigdashes", -1L },
 { "signature", -1L },
 { "softkeys", -1L },
 { "sortby", -1L },
 { "timeout", -1L },
 { "titles", -1L },
 { "tmpdir", -1L },
 { "userlevel", -1L },
 { "warnings", -1L },
 { "weed", -1L },
 { "weedout", -1L },
};
