/*
**	Copyright (c) 2004 Actian Corporation
*/

# include	<te.h>
# include	<cmcl.h>
# include	<st.h>
# include	<me.h>
# include	<ex.h>
# include	<si.h>
# include	<termios.h>

/*
**
**	SIhistgetrec
**
**	Similar to SIgetrec. Get next input record for terminal monitor. 
**	Simulates a line editor which maintains command history using arrow keys. 
**
**	Get the next record from stream, previously opened for reading.
**	SIgetrec reads n-1 characters, or up to a full record, whichever
**	comes first, into buf.  The last char is followed by a newline.
**	On unix, a record on a file is a string of bytes terminated by a newline.  
**	On systems with formatted files, a record may be defined differently.
**
**	This file maintains command history in a doubly linked list. 
**
**	This file defines following functions:
**
**	SIhistgetrec() similar to SIgetrec(), but has line editing capability
**		using arrow keys and also recollects history of lines typed.
**	SIgetprevhist() gets the previous line in the history.
**	SIgetnxthist() gets the next line in the history.
**	SIaddhistnode()	adds a line to the doubly linked list containing history.
**	SIclearhistory() clears the doubly-linked list memory.
**	SIeraseline() erases the current line being edited.
**	SIeraselineend() erases the current line from cursor till end.
**
**	History
**	31-mar-2004 (kodse01)
**		SIR 112068
**		Written.
**	29-apr-2004 (kodse01)
**		Fixed handling of special keys at buffer boundary condition.
**		Added support for control-u key to erase complete line.
**		Added support for control-k key to erase line from cursor
**		till end.
**	27-Jun-2007 (kiria01) b110040
**		Executing \i caused the SIclearhistory() but did not
**		clear the hist pointer thereby meaning that the next command
**		added to the list would get the old block reallocated via
**		the stale hist pointer. This resulted in the block pointing
**		to itself and a cpu loop waiting to happen.
**	20-Apr-2010 (hanje04)
**	    SIR 123622
**	    Add control-c (interupt) and contol-d (quit) support.
**	    Replace references to int with i4.
**	15-nov-2010 (stephenb)
**	    Include si/h and correctly define funcs for prototyping.
**	12-Aug-2011 (kiria01) b125060
**	    Work in support for UTF8 line editing.
*/

# define	MAX_BUF_LENGTH	512 /* maximum buffer length */

# ifndef	EBCDIC
# define	ESC		'\033'
# define	BS		'\010'
# define	DEL		'\177'
# define	NAK		'\025'
# define	VT		'\013'
# define        ETX             '\003'
# define        EOT             '\004'
# else
# define	ESC		'\047'
# define	BS		'\026'
# define	DEL		'\007'
# define	NAK		'\075'
# define	VT		'\013'
# define        ETX             '\003'
# define        EOT             '\004'
# endif

/* Structure defining the doubly linked list node to maintain history */
typedef struct historynode
{ 
	char his_str[MAX_BUF_LENGTH]; 
	struct historynode *fptr; 
	struct historynode *bptr;
} HISTORYNODE;

/* Doubly linked list master pointer */
HISTORYNODE *hist = NULL;  /* points to the current history line */

int curline = 1; /* 1 if in the current line, 0 if in history line */

/* gets the previous line in the history */
static void 
SIgetprevhist(
	char *buf)
{ 
	if (hist != NULL) 
	{ 
		if (curline) 
			curline = 0; 
		else if (hist->bptr != NULL) 
			hist = hist->bptr; 
		STcopy(hist->his_str, buf); 
	}
	else
		*buf = EOS;
}

/* gets the next line in the history */
static void 
SIgetnxthist(
	char *buf)
{
	if (hist != NULL)
	{
		if (hist->fptr != NULL) 
		{ 
			hist = hist->fptr; 
			STcopy(hist->his_str, buf); 
		} 
		else 
			curline = 1; 
	}
	else
		*buf = EOS;
}

