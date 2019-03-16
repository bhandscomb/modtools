/*
 * ProRunner2.c   Copyright (C) 1996-1999 Asle / ReDoX
 *                Copyright (C) 2006-2007 Claudio Matsuoka
 *
 * Converts ProRunner v2 packed MODs back to Protracker
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

static int test_pru2 (uint8 *, int);
static int depack_pru2 (FILE *, FILE *);

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

inline uint8 read8(FILE *f)
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

inline void write8(FILE *f, uint8 b)
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

#if 0
struct pw_format pw_pru2 = {
	"PRU2",
	"Prorunner 2.0",
	0x00,
	test_pru2,
	depack_pru2
};
#endif

static int depack_pru2(FILE *in, FILE *out)
{
	uint8 header[2048];
	uint8 c1, c2, c3, c4;
	uint8 npat;
	uint8 ptable[128];
	uint8 max = 0;
	uint8 v[4][4];
	int size, ssize = 0;
	int i, j;

	memset(header, 0, 2048);
	memset(ptable, 0, 128);

	pw_write_zero(out, 20);				/* title */

	fseek(in, 8, SEEK_SET);

	for (i = 0; i < 31; i++) {
		pw_write_zero(out, 22);			/*sample name */
		write16b(out, size = read16b(in));	/* size */
		ssize += size * 2;
		write8(out, read8(in));			/* finetune */
		write8(out, read8(in));			/* volume */
		write16b(out, read16b(in));		/* loop start */
		write16b(out, read16b(in));		/* loop size */
	}

	write8(out, npat = read8(in));			/* number of patterns */
	write8(out, read8(in));				/* noisetracker byte */

	for (i = 0; i < 128; i++) {
		write8(out, c1 = read8(in));
		max = (c1 > max) ? c1 : max;
	}

	write32b(out, PW_MOD_MAGIC);

	/* pattern data stuff */
	fseek(in, 770, SEEK_SET);

	for (i = 0; i <= max; i++) {
		for (j = 0; j < 256; j++) {
			c1 = c2 = c3 = c4 = 0;
			header[0] = read8(in);
			if (header[0] == 0x80) {
				write32b(out, 0);
			} else if (header[0] == 0xC0) {
				fwrite(v[0], 4, 1, out);
				c1 = v[0][0];
				c2 = v[0][1];
				c3 = v[0][2];
				c4 = v[0][3];
			} else if (header[0] != 0xC0 && header[0] != 0xC0) {
				header[1] = read8(in);
				header[2] = read8(in);

				c1 = (header[1] & 0x80) >> 3;
				c1 |= ptk_table[(header[0] >> 1)][0];
				c2 = ptk_table[(header[0] >> 1)][1];
				c3 = (header[1] & 0x70) << 1;
				c3 |= (header[0] & 0x01) << 4;
				c3 |= (header[1] & 0x0f);
				c4 = header[2];

				write8(out, c1);
				write8(out, c2);
				write8(out, c3);
				write8(out, c4);
			}

			/* rol previous values */
			v[0][0] = v[1][0];
			v[0][1] = v[1][1];
			v[0][2] = v[1][2];
			v[0][3] = v[1][3];

			v[1][0] = v[2][0];
			v[1][1] = v[2][1];
			v[1][2] = v[2][2];
			v[1][3] = v[2][3];

			v[2][0] = v[3][0];
			v[2][1] = v[3][1];
			v[2][2] = v[3][2];
			v[2][3] = v[3][3];

			v[3][0] = c1;
			v[3][1] = c2;
			v[3][2] = c3;
			v[3][3] = c4;
		}
	}

	/* sample data */
	pw_move_data(out, in, ssize);

	return 0;
}


static int test_pru2 (uint8 *data, int s)
{
	int k;
	int start = 0;

	PW_REQUEST_DATA(s, 12 + 31 * 8);

	if (data[0]!='S' || data[1]!='N' || data[2]!='T' || data[3]!='!')
		return -1;

#if 0
	/* check sample address */
	j = (data[i + 4] << 24) + (data[i + 5] << 16) + (data[i + 6] << 8) +
		data[i + 7];

	PW_REQUEST_DATA (s, j);
#endif

	/* test volumes */
	for (k = 0; k < 31; k++) {
		if (data[start + 11 + k * 8] > 0x40)
			return -1;
	}

	/* test finetunes */
	for (k = 0; k < 31; k++) {
		if (data[start + 10 + k * 8] > 0x0F)
			return -1;
	}

	return 0;
}

int main (int argc, char *argv[])
{
  FILE *ifp, *ofp;
  int size;
  uint8 *buff;
  if (argc != 3)
  {
    puts ("Usage: pru22mod in out");
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
  if (test_pru2 (buff, size) < 0)
  {
    free (buff);
    fclose (ifp);
    puts ("Not a Prorunner 2.0 module");
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
  depack_pru2 (ifp, ofp);
  fclose (ofp);
  free (buff);
  fclose (ifp);
  return 0;
}
