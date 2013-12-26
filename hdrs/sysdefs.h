/* : sysdefs.SH,v 4.1.1.3 90/10/10 12:45:21 syd Exp $ */
/*******************************************************************************
 *  The Elm Mail System  -  : 4.1.1.3 $   : Exp $
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
 * $Log:	sysdefs.SH,v $
 * Revision 4.1.1.3  90/10/10  12:45:21  syd
 * Make the symbol look less like a typo, its real
 * From: Syd
 * 
 * Revision 4.1.1.2  90/10/07  19:48:17  syd
 * fix the bounce problem reported earlier when using MMDF submit as the MTA.
 * From: Jim Clausing <jac%brahms.tinton.ccur.com@RELAY.CS.NET>
 * 
 * Revision 4.1.1.1  90/06/09  22:28:42  syd
 * Allow use of submit with mmdf instead of sendmail stub
 * From: martin <martin@hppcmart.grenoble.hp.com>
 * 
 * Revision 4.1  90/04/28  22:42:14  syd
 * checkin of Elm 2.3 as of Release PL0
 * 
 *
 ******************************************************************************/

/**  System level, configurable, defines for the ELM mail system.  **/

#define FIND_DELTA	10		/* byte region where the binary search
					   on the path alias file is fruitless 
                                           (can't be within this boundary)    */

#define MAX_IN_WEEDLIST 150	/* max headers to weed out               */

#define MAX_HOPS	35	/* max hops in return addr to E)veryone  */

#define DEFAULT_BATCH_SUBJECT  "no subject (file transmission)"

#define DEFAULT_DOMAIN  ".UUCP"  /* if mydomain file is missing */

/** If you want to implement 'site hiding' in the mail, then you'll need to
    uncomment the following lines and set them to reasonable values.  See 
    the configuration guide for more details....(actually these are undoc-
    umented because they're fairly dangerous to use.  Just ignore 'em and
    perhaps one day you'll find out what they do, ok?)
**/

/****************************************************************************

#define   SITE_HIDING
#define   HIDDEN_SITE_NAME	"fake-machine-name"
#define   HIDDEN_SITE_USERS	"/usr/mail/lists/hidden_site_users"

****************************************************************************/

#define system_text_file        "aliases.txt"
#define system_hash_file        "aliases.hsh"
#define system_data_file        "aliases.dat"

#define ALIAS_TEXT		"elm/aliases.txt"
#define ALIAS_TEMP		"elm/aliases.tmp"
#define ALIAS_HASH		"elm/aliases.hsh"
#define ALIAS_DATA		"elm/aliases.dat"

#define AUTOREP_FILE            "autorep.dat"
#define AUTOREP_LOG             "autorep.log"
#define AUTOREP_LOCK            "autorep.lck"

#define pathfile		"nmail.pat"
#define domains			"domains"
#define hostdomfile             "domain"

/** where to put the output of the elm -d command... (in home dir) **/
#define DEBUGFILE	"ELM_dbg.inf"
#define OLDEBUG		"ELM_dbg.lst"

#define	default_temp    tempdir
#define temp_file	".snd"
#define temp_form_file	".frm"
#define temp_mbox	".mbx"
#define temp_print      ".prt"
#define temp_edit	".elm"
#define temp_uuname	".uun"
#define readmsg_file	"current.elm"

#define emacs_editor	"emacs"

#define submitmail      mailer
#define submitflags     "-t"
#define sendmail	"sendmail"
#define rmail		"rmail"
#define mailx		"mail"

#define dotelm          "elm"
#define helphome        elmhome
#define helpfile	"elm-help"

#define elmrc_info      "elmrc.inf"

#define elmrcfile	"elm/elmrc"
#define old_elmrcfile	"elm/elmrc.old"
#define mailheaders	"elm/elmhdrs"
#define dead_letter	"dead.mbx"

#define alias_file	"aliases.ini"
#define group_file	"groups.ini"
#define system_file	"systems.ini"

#define default_folders		"mail"
#define default_recvdmail	"=received"
#define default_sentmail	"=sent"

#define unedited_mail	"emergenc.mbx"

#define newalias	"newalias 2>nul >nul"
#define readmsg		"readmsg"

#define cat		"cat"		/* how to display files */
#define sed_cmd		"sed"		/* how to access sed */
#define move_cmd	"mv"		/* how to access sed */
#define uuname		"uuname"	/* how to get a uuname  */

#define MSG_SEPERATOR	"\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001\n"	/* mmdf message seperator */



