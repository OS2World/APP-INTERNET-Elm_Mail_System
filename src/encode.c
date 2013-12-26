
static char rcsid[] = "@(#)$Id: encode.c,v 4.1 90/04/28 22:42:57 syd Exp $";

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
 * $Log:	encode.c,v $
 * Revision 4.1  90/04/28  22:42:57  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This is a heavily mangled version of the 'cypher' program written by
    person or persons unknown.

**/

#include "headers.h"

#define RS	94
#define RN	4
#define RMASK	0x7fff	/* use only 15 bits */

static char r[RS][RN];		/* rotors */
static char ir[RS][RN];		/* inverse rotors */
static char h[RS];		/* half rotor */
static char s[RS];		/* shuffle vector */
static int  p[RN];		/* rotor indices */

static char the_key[SLEN];	/* unencrypted key */
static char *encrypted_key;	/* encrypted key   */

char *strncpy(), *strcpy();
unsigned long sleep();

#define DECRYPT_PROMPT		"Enter decryption key: "
#define FIRST_ENC_PROMPT	"Enter encryption key: "
#define SECOND_ENC_PROMPT	"Please enter it again: "
#define PROMPT_LINE		LINES-1

getkey(send)
int send;
{
	/** this routine prompts for and returns an encode/decode
	    key for use in the rest of the program. **/

	char buffer[2][NLEN];

	while (1) {
	  PutLine0(PROMPT_LINE, 0, (send ? FIRST_ENC_PROMPT : DECRYPT_PROMPT));
	  CleartoEOLN();
	  optionally_enter(buffer[0], PROMPT_LINE,
	    strlen(send ? FIRST_ENC_PROMPT : DECRYPT_PROMPT), FALSE, TRUE);
	  if (send) {
	    PutLine0(PROMPT_LINE, 0, SECOND_ENC_PROMPT);
	    CleartoEOLN();
	    optionally_enter(buffer[1], PROMPT_LINE, strlen(SECOND_ENC_PROMPT),
	      FALSE, TRUE);
	    if(strcmp(buffer[0], buffer[1]) != 0) {
	      error("Your keys were not the same!");
	      sleep(1);
	      clear_error();
	      continue;
	    }
	  }
	  break;
	}
        strcpy(the_key, buffer[0]);	/* save unencrypted key */
	makekey(buffer[0]);

	setup();		/** initialize the rotors etc. **/

	ClearLine(PROMPT_LINE);
	clear_error();
}

get_key_no_prompt()
{
	/** This performs the same action as get_key, but assumes that
	    the current value of 'the_key' is acceptable.  This is used
	    when a message is encrypted twice... **/

	char buffer[SLEN];

	strcpy(buffer, the_key);

	makekey( buffer );

	setup();
}

encode(line)
char *line;
{
	/** encrypt or decrypt the specified line.  Uses the previously
	    entered key... **/

	register int i, j, ph = 0;

	for (; *line; line++) {
	  i = (int) *line;

	  if ( (i >= ' ') && (i < '~') ) {
	    i -= ' ';

	    for ( j = 0; j < RN; j++ )		/* rotor forwards */
	      i = r[(i+p[j])%RS][j];

	    i = ((h[(i+ph)%RS])-ph+RS)%RS;	/* half rotor */

	    for ( j--  ; j >= 0; j-- )		/* rotor backwards */
	      i = (ir[i][j]+RS-p[j])%RS;

	    j = 0;				/* rotate rotors */
	    p[0]++;
	    while ( p[j] == RS ) {
	      p[j] = 0;
	      j++;
	      if ( j == RN ) break;
	      p[j]++;
            }

	    if ( ++ph == RS )
	      ph = 0;

	    i += ' ';
	  }

	  *line = (char) i;	/* replace with altered one */
	}
}


makekey( rkey)
char *rkey;
{
	/** encrypt the key using the system routine 'crypt' **/

	char key[9], salt[2], *crypt();

	strncpy( key, rkey, 8);
	key[8] = '\0';
	salt[0] = key[0];
	salt[1] = key[1];
#ifdef CRYPT
	encrypted_key = crypt( key, salt);
#else
	encrypted_key = key;
#endif
}

/*
 * shuffle rotors.
 * shuffle each of the rotors indiscriminately.  shuffle the half-rotor
 * using a special obvious and not very tricky algorithm which is not as
 * sophisticated as the one in crypt(1) and Oh God, I'm so depressed.
 * After all this is done build the inverses of the rotors.
 */

setup()
{
	register long i, j, k, temp;
	long seed;

	for ( j = 0; j < RN; j++ ) {
		p[j] = 0;
		for ( i = 0; i < RS; i++ )
			r[i][j] = i;
	}

	seed = 123;
	for ( i = 0; i < 13; i++)		/* now personalize the seed */
	  seed = (seed*encrypted_key[i] + i) & RMASK;

	for ( i = 0; i < RS; i++ )		/* initialize shuffle vector */
	  h[i] = s[i] = i;

	for ( i = 0; i < RS; i++) {		/* shuffle the vector */
	  seed = (5 * seed + encrypted_key[i%13]) & RMASK;;
	  k = ((seed % 65521) & RMASK) % RS;
	  temp = s[k];
	  s[k] = s[i];
	  s[i] = temp;
	}

	for ( i = 0; i < RS; i += 2 ) {	/* scramble the half-rotor */
	  temp = h[s[i]];			/* swap rotor elements ONCE */
	  h[s[i]] = h[s[i+1]];
	  h[s[i+1]] = temp;
	}

	for ( j = 0; j < RN; j++) {			/* select a rotor */

	  for ( i = 0; i < RS; i++) {		/* shuffle the vector */
	    seed = (5 * seed + encrypted_key[i%13]) & RMASK;;
	    k = ((seed % 65521) & RMASK) % RS;
	    temp = r[i][j];
	    r[i][j] = r[k][j];
	    r[k][j] = temp;
	  }

	  for ( i = 0; i < RS; i++) 		/* create inverse rotors */
	    ir[r[i][j]][j] = i;
       }
}
