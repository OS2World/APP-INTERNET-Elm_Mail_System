/* rcvmail.c - simple delivery agent for IBM sendmail
 *
 * Author:  Kai Uwe Rommel <rommel@ars.de>
 * Created: Mon Jul 22 1996
 */

static char *rcsid =
"$Id: rcvmail.c,v 1.1 1997/02/02 20:23:22 rommel Exp rommel $";
static char *rcsrev = "$Revision: 1.1 $";

/*
 * $Log: rcvmail.c,v $
 * Revision 1.1  1997/02/02 20:23:22  rommel
 * Initial revision
 * 
 */

#include <stdio.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <fcntl.h>

#define INCL_NOPM
#include <os2.h>

#include <os2elm.h>
#include <pwd.h>

char *mailboxdir;
char *recipient;

char *uupc;
char uupcdelimiter[21];

char from[256], to[32768];
int bytes;

char *strip_parens(char *string)
{
  /**
    Remove parenthesized information from a string.  More specifically,
    comments as defined in RFC822 are removed.  This procedure is
    non-destructive - a pointer to static data is returned.
    **/
  static char  buffer[32768];
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

void checkfirst(char *line)
{
  char buffer[5120], *ptr, *timestr;
  time_t now;
  
  if (strncmp(line, "From ", 5) == 0)
  {
    ptr = strchr(line + 5, ' ');

    /* Work around a bug in IBM's sendmail: Sometimes,
       sendmail does not replace $d with the current date.
       Rather, it just drops $d and the line will have
       the two spaces surrounding $d in the `Dl' line of
       sendmail.cf). */

    if (ptr && ptr[0] == ' ' && ptr[1] == ' ')
    {
      ptr[0] = 0;
      ptr += 2;
      time(&now);
      timestr = ctime(&now);
      timestr[strlen(timestr) - 1] = 0; /* kill \n */
      sprintf (buffer, "%s %s %s", line, timestr, ptr);
      strcpy(line, buffer);
    }
  }
}

int scanmail(FILE *mail, FILE *mailbox, char *from, char *to)
{
  char buffer[32768], buffer2[32768];
  int len, bytes = 0, inheader = 1, first = 1;

  *from = *to = 0;

  while ((len = nextline(mail, buffer, sizeof(buffer))) != -1)
  {
    if (first)
    {
      first = 0;
      checkfirst(buffer);
    }

    fprintf(mailbox, "%s\n", buffer);
    bytes += len + 1; /* don't forget \n */

    if (inheader)
    {
      while (buffer[strlen(buffer) - 1] == ',')
      {
	if ((len = nextline(mail, buffer2, sizeof(buffer2))) == -1)
	  break;

	fprintf(mailbox, "%s\n", buffer2);
	strcat(buffer, buffer2);
	bytes += len + 1;
      }

      if (strnicmp(buffer, "Resent-To:", 10) == 0)
	get_address_from("Resent-To:", buffer, to);
      if (strnicmp(buffer, "To:", 3) == 0 && *to == 0)
	get_address_from("To:", buffer, to);

      if (strnicmp(buffer, "Resent-From:", 12) == 0)
	get_address_from("Resent-From:", buffer, from);
      if (strnicmp(buffer, "From:", 5) == 0 && *from == 0)
	get_address_from("From:", buffer, from);

      if (len == 0)
	inheader = 0;
    }
  }

  return bytes;
}

void logentry(char *msg)
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
  strcat(filename, "rcvmail.log");

  if ((log = fopen(filename, "a")) != NULL)
  {
    fprintf(log, "%s %s\n", datetime, msg);
    fclose(log);
  }
}

void success(int bytes, char *from, char *to, char *realto)
{
  char buffer[32768];

  sprintf(buffer, "delivering mail (%d bytes) from %s to %s (%s)", 
	  bytes, from, realto, to);

  logentry(buffer);
}

int deliver(FILE *mail, FILE *file, char *delim)
{
  char buffer[5120];

  rewind(mail);

  if (delim)
    fprintf(file, "%s\n", delim);

  while (fgets(buffer, sizeof(buffer), mail) != NULL)
    if (fputs(buffer, file) == EOF)
      return 1;

  return 0;
}

