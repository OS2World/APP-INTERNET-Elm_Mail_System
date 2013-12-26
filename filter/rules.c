
static char rcsid[] ="@(#)$Id: rules.c,v 4.1 90/04/28 22:42:00 syd Exp $";

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
 * $Log:	rules.c,v $
 * Revision 4.1  90/04/28  22:42:00  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This file contains all the rule routines, including those that apply the
    specified rules and the routine to print the rules out.

**/

#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include "defs.h"
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#include <fcntl.h>

#include "filter.h"

char *listrule();

int
action_from_ruleset()
{
	/** Given the set of rules we've read in and the current to, from,
	    and subject, try to match one.  Return the ACTION of the match
            or LEAVE if none found that apply.
	**/

	register int iindex = 0, not, relation, try_next_rule, x;
	struct condition_rec *cond;

	while (iindex < total_rules) {
	  cond = rules[iindex].condition;
	  try_next_rule = 0;

	  while (cond != NULL && ! try_next_rule) {

	    not = (cond->relation < 0);
	    relation = abs(cond->relation);

	    switch (cond->matchwhat) {

	      case TO     : x = contains(to, cond->argument1); 		break;
	      case FROM   : x = contains(from, cond->argument1); 	break;
	      case SUBJECT: x = contains(subject, cond->argument1);	break;
	      case LINES  : x = compare(lines, relation, cond->argument1);break;

	      case CONTAINS: if (outfd != NULL) fprintf(outfd,
       "filter (%s): Error: rules based on 'contains' are not implemented!\n",
			    username);
			    if (outfd != NULL) fclose(outfd);
			    exit(0);

	      case ALWAYS: not = FALSE; x = TRUE;			break;
	    }

	    if ((not && x) || ((! not) && (! x))) /* this test failed (LISP?) */
	      try_next_rule++;
	    else
	      cond = cond->next;		  /* next condition, if any?  */
	  }

	  if (! try_next_rule) {
	    rule_choosen = iindex;
 	    return(rules[rule_choosen].action);
	  }
	  iindex++;
	}

	rule_choosen = -1;
	return(LEAVE);
}

#define get_the_time()	if (!gotten_time) { 		  \
			   thetime = time( (long *) 0);   \
			   timerec = localtime(&thetime); \
			   gotten_time++; 		  \
			}

expand_macros(word, buffer, line, display)
char *word, *buffer;
int  line, display;
{
	/** expand the allowable macros in the word;
		%d	= day of the month
		%D	= day of the week
	        %h	= hour (0-23)
		%m	= month of the year
		%r	= return address of sender
	   	%s	= subject of message
	   	%S	= "Re: subject of message"  (only add Re: if not there)
		%t	= hour:minute
		%y	= year
	    or simply copies word into buffer. If "display" is set then
	    instead it puts "<day-of-month>" etc. etc. in the output.
	**/

#ifndef	_POSIX_SOURCE
	struct tm *localtime();
	time_t time();
#endif
	struct tm *timerec;
	long	thetime;
	register int i, j=0, gotten_time = 0, reading_a_percent_sign = 0, len;

	for (i = 0, len = strlen(word); i < len; i++) {
	  if (reading_a_percent_sign) {
	    reading_a_percent_sign = 0;
	    switch (word[i]) {

	      case 'r' : buffer[j] = '\0';
			 if (display)
	 		   strcat(buffer, "<return-address>");
			 else
			   strcat(buffer, from);
	                 j = strlen(buffer);
			 break;

	      case 's' : buffer[j] = '\0';
			 if (display)
	 		   strcat(buffer, "<subject>");
			 else {
			   strcat(buffer, "\"");
			   strcat(buffer, subject);
			   strcat(buffer, "\"");
			 }
	                 j = strlen(buffer);
			 break;

	      case 'S' : buffer[j] = '\0';
			 if (display)
	 		   strcat(buffer, "<Re: subject>");
			 else {
			   if (! the_same(subject, "Re:"))
			     strcat(buffer, "\"Re: ");
			   strcat(buffer, subject);
			   strcat(buffer, "\"");
			 }
	                 j = strlen(buffer);
			 break;

	      case 'd' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer, "<day-of-month>");
			 else
			   strcat(buffer, itoa(timerec->tm_mday,FALSE));
	                 j = strlen(buffer);
			 break;

	      case 'D' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer, "<day-of-week>");
			 else
			   strcat(buffer, itoa(timerec->tm_wday,FALSE));
	                 j = strlen(buffer);
			 break;

	      case 'm' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer, "<month>");
			 else
			   strcat(buffer, itoa(timerec->tm_mon+1,FALSE));
	                 j = strlen(buffer);
			 break;

	      case 'y' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer, "<year>");
			 else
			   strcat(buffer, itoa(timerec->tm_year,FALSE));
	                 j = strlen(buffer);
			 break;

	      case 'h' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer, "<hour>");
			 else
			   strcat(buffer, itoa(timerec->tm_hour,FALSE));
	                 j = strlen(buffer);
			 break;

	      case 't' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer, "<time>");
		         else {
			   strcat(buffer, itoa(timerec->tm_hour,FALSE));
			   strcat(buffer, ":");
			   strcat(buffer, itoa(timerec->tm_min,TRUE));
			 }
	                 j = strlen(buffer);
			 break;

	      default  : if (outfd != NULL) fprintf(outfd,
   "filter (%s): Error on line %d translating %%%c macro in word \"%s\"!\n",
			         username, line, word[i], word);
			 if (outfd != NULL) fclose(outfd);
			 exit(1);
	    }
	  }
	  else if (word[i] == '%')
	    reading_a_percent_sign++;
	  else
	    buffer[j++] = (word[i] == '_' ? ' ' : word[i]);
	}
	buffer[j] = '\0';
}

