/* REXX: rmailm.cmd */

/* Author:  Kai-Uwe Rommel <rommel@ars.de>
 * Created: Fri Aug 23 1996
 *
 * mail filter for Elm with rmail transport
 *
 * $Id: rmailm.cmd,v 1.2 1997/02/02 10:56:09 rommel Exp rommel $
 * $Revision: 1.2 $
 */

/*
 * $Log: rmailm.cmd,v $
 * Revision 1.2  1997/02/02 10:56:09  rommel
 * handle both remote and local cases
 * 
 */

Parse Arg arg1 arg2 arg3

/* determine the UUCP system name of our local machine */

Queue = RxQueue('create')
Call RxQueue 'Set', Queue
'@uuname -d | rxqueue 'Queue
Parse Pull ourname
Call RxQueue 'Delete', Queue

/* handle rmail with -f argument or input from stdin */

temp = ''

If arg1 = '-f'           /* with file argument */
Then Do
  
  file = arg2
  recipients = arg3
  
End
Else Do        		/* processing stdin */
  
  tempdir = Value('TEMP', , 'OS2ENVIRONMENT')'\'
  temp = SysTempFileName(tempdir'rmail???.tmp')

  Do Forever
  
    line = LineIn('stdin')
  
    If Stream('stdin', 'S') = 'NOTREADY'
    Then Leave
  
    Call LineOut temp, line
  
  End

  Call Stream temp, 'C', 'CLOSE'
  
  file = temp
  recipients = arg1' 'arg2' 'arg3
  
End

/* take care of % expansion for batch files */

Do Forever
  
  percent = Pos('%', recipients)

  If percent = 0
  Then Leave
  
  recipients = Overlay('$', recipients, percent)
  recipients = Insert('$', recipients, percent)
  
End

recipients = Translate(recipients, '%', '$')

/* separate recipients into local and remote ones */

local = ''
remote = ''

Do i=1 To Words(recipients)
  
  addr = Word(recipients, i)
  Parse Var addr username '@' sysname
  
  If sysname = ourname
  Then local = local' 'addr
  Else remote = remote' 'addr
  
End

/* deliver to local recipients */

If remote \= ''
Then Do
  '@call sndfilt 'file
  '@rmail -f 'file' 'remote
End

/* spool to remote recipients */

If local \= ''
Then Do
  '@call rcvfilt 'file
  '@rmail -f 'file' 'local
End

/* clean up */

If temp \= ''
Then Call SysFileDelete temp

/* end of rmailm.cmd */
