/* REXX: rcvfilt.cmd */

/* Author:  Kai-Uwe Rommel <rommel@ars.de>
 * Created: Fri Aug 23 1996
 *
 * inbound mail filter for Elm with sendmail transport
 *
 * $Id: rcvfilt.cmd,v 1.5 1996/10/13 10:19:53 rommel Exp rommel $
 * $Revision: 1.5 $
 */

/*
 * $Log: rcvfilt.cmd,v $
 * Revision 1.5  1996/10/13 10:19:53  rommel
 * added comments
 *
 * Revision 1.4  1996/09/03 12:41:38  rommel
 * add support for multi-part messages
 *
 * Revision 1.3  1996/09/03 07:20:00  rommel
 * fix quotes for charset= in Content-Type: lines
 *
 * Revision 1.2  1996/08/30 08:56:46  rommel
 * earlier exit if nothing to do
 *
 * Revision 1.1  1996/08/28 20:52:58  rommel
 * Initial revision
 *
 * 
 */

/* Latin-1 to cp850 recoding only for german umlauts at the moment */

cp850  = 'ÑîÅéôö·'
latin1 = 'e4f6fcc4d6dcdf'X

Parse Arg file receiver

file = Translate(file, '\', '/')
path = FileSpec('d', file) || FileSpec('p', file)
name = FileSpec('n', file)
temp = path'flt'SubStr(name, 4)

header = 1
multi  = ''
quoted = 0
is8859 = 0

Do Forever
  
  line = LineIn(file)
  
  If Stream(file, 'S') = 'NOTREADY'
  Then Leave
  
  If header = 1
  Then Do
    
    If line = ''
    Then Do
      
      header = 0
      
      /* header is done, now body follows */
      
      If quoted = 0 & is8859 = 0 & multi = ''
      Then Leave /* nothing to do */
	
    End
    Else Do
      
      line = Translate(line, ' ', '09'X)
      
      Parse Upper Var line key val1 val2
      
      /* check MIME encoding */
      
      If key = 'CONTENT-TRANSFER-ENCODING:' & val1 = 'QUOTED-PRINTABLE'
      Then Do
	quoted = 1
	Iterate
      End
      
      /* check MIME character set */
      
      If key = 'CONTENT-TYPE:' & val1 = 'TEXT/PLAIN;'
      Then Do
	
	Parse Var val2 'CHARSET=' val2
	
	If Left(val2, 1) = """"
	Then Parse Var val2 """" val2 """"
	
	If val2 = 'ISO-8859-1'
	Then Do
	  is8859 = 1
	  line = 'Content-Type: text/plain; charset="ibm850"'
	End

      End
      
      /* check for MIME multipart messages */
      
      If key = 'CONTENT-TYPE:' & val1 = 'MULTIPART/MIXED;'
      Then Do
	
	Parse Var val2 'BOUNDARY="' multi '"'
	
      End
      
    End
    
  End
  Else Do
    
    /* check for boundary of MIME multipart messages */
    
    If multi \= '' & Right(line, Length(multi)) = multi
    Then Do
      
      /* this body is done, next header follows */
      
      header = 1
      quoted = 0
      is8859 = 0
      
    End
    
    /* undo MIME encoding */
    
    If quoted = 1
    Then Do
      
      /* check for MIME line continuation marks */
      
      Do While Right(line, 1) = '='
	line = Left(line, Length(line) - 1) || LineIn(file)
      End
      
      /* decode all characters */
  
      Do Forever
    
	x = Pos('=', line)
    
	If x = 0
	Then Leave
    
	repl = SubStr(line, x + 1, 2)
	
	/* in case even line breaks are MIME encoded, the CR's are
	   discarded (if present) and only the LF's are decoded */
    
	Select
	  When repl = '0D' Then repl = ''
	  When repl = '3D' Then repl = '00'X
	  Otherwise repl = X2C(repl)
	End
    
	line = Left(line, x - 1) || repl || SubStr(line, x + 3)
    
      End
      
      /* any = characters got a special treatment above (because the
	 = is the quoting character) that needs to be handled now */
  
      line = Translate(line, '=', '00'X)
    
    End
    
    /* convert to IBM 850 character set */
    
    If is8859 = 1
    Then Do
      
      line = Translate(line, cp850, latin1)
      
    End
    
  End

  Call LineOut temp,line
    
End

Call Stream file, 'C', 'CLOSE'
Call Stream temp, 'C', 'CLOSE'

/* if there was nothing really to do, forget everything */

If quoted = 0 & is8859 = 0 & multi = ''
Then Do
  '@del 'temp
End
Else Do
  '@del 'file
  '@ren 'temp' 'name
End
  
/* end of rcvfilt.cmd */