print_rules()
{
	/** print the rules out.  A double check, of course! **/

	register int i = -1;
	char     *whatname(), *actionname();
	struct   condition_rec *cond;

	if (outfd == NULL) return;	/* why are we here, then? */

	while (++i < total_rules) {
	  if (rules[i].condition->matchwhat == ALWAYS) {
	    fprintf(outfd, "\nRule %d:  ** always ** \n\t%s %s\n", i+1,
		 actionname(rules[i].action), listrule(rules[i].argument2));
	    continue;
	  }

	  fprintf(outfd, "\nRule %d:  if (", i+1);

	  cond = rules[i].condition;

	  while (cond != NULL) {
	    if (cond->relation < 0)
	      fprintf(outfd, "not %s %s %s%s%s",
		      whatname(cond->matchwhat),
		      relationname(- (cond->relation)),
		      quoteit(cond->matchwhat),
		      cond->argument1,
		      quoteit(cond->matchwhat));
	    else
	      fprintf(outfd, "%s %s %s%s%s",
		      whatname(cond->matchwhat),
		      relationname(cond->relation),
		      quoteit(cond->matchwhat),
		      cond->argument1,
		      quoteit(cond->matchwhat));

	    cond = cond->next;

	    if (cond != NULL) fprintf(outfd, " and ");
	  }

	  fprintf(outfd, ") then\n\t  %s %s\n",
		 actionname(rules[i].action),
		 listrule(rules[i].argument2));
	}
	fprintf(outfd, "\n");
}

char *whatname(n)
int n;
{
	static char buffer[10];

	switch(n) {
	  case FROM   : return("from");
	  case TO     : return("to");
	  case SUBJECT: return("subject");
	  case LINES  : return ("lines");
	  case CONTAINS: return("contains");
	  default     : sprintf(buffer, "?%d?", n); return((char *)buffer);
	}
}

char *actionname(n)
int n;
{
	switch(n) {
	  case DELETE_MSG : return("Delete");
	  case SAVE       : return("Save");
	  case SAVECC     : return("Copy and Save");
	  case FORWARD    : return("Forward");
	  case LEAVE      : return("Leave");
	  case EXEC       : return("Execute");
	  default         : return("?action?");
	}
}

int
compare(line, relop, arg)
int line, relop;
char *arg;
{
	/** Given the actual number of lines in the message, the relop
	    relation, and the number of lines in the rule, as a string (!),
   	    return TRUE or FALSE according to which is correct.
	**/

	int rule_lines;

	rule_lines = atoi(arg);

	switch (relop) {
	  case LE: return(line <= rule_lines);
	  case LT: return(line <  rule_lines);
	  case GE: return(line >= rule_lines);
	  case GT: return(line >  rule_lines);
	  case NE: return(line != rule_lines);
	  case EQ: return(line == rule_lines);
	}
	return(-1);
}

char *listrule(rule)
char *rule;
{
	/** simply translates all underscores into spaces again on the
	    way past... **/

	static char buffer[SLEN];
	register int i;

	i = strlen(rule);
	buffer[i] = '\0';
	while (--i >= 0)
	  buffer[i] = (rule[i] == '_' ? ' ' : rule[i]);

	return( (char *) buffer);
}
