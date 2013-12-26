#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <termios.h>
#include <pwd.h>
#include <os2elm.h>

#undef min
#include "..\hdrs\defs.h"

#ifdef __EMX__
#define CCHMAXPATH 256
#else
#undef SHORT
#define INCL_NOPM
#define INCL_SUB
#define INCL_DOS
#include <os2.h>
#endif


char default_editor[SLEN]   = "builtin";
char default_pager[SLEN]    = "builtin";

char default_shell[SLEN]    = "cmd.exe";
char default_printout[SLEN] = "print %s";

char uupchome[CCHMAXPATH]   = "c:/uupc";
char elmhome[CCHMAXPATH]    = "c:/uupc/elm";
char tempdir[CCHMAXPATH]    = "/";
char logdir[CCHMAXPATH]     = "c:/uupc";

char mailer[CCHMAXPATH]     = "rmail";
int background              = 0;

char mailhome[CCHMAXPATH]   = "c:/uupc/mail/";
char mailext[CCHMAXPATH]    = "";
int  maildir                = 0;
char postmaster[SLEN]       = "postmaster";

char _logname[NLEN]         = "unknown";
char _fullname[NLEN]        = "Unknown User";
char _reply_to[SLEN]         = "";
char _homedir[SLEN]         = "c:/uupc/user";

char _hostname[NLEN]        = "local";
char _domainname[SLEN]      = "local.UUCP";
char _fromdomainname[SLEN]  = "";


#ifndef __EMX__
void sleep(int seconds)
{
  DosSleep(1000L * seconds);
}
#endif

int setuid(int id)
{
  return 0;
}

int setgid(int id)
{
  return 0;
}

int getuid(void)
{
  return 0;
}

int getgid(void)
{
  return 0;
}

int chown(char *file, int uid, int gid)
{
  return 0;
}

int link(char *old, char *new)
{
  errno = EXDEV;
  return -1;
}


struct passwd *getpwnam(char *name)
{
  static struct passwd pw;
  static char buffer[256];
  char *ptr, *logname, *fullname, *homedir;
  FILE *passwd;
  int i, found = 0;

  strcpy(buffer, uupchome);
  strcat(buffer, "/passwd");

  pw.pw_name  = _logname;
  pw.pw_dir   = _homedir;
  pw.pw_gecos = _fullname;

  if ( (passwd = fopen(buffer, "r")) == NULL )
    if ( strcmp(name, _logname) == 0 )
      return &pw;
    else
      return NULL;

  while ( fgets(buffer, sizeof(buffer), passwd) != NULL )
  {
    buffer[strlen(buffer) - 1] = 0;

    if ( buffer[0] == '#' )
      continue;

    if ( (ptr = strchr(buffer, ':')) != NULL )
      *ptr++ = 0;
    else
      continue;

    if ( stricmp(name, buffer) == 0 )
    {
      logname = buffer;

      for ( i = 0; i < 3; i++ )
        if ( (ptr = strchr(ptr, ':')) != NULL )
          *ptr++ = 0;
        else
          continue;

      fullname = ptr;

      if ( (ptr = strchr(ptr, ':')) != NULL )
        *ptr++ = 0;
      else
        continue;

      homedir = ptr;

      if (ptr[1] == ';')
	ptr[1] = ':';

      if ( ptr[0] && ptr[1] && (ptr = strchr(ptr + 2, ':')) != NULL )
        *ptr++ = 0;   /* skip drive: */
      
      pw.pw_name  = logname;
      pw.pw_gecos = fullname;
      pw.pw_dir   = homedir;
      found = 1;

      break;
    }
  }

  fclose(passwd);

  if ( !found && strcmp(name, _logname) != 0 )
    return NULL;

  return &pw;
}

struct passwd *getpwuid(int uid)
{
  char *logname = getenv("LOGNAME");
  return getpwnam(logname ? logname : _logname);
}


