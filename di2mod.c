/*
 * Digital_Illusion.c   Copyright (C) 1997 Asle / ReDoX
 *			Copyright (C) 2006-2007 Claudio Matsuoka
 *
 * Converts DI packed MODs back to PTK MODs
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include "prowiz.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
#define MAGIC4(a,b,c,d) \
    (((uint32)(a)<<24)|((uint32)(b)<<16)|((uint32)(c)<<8)|(d))
#define PW_MOD_MAGIC	MAGIC4('M','.','K','.')
#define PW_REQUEST_DATA(s,n) \
	do { if ((s)<(n)) return ((n)-(s)); } while (0)

const uint8 ptk_table[37][2] = {
	{ 0x00, 0x00 },

	{ 0x03, 0x58 },
        { 0x03, 0x28 },
        { 0x02, 0xfa },
        { 0x02, 0xd0 },
        { 0x02, 0xa6 },
        { 0x02, 0x80 },   /*  1  */
        { 0x02, 0x5c },
        { 0x02, 0x3a },
        { 0x02, 0x1a },
        { 0x01, 0xfc },
        { 0x01, 0xe0 },
        { 0x01, 0xc5 },

        { 0x01, 0xac },
        { 0x01, 0x94 },
        { 0x01, 0x7d },
        { 0x01, 0x68 },
        { 0x01, 0x53 },
        { 0x01, 0x40 },   /*  2  */
        { 0x01, 0x2e },
        { 0x01, 0x1d },
        { 0x01, 0x0d },
        { 0x00, 0xfe },
        { 0x00, 0xf0 },
        { 0x00, 0xe2 },

        { 0x00, 0xd6 },
        { 0x00, 0xca },
        { 0x00, 0xbe },
        { 0x00, 0xb4 },
        { 0x00, 0xaa },
        { 0x00, 0xa0 },   /*  3  */
        { 0x00, 0x97 },
        { 0x00, 0x8f },
        { 0x00, 0x87 },
        { 0x00, 0x7f },
        { 0x00, 0x78 },
        { 0x00, 0x71 }
};

int pw_move_data(FILE *out, FILE *in, int len)
{
	uint8 buf[1024];
	int l;

	do {
		l = fread(buf, 1, len > 1024 ? 1024 : len, in);
		fwrite(buf, 1, l, out);
		len -= l;
	} while (l > 0 && len > 0);

	return 0;
}

int pw_write_zero(FILE *out, int len)
{
	uint8 buf[1024];
	int l;
	
	do {
		l = len > 1024 ? 1024 : len;
		memset(buf, 0, l);
		fwrite(buf, 1, l, out);
		len -= l;
	} while (l > 0 && len > 0);

	return 0;
}

uint8 read8(FILE *f)
{
	return (uint8)fgetc(f);
}

uint16 read16b(FILE *f)
{
	uint32 a, b;

	a = read8(f);
	b = read8(f);

	return (a << 8) | b;
}

uint32 read32b(FILE *f)
{
	uint32 a, b, c, d;

	a = read8(f);
	b = read8(f);
	c = read8(f);
	d = read8(f);

	return (a << 24) | (b << 16) | (c << 8) | d;
}

void write8(FILE *f, uint8 b)
{
	fputc(b, f);
}

void write32b(FILE *f, uint32 w)
{
	write8(f, (w & 0xff000000) >> 24);
	write8(f, (w & 0x00ff0000) >> 16);
	write8(f, (w & 0x0000ff00) >> 8);
	write8(f, w & 0x000000ff);
}

void write16b(FILE *f, uint16 w)
{
	write8(f, (w & 0xff00) >> 8);
	write8(f, w & 0x00ff);
}


static int test_di (uint8 *data, int s);
static int depack_di (FILE *in, FILE *out);

#if 0
struct pw_format pw_di = {
	"DI",
	"Digital Illusions",
	0x00,
	test_di,
	depack_di
};
#endif

