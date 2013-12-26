
static char rcsid[] = "@(#)$Id: addr_util.c,v 4.1.1.1 90/10/07 20:44:56 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.1 $   $State: Exp $
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
 * $Log:	addr_util.c,v $
 * Revision 4.1.1.1  90/10/07  20:44:56  syd
 * Make time to seconds
 * From: rhg@cpscom
 *
 * Revision 4.1  90/04/28  22:42:21  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This file contains addressing utilities

**/

#include "headers.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef PWDINSYS
#  include <sys/pwd.h>
#else
#  include <pwd.h>
#endif

#include <ctype.h>

#ifdef BSD
#undef tolower
#undef toupper
#endif

char *get_alias_address(), *get_token();
char *strtok(), *strcpy(), *strcat(), *strncpy(), *index(), *rindex();


#define SKIP_WS(p) while (isspace(*p)) p++
#define SKIP_ALPHA(p) while (isalpha(*p)) p++
#define SKIP_DIGITS(p) while (isdigit(*p)) p++

static char *day_name[8] = {
    "sun", "mon", "tue", "wed", "thu", "fri", "sat", 0
};

static char *month_name[13] = {
    "jan", "feb", "mar", "apr",
    "may", "jun", "jul", "aug",
    "sep", "oct", "nov", "dec", 0
};

static int month_len[12] = {
    31, 28, 31, 30, 31, 30, 31,
    31, 30, 31, 30, 31 };

/* The following time zones are taken from a variety of sources.  They
 * are by no means exhaustive, but seem to include most of those
 * in common usage.  A comprehensive list is impossible, since the same
 * abbreviation is sometimes used to mean different things in different
 * parts of the world.
 */
static struct tzone {
    char *str;
    int offset; /* offset, in minutes, EAST of GMT */
} tzone_info[] = {
    /* the following are from rfc822 */
    "ut", 0, "gmt", 0,
    "est", -5*60, "edt", -4*60,
    "cst", -6*60, "cdt", -5*60,
    "mst", -7*60, "mdt", -6*60,
    "pst", -8*60, "pdt", -7*60,
    "z", 0, /* zulu time (the rest of the military codes are bogus) */

    /* these are also popular in Europe */
    "wet", 0*60, "wet dst", 1*60, /* western european */
    "met", 1*60, "met dst", 2*60, /* middle european */
    "eet", 2*60, "eet dst", 3*60, /* eastern european */
    "bst", 1*60, /* ??? british summer time (=+0100) */

    /* ... and Canada */
    "ast", -4*60, "adt", -3*60, /* atlantic */
    "nst", -3*60-30, "ndt", -2*60-30, /* newfoundland */
    "yst", -9*60, "ydt", -8*60, /* yukon */
    "hst", -10*60, /* hawaii (not really canada) */

    /* ... and Asia */
    "jst", 9*60, /* japan */
    "sst", 8*60, /* singapore */

    /* ... and the South Pacific */
    "nzst", 12*60, "nzdt", 13*60, /* new zealand */
    "wst", 8*60, "wdt", 9*60, /* western australia */
    /* there's also central and eastern australia, but they insist on using
     * cst, est, etc., which would be indistinguishable for the us zones */
     (char *) 0, 0
};

#ifndef OS2
char *
gcos_name(gcos_field, logname)
char *logname, *gcos_field;
{
    /** Return the full name found in a passwd file gcos field **/

#ifdef BERKNAMES

    static char fullname[SLEN];
    register char *fncp, *gcoscp, *lncp, *end;


    /* full name is all chars up to first ',' (or whole gcos, if no ',') */
    /* replace any & with logname in upper case */

    for(fncp = fullname, gcoscp= gcos_field, end = fullname + SLEN - 1;
        (*gcoscp != ',' && *gcoscp != '\0' && fncp != end);
	gcoscp++) {

	if(*gcoscp == '&') {
	    for(lncp = logname; *lncp; fncp++, lncp++)
		*fncp = toupper(*lncp);
	} else {
	    *fncp++ = *gcoscp;
	}
    }

    *fncp = '\0';
    return(fullname);
#else
#ifdef USGNAMES

    char *firstcp, *lastcp;

    /* The last character of the full name is the one preceding the first
     * '('. If there is no '(', then the full name ends at the end of the
     * gcos field.
     */
    if(lastcp = index(gcos_field, '('))
	*lastcp = '\0';

    /* The first character of the full name is the one following the
     * last '-' before that ending character. NOTE: that's why we
     * establish the ending character first!
     * If there is no '-' before the ending character, then the fullname
     * begins at the beginning of the gcos field.
     */
    if(firstcp = rindex(gcos_field, '-'))
	firstcp++;
    else
	firstcp = gcos_field;

    return(firstcp);

#else
    /* use full gcos field */
    return(gcos_field);
#endif
#endif
}
#endif

