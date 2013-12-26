/* sndmail.c - wrapper around IBM sendmail
 * for filtering, detaching, temp file deletion and logging
 *
 * Author:  Kai Uwe Rommel <rommel@ars.de>
 * Created: Fri Apr 22 1994
 */

static char *rcsid =
"$Id: sndmail.c,v 1.1 1997/02/02 20:23:36 rommel Exp rommel $";
static char *rcsrev = "$Revision: 1.1 $";

/*
 * $Log: sndmail.c,v $
 * Revision 1.1  1997/02/02 20:23:36  rommel
 * Initial revision
 * 
 */

#include <stdio.h>
#include <string.h>
#include <process.h>
#include <time.h>

#define INCL_NOPM
#include <os2.h>

#include <os2elm.h>

char *strip_parens(char *string)
{
  /**
    Remove parenthesized information from a string.  More specifically,
    comments as defined in RFC822 are removed.  This procedure is
    non-destructive - a pointer to static data is returned.
    **/
  static char  buffer[5120];
  register char *bufp;
  register int depth;

  for ( bufp = buffer, depth = 0 ; *string != '\0' ; ++string ) {
    switch ( *string ) {
    case '(':			/* begin comment on '('		*/
      ++depth;
      break;
    case ')':			/* decr nesting level on ')'	*/
      --depth;
      break;
    case '\\':			/* treat next char literally	*/
      if ( *++string == '\0' ) {		/* gracefully handle	*/
	*bufp++ = '\\';			/* '\' at end of string	*/
	--string;				/* even tho it's wrong	*/
      } else if ( depth == 0 ) {
	*bufp++ = '\\';
	*bufp++ = *string;
      }
      break;
    default:			/* a regular char		*/
      if ( depth == 0 )
	*bufp++ = *string;
      break;
    }
  }
  *bufp = '\0';
  return( (char *) buffer);
}

void get_address_from(char *prefix, char *line, char *buffer)
{
  register char *s;

  /**  Skip start of line over prefix, e.g. "From:".  **/
  line += strlen(prefix);

  /**  If there is a '<' then copy from it to '>' into the buffer.  **/
  if ( (s = strchr(line,'<')) != NULL ) 
  {
    while ( ++s , *s != '\0' && *s != '>' ) 
    {
      if ( !isspace(*s) )
	*buffer++ = *s;
    }
    *buffer = '\0';
    return;
  }

  /**  Otherwise, strip comments and get address with whitespace elided.  **/
  for ( s = strip_parens(line) ; *s != '\0' ; ++s ) 
  {
    if ( !isspace(*s) )
      *buffer++ = *s;
  }
  *buffer = '\0';
}

int nextline(FILE *file, char *buffer, int size)
{
  int len;

  if (fgets(buffer, size, file) == NULL)
    return -1;

  len = strlen(buffer);

  if (buffer[len - 1] == '\n')
    buffer[--len] = 0;

  return len;
}

int scanmail(char *filename, char *from, char *to, char *cc)
{
  char buffer[5120], buffer2[5120];
  FILE *mail;
  int len, bytes = 0, inheader = 1;

  *from = *to = *cc = 0;

  if ((mail = fopen(filename, "r")) != NULL)
  {
    while ((len = nextline(mail, buffer, sizeof(buffer))) != -1)
    {
      bytes += len + 1; /* don't forget \n */

      if (inheader)
      {
	while (buffer[strlen(buffer) - 1] == ',')
	{
	  if ((len = nextline(mail, buffer2, sizeof(buffer2))) == -1)
	    break;

	  strcat(buffer, buffer2);
	  bytes += len + 1;
	}

	if (strnicmp(buffer, "Resent-From:", 12) == 0)
	  get_address_from("Resent-From:", buffer, from);
	if (strnicmp(buffer, "From:", 5) == 0 && *from == 0)
	  get_address_from("From:", buffer, from);

	if (strnicmp(buffer, "Resent-To:", 10) == 0)
	  get_address_from("Resent-To:", buffer, to);
	if (strnicmp(buffer, "To:", 3) == 0 && *to == 0)
	  get_address_from("To:", buffer, to);

	if (strnicmp(buffer, "Resent-Cc:", 10) == 0)
	  get_address_from("Resent-Cc:", buffer, cc);
	if (strnicmp(buffer, "Cc:", 3) == 0 && *cc == 0)
	  get_address_from("Cc:", buffer, cc);

	if (len == 0)
	  inheader = 0;
      }
    }

    fclose(mail);
  }

  return bytes;
}

void logentry(int rc, int bytes, char *from, char *to)
{
  char filename[256], datetime[32];
  FILE *log;
  time_t now;
  struct tm *tm;

  time(&now);
  tm = localtime(&now);
  strftime(datetime, sizeof(datetime), "%m/%d-%H:%M", tm);

  strcpy(filename, logdir);
  strcat(filename, "/");
  strcat(filename, "sndmail.log");

  if ((log = fopen(filename, "a")) != NULL)
  {
    fprintf(log, "%s sent(%d) mail (%d bytes) from %s to %s\n", datetime, rc, bytes, from, to);
    fclose(log);
  }
}
 
int main(int argc, char **argv)
{
  char from[256], to[5120], cc[5120], cmd[1024];
  int rc, bytes, i;

  if (argc == 1)
    return printf("This program is only called internally by a mail application.\n"), 1;

  initpaths();
  
  if (stricmp(argv[1], "-bg") == 0)
  {
    argv[1] = argv[0];
    argv++;

    rc = spawnvp(P_DETACH, argv[0], (char * const *) argv);

    return (rc == -1);
  }

  bytes = from[0] = to[0] = cc[0] = 0;

  if (stricmp(argv[1], "-af") == 0)
  {
    bytes = scanmail(argv[2], from, to, cc);
    if (cc[0])
      strcat(strcat(to, ","), cc);
  }

  if (stricmp(argv[3], "-f") == 0)
    strcpy(from, argv[4]);

  if (stricmp(argv[5], "-t") == 0)
    i = 6; /* even with -t there may be more recipients, such as Bcc: ones */
  else
  {
    to[0] = 0; /* ONLY command line passed recipients */
    i = 5;
  }

  for ( ; i < argc; i++)
  {
    if (to[0] != 0)
      strcat(to, ",");
    strcat(to, argv[i]);
  }

  if (stricmp(argv[1], "-af") == 0)
  {
    sprintf(cmd,"sndfilt %s %s %s", argv[2], from, to);
    system(cmd);
  }

  argv[0] = "sendmail";
  rc = spawnvp(P_WAIT, argv[0], (char * const *) argv);

  if (stricmp(argv[1], "-af") == 0)
    unlink(argv[2]);

  logentry(rc, bytes, from, to);
  
  return rc;
}

/* end of sndmail.c */