static int depack_di(FILE * in, FILE * out)
{
	uint8 c1, c2, c3;
	uint8 note, ins, fxt, fxp;
	uint8 ptk_tab[5];
	uint8 nins, npat, max;
	uint8 ptable[128];
	uint16 paddr[128];
	uint8 tmp[50];
	int i, k;
	int seq_offs, pat_offs, smp_offs;
	int pos;
	int size, ssize = 0;

	memset(ptable, 0, 128);
	memset(ptk_tab, 0, 5);
	memset(paddr, 0, 128);

	pw_write_zero(out, 20);			/* title */

	nins = read16b(in);
	seq_offs = read32b(in);
	pat_offs = read32b(in);
	smp_offs = read32b(in);

	for (i = 0; i < nins; i++) {
		pw_write_zero(out, 22);			/* name */
		write16b(out, size = read16b(in));	/* size */
		ssize += size * 2;
		write8(out, read8(in));			/* finetune */
		//write8(out, read8(in));			/* volume */
                {
                uint8 v;
                v = read8(in);
                if (v==0xfc) v=0x40; /* libxmp compat fix */
                write8(out, v);
                }
		write16b(out, read16b(in));		/* loop start */
		write16b(out, read16b(in));		/* loop size */
	}

	tmp[29] = 0x01;
	for (i = nins; i < 31; i++)
		fwrite(tmp, 30, 1, out);

	pos = ftell(in);
	fseek (in, seq_offs, 0);

	i = 0;
	do {
		c1 = read8(in);
		ptable[i++] = c1;
	} while (c1 != 0xff);

	ptable[i - 1] = 0;
	write8(out, npat = i - 1);

	write8(out, 0x7f);

	for (max = i = 0; i < 128; i++) {
		write8(out, ptable[i]);
		if (ptable[i] > max)
			max = ptable[i];
	}

	write32b(out, PW_MOD_MAGIC);

	fseek(in, pos, 0);
	for (i = 0; i <= max; i++)
		paddr[i] = read16b(in);

	for (i = 0; i <= max; i++) {
		fseek (in, paddr[i], 0);
		for (k = 0; k < 256; k++) {	/* 256 = 4 voices * 64 rows */
			memset(ptk_tab, 0, 5);
			c1 = read8(in);
			if ((c1 & 0x80) == 0) {
				c2 = read8(in);
				note = ((c1 << 4) & 0x30) | ((c2 >> 4) & 0x0f);
				ptk_tab[0] = ptk_table[note][0];
				ptk_tab[1] = ptk_table[note][1];
				ins = (c1 >> 2) & 0x1f;
				ptk_tab[0] |= (ins & 0xf0);
				ptk_tab[2] = (ins << 4) & 0xf0;
				fxt = c2 & 0x0f;
				ptk_tab[2] |= fxt;
				fxp = 0x00;
				ptk_tab[3] = fxp;
				fwrite (ptk_tab, 4, 1, out);
				continue;
			}
			if (c1 == 0xff) {
				memset(ptk_tab, 0, 5);
				fwrite (ptk_tab, 4, 1, out);
				continue;
			}
			c2 = read8(in);
			c3 = read8(in);
			note = (((c1 << 4) & 0x30) | ((c2 >> 4) & 0x0f));
			ptk_tab[0] = ptk_table[note][0];
			ptk_tab[1] = ptk_table[note][1];
			ins = (c1 >> 2) & 0x1f;
			ptk_tab[0] |= (ins & 0xf0);
			ptk_tab[2] = (ins << 4) & 0xf0;
			fxt = c2 & 0x0f;
			ptk_tab[2] |= fxt;
			fxp = c3;
			ptk_tab[3] = fxp;
			fwrite(ptk_tab, 4, 1, out);
		}
	}

	fseek(in, smp_offs, 0);
	pw_move_data(out, in, ssize);

	return 0;
}