char *
get_full_name(logname)
char *logname;
{
	/* return a pointer to the full user name for the passed logname
	 * or NULL if cannot be found
	 * If PASSNAMES get it from the gcos field, otherwise get it
	 * from ~/.fullname.
	 */

#ifndef PASSNAMES
	FILE *fp;
	char fullnamefile[SLEN];
#endif
	static char fullname[SLEN];
	struct passwd *getpwnam(), *pass;

	if((pass = getpwnam(logname)) == NULL)
	  return(NULL);
#ifdef PASSNAMES	/* get full_username from gcos field */
	strcpy(fullname, gcos_name(pass->pw_gecos, logname));
#else			/* get full_username from ~/.fullname file */
	sprintf(fullnamefile, "%s/.fullname", pass->pw_dir);

	if(can_access(fullnamefile, READ_ACCESS) != 0)
	  return(NULL);		/* fullname file not accessible to user */
	if((fp = fopen(fullnamefile, "r")) == NULL)
	  return(NULL);		/* fullname file cannot be opened! */
	if(fgets(fullname, SLEN, fp) == NULL) {
	  fclose(fp);
	  return(NULL);		/* fullname file empty! */
	}
	fclose(fp);
	no_ret(fullname);	/* remove trailing '\n' */
#endif
	return(fullname);
}

int
talk_to(sitename)
char *sitename;
{
	/** If we talk to the specified site, return true, else
	    we're going to have to expand this baby out, so
	    return false! **/

	struct lsys_rec  *sysname;

	sysname = talk_to_sys;

	if (sysname == NULL) {
	 dprint(2, (debugfile,
		"Warning: talk_to_sys is currently set to NULL!\n"));
	 return(0);
	}

	while (sysname != NULL) {
	  if (strcmp(sysname->name, sitename) == 0)
	    return(1);
	  else
	    sysname = sysname->next;
	}

	return(0);
}

add_site(buffer, site, lastsite)
char *buffer, *site, *lastsite;
{
	/** add site to buffer, unless site is 'uucp' or site is
	    the same as lastsite.   If not, set lastsite to site.
	**/

	char local_buffer[SLEN], *stripped;
	char *strip_parens();

	stripped = strip_parens(site);

	if (strcmp(stripped, "uucp") != 0)
	  if (strcmp(stripped, lastsite) != 0) {
	    if (buffer[0] == '\0')
	      strcpy(buffer, stripped);         /* first in list! */
	    else {
	      sprintf(local_buffer,"%s!%s", buffer, stripped);
	      strcpy(buffer, local_buffer);
	    }
	    strcpy(lastsite, stripped); /* don't want THIS twice! */
	  }
}

#ifdef USE_EMBEDDED_ADDRESSES

get_address_from(prefix, line, buffer)
char *prefix, *line, *buffer;
{
	/** This routine extracts the address from either a 'From:' line
	    or a 'Reply-To:' line.  The strategy is as follows:  if the
	    line contains a '<', then the stuff enclosed is returned.
	    Otherwise we go through the line and strip out comments
	    and return that.  White space will be elided from the result.
	**/

    register char *s;

    /**  Skip start of line over prefix, e.g. "From:".  **/
    line += strlen(prefix);

    /**  If there is a '<' then copy from it to '>' into the buffer.  **/
    if ( (s = index(line,'<')) != NULL ) {
	while ( ++s , *s != '\0' && *s != '>' ) {
	    if ( !isspace(*s) )
		*buffer++ = *s;
	}
	*buffer = '\0';
	return;
    }

    /**  Otherwise, strip comments and get address with whitespace elided.  **/
    for ( s = strip_parens(line) ; *s != '\0' ; ++s ) {
	if ( !isspace(*s) )
	    *buffer++ = *s;
    }
    *buffer = '\0';

}

#endif

translate_return(addr, ret_addr)
char *addr, *ret_addr;
{
	/** Return ret_addr to be the same as addr, but with the login
            of the person sending the message replaced by '%s' for
            future processing...
	    Fixed to make "%xx" "%%xx" (dumb 'C' system!)
	**/

	register int loc, loc2, iindex = 0;

	loc2 = chloc(addr,'@');
	if ((loc = chloc(addr, '%')) < loc2)
	  loc2 = loc;

	if (loc2 != -1) {	/* ARPA address. */
	  /* algorithm is to get to '@' sign and move backwards until
	     we've hit the beginning of the word or another metachar.
	  */
	  for (loc = loc2 - 1; loc > -1 && addr[loc] != '!'; loc--)
	     ;
	}
	else {			/* usenet address */
	  /* simple algorithm - find last '!' */

	  loc2 = strlen(addr);	/* need it anyway! */

	  for (loc = loc2; loc > -1 && addr[loc] != '!'; loc--)
	      ;
	}

	/** now copy up to 'loc' into destination... **/

	while (iindex <= loc) {
	  ret_addr[iindex] = addr[iindex];
	  iindex++;
	}

	/** now append the '%s'... **/

	ret_addr[iindex++] = '%';
	ret_addr[iindex++] = 's';

	/** and, finally, if anything left, add that **/

	loc = strlen(addr);
	while (loc2 < loc) {
	  ret_addr[iindex++] = addr[loc2++];
	  if (addr[loc2-1] == '%')	/* tweak for "printf" */
	    ret_addr[iindex++] = '%';
	}

	ret_addr[iindex] = '\0';
}