/* adds a line to the doubly linked list containing history */
static i4 
SIaddhistnode(
	char *buf)
{ 
	HISTORYNODE *tnode; 
	if ((buf != NULL) && (STlength(buf) != 0)) 
	{ 
		if ((tnode = (HISTORYNODE *) MEreqmem (0, sizeof(HISTORYNODE), 
			TRUE, NULL)) == NULL) 
			return -1; 
		tnode->bptr = tnode->fptr = NULL; 
		STcopy(buf, tnode->his_str); 
		if (hist != NULL) 
		{ 
			while (hist->fptr != NULL) 
				hist = hist->fptr; 
			hist->fptr = tnode; 
			tnode->bptr = hist; 
			hist = hist->fptr; 
		} 
		else 
			hist = tnode; 
	} 
	return 0;
}

/* clears the doubly-linked list memory */
void 
SIclearhistory(void)
{ 
	HISTORYNODE *temp; 
	if (hist != NULL) 
	{ 
		while (hist->fptr != NULL) 
			hist = hist->fptr; 
		while (hist->bptr != NULL) 
		{ 
			temp = hist; 
			hist = hist->bptr; 
			MEfree((PTR)temp); 
		} 
		MEfree((PTR)hist); 
		/* Having released last buffer - don't use it again */
		hist = NULL;
	}
}

/* Determine whether character has full character display width.
** Most just use single (1/2 Em space) width but Ideographs such
** as used in Japanese may use full cell (1 Em space). We have to
** know if we are backing up past such a character with backspace
** as one might not be enough! */
static bool
SIch_is_wide(
	char *p,
	i4 len)
{
	return CMischarset_utf8() &&
		CM_UTF8_twidth(p, p + len) > 1;

}

/* erases the current line being edited */
static void 
SIeraseline(
	char *octets,
	char *charlen,
	i4 cur_char,
	i4 n_chars)
{
	i4 i, count;

	for (i = count = 0; i < n_chars; i++)
	{
	    i4 len = charlen[i];
	    if (i < cur_char)
	    {
		SIprintf("\b");
		count++;
		if (SIch_is_wide(octets, len))
		{
		    count++;
		    SIprintf("\b");
		}
	    }
	    else if (SIch_is_wide(octets, len))
		count += 2;
	    else
		count++;
	    octets += len;
	}

	for (i = 0; i < count; i++)
		SIprintf(" ");
	for (i = 0; i < count; i++)
		SIprintf("\b");
}

/* erases the current line from cursor till end */
static void 
SIeraselineend(
	char *octets,
	char *charlen,
	i4 cur_char,
	i4 n_chars)
{
	i4 i, count;

	for (i = count = 0; i < n_chars; i++)
	{
	    i4 len = charlen[i];
	    if (i == cur_char)
		count = 0;
	    if (SIch_is_wide(octets, len))
		count++;
	    count++;
	    octets += len;
	}

	for (i = 0; i < count; i++)
		SIprintf(" ");
	for (i = 0; i < count; i++)
		SIprintf("\b");
}

static void
SIsetup_char_lengths(
	const char *buf,
	char *lens,
	i4 *n_chars)
{
    const char *p = buf;
    i4 cp = 0;

    while (*p)
    {
	i4 clen = CMbytecnt(p);
	lens[cp++] = clen;
	p += clen;
    }
    *n_chars = cp;
}

/*{
**  NAME: SIHISTGETREC -- terminal monitors version of SIgetrec which contains 
**		command history feature.
**  Description:
**	This function sets the terminal settings to capture arrow keys. The 
**	up-arrow and down-arrow are used to navigate through the history. The 
**	left-arrow, right-arrow and backspace (^H) are used to edit the line.
**	
**	Known limitations:
**		Uses ANSI sequence of characters for arrow keys. Editing of line is
**	limited to a single line on the screen.
**	
**	History
**	31-mar-2004 (kodse01)
**		Written.
**	29-apr-2004 (kodse01)
**		Fixed handling of special keys at buffer boundary condition.
**		Added support for control-u key to erase complete line.
**		Added support for control-k key to erase line from cursor till end.
*/

