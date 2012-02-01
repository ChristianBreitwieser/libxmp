/* Extended Module Player
 * Copyright (C) 1996-2012 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "load.h"


static int ssn_test (FILE *, char *, const int);
static int ssn_load (struct xmp_context *, FILE *, const int);

struct xmp_loader_info ssn_loader = {
    "669",
    "Composer 669",
    ssn_test,
    ssn_load
};

static int ssn_test(FILE *f, char *t, const int start)
{
    uint16 id;

    id = read16b(f);
    if (id != 0x6966 && id != 0x4a4e)
	return -1;

    fseek(f, 238, SEEK_CUR);
    if (read8(f) != 0xff)
	return -1;

    fseek(f, 2, SEEK_CUR);
    read_title(f, t, 36);

    return 0;
}



struct ssn_file_header {
    uint8 marker[2];		/* 'if'=standard, 'JN'=extended */
    uint8 message[108];		/* Song message */
    uint8 nos;			/* Number of samples (0-64) */
    uint8 nop;			/* Number of patterns (0-128) */
    uint8 loop;			/* Loop order number */
    uint8 order[128];		/* Order list */
    uint8 tempo[128];		/* Tempo list for patterns */
    uint8 pbrk[128];		/* Break list for patterns */
};

struct ssn_instrument_header {
    uint8 name[13];		/* ASCIIZ instrument name */
    uint32 length;		/* Instrument length */
    uint32 loop_start;		/* Instrument loop start */
    uint32 loopend;		/* Instrument loop end */
};


#define NONE 0xff

/* Effects bug fixed by Miod Vallat <miodrag@multimania.com> */

static uint8 fx[] = {
    FX_PER_PORTA_UP,
    FX_PER_PORTA_DN,
    FX_PER_TPORTA,
    FX_FINETUNE,
    FX_PER_VIBRATO,
    FX_TEMPO_CP
};