int deliver_file(FILE *mail, char *name)
{
  char mailbox[256], msg[512];
  FILE *file;
  int rc, i;

  strcpy(mailbox, mailboxdir);
  strcat(mailbox, "/");
  strcat(mailbox, name);

  for (i = 0; i < 5; i++)
    if ((file = fopen(mailbox, "a")) != 0)
      break;
    else
      sleep(1);

  if (file == NULL)
  {
    sprintf(msg, "failed to open to %s", mailbox);
    logentry(msg);
    return 0;
  }

  rc = deliver(mail, file, uupc);

  fclose(file);

  if (rc == 0)
  {
    success(bytes, from, to, name);
    return 1;
  }

  return 0;
}

int deliver_pipe(FILE *mail, char *cmd)
{
  char msg[512];
  FILE *pipe;
  int rc;

  if ((pipe = popen(cmd + 1, "w")) == NULL)
  {
    sprintf(msg, "failed to execute '%s'", cmd + 1);
    logentry(msg);
    return 0;
  }

  rc = deliver(mail, pipe, NULL);

  pclose(pipe);

  if (rc == 0)
  {
    success(bytes, from, to, cmd);
    return 1;
  }

  return 0;
}

int deliver_remote(FILE *mail, char *address)
{
  char filename[256], cmd[512];
  FILE *file;
  int rc, i;

  for (i = 0; i < 5; i++)
  {
    if (tmpnam(filename) != NULL)
      if ((file = fopen(filename, "w")) != 0)
	break;

    sleep(1);
  }

  if (file == NULL)
    return logentry("failed to create temporary file"), 0;

  rc = deliver(mail, file, NULL);

  fclose(file);

  sprintf(cmd, "sndmail -bg -af %s -f %s %s", filename, recipient, address);
  rc = system(cmd);

  if (rc == 0)
  {
    success(bytes, from, to, address);
    return 1;
  }

  return 0;
}

int deliver_user(FILE *mail, char *name)
{
  struct passwd *user;
  char filename[256], buffer[5120];
  FILE *fwd;
  int len, delivered = 0;

  if ((user = getpwnam(name)) == NULL)
  {
    if (stricmp(name, recipient) == 0)
      name = postmaster; /* original recipient unknown */
    else
      return 0; /* else deliver to original recipient */
  }

  if (user && user->pw_dir)
  {
    strcpy(filename, user->pw_dir);
    strcat(filename, "/forward");

    if ((fwd = fopen(filename, "r")) != NULL)
    {
      fcntl(fileno(fwd), F_SETFD, FD_CLOEXEC);

      while (fgets(buffer, sizeof(buffer), fwd) != NULL)
      {
	len = strlen(buffer);

	if (buffer[len - 1] == '\n')
	  buffer[--len] = 0;

	switch (buffer[0])
	{
	case '|':
	  delivered += deliver_pipe(mail, buffer);
	  break;
	case '\\':
	  delivered += deliver_file(mail, buffer + 1);
	  break;
	default:
	  if (stricmp(buffer, recipient) != 0) /* no circular forwarding! */
	  {
	    if (strchr(buffer, '@'))
	      delivered += deliver_remote(mail, buffer);
	    else
	      delivered += deliver_user(mail, buffer);
	  }
	  break;
	}
      }

      fclose(fwd);
    }
  }

  if (!delivered)
    delivered += deliver_file(mail, name);

  return delivered;
}
 
int main(int argc, char **argv)
{
  char mailfile[256], cmd[512];
  FILE *mail;
  int i;
  struct passwd *pwent;

  if (argc != 3 && argc != 4)
    return printf("This program is only called internally from sendmail.\n"), 1;

  initpaths();

  if (stricmp(argv[1], "-u") == 0)
  {
    memset(uupcdelimiter, '\001', 20);
    uupcdelimiter[20] = 0;
    uupc = uupcdelimiter;

    argc--;
    argv++;
  }

  mailboxdir = argv[1];
  recipient = argv[2];

  for (i = 0; i < 5; i++)
  {
    if (tmpnam(mailfile) != NULL)
      if ((mail = fopen(mailfile, "w")) != 0)
	break;

    sleep(1);
  }

  if (mail == NULL)
    return logentry("failed to create temporary file"), 1;

  bytes = scanmail(stdin, mail, from, to);
  fclose(mail);

  sprintf(cmd,"rcvfilt %s %s", mailfile, recipient);
  system(cmd);

  if ((mail = fopen(mailfile, "r")) == 0)
    return logentry("failed to reopen temporary file"), 1;

  fcntl(fileno(mail), F_SETFD, FD_CLOEXEC);

  deliver_user(mail, recipient);

  fclose(mail);
  unlink(mailfile);

  return 0;
}

/* end of rcvmail.c */
