
;
; Maciej 'YTM/Alliance' Witkowiak
;
; 25.12.99

; char SaveFile  (struct fileheader *myHeader);

	    .export _SaveFile

	    .include "../inc/jumptab.inc"
	    .include "../inc/geossym.inc"
	
_SaveFile:
	sta r9L
	stx r9H
	jsr SaveFile
	stx errno
	txa
	rts