STATUS
SIhistgetrec(
	char	*buf,		/* read record into buf and null terminate */
	i4	n,		/* (max number of characters - 1) to be read */
	FILE	*stream)	/* get record from file stream */
{
	struct termios new_tty_termios, org_tty_termios;

	/* Raw byte data will be read into 'octets' and managed with
	** n_octets as active length and cur_octet which will index
	** into 'octets' at the start of the current character */
	char octets[MAX_BUF_LENGTH];
	i4 n_octets = 0;	/* Octet length of current line */
	i4 cur_octet = 0;	/* Index into current line (octets) */
	/* As each character (poss multibyte) is read into 'octets',
	** the character length is tracked in 'charlen'. 'charlen' is
	** managed by n_chars tracking current character count and
	** cur_char. Needless to say that in a multibyte environment
	** n_chars will not necessarily be the same as n_octets and
	** similar for cur_octet & cur_char */
	char charlen[MAX_BUF_LENGTH];	/* Character byte lengths */
	i4 n_chars = 0;		/* Character length of current line */
	i4 cur_char = 0;	/* The current character position */
	i4 next_ch = 0;		/* Next character from stream */
	i4 i = 0;
	i4 count;

	if (TEsettmattr() < 0)
		return ENDFILE;

	if (n > MAX_BUF_LENGTH)
	{
		TEresettmattr();
		return ENDFILE;
	}

	while ((next_ch != EOF) && (next_ch != '\n') && (next_ch != '\r'))
	{
		next_ch = SIgetc(stream);
		if (next_ch == EOF) break;

		if (next_ch == ESC)
		{
			next_ch = SIgetc(stream);
			if (next_ch == EOF) break;
			if (next_ch == '[')
			{
				next_ch = SIgetc(stream);
				if (next_ch == EOF) break;
				switch (next_ch)
				{
				case 'A': /* Up arrow pressed */
				/* erase the line and display previous line */
					SIeraseline(octets, charlen, cur_char, n_chars);
					SIgetprevhist(octets);
					SIsetup_char_lengths(octets, charlen, &n_chars);
					cur_char = n_chars;
					n_octets = cur_octet = STlength(octets);
					SIprintf("%s", octets);
					break;
				case 'B': /* Down arrow pressed */
				/* if next line available erase the line and display next line*/
					SIeraseline(octets, charlen, cur_char, n_chars);
					SIgetnxthist(octets);
					SIsetup_char_lengths(octets, charlen, &n_chars);
					cur_char = n_chars;
					n_octets = cur_octet = STlength(octets);
					SIprintf("%s", octets);
					break;
				case 'C': /* Right arrow pressed */
				/* if in the middle of line, go forward. else do nothing */
					if (cur_octet < n_octets)
					{
						SIprintf("%*.*s",
							charlen[cur_char],
							charlen[cur_char],
							&octets[cur_octet]);
						cur_octet += charlen[cur_char++];
					}
					break;
				case 'D': /* Left arrow pressed */
				/* if start of line, do nothing, else go backward */
					if (cur_octet > 0)
					{
						cur_char--;
						cur_octet -= charlen[cur_char];
						SIprintf("\b");
						if (SIch_is_wide(octets + cur_octet,
								charlen[cur_char]))
							SIprintf("\b");
					}
					break;
				default:
				/* ignore other function keys */
					break;
				}
			/* ignore other function keys */
			}
		}
		else if (next_ch == BS || next_ch == DEL) /* Backspace, Delete */
		{
			/*Delete the previous char if available*/
			if (cur_octet > 0)
			{
				i4 lostsz = charlen[cur_char - 1];
				char *nxt;
				octets[n_octets] = EOS;
				if (SIch_is_wide(octets + cur_octet - lostsz, lostsz))
					SIprintf("\b");
				nxt = &octets[cur_octet];
				SIprintf("\b%s  \b\b", nxt);
				for (i = cur_char; i < n_chars; i++)
				{
					SIprintf("\b");
					if (SIch_is_wide(nxt, charlen[i]))
						SIprintf("\b");
					nxt += charlen[i];
				}
				if (n_octets > cur_octet)
					memmove(octets + cur_octet - lostsz,
						octets + cur_octet,
						n_octets - cur_octet);
				cur_octet -= lostsz;
				n_octets -= lostsz;
				octets[n_octets] = EOS;
				if (n_chars > cur_char)
					memmove(charlen + cur_char - 1,
						charlen + cur_char,
						n_chars - cur_char);
				cur_char--;
				n_chars--;
			}
		}
		else if (next_ch == NAK) /* control-u */
		{
			SIeraseline(octets, charlen, cur_char, n_chars);	
			n_octets = cur_octet = 0;
			n_chars = cur_char = 0;
		}
		else if (next_ch == VT) /* control-k */
		{
			SIeraselineend(octets, charlen, cur_char, n_chars);	
			n_octets = cur_octet;
			n_chars = cur_char;
		}
		else if (next_ch == ETX) /* control-c */
		{
		        TEresettmattr();
			/* raise an interrupt */
			EXsignal(EXINTR, 0);
		}
		else if (next_ch == EOT) /* control-c */
		{
		        TEresettmattr();
			/* raise an interrupt */
			return(ENDFILE);
		}
		else if (n_octets >= n-1)
		{
			SIungetc(next_ch, stream);
			break;
		}
		else if (next_ch == '\t') /* Tab */
		{
			/* Ignore tab operation */
		}
		else if (next_ch == '\n' || next_ch == '\r')
		{
			octets[n_octets++] = next_ch;
			octets[n_octets] = EOS;
			SIprintf("%s", octets+cur_octet);
			cur_octet = n_octets;
			charlen[n_chars++] = 1;
			cur_char = n_chars;
			curline = 1;
		}
		else if (CMcntrl(&next_ch))
		{
			/* Ignore other control characters like ^L */
		}
		else  /* All other characters -- printable */
		{
			char tmpch[32];
			i4 clen = CMbytecnt(&next_ch), i = 0;
			
			/* Store multi-byte input */
			tmpch[i++] = next_ch;
			while (i < clen)
			{
			    next_ch = SIgetc(stream);
			    if (next_ch == EOF) break;
			    tmpch[i++] = next_ch;
			}
			if (next_ch == EOF) break;

			tmpch[i++] = EOS;
			if (cur_octet >= n_octets)
			{
				/* Output char */
				SIprintf("%s", tmpch);
			}
			else
			{
				char *nxt = &octets[cur_octet];
				/* Output new char plus rest of line */
				octets[n_octets] = EOS;
				SIprintf("%s%s", tmpch, nxt);
				/* Back space n CHARACTERS */
				for (i = cur_char; i < n_chars; i++)
				{
					SIprintf("\b");
					if (SIch_is_wide(nxt, charlen[i]))
						SIprintf("\b");
					nxt += charlen[i];
				}
				/* Make space for inserted char in the buffers */
				memmove(octets + cur_octet + clen, octets + cur_octet,
					    n_octets - cur_octet);
				memmove(charlen + cur_char + 1, charlen + cur_char,
					    n_chars - cur_char);
			}
			/* Append/insert to input buffer */
			memcpy(octets + cur_octet, tmpch, clen);
			cur_octet += clen;
			n_octets += clen;
			octets[n_octets] = EOS;

			/* Remember char len */
			charlen[cur_char] = clen;
			n_chars++;
			cur_char++;
			curline = 1;
		}
	}	/*end of while loop */

	if ((n_octets >= n-1) && next_ch != '\n' && next_ch != '\r')
		SIprintf("\n");
	
	if (next_ch == EOF)
	{
		TEresettmattr();
		return ENDFILE;
	}

	octets[n_octets++] = 0;
	memcpy(buf, octets, n_octets); /* set output parameter */
	/* donot put newline in history */
	if (octets[n_octets - 2] == '\n')
		octets[n_octets - 2] = EOS;
	STtrmwhite(octets); /* Remove trailing spaces */
	SIaddhistnode(octets); /* Add line to the history at the end */

	if (TEresettmattr() < 0)
		return ENDFILE;

	return OK;
}