int
build_address(to, full_to)
char *to, *full_to;
{
	/** loop on all words in 'to' line...append to full_to as
	    we go along, until done or length > len.  Modified to
	    know that stuff in parens are comments...Returns non-zero
	    if it changed the information as it copied it across...
	**/

	register int i, j, changed = 0, in_parens = 0, expanded_information = 0;
	char word[SLEN], next_word[SLEN], *ptr, buffer[SLEN];
	char new_to_list[SLEN];
	char *strpbrk(), *strcat(), *gecos;
#ifndef DONT_TOUCH_ADDRESSES
	char *expand_system();
#endif

	new_to_list[0] = '\0';

	i = get_word(to, 0, word);

	full_to[0] = '\0';

	while (i > 0) {

	  j = get_word(to, i, next_word);

try_new_word:
	  if(word[0] == '(')
	    in_parens++;

	  if (in_parens) {
	    if(word[strlen(word)-1] == ')')
	      in_parens--;
	    strcat(full_to, " ");
	    strcat(full_to, word);
	  }
	  else if (strpbrk(word,"!@:") != NULL) {
#ifdef DONT_TOUCH_ADDRESSES
	    sprintf(full_to, "%s%s%s", full_to,
                    full_to[0] != '\0'? ", " : "", word);
#else
	    sprintf(full_to, "%s%s%s", full_to,
                    full_to[0] != '\0'? ", " : "", expand_system(word, 1));
#endif
	  }
	  else if ((ptr = get_alias_address(word, TRUE)) != NULL) {
	    sprintf(full_to, "%s%s%s", full_to,
                    full_to[0] != '\0'? ", " : "", ptr);
	    expanded_information++;
	  }
	  else if (strlen(word) > 0) {
	    if (valid_name(word)) {
	      if (j > 0 && next_word[0] == '(')	/* already has full name */
		gecos = NULL;
	      else				/* needs a full name */
		gecos=get_full_name(word);
#if defined(INTERNET) & defined(USE_DOMAIN)
	      sprintf(full_to, "%s%s%s@%s%s%s%s",
		      full_to,
		      (full_to[0] != '\0'? ", " : ""),
		      word,
		      hostfullname,
		      (gecos ? " (" : ""),
		      (gecos ? gecos : ""),
		      (gecos ? ")" : ""));
#else /* INTERNET and USE_DOMAIN */
	      sprintf(full_to, "%s%s%s%s%s%s",
		      full_to,
		      (full_to[0] != '\0'? ", " : ""),
		      word,
		      (gecos ? " (" : ""),
		      (gecos ? gecos : ""),
		      (gecos ? ")" : ""));
#endif /* INTERNET and USE_DOMAIN */
	    } else if (check_only) {
	      printf("(alias \"%s\" is unknown)\n\r", word);
	      changed++;
	    }
	    else if (! isatty(fileno(stdin)) ) {	/* batch mode error! */
	      fprintf(stderr,"Cannot expand alias '%s'!\n\r", word);
	      fprintf(stderr,"Use \"checkalias\" to find valid addresses!\n\r");
	      dprint(1, (debugfile,
		      "Can't expand alias %s - bailing out of build_address\n",
		      word));
	      leave(0);
	    }
	    else {
	      dprint(2,(debugfile,"Entered unknown address %s\n", word));
	      sprintf(buffer, "'%s' is an unknown address.  Replace with: ",
	              word);
	      word[0] = '\0';
	      changed++;

	      PutLine0(LINES, 0, buffer);

	      (void)optionally_enter(word, LINES, strlen(buffer), FALSE, FALSE);
	      clear_error();
	      if (strlen(word) > 0) {
	        dprint(3,(debugfile, "Replaced with %s in build_address\n",
			 word));
		goto try_new_word;
	      }
	      else
		dprint(3,(debugfile,
		    "Address removed from TO list by build_address\n"));
	      continue;
	    }
	  }

	  /* and this word to the new to list */
	  if(*new_to_list != '\0')
	    strcat(new_to_list, " ");
	  strcat(new_to_list, word);

	  if((i = j) > 0)
	    strcpy(word, next_word);
	}

	/* if new to list is different from original, update original */
	if (changed)
	  strcpy(to, new_to_list);

	return( expanded_information > 0 ? 1 : 0 );
}

int
real_from(buffer, entry)
char *buffer;
struct header_rec *entry;
{
	/***** Returns true iff 's' has the seven 'from' fields, (or
	       8 - some machines include the TIME ZONE!!!)
	       Initialize the date and from entries in the record
	       and also the message received date/time if 'entry'
	       is not NULL.  *****/

