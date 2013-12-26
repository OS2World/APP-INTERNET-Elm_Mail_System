/* REXX: sndfilt.cmd */

/* Author:  Kai-Uwe Rommel <rommel@ars.de>
 * Created: Fri Aug 23 1996
 *
 * outbound mail filter for Elm with sendmail transport
 *
 * $Id: sndfilt.cmd,v 1.10 1997/01/26 18:43:39 rommel Exp rommel $
 * $Revision: 1.10 $
 */

/*
 * $Log: sndfilt.cmd,v $
 * Revision 1.10  1997/01/26 18:43:39  rommel
 * do no longer distinguish between local and foreign recipients
 *
 * Revision 1.9  1997/01/26 18:24:22  rommel
 * fixed tab character quoting, if no other 8-bit characters
 *
 * Revision 1.8  1996/10/13 10:02:37  rommel
 * do not encode if there are only 7-bit characters
 *
 * Revision 1.7  1996/09/11 06:28:25  rommel
 * fix duplication of MIME header lines
 *
 * Revision 1.6  1996/09/05 05:49:07  rommel
 * add forgotten Mime-Version header
 *
 * Revision 1.5  1996/09/04 13:37:03  rommel
 * removed debug line
 *
 * Revision 1.4  1996/09/04 13:35:47  rommel
 * added forgotten Latin-1 translation (for german umlauts only for now)
 * encoding now for all non-7-bit-printable characters
 *
 * Revision 1.3  1996/09/04 08:08:23  rommel
 * use MIME encoding instead of transscription for german umlauts
 *
 * Revision 1.2  1996/08/28 20:52:34  rommel
 * fixed trailing empty line buglet
 *
 * Revision 1.1  1996/08/23 16:51:52  rommel
 * Initial revision
 * 
 */

/* cp850 to Latin-1 recoding only for german umlauts at the moment */

cp850  = 'ÑîÅéôö·'
latin1 = 'e4f6fcc4d6dcdf'X

Parse Arg file sender receivers

file = Translate(file, '\', '/')
path = FileSpec('d', file) || FileSpec('p', file)
name = FileSpec('n', file)
temp = path'flt'SubStr(name, 4)

header = 1
changes = 0

Do Forever

  line = LineIn(file)
  
  If Stream(file, 'S') = 'NOTREADY'
  Then Leave

  If header = 1
  Then Do
    
    line = Translate(line, ' ', '09'X)
    Parse Upper Var line key val1 val2
    
    /* remove old MIME header lines */
    
    If key = 'CONTENT-TRANSFER-ENCODING:' | key = 'CONTENT-TYPE:' | key = 'MIME-VERSION:'
    Then Iterate
    
    If line = ''
    Then Do
      
      header = 0
      
      /* write new MIME lines */

      Call LineOut temp,'Mime-Version: 1.0'      
      Call LineOut temp,'Content-Type: text/plain; charset="iso-8859-1"'
      Call LineOut temp,'Content-Transfer-Encoding: quoted-printable'
      
    End
    
  End
  Else Do
    
    line = Encode(Translate(line, latin1, cp850))
    
  End
  
  Call LineOut temp,line

End

Call Stream file, 'C', 'CLOSE'
Call Stream temp, 'C', 'CLOSE'

/* if there were no 8-bit characters, forget everything */

If changes = 0
Then Do
  '@del 'temp
End
Else Do
  '@del 'file
  '@ren 'temp' 'name
End

Exit

Encode: Procedure Expose changes

  Parse Arg string
  
  Do x = Length(string) To 1 By -1
    
    c = SubStr(string, x, 1)
    
    If C2D(c) < 32 | 126 < C2D(c) | c = '='
    Then Do
      
      string = Left(string, x - 1) || '=' || C2X(c) || SubStr(string, x + 1)
      
      /* = is a 7-bit character and would really need to be encoded
	 but it is the quoting character, so it needs to be encoded in the
	 case that _other_ characters need to be encoded, otherwise not */
      
      If c \= '=' & C2D(c) \= 9
      Then changes = changes + 1
      
    End
    
  End
  
Return string

/* end of sndfilt.cmd */
