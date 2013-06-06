/* Extended Module Player
 * Copyright (C) 1996-2013 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU Lesser General Public License. See COPYING.LIB
 * for more information.
 */

#include <stdio.h>
#include "loader.h"
#include "iff.h"

#define MAGIC_FORM	MAGIC4('F','O','R','M')
#define MAGIC_EMOD	MAGIC4('E','M','O','D')
#define MAGIC_EMIC	MAGIC4('E','M','I','C')


static int emod_test (HIO_HANDLE *, char *, const int);
static int emod_load (struct module_data *, HIO_HANDLE *, const int);

const struct format_loader emod_loader = {
    "Quadra Composer (EMOD)",
    emod_test,
    emod_load
};

static int emod_test(HIO_HANDLE *f, char *t, const int start)
{
    if (hio_read32b(f) != MAGIC_FORM)
	return -1;

    hio_read32b(f);

    if (hio_read32b(f) != MAGIC_EMOD)
	return -1;

    if (hio_read32b(f) == MAGIC_EMIC) {
        hio_read32b(f);		/* skip size */
        hio_read16b(f);		/* skip version */
        read_title(f, t, 20);
    } else {
        read_title(f, t, 0);
    }

    return 0;
}


static void get_emic(struct module_data *m, int size, HIO_HANDLE *f, void *parm)
{
    struct xmp_module *mod = &m->mod;
    int i, ver;
    uint8 reorder[256];

    ver = hio_read16b(f);
    hio_read(mod->name, 1, 20, f);
    hio_seek(f, 20, SEEK_CUR);
    mod->bpm = hio_read8(f);
    mod->ins = hio_read8(f);
    mod->smp = mod->ins;

    m->quirk |= QUIRK_MODRNG;

    snprintf(mod->type, XMP_NAME_SIZE, "Quadra Composer EMOD v%d", ver);
    MODULE_INFO();

    INSTRUMENT_INIT();

    for (i = 0; i < mod->ins; i++) {
	mod->xxi[i].sub = calloc(sizeof (struct xmp_subinstrument), 1);

	hio_read8(f);		/* num */
	mod->xxi[i].sub[0].vol = hio_read8(f);
	mod->xxs[i].len = 2 * hio_read16b(f);
	hio_read(mod->xxi[i].name, 1, 20, f);
	mod->xxs[i].flg = hio_read8(f) & 1 ? XMP_SAMPLE_LOOP : 0;
	mod->xxi[i].sub[0].fin = hio_read8(f);
	mod->xxs[i].lps = 2 * hio_read16b(f);
	mod->xxs[i].lpe = mod->xxs[i].lps + 2 * hio_read16b(f);
	hio_read32b(f);		/* ptr */

	mod->xxi[i].nsm = 1;
	mod->xxi[i].sub[0].pan = 0x80;
	mod->xxi[i].sub[0].sid = i;

	D_(D_INFO "[%2X] %-20.20s %05x %05x %05x %c V%02x %+d",
		i, mod->xxi[i].name, mod->xxs[i].len, mod->xxs[i].lps,
		mod->xxs[i].lpe, mod->xxs[i].flg & XMP_SAMPLE_LOOP ? 'L' : ' ',
		mod->xxi[i].sub[0].vol, mod->xxi[i].sub[0].fin >> 4);
    }

    hio_read8(f);			/* pad */
    mod->pat = hio_read8(f);

    mod->trk = mod->pat * mod->chn;

    PATTERN_INIT();

    memset(reorder, 0, 256);

    for (i = 0; i < mod->pat; i++) {
	reorder[hio_read8(f)] = i;
	PATTERN_ALLOC(i);
	mod->xxp[i]->rows = hio_read8(f) + 1;
	TRACK_ALLOC(i);
	hio_seek(f, 20, SEEK_CUR);		/* skip name */
	hio_read32b(f);			/* ptr */
    }

    mod->len = hio_read8(f);

    D_(D_INFO "Module length: %d", mod->len);

    for (i = 0; i < mod->len; i++)
	mod->xxo[i] = reorder[hio_read8(f)];
}


static void get_patt(struct module_data *m, int size, HIO_HANDLE *f, void *parm)
{
    struct xmp_module *mod = &m->mod;
    int i, j, k;
    struct xmp_event *event;
    uint8 x;

    D_(D_INFO "Stored patterns: %d", mod->pat);

    for (i = 0; i < mod->pat; i++) {
	for (j = 0; j < mod->xxp[i]->rows; j++) {
	    for (k = 0; k < mod->chn; k++) {
		event = &EVENT(i, k, j);
		event->ins = hio_read8(f);
		event->note = hio_read8(f) + 1;
		if (event->note != 0)
		    event->note += 48;
		event->fxt = hio_read8(f) & 0x0f;
		event->fxp = hio_read8(f);

		/* Fix effects */
		switch (event->fxt) {
		case 0x04:
		    x = event->fxp;
		    event->fxp = (x & 0xf0) | ((x << 1) & 0x0f);
		    break;
		case 0x09:
		    event->fxt <<= 1;
		    break;
		case 0x0b:
		    x = event->fxt;
		    event->fxt = 16 * (x / 10) + x % 10;
		    break;
		}
	    }
	}
    }
}


static void get_8smp(struct module_data *m, int size, HIO_HANDLE *f, void *parm)
{
    struct xmp_module *mod = &m->mod;
    int i;

    D_(D_INFO "Stored samples : %d ", mod->smp);

    for (i = 0; i < mod->smp; i++) {
	load_sample(m, f, 0, &mod->xxs[i], NULL);
    }
}


static int emod_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
    iff_handle handle;

    LOAD_INIT();

    hio_read32b(f);		/* FORM */
    hio_read32b(f);
    hio_read32b(f);		/* EMOD */

    handle = iff_new();
    if (handle == NULL)
	return -1;

    /* IFF chunk IDs */
    iff_register(handle, "EMIC", get_emic);
    iff_register(handle, "PATT", get_patt);
    iff_register(handle, "8SMP", get_8smp);

    /* Load IFF chunks */
    while (!hio_eof(f)) {
	iff_chunk(handle, m, f, NULL);
    }

    iff_release(handle);

    return 0;
}