static int test_di (uint8 *data, int s)
{
	int ssize, start = 0;
	int j, k, l, m, n, o;

	PW_REQUEST_DATA (s, 21);

#if 0
	/* test #1 */
	if (i < 17) {
		Test = BAD;
		return;
	}
#endif

	/* test #2  (number of sample) */
	k = (data[start] << 8) + data[start + 1];
	if (k > 31)
		return -1;

	/* test #3 (finetunes and whole sample size) */
	/* k = number of samples */
	l = 0;
	for (j = 0; j < k; j++) {
		o = (((data[start + 14] << 8) + data[start + 15]) * 2);
		m = (((data[start + 18] << 8) + data[start + 19]) * 2);
		n = (((data[start + 20] << 8) + data[start + 21]) * 2);

		if ((o > 0xffff) || (m > 0xffff) || (n > 0xffff))
			return -1;

		if ((m + n) > o)
			return -1;

		if (data[start + 16 + j * 8] > 0x0f)
			return -1;

		if (data[start + 17 + j * 8] > 0x40)
			return -1;

		/* get total size of samples */
		l += o;
	}
	if (l <= 2)
		return -1;

	/* test #4 (addresses of pattern in file ... ptk_tableible ?) */
	/* k is still the number of sample */

	ssize = k;

	/* j is the address of pattern table now */
	j = (data[start + 2] << 24) + (data[start + 3] << 16)
		+ (data[start + 4] << 8) + data[start + 5];

	/* k is the address of the pattern data */
	k = (data[start + 6] << 24) + (data[start + 7] << 16)
		+ (data[start + 8] << 8) + data[start + 9];

	/* l is the address of the pattern data */
	l = (data[start + 10] << 24) + (data[start + 11] << 16)
		+ (data[start + 12] << 8) + data[start + 13];

	if (k <= j || l <= j || l <= k)
		return -1;

	if ((k - j) > 128)
		return -1;

#if 0
	if (k > in_size || l > in_size || l > in_size)
		return -1;
#endif

	/* test #4,1 :) */
	ssize *= 8;
	ssize += 2;
	if (j < ssize)
		return -1;

#if 0
	/* test #5 */
	if ((k + start) > in_size) {
		Test = BAD;
		return;
	}
#endif

	PW_REQUEST_DATA (s, start + k - 1);

	/* test pattern table reliability */
	for (m = j; m < (k - 1); m++) {
		if (data[start + m] > 0x80)
			return -1;
	}

	/* test #6  ($FF at the end of pattern list ?) */
	if (data[start + k - 1] != 0xFF)
		return -1;

	/* test #7 (addres of sample data > $FFFF ? ) */
	/* l is still the address of the sample data */
	if (l > 65535)
		return -1;

	return 0;
}

int main (int argc, char *argv[])
{
  FILE *ifp, *ofp;
  int size;
  uint8 *buff;
  if (argc != 3)
  {
    puts ("Usage: di2mod in out");
    return 0;
  }
  ifp = fopen (argv[1], "rb");
  if (ifp == NULL)
  {
    puts ("Unable to open input file");
    return 0;
  }
  fseek (ifp, 0, SEEK_SET);
  size = (int) ftell (ifp);
  fseek (ifp, 0, SEEK_SET);
  buff = (uint8 *) malloc (size);
  if (buff == NULL)
  {
    fclose (ifp);
    puts ("Memory error");
    return 0;
  }
  fread (buff, size, 1, ifp);
  if (test_di (buff, size) < 0)
  {
    free (buff);
    fclose (ifp);
    puts ("Not a DI module");
    return 0;
  }
  fseek (ifp, 0, SEEK_SET);
  ofp = fopen (argv[2], "wb");
  if (ofp == NULL)
  {
    free (buff);
    fclose (ifp);
    puts ("Unable to open output file");
    return 0;
  }
  depack_di (ifp, ofp);
  fclose (ofp);
  free (buff);
  fclose (ifp);
  return 0;
}

