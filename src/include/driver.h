/* Extended Module Player
 * Copyright (C) 1996-2007 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 *
 * $Id: driver.h,v 1.19 2007-10-20 18:42:37 cmatsuoka Exp $
 */

#ifndef __XMP_DRIVER_H
#define __XMP_DRIVER_H

#include "xmpi.h"

#define XMP_PATCH_FM	-1

#if !defined(DRIVER_OSS_SEQ) && !defined(DRIVER_OSS_MIX)

#define GUS_PATCH 1

struct patch_info {
	unsigned short key;
	short device_no;		/* Synthesizer number */
	short instr_no;			/* Midi pgm# */
	unsigned int mode;
	int len;			/* Size of the wave data in bytes */
	int loop_start, loop_end;	/* Byte offsets from the beginning */
	unsigned int base_freq;
	unsigned int base_note;
	unsigned int high_note;
	unsigned int low_note;
	int panning;			/* -128=left, 127=right */
	int detuning;
	int volume;
	char data[1];			/* The waveform data starts here */
};

#endif

/* Sample flags */
#define XMP_SMP_DIFF		0x01	/* Differential */
#define XMP_SMP_UNS		0x02	/* Unsigned */
#define XMP_SMP_8BDIFF		0x04
#define XMP_SMP_7BIT		0x08
#define XMP_SMP_NOLOAD		0x10	/* Get from buffer, don't load */
#define XMP_SMP_8X		0x20
#define XMP_SMP_BIGEND		0x40	/* Big-endian */
#define XMP_SMP_VIDC		0x80	/* Archimedes VIDC logarithmic */

#define XMP_ACT_CUT		XXM_NNA_CUT
#define XMP_ACT_CONT		XXM_NNA_CONT
#define XMP_ACT_OFF		XXM_NNA_OFF
#define XMP_ACT_FADE		XXM_NNA_FADE

#define XMP_CHN_ACTIVE		0x100
#define XMP_CHN_DUMB		-1

#define parm_init() for (parm = o->parm; *parm; parm++) { \
	token = strtok (*parm, ":="); token = strtok (NULL, "");
#define parm_end() }
#define parm_error() { \
	fprintf (stderr, "xmp: incorrect parameters in -D %s\n", *parm); \
	exit (-4); }
#define chkparm1(x,y) { \
	if (!strcmp (*parm,x)) { \
	    if (token == NULL) parm_error ()  else { y; } } }
#define chkparm2(x,y,z,w) { if (!strcmp (*parm,x)) { \
	if (2 > sscanf (token, y, z, w)) parm_error (); } }


/* PROTOTYPES */

void	xmp_drv_register	(struct xmp_drv_info *);
int	xmp_drv_open		(struct xmp_context *);
int	xmp_drv_set		(struct xmp_context *);
void	xmp_drv_close		(struct xmp_context *);
int	xmp_drv_on		(struct xmp_context *, int);
void	xmp_drv_off		(struct xmp_context *);
void	xmp_drv_mute		(int, int);
int	xmp_drv_flushpatch	(struct xmp_context *, int);
int	xmp_drv_writepatch	(struct xmp_context *, struct patch_info *);
int	xmp_drv_setpatch	(struct xmp_context *, int, int, int, int, int, int, int, int);
int	xmp_drv_cvt8bit		(void);
int	xmp_drv_crunch		(struct patch_info **, unsigned int);
void	xmp_drv_setsmp		(struct xmp_context *, int, int);
void	xmp_drv_setnna		(struct xmp_context *, int, int);
void	xmp_drv_pastnote	(struct xmp_context *, int, int);
void	xmp_drv_retrig		(struct xmp_context *, int);
void	xmp_drv_setvol		(struct xmp_context *, int, int);
void	xmp_drv_voicepos	(struct xmp_context *, int, int);
void	xmp_drv_setbend		(struct xmp_context *, int, int);
void	xmp_drv_setpan		(struct xmp_context *, int, int);
void	xmp_drv_seteffect	(struct xmp_context *, int, int, int);
int	xmp_drv_cstat		(struct xmp_context *, int);
void	xmp_drv_resetchannel	(struct xmp_context *, int);
void	xmp_drv_reset		(struct xmp_context *);
double	xmp_drv_sync		(struct xmp_context *, double);
int	xmp_drv_getmsg		(struct xmp_context *);
void	xmp_drv_stoptimer	(struct xmp_context *);
void	xmp_drv_clearmem	(struct xmp_context *);
void	xmp_drv_starttimer	(struct xmp_context *);
void	xmp_drv_echoback	(struct xmp_context *, int);
void	xmp_drv_bufwipe		(struct xmp_context *);
void	xmp_drv_bufdump		(struct xmp_context *);
int	xmp_drv_loadpatch 	(struct xmp_context *, FILE *, int, int, int,
				 struct xxm_sample *, char *);

void xmp_init_drivers (void);
struct xmp_drv_info *xmp_drv_array (void);

#endif /* __XMP_DRIVER_H */