	struct header_rec temp_rec, *rec_ptr;
	char junk[STRING], timebuff[STRING], holding_from[SLEN], hold_tz[12];
	char mybuf[BUFSIZ], timebuf2[STRING], *p, *q;
	int  eight_fields = 0;
        int mday, month, year, minutes, seconds, tz, i;
        long gmttime;

	/* set rec_ptr according to whether the data is to be returned
	 * in the second argument */
	rec_ptr = (entry == NULL ? &temp_rec : entry);

	rec_ptr->year[0] = '\0';
	timebuff[0] = '\0';
	junk[0] = '\0';
	hold_tz[0] = '\0';

	/* From <user> <day> <month> <day> <hr:min:sec> <year> */

	sscanf(buffer, "%*s %*s %*s %*s %*s %s %s %s", timebuff, timebuf2, junk);

	if (strlen(timebuff) < 3 && strlen(timebuf2) < 3) {
	  dprint(3,(debugfile,
		"Real_from returns FAIL [no time field] on\n-> %s\n",
		buffer));
	  return(FALSE);
	}

	if (timebuff[1] != ':' && timebuff[2] != ':' &&
	    timebuf2[1] != ':' && timebuf2[2] != ':') { /* UUPC ! */
	  dprint(3,(debugfile,
		"Real_from returns FAIL [bad time field] on\n-> %s\n",
		buffer));
	  return(FALSE);
	}
	if (junk[0] != '\0' && strcmp(junk, "remote")) { /* try for 8 field entry */
	  junk[0] = '\0';
	  sscanf(buffer, "%*s %*s %*s %*s %*s %*s %*s %*s %s", junk);
	  if (junk[0] != '\0' && strcmp(junk, "remote")) {
	    dprint(3, (debugfile,             /* ^ UUPC ! */
		  "Real_from returns FAIL [too many fields] on\n-> %s\n",
		  buffer));
	    return(FALSE);
	  }
	  eight_fields++;
	}

	/** now get the info out of the record! **/

	if (eight_fields)
	  sscanf(buffer, "%s %s %s %s %s %s %s %s",
	            junk, holding_from, rec_ptr->dayname, rec_ptr->month,
                    rec_ptr->day, rec_ptr->time, hold_tz, rec_ptr->year);
	else
	  sscanf(buffer, "%s %s %s %s %s %s %s",
	            junk, holding_from, rec_ptr->dayname, rec_ptr->month,
                    rec_ptr->day, rec_ptr->time, rec_ptr->year);

        /* non-standard UUPC From line? */

        /* This is from UUPC rmail: */
        /* From fkk Sat, 14 Mar 1992 14:53:27 MET remote from stasys */

        /* while this were normal: */
        /* From fkk Sat Mar 14 14:53:27 1992 [MET] */

        if ( rec_ptr->dayname[strlen(rec_ptr->dayname) - 1] == ',' )
        {
	  sscanf(buffer, "%s %s %s %s %s %s %s %s",
	         junk, holding_from, rec_ptr->dayname, rec_ptr->day,
                 rec_ptr->month, rec_ptr->year, rec_ptr->time, hold_tz);
          rec_ptr->dayname[strlen(rec_ptr->dayname) - 1] = 0;
        }

	strncpy(rec_ptr->from, holding_from, STRING-1);
	rec_ptr->from[STRING-1] = '\0';
	resolve_received(rec_ptr);

        /* first get everything into lower case */
        for (p=mybuf, q=mybuf+sizeof mybuf;
	     *buffer && p<q;
	     p++, buffer++) {
	  *p = isupper(*buffer) ? tolower(*buffer) : *buffer;
        }
	*p = 0;
	p = mybuf;
	while (!isspace(*p)) p++;	/* skip "from" */
	SKIP_WS(p);
	while (!isspace(*p)) p++;	/* skip from address */
	SKIP_WS(p);
	while (!isspace(*p)) p++;	/* skip day of week */
	SKIP_WS(p);
	month = prefix(month_name, p);
	get_unix_date(p,&year, &mday, &minutes, &seconds, &tz);
	month_len[1] = (year%4) ? 28 : 29;
	if (mday < 0 || mday>month_len[month]) {
	  dprint(5,(debugfile, "ridiculous day %d of month %d\n",mday,month));
	}

	minutes -= tz;
	if (tz > 0) { /* east of Greenwich */
	  if (minutes < 0) {
	    if (--mday < 0) {
	      if (--month < 0) {
		year--; /* don't worry about 1900! */
		month = 11;
	      }
	      mday = month_len[month];
	    }
	    minutes += 24*60;
	  }
	}
	if (tz < 0) { /* west of Greenwich */
	  if (minutes <= 24*60) {
	    if (++mday > month_len[month]) {
	      if (++month >= 12) {
		year++; /* don't worry about 1999! yet?? */
		month = 0;
	      }
	      mday = 0;
	    }
	    minutes -= 24*60;
	  }
	}
        gmttime = year - 70;		 /* make base year */
        if (gmttime < 0)
  	  gmttime += 100;
        gmttime = gmttime * 365 + (gmttime + 1) / 4;  /* now we have days adjusted for leap years */
        for (i = 0; i < month; i++)
  	  gmttime += month_len[i];
        if (month > 1 && (year % 4) == 0)
  	  gmttime++;			/* now to month adjusted for leap year if after feb */
        gmttime += mday - 1;		/* and now to the day */
        gmttime *= 24 * 60;			/* convert to minutes */
        gmttime += minutes;
        rec_ptr->time_sent = gmttime * 60 + seconds;	/* now unix seconds since 1/1/70 00:00 GMT */