int gethostname(char *buf, int size)
{
  strcpy(buf, _domainname /* was: _hostname */ );
  return 0;
}

int gethostdomain(char *hostdom, int size)
{
  strcpy(hostdom, _domainname);
  return 0;
}

int getfromdomain(char *hostdom, int size)
{
  strcpy(hostdom, _fromdomainname[0] ? _fromdomainname : _domainname);
  return 0;
}


int tcgetattr(int file, struct termios *buf)
{
  if ( buf )
  {
    buf -> c_cc[0] = 8;
    buf -> c_cc[1] = 127;
  }
  return 0;
}

int tcsetattr(int file, int id, struct termios *buf)
{
  return 0;
}


int readkey(void)
{
  int chr;
#ifdef __EMX__
  #define keycode()    _read_kbd(0, 1, 0)
  #define scancode()   _read_kbd(0, 1, 0)
#else
  KBDKEYINFO ki;
  #define keycode()    (KbdCharIn(&ki, IO_WAIT, 0) ? -1 : ki.chChar)
  #define scancode()   ki.chScan
#endif

again:
  chr = keycode();

  if ( chr == -1 )
  {
    errno = EINTR;
    return -1;
  }

  if ( chr == 0 || chr == 0xE0 )
    switch ( scancode() )
    {
    case 0x48: /* up */
      return 'K';
    case 0x50: /* down */
      return 'J';
    case 0x49: /*page up */
    case 0x4B: /* left */
      return '-';
    case 0x51: /* page down */
    case 0x4D: /* right */
      return '+';
    case 0x47: /* home */
      return '=';
    case 0x4F: /* end */
      return '*';
    case 0x52: /* insert */
      return 'u';
    case 0x92: /* ctrl insert */
      return 'U' - 64;
    case 0x53: /* delete */
      return 'd';
    case 0x93: /* ctrl delete */
      return 'D' - 64;
    case 0x94: /* ctrl tab */
      return 'T' - 64;
    default:
      goto again;
    }

  switch ( chr )
  {
    case 0x09: /* tab */
      return 't';
    case 0x1B: /* escape */
      return 'q';
  }

  return chr;
}


void fixline(char *buffer)
{
  char *end;

  end = buffer + strlen(buffer);

  if ( end - buffer > 1 )
    if ( end[-1] == '\n' && end[-2] == '\r' )
    {
      end[-1] = 0;
      end[-2] = '\n';
    }
}


void _ScreenSize(lines, columns)
int *lines, *columns;
{
#ifdef __EMX__
  int dst[2];
  _scrsize(dst);
  *lines = dst[1];
  *columns = dst[0];
#else
  VIOMODEINFO vmi;
  vmi.cb = sizeof(vmi);
  VioGetMode(&vmi, 0);
  *lines = vmi.row;
  *columns = vmi.col;
#endif
}


void unixpath(char *path)
{
  for ( ; *path; path++ )
    if ( *path == '\\' )
      *path = '/';
}


void os2path(char *path)
{
  for ( ; *path; path++ )
    if ( *path == '/' )
      *path = '\\';
}


#if 0
/* no longer necessary */
void move_incoming_mail(void)
{
  char inbox[CCHMAXPATH], mailbox[CCHMAXPATH];

  if ( stricmp(mailer, "sendmail") == 0 )
  {
    strcpy(inbox, uupchome);
    strcat(inbox, "/mail/inbox.ndx");
    strcpy(mailbox, mailhome);
    strcat(mailbox, _logname);
    movemail(inbox, mailbox);
  }
}
#endif


