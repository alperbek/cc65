/*
 * ace.h
 *
 * Ullrich von Bassewitz, 06.06.1998
 *
 */



#ifndef _ACE_H
#define _ACE_H



#ifndef _STDDEF_H
#include <stddef.h>
#endif



struct aceDirentBuf {
    unsigned long   ad_size;   		/* Size in bytes */
    unsigned char   ad_date [8];   	/* YY:YY:MM:DD:HH:MM:SS:TW */
    char            ad_type [4];	/* File type as ASCIIZ string */
    unsigned char   ad_flags;	        /* File flags */
    unsigned char   ad_usage;           /* More flags */
    unsigned char   ad_namelen;         /* Length of name */
    char            ad_name [17];       /* Name itself, ASCIIZ */
};

int aceDirOpen (char* dir);
int aceDirClose (int handle);
int aceDirRead (int handle, struct aceDirentBuf* buf);

/* Type of an ACE key. Key in low byte, shift mask in high byte */
typedef unsigned int aceKey;

/* #defines for the shift mask returned by aceConGetKey */
#define aceSH_KEY		0x00FF	/* Mask key itself */
#define aceSH_MASK		0xFF00	/* Mask shift mask */
#define aceSH_EXT		0x2000	/* Extended key */
#define aceSH_CAPS 		0x1000	/* Caps lock key */
#define aceSH_ALT  		0x0800	/* Alternate key */
#define aceSH_CTRL		0x0400	/* Ctrl key */
#define aceSH_CBM		0x0200	/* Commodore key */
#define aceSH_SHIFT		0x0100	/* Shift key */

/* #defines for the options in aceConSetOpt/aceConGetOpt */
#define aceOP_PUTMASK		1	/* Console put mask */
#define	aceOP_CHARCOLOR		2	/* Character color */
#define aceOP_CHARATTR		3	/* Character attribute */
#define aceOP_FILLCOLOR		4	/* Fill color */
#define aceOP_FILLATTR		5	/* Fill attribute */
#define aceOP_CRSCOLOR		6	/* Cursor color */
#define aceOP_CRSWRAP		7	/* Force cursor wrap */
#define aceOP_SHSCROLL		8	/* Shift keys for scrolling */
#define aceOP_MOUSCALE		9	/* Mouse scaling */
#define aceOP_RPTDELAY		10	/* Key repeat delay */
#define aceOP_RPTRATE		11	/* Key repeat rate */

/* Console functions */
void aceConWrite (char* buf, size_t count);
void aceConPutLit (int c);
void aceConPos (unsigned x, unsigned y);
void aceConGetPos (unsigned* x, unsigned* y);
unsigned aceConGetX (void);
unsigned aceConGetY (void);
char* aceConInput (char* buf, unsigned initial);
int aceConStopKey (void);
aceKey aceConGetKey (void);
int aceConKeyAvail (aceKey* key);
void aceConKeyMat (char* matrix);
void aceConSetOpt (unsigned char opt, unsigned char val);
int aceConGetOpt (unsigned char opt);

/* Misc stuff */
int aceMiscIoPeek (unsigned addr);
void aceMiscIoPoke (unsigned addr, unsigned char val);



/* End of ace.h */
#endif