	return(rec_ptr->year[0] != '\0');
}

forwarded(buffer, entry)
char *buffer;
struct header_rec *entry;
{
	/** Change 'from' and date fields to reflect the ORIGINATOR of
	    the message by iteratively parsing the >From fields...
	    Modified to deal with headers that include the time zone
	    of the originating machine... **/

	char machine[SLEN], buff[SLEN], holding_from[SLEN];

	machine[0] = holding_from[0] = '\0';

	sscanf(buffer, "%*s %s %s %s %s %s %s %*s %*s %s",
	            holding_from, entry->dayname, entry->month,
                    entry->day, entry->time, entry->year, machine);

	if (isdigit(entry->month[0])) { /* try for veeger address */
	  sscanf(buffer, "%*s %s %s%*c %s %s %s %s %*s %*s %s",
	            holding_from, entry->dayname, entry->day, entry->month,
                    entry->year, entry->time, machine);
	}
	if (isalpha(entry->year[0])) { /* try for address including tz */
	  sscanf(buffer, "%*s %s %s %s %s %s %*s %s %*s %*s %s",
	            holding_from, entry->dayname, entry->month,
                    entry->day, entry->time, entry->year, machine);
	}

	/* the following fix is to deal with ">From xyz ... forwarded by xyz"
	   which occasionally shows up within AT&T.  Thanks to Bill Carpenter
	   for the fix! */

	if (strcmp(machine, holding_from) == 0)
	  machine[0] = '\0';

	if (machine[0] == '\0')
	  strcpy(buff, holding_from[0] ? holding_from : "anonymous");
	else
	  sprintf(buff,"%s!%s", machine, holding_from);

	strncpy(entry->from, buff, STRING-1);
	entry->from[STRING-1] = '\0';
}

parse_arpa_who(buffer, newfrom, is_really_a_to)
char *buffer, *newfrom;
int is_really_a_to;
{
	/** try to parse the 'From:' line given... It can be in one of
	    two formats:
		From: Dave Taylor <hplabs!dat>
	    or  From: hplabs!dat (Dave Taylor)

	    Added: removes quotes if name is quoted (12/12)
	    Added: only copies STRING characters...
	    Added: if no comment part, copy address instead!
	    Added: if is_really_a_to, this is really a 'to' line
		   and treat as if we allow embedded addresses
	**/

	int use_embedded_addresses;
	char temp_buffer[SLEN], *temp;
	register int i, j = 0, in_parens;

	temp = (char *) temp_buffer;
	temp[0] = '\0';

	no_ret(buffer);		/* blow away '\n' char! */

	if (lastch(buffer) == '>') {
	  for (i=strlen("From: "); buffer[i] != '\0' && buffer[i] != '<' &&
	       buffer[i] != '('; i++)
	    temp[j++] = buffer[i];
	  temp[j] = '\0';
	}
	else if (lastch(buffer) == ')') {
	  in_parens = 1;
	  for (i=strlen(buffer)-2; buffer[i] != '\0' && buffer[i] != '<'; i--) {
	    switch(buffer[i]) {
	    case ')':	in_parens++;
			break;
	    case '(':	in_parens--;
			break;
	    }
	    if(!in_parens) break;
	    temp[j++] = buffer[i];
	  }
	  temp[j] = '\0';
	  reverse(temp);
	}

#ifdef USE_EMBEDDED_ADDRESSES
	use_embedded_addresses = TRUE;
#else
	use_embedded_addresses = FALSE;
#endif

	if(use_embedded_addresses || is_really_a_to) {
	  /** if we have a null string at this point, we must just have a
	      From: line that contains an address only.  At this point we
	      can have one of a few possibilities...

		  From: address
		  From: <address>
		  From: address ()
	  **/

	  if (strlen(temp) == 0) {
	    if (lastch(buffer) != '>') {
	      for (i=strlen("From:");buffer[i] != '\0' && buffer[i] != '('; i++)
		temp[j++] = buffer[i];
	      temp[j] = '\0';
	    }
	    else {	/* get outta '<>' pair, please! */
	      for (i=strlen(buffer)-2;buffer[i] != '<' && buffer[i] != ':';i--)
		temp[j++] = buffer[i];
	      temp[j] = '\0';
	      reverse(temp);
	    }
	  }
	}

	if (strlen(temp) > 0) {		/* mess with buffer... */

	  /* remove leading spaces and quotes... */

	  while (whitespace(temp[0]) || quote(temp[0]))
	    temp = (char *) (temp + 1);		/* increment address! */

	  /* remove trailing spaces and quotes... */

	  i = strlen(temp) - 1;

	  while (whitespace(temp[i]) || quote(temp[i]))
	   temp[i--] = '\0';

	  /* if anything is left, let's change 'from' value! */

	  if (strlen(temp) > 0) {
	    strncpy(newfrom, temp, STRING-1);
	    newfrom[STRING-1] = '\0';
	  }
	}
}