static int ssn_load(struct xmp_context *ctx, FILE *f, const int start)
{
    struct xmp_mod_context *m = &ctx->m;
    int i, j;
    struct xmp_event *event;
    struct ssn_file_header sfh;
    struct ssn_instrument_header sih;
    uint8 ev[3];

    LOAD_INIT();

    fread(&sfh.marker, 2, 1, f);	/* 'if'=standard, 'JN'=extended */
    fread(&sfh.message, 108, 1, f);	/* Song message */
    sfh.nos = read8(f);			/* Number of samples (0-64) */
    sfh.nop = read8(f);			/* Number of patterns (0-128) */
    sfh.loop = read8(f);		/* Loop order number */
    fread(&sfh.order, 128, 1, f);	/* Order list */
    fread(&sfh.tempo, 128, 1, f);	/* Tempo list for patterns */
    fread(&sfh.pbrk, 128, 1, f);	/* Break list for patterns */

    m->mod.xxh->chn = 8;
    m->mod.xxh->ins = sfh.nos;
    m->mod.xxh->pat = sfh.nop;
    m->mod.xxh->trk = m->mod.xxh->chn * m->mod.xxh->pat;
    for (i = 0; i < 128; i++)
	if (sfh.order[i] > sfh.nop)
	    break;
    m->mod.xxh->len = i;
    memcpy (m->mod.xxo, sfh.order, m->mod.xxh->len);
    m->mod.xxh->tpo = 6;
    m->mod.xxh->bpm = 76;		/* adjusted using Flux/sober.669 */
    m->mod.xxh->smp = m->mod.xxh->ins;
    m->mod.xxh->flg |= XXM_FLG_LINEAR;

    copy_adjust(m->mod.name, sfh.message, 36);
    strcpy(m->mod.type, strncmp((char *)sfh.marker, "if", 2) ?
				"669 (UNIS 669)" : "669 (Composer 669)");

    MODULE_INFO();

    m->comment = malloc(109);
    memcpy(m->comment, sfh.message, 108);
    m->comment[108] = 0;
    
    /* Read and convert instruments and samples */

    INSTRUMENT_INIT();

    _D(_D_INFO "Instruments: %d", m->mod.xxh->pat);

    for (i = 0; i < m->mod.xxh->ins; i++) {
	m->mod.xxi[i].sub = calloc(sizeof (struct xmp_subinstrument), 1);

	fread (&sih.name, 13, 1, f);		/* ASCIIZ instrument name */
	sih.length = read32l(f);		/* Instrument size */
	sih.loop_start = read32l(f);		/* Instrument loop start */
	sih.loopend = read32l(f);		/* Instrument loop end */

	m->mod.xxi[i].nsm = !!(m->mod.xxs[i].len = sih.length);
	m->mod.xxs[i].lps = sih.loop_start;
	m->mod.xxs[i].lpe = sih.loopend >= 0xfffff ? 0 : sih.loopend;
	m->mod.xxs[i].flg = m->mod.xxs[i].lpe ? XMP_SAMPLE_LOOP : 0;	/* 1 == Forward loop */
	m->mod.xxi[i].sub[0].vol = 0x40;
	m->mod.xxi[i].sub[0].pan = 0x80;
	m->mod.xxi[i].sub[0].sid = i;

	copy_adjust(m->mod.xxi[i].name, sih.name, 13);

	_D(_D_INFO "[%2X] %-14.14s %04x %04x %04x %c", i,
		m->mod.xxi[i].name, m->mod.xxs[i].len, m->mod.xxs[i].lps, m->mod.xxs[i].lpe,
		m->mod.xxs[i].flg & XMP_SAMPLE_LOOP ? 'L' : ' ');
    }

    PATTERN_INIT();

    /* Read and convert patterns */
    _D(_D_INFO "Stored patterns: %d", m->mod.xxh->pat);
    for (i = 0; i < m->mod.xxh->pat; i++) {
	PATTERN_ALLOC (i);
	m->mod.xxp[i]->rows = 64;
	TRACK_ALLOC (i);

	EVENT(i, 0, 0).f2t = FX_TEMPO_CP;
	EVENT(i, 0, 0).f2p = sfh.tempo[i];
	EVENT(i, 1, sfh.pbrk[i]).f2t = FX_BREAK;
	EVENT(i, 1, sfh.pbrk[i]).f2p = 0;

	for (j = 0; j < 64 * 8; j++) {
	    event = &EVENT(i, j % 8, j / 8);
	    fread(&ev, 1, 3, f);

	    if ((ev[0] & 0xfe) != 0xfe) {
		event->note = 1 + 24 + (ev[0] >> 2);
		event->ins = 1 + MSN(ev[1]) + ((ev[0] & 0x03) << 4);
	    }

	    if (ev[0] != 0xff)
		event->vol = (LSN(ev[1]) << 2) + 1;

	    if (ev[2] != 0xff) {
		if (MSN(ev[2]) > 5)
		    continue;

		/* If no instrument is playing on the channel where the
		 * command was encountered, there will be no effect (except
		 * for command 'f', it always changes the tempo). 
		 */
		if (MSN(ev[2] < 5) && !event->ins)
		    continue;

		event->fxt = fx[MSN(ev[2])];

		switch (event->fxt) {
		case FX_PER_PORTA_UP:
		case FX_PER_PORTA_DN:
		case FX_PER_TPORTA:
		    event->fxp = LSN(ev[2]);
		    break;
		case FX_PER_VIBRATO:
		    event->fxp = 0x40 || LSN(ev[2]);
		    break;
		case FX_FINETUNE:
		    event->fxp = 0x80 + (LSN(ev[2]) << 4);
		    break;
		case FX_TEMPO_CP:
		    event->fxp = LSN(ev[2]);
		    event->f2t = FX_PER_CANCEL;
		    break;
		}
	    }
	}
    }

    /* Read samples */
    _D(_D_INFO "Stored samples: %d", m->mod.xxh->smp);

    for (i = 0; i < m->mod.xxh->ins; i++) {
	if (m->mod.xxs[i].len <= 2)
	    continue;
	xmp_drv_loadpatch(ctx, f, m->mod.xxi[i].sub[0].sid,
	    XMP_SMP_UNS, &m->mod.xxs[i], NULL);
    }

    for (i = 0; i < m->mod.xxh->chn; i++)
	m->mod.xxc[i].pan = (i % 2) * 0xff;

    m->quirk |= XMP_QRK_PERPAT;	    /* Cancel persistent fx at each new pat */

    return 0;
}
