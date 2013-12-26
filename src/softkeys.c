
static char rcsid[] = "@(#)$Id: softkeys.c,v 4.1 90/04/28 22:44:11 syd Exp $";

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
 * $Log:	softkeys.c,v $
 * Revision 4.1  90/04/28  22:44:11  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 *******************************************************************************
 */

#include "headers.h"

define_softkeys(level)
int level;
{
	if (! hp_softkeys) return;

	if (level == MAIN) {

	  define_key(f1, " Display   Msg",   "\r");
	  define_key(f2, "  Mail     Msg",   "m");
	  define_key(f3, "  Reply  to Msg",  "r");

	  if (user_level == 0) {
	    define_key(f4, "  Save     Msg",   "s");
	    define_key(f5, " Delete    Msg",   "d");
	    define_key(f6, "Undelete   Msg",   "u");
   	  }
	  else {
	    define_key(f4, " Change  Folder", "c");
	    define_key(f5, "  Save     Msg",   "s");
	    define_key(f6, " Delete/Undelete", "^");
	  }

	  define_key(f7, " Print     Msg",   "p");
	  define_key(f8, "  Quit     ELM",   "q");
	}
	else if (level == ALIAS) {
	  define_key(f1, " Alias  Current",  "a");
	  define_key(f2, " Check  Person",   "p");
	  define_key(f3, " Check  System",   "s");
	  define_key(f4, " Make    Alias",   "m");
	  clear_key(f5);
	  clear_key(f6);
	  clear_key(f7);
	  define_key(f8, " Return  to ELM",  "r");
	}
	else if (level == YESNO) {
	  define_key(f1, "  Yes",  "y");
	  clear_key(f2);
	  clear_key(f3);
	  clear_key(f4);
	  clear_key(f5);
	  clear_key(f6);
	  clear_key(f7);
	  define_key(f8, "   No",  "n");
	}
	else if (level == READ) {
	  define_key(f1, "  Next    Page  ", " ");
	  clear_key(f2);
	  define_key(f3, "  Next    Msg   ", "j");
	  define_key(f4, "  Prev    Msg   ", "k");
	  define_key(f5, "  Reply  to Msg ", "r");
	  define_key(f6, " Delete   Msg   ", "d");
	  define_key(f7, "  Send    Msg   ", "m");
	  define_key(f8, " Return  to ELM ", "q");
	}
	else if (level == CHANGE) {
	  define_key(f1, "  Mail  Directry", "=/");
	  define_key(f2, "  Home  Directry", "~/");
	  clear_key(f3);
	  define_key(f4, "Incoming Mailbox", "!\n");
	  define_key(f5, "\"Received\" Folder", ">\n");
	  define_key(f6, "\"Sent\"   Folder ", "<\n");
	  clear_key(f7);
	  define_key(f8, " Cancel", "\n");
	}

	softkeys_on();
}

define_key(key, display, send)
int key;
char *display, *send;
{

	char buffer[30];

	sprintf(buffer,"%s%s", display, send);

	fprintf(stderr, "%c&f%dk%dd%dL%s", ESCAPE, key,
		strlen(display), strlen(send), buffer);
	fflush(stdout);
}

softkeys_on()
{
	/* enable (esc&s1A) turn on softkeys (esc&jB) and turn on MENU
	   and USER/SYSTEM options. */

	if (hp_softkeys) {
	  fprintf(stderr, "%c&s1A%c&jB%c&jR", ESCAPE, ESCAPE, ESCAPE);
	  fflush(stdout);
	}

}

softkeys_off()
{
	/* turn off softkeys (esc&j@) */

	if (hp_softkeys) {
	  fprintf(stderr, "%c&s0A%c&j@", ESCAPE, ESCAPE);
	  fflush(stdout);
	}
}

clear_key(key)
{
	/** set a key to nothing... **/

	if (hp_softkeys)
	   define_key(key, "                ", "");
}