/*
Quoting from RFC 822:
     5.  DATE AND TIME SPECIFICATION

     5.1.  SYNTAX

     date-time   =  [ day "," ] date time        ; dd mm yy
						 ;  hh:mm:ss zzz

     day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
		 /  "Fri"  / "Sat" /  "Sun"

     date        =  1*2DIGIT month 2DIGIT        ; day month year
						 ;  e.g. 20 Jun 82

     month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
		 /  "May"  /  "Jun" /  "Jul"  /  "Aug"
		 /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"

     time        =  hour zone                    ; ANSI and Military

     hour        =  2DIGIT ":" 2DIGIT [":" 2DIGIT]
						 ; 00:00:00 - 23:59:59

     zone        =  "UT"  / "GMT"                ; Universal Time
						 ; North American : UT
		 /  "EST" / "EDT"                ;  Eastern:  - 5/ - 4
		 /  "CST" / "CDT"                ;  Central:  - 6/ - 5
		 /  "MST" / "MDT"                ;  Mountain: - 7/ - 6
		 /  "PST" / "PDT"                ;  Pacific:  - 8/ - 7
		 /  1ALPHA                       ; Military: Z = UT;
						 ;  A:-1; (J not used)
						 ;  M:-12; N:+1; Y:+12
		 / ( ("+" / "-") 4DIGIT )        ; Local differential
						 ;  hours+min. (HHMM)
*/

/* Translate a symbolic timezone name (e.g. EDT or NZST) to a number of
 * minutes *east* of gmt (if the local time is t, the gmt equivalent is
 * t - tz_lookup(zone)).
 * Return 0 if the timezone is not recognized.
 */
static int tz_lookup(str)
char *str;
{
    struct tzone *p;

    for (p = tzone_info; p->str; p++) {
	if (strcmp(p->str,str)==0) return p->offset;
    }
    dprint(5,(debugfile,"unknown time zone %s\n",str));
    return 0;
}

/* Return smallest i such that table[i] is a prefix of str.  Return -1 if not
 * found.
 */
int prefix(table, str)
char **table;
char *str;
{
    int i;

    for (i=0;table[i];i++)
	if (strncmp(table[i],str,strlen(*table))==0)
	    return i;
    return -1;
}

/* The following routines, get_XXX(p,...), expect p to point to a string
 * of the appropriate syntax.  They return decoded values in result parameters,
 * and return p updated to point past the parsed substring (also stripping
 * trailing whitespace).
 * Return 0 on syntax errors.
 */

/* Parse a year: ['1' '9'] digit digit WS
 */
static char *
get_year(p, result)
char *p;
int *result;
{
    int year;

    if (!isdigit(*p)) {
	dprint(5,(debugfile,"missing year: %s\n",p));
	return 0;
    }
    year = atoi(p);
    /* be nice and allow 19xx, althought that's not really kosher */
    if (year>=1900 && year <=1999) year -= 1900;
    if (year<0 || year>99) {
	dprint(5,(debugfile,"ridiculous year %d\n",year));
	return 0;
    }
    SKIP_DIGITS(p);
    SKIP_WS(p);
    *result = year;
    return p;
}

/* Parse a time: hours ':' minutes [ ':' seconds ] WS
 * Check that 0<=hours<24, 0<=minutes,seconds<60.
 * Also allow the syntax "digit digit digit digit" with implied ':' in the
 * middle.
 * Convert to minutes and seconds, with results in (*m,*s).
 */
static char *
get_time(p,m,s)
char *p;
int *m, *s;
{
    int hours, minutes, seconds;

    /* hour */
    if (!isdigit(*p)) {
	dprint(5,(debugfile,"missing time: %s\n",p));
	return 0;
    }
    hours = atoi(p);
    SKIP_DIGITS(p);
    if (*p++ != ':') {
	/* perhaps they just wrote hhmm instead of hh:mm */
	minutes = hours % 60;
	hours /= 60;
    }
    else {
	if (hours<0 || hours>23) {
	    dprint(5,(debugfile,"ridiculous hour: %d\n",hours));
	    return 0;
	}
	minutes = atoi(p);
	if (minutes<0 || minutes>59) {
	    dprint(5,(debugfile,"ridiculous minutes: %d\n",minutes));
	    return 0;
	}
    }
    SKIP_DIGITS(p);
    if (*p == ':') {
	p++;
	seconds = atoi(p);
	if (seconds<0 || seconds>59) {
	    dprint(5,(debugfile,"ridiculous seconds: %d\n",seconds));
	    return 0;
	}
	SKIP_DIGITS(p);
    }
    else seconds = 0;
    minutes += hours*60;
    SKIP_WS(p);
    *m = minutes;
    *s = seconds;
    return p;
}