void parserc(char *rc)
{
  FILE *fp;
  char buffer[SLEN];
  char *ptr;

  if ( rc == NULL )
  {
    printf("Cannot find UUPC system!\n");
    exit(1);
  }

  if ( (fp = fopen(rc, "r")) == NULL )
  {
    printf("Cannot find UUPC configuration file '%s'!\n", rc);
    exit(1);
  }

  while ( fgets(buffer, sizeof(buffer) - 1, fp) )
  {
    buffer[strlen(buffer) - 1] = 0;

    if ( strnicmp(buffer, "NodeName=", 9) == 0 )
      strcpy(_hostname, buffer + 9);
    else if ( strnicmp(buffer, "Domain=", 7) == 0 )
      strcpy(_domainname, buffer + 7);
    else if ( strnicmp(buffer, "FromDomain=", 11) == 0 )
      strcpy(_fromdomainname, buffer + 11);
    else if ( strnicmp(buffer, "Postmaster=", 11) == 0 )
      strcpy(postmaster, buffer + 11);
    else if ( strnicmp(buffer, "ConfDir=", 8) == 0 )
      strcpy(uupchome, buffer + 8);
    else if ( strnicmp(buffer, "SpoolDir=", 9) == 0 )
      strcpy(logdir, buffer + 9);
    else if ( strnicmp(buffer, "TempDir=", 8) == 0 )
      strcpy(tempdir, buffer + 8);
    else if ( strnicmp(buffer, "MailDir=", 8) == 0 )
      strcpy(mailhome, buffer + 8);
    else if ( strnicmp(buffer, "MailExt=", 8) == 0 )
    {
      strcpy(mailext, ".");
      strcat(mailext, buffer + 8);
    }
    else if ( strnicmp(buffer, "Rmail=", 6) == 0 )
      strcpy(mailer, buffer + 6);
    else if ( strnicmp(buffer, "Mailbox=", 8) == 0 )
      strcpy(_logname, buffer + 8);
    else if ( strnicmp(buffer, "Name=", 5) == 0 )
      strcpy(_fullname, buffer + 5);
    else if ( strnicmp(buffer, "ReplyTo=", 8) == 0 )
      strcpy(_reply_to, buffer + 8);
    else if ( strnicmp(buffer, "Home=", 5) == 0 )
      strcpy(_homedir, buffer + 5);
    else if ( strnicmp(buffer, "Editor=", 7) == 0 )
    {
      strcpy(default_editor, buffer + 7);
      if ( (ptr = strchr(default_editor, ' ')) != NULL )
        *ptr = 0;  /* first word only */
    }
    else if ( strnicmp(buffer, "Pager=", 6) == 0 )
    {
      strcpy(default_pager, buffer + 6);
      if ( (ptr = strchr(default_pager, ' ')) != NULL )
        *ptr = 0;
    }
    else if ( strnicmp(buffer, "Options=", 8) == 0 )
    {
      for ( ptr = strtok(buffer + 8, " \t"); ptr; ptr = strtok(NULL, " \t") )
        if ( stricmp(ptr, "directory") == 0 )
          maildir = 1;
        else if ( stricmp(ptr, "background") == 0 )
	  background = 1;
    }
  }

  fclose(fp);
}


void initpaths(void)
{
  char *sysrc = getenv("UUPCSYSRC");
  char *usrrc = getenv("UUPCUSRRC");

  // putenv("TMP=");
  tzset();

  parserc(sysrc);
  if (stricmp(sysrc, usrrc) != 0)
    parserc(usrrc);

  strcpy(elmhome, uupchome);
  strcat(elmhome, "/");
  strcat(elmhome, dotelm);
  
  unixpath(elmhome);
  unixpath(tempdir);
  unixpath(mailhome);

  os2path(default_editor);
  os2path(default_pager);

  if ( tempdir[strlen(tempdir) - 1] != '/' )
    strcat(tempdir, "/");
  if ( mailhome[strlen(mailhome) - 1] != '/' )
    strcat(mailhome, "/");

#if 0
  /* no longer necessary, since gethostname returns the FQDN */
  if ( strnicmp(_hostname, _domainname, strlen(_hostname)) == 0 )
    strcpy(_domainname, _domainname + strlen(_hostname));
#endif

#if 0
  /* with the addition of rcvmail.exe, this is no longer necessary */
  move_incoming_mail();
#endif
}
