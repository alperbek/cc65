/*
 * fputc.c
 *
 * Ullrich von Bassewitz, 02.06.1998
 */



#include <stdio.h>
#include <fcntl.h>
#include "_file.h"



int fputc (int c, FILE* f)
{
    /* Check if the file is open or if there is an error condition */
    if ((f->f_flags & _FOPEN) == 0 || (f->f_flags & (_FERROR | _FEOF)) != 0) {
    	return -1;
    }

    /* Write the byte (knows about byte order!) */
    if (write (f->f_fd, &c, 1) <= 0) {
   	/* Error */
	f->f_flags |= _FERROR;
	return -1;
    }

    /* Return the byte written */
    return c & 0xFF;
}