/* Parse a Unix date from which the leading week-day has been stripped.
 * The syntax is "Jun 21 06:45:44 CDT 1989" with timezone optional.
 * i.e., month day time [ zone ] year
 * where day::=digit*, year and time are as defined above,
 * and month and zone are alpha strings starting with a known 3-char prefix.
 * The month has already been processed by the caller, so we just skip over
 * a leading alpha* WS.
 *
 * Unlike the preceding routines, the result is not an updated pointer, but
 * simply 1 for success and 0 for failure.
 */
int
get_unix_date(p,y,d,m,s,t)
char *p;
int *y, *d, *m, *s, *t;
{

    SKIP_ALPHA(p);
    SKIP_WS(p);
    if (!isdigit(*p)) return 0;
    *d = atoi(p);  /* check the value for sanity after we know the month */
    SKIP_DIGITS(p);
    SKIP_WS(p);
    p = get_time(p,m,s);
    if (!p) return 0;
    if (isalpha(*p)) {
	*t = tz_lookup(p);
	SKIP_ALPHA(p);
	SKIP_WS(p);
    }
    else *t = 0;
    p = get_year(p,y);
    if (!p) return 0;
    return 1;
}


/* Parse an rfc822 (with extensions) date.  Return 1 on success, 0 on failure.
 */
parse_arpa_date(string, entry)
char *string;
struct header_rec *entry;
{
    char buffer[BUFSIZ], *p, *q;
    int mday, month, year, minutes, seconds, tz, i;
    long gmttime;

    /* first get everything into lower case */
    for (p=buffer, q=buffer+sizeof buffer; *string && p<q; p++, string++) {
	*p = isupper(*string) ? tolower(*string) : *string;
    }
    *p = 0;
    p = buffer;
    SKIP_WS(p);

    if (prefix(day_name,p)>=0) {
	/* accept anything that *starts* with a valid day name */
	/* also, don't check whether it's right! */

	(void)strncpy(entry->dayname, p, 3);
	entry->dayname[3] = 0;
	SKIP_ALPHA(p);
	SKIP_WS(p);

	if (*p==',') {
	    p++;
	    SKIP_WS(p);
	}
	/* A comma is required here, but we'll be nice guys and look the other
	 * way if it's missing.
	 */
    }

    /* date */

    /* day of the month */
    if (!isdigit(*p)) {
	/* Missing day.  Maybe this is a Unix date?
	 */
	month = prefix(month_name,p);
	if (month >= 0 &&
	    get_unix_date(p, &year, &mday, &minutes, &seconds, &tz)) {
		goto got_date;
	}
	dprint(5,(debugfile,"missing day: %s\n",p));
	return 0;
    }
    mday = atoi(p);  /* check the value for sanity after we know the month */
    SKIP_DIGITS(p);
    SKIP_WS(p);

    /* month name */
    month = prefix(month_name,p);
    if (month < 0) {
	dprint(5,(debugfile,"missing month: %s\n",p));
	return 0;
    }
    SKIP_ALPHA(p);
    SKIP_WS(p);

    /* year */
    if (!(p = get_year(p,&year))) return 0;

    /* time */
    if (!(p = get_time(p,&minutes,&seconds))) return 0;

    /* zone */
    for (q=p; *q && !isspace(*q); q++) continue;
    *q = 0;
    if (*p=='-' || *p=='+') {
	char sign = *p++;

	if (isdigit(*p)) {
	    for (i=0; i<4; i++) {
		if (!isdigit(p[i])) {
		    dprint(5,(debugfile,"ridiculous numeric timezone: %s\n",p));
		    return 0;
		}
		p[i] -= '0';
	    }
	    tz = (p[0]*10 + p[1])*60 + p[2]*10 + p[3];
	    if (sign=='-') tz = -tz;
	    sprintf(entry->time_zone, "%d", tz);
	}
	else {
	    /* some brain-damaged dates use a '-' before a symbolic time zone */
	    SKIP_WS(p);
	    strncpy(entry->time_zone, p, sizeof(entry->time_zone) - 1);
	    tz = tz_lookup(p);
	}
    }
    else {
	tz = tz_lookup(p);
	strncpy(entry->time_zone, p, sizeof(entry->time_zone) - 1);
    }

got_date:
    month_len[1] = (year%4) ? 28 : 29;
    if (mday<0 || mday>month_len[month]) {
	dprint(5,(debugfile,"ridiculous day %d of month %d\n",mday,month));
	return 0;
    }

    /* convert back to symbolic form (silly, but the rest of the program
     * expects it and I'm not about to change all that!)
     */
    sprintf(entry->year, "%02d", year);
    sprintf(entry->month, "%s", month_name[month]);
    entry->month[0] = toupper(entry->month[0]);
    sprintf(entry->day, "%d", mday);
    sprintf(entry->time, "%02d:%02d:%02d",minutes/60,minutes%60,seconds);

    /* shift everything to UTC (aka GMT) before making long time for sorting */
    minutes -= tz;
    if (tz > 0) { /* east of Greenwich */
	if (minutes < 0) {
	    if (--mday < 0) {
		if (--month < 0) {
		    year--; /* don't worry about 1900! */
		    month = 11;
		}
		mday = month_len[month];
	    }
	    minutes += 24*60;
	}
    }
    if (tz < 0) { /* west of Greenwich */
	if (minutes >= 24*60) {
	    if (++mday > month_len[month]) {
		if (++month >= 12) {
		    year++; /* don't worry about 1999! */
		    month = 0;
		}
		mday = 0;
	    }
	    minutes -= 24*60;
	}
    }
    gmttime = year - 70;		 /* make base year */
    if (gmttime < 0)
	gmttime += 100;
    gmttime = gmttime * 365 + (gmttime + 1) / 4;  /* now we have days adjusted for leap years */
    for (i = 0; i < month; i++)
	gmttime += month_len[i];
    if (month > 1 && (year % 4) == 0)
	gmttime++;			/* now to month adjusted for leap year if after feb */
    gmttime += mday - 1;		/* and now to the day */
    gmttime *= 24 * 60;			/* convert to minutes */
    gmttime += minutes;
    entry->time_sent = gmttime * 60;	/* now unix seconds since 1/1/70 00:00 GMT */

    return 1;
}

fix_arpa_address(address)
char *address;
{
	/** Given a pure ARPA address, try to make it reasonable.

	    This means that if you have something of the form a@b@b make
            it a@b.  If you have something like a%b%c%b@x make it a%b@x...
	**/

	register int host_count = 0, i;
	char     hosts[MAX_HOPS][NLEN];	/* array of machine names */
	char     *host, *addrptr;

	/*  break down into a list of machine names, checking as we go along */

	addrptr = (char *) address;

	while ((host = get_token(addrptr, "%@", 2)) != NULL) {
	  for (i = 0; i < host_count && ! equal(hosts[i], host); i++)
	      ;

	  if (i == host_count) {
	    strcpy(hosts[host_count++], host);
	    if (host_count == MAX_HOPS) {
	       dprint(2, (debugfile,
           "Can't build return address - hit MAX_HOPS in fix_arpa_address\n"));
	       error("Can't build return address - hit MAX_HOPS limit!");
	       return(1);
	    }
	  }
	  else
	    host_count = i + 1;
	  addrptr = NULL;
	}

	/** rebuild the address.. **/

	address[0] = '\0';

	for (i = 0; i < host_count; i++)
	  sprintf(address, "%s%s%s", address,
	          address[0] == '\0'? "" :
	 	    (i == host_count - 1 ? "@" : "%"),
	          hosts[i]);

	return(0);
}

figure_out_addressee(buffer, mail_to)
char *buffer;
char *mail_to;
{
	/** This routine steps through all the addresses in the "To:"
	    list, initially setting it to the first entry (if mail_to
	    is NULL) or, if the user is found (eg "alternatives") to
	    the current "username".

	    Modified to know how to read quoted names...
	    also modified to look for a comma or eol token and then
	    try to give the maximal useful information when giving the
	    default "to" entry (e.g. "Dave Taylor <taylor@hpldat>"
	    will now give "Dave Taylor" rather than just "Dave")
	**/

	char *address, *bufptr, mybuf[SLEN];
	register int index2 = 0;

	if (equal(mail_to, username)) return;	/* can't be better! */

	bufptr = (char *) buffer;	       /* use the string directly   */

	if (index(buffer,'"') != NULL) {	/* we have a quoted string */
	  while (*bufptr != '"')
	    bufptr++;
	  bufptr++;	/* skip the leading quote */
	  while (*bufptr != '"' && *bufptr)
	    mail_to[index2++] = *bufptr++;
	  mail_to[index2] = '\0';
	}

	else  {

	  while ((address = strtok(bufptr, ",\t\n\r")) != NULL) {

	    if (! okay_address(address, "don't match me!")) {
	      strcpy(mail_to, username);	/* it's to YOU! */
	      return;
	    }
	    else if (strlen(mail_to) == 0) {	/* it's SOMEthing! */

	      /** this next bit is kinda gory, but allows us to use the
		  existing routines to parse the address - by pretending
		  it's a From: line and going from there...
	          Ah well - you get what you pay for, right?
	      **/

	      if (strlen(address) > (sizeof mybuf) - 7)	/* ensure it ain't */
		address[(sizeof mybuf)-7] = '\0';	/*  too long mon!  */

	      sprintf(mybuf, "From: %s", address);
	      parse_arpa_who(mybuf, mail_to, TRUE);
/**
	      get_return_name(address, mail_to, FALSE);
**/
	    }

	    bufptr = (char *) NULL;	/* set to null */
	  }
	}

	return;
}
