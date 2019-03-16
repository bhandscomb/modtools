/* printmod based on soloins */

/*
MOD FORMAT
==========
20 bytes module title
31 instrument definitions (each 30 bytes)
1 byte number of patterns in play list
1 byte fixed (0x7f)
128 bytes pattern play list
4 bytes magic value "M.K."
np patterns (each 64 rows, 4 channels, 4 bytes per note, thus each 1024 bytes)
sampledata (variable length)

INSTRUMENT DEFINITION FORMAT
============================
22 bytes instrument name
2 bytes sample size in words
1 byte finetune
1 byte volume
2 bytes loop start (words)
2 bytes loop size (words), min = 1

NP CALC
=======
You need to scan the 128 byte play list and find the highest
value. The number of patterns stored in the song is one more
than this value (pattern numbering starts from 0)

SAMPLE DATA
===========
Each sample follows sequentially from the "song data"

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int freq[] =
{
  0x358,0x328,0x2fa,0x2d0,0x2a6,0x280,
  0x25c,0x23a,0x21a,0x1fc,0x1e0,0x1c5,
  0x1ac,0x194,0x17d,0x168,0x153,0x140,
  0x12e,0x11d,0x10d,0x0fe,0x0f0,0x0e2,
  0x0d6,0x0ca,0x0be,0x0b4,0x0aa,0x0a0,
  0x097,0x08f,0x087,0x07f,0x078,0x071
};

char n1[] = "CCDDEFFGGAAB";
char n2[] = "-#-#--#-#-#-";

int main (int argc, char *argv[])
{
  FILE *fp;
  unsigned char *mod, *pl, *insdef, *patt;
  int msize, maxp = 0, n, sampdata, x, r, c, f, unkpitch = -1;
  if (argc != 2)
  {
    puts ("Usage: printmod modfile");
    return 0;
  }
  fp = fopen (argv[1], "rb");
  if (fp == NULL)
  {
    puts ("Unable to open input file");
    return 0;
  }
  fseek (fp, 0, SEEK_END);
  msize = (int) ftell (fp);
  fseek (fp, 0, SEEK_SET);
  mod = (unsigned char *) malloc (msize);
  if (mod == NULL)
  {
    fclose (fp);
    puts ("Memory error");
    return 0;
  }
  fread (mod, msize, 1, fp);
  fclose (fp);
  pl = mod + 0x3b8;
  printf ("PATTERN SEQUENCE");
  for (n = 0; n < mod[0x3b6]; n++)
  {
    if (pl[n] > maxp)
      maxp = pl[n];
    if ((n % 16) == 0)
      putchar ('\n');
    else
      putchar (' ');
    printf ("%02X", pl[n]);
  }
  putchar ('\n');
  sampdata = 0x43c + (maxp + 1) * 0x400;
  insdef = mod + 20;
  for (n = 1; n <= 31; n++)
  {
    int inslen;
    inslen = (((int) insdef[22]) << 9) + (((int) insdef[23]) << 1);
    if (inslen > 0)
    {
      printf
      (
        "INS %02X @ 0x%08X LEN 0x%08X ",
        n,
        sampdata,
        inslen
      );
      for (x = 0; x < 22; x++)
        if (insdef[x] == 0)
          break;
        else
          putchar (insdef[x]);
      putchar ('\n');
      sampdata += inslen;
    }
    insdef += 30;
  }
  patt = mod + 0x43c;
  for (n = 0; n <= maxp; n++)
  {
    printf ("PATTERN %02X\n", n);
    for (r = 0; r < 64; r++)
    {
      int temporow = 0, breakorloop = 0;
      printf ("PATT %02X  %02X  ", n, r);
      for (c = 0; c < 4; c++)
      {
        int n_pitch, n_ins, n_cmd, n_par;
        char note [4];
        // note format
        // 00 11 22 33
        // AB CD EF GH
        // 00 pitch(B), ins-hi(A)
        // 01 pitch(CD)
        // 02 inslo(E), fxcmd(F)
        // 03 fxpar(GH)
        n_ins = (int) patt[0] & 0xf0;
        n_pitch = ((int) patt[0] & 0x0f) << 8;
        n_pitch |= (int) patt[1];
        n_ins |= ((int) patt[2] & 0xf0) >> 4;
        n_cmd = (int) patt[2] & 0x0f;
        n_par = (int) patt[3];
        // Fxx = tempo
        if (n_cmd == 0xf)
          temporow = n_par;
        // Bxx = pattern/position jump
        // Dxx = pattern break
        if ((n_cmd == 0xb) || (n_cmd == 0xd))
          breakorloop = (n_cmd << 8) | n_par;
        // E6x = loop
        if ((n_cmd == 0xe) && ((n_par & 0xf0) == 0x60))
          breakorloop = (n_cmd << 8) | n_par;
        patt += 4;
        strcpy (note, "   ");
        for (f = 0; f < 36; f++)
        {
          if (n_pitch == freq[f])
          {
            note[0] = n1[f % 12];
            note[1] = n2[f % 12];
            note[2] = '1' + f / 12;
            break;
          }
        }
        if (n_pitch == 0)
          strcpy (note, "---");
        if (note[0] == ' ')
        {
          sprintf (note, "%03X", n_pitch);
          unkpitch = n_pitch;
        }
        printf
        (
          "%s %02X %1X%02X",
          note,
          n_ins,
          n_cmd,
          n_par
        );
        if (c < 3)
          printf ("  ");
      }
      if (temporow)
        if (temporow <= 0x0f)
          printf ("  TEMPO! %02X == %.4f", temporow, (float) 750 / temporow);
        else
          printf ("  CIA TEMPO! %02X", temporow);
      if (breakorloop)
        printf ("  BREAK/LOOP == %03X", breakorloop);
      putchar ('\n');
    }
  }
  free (mod);
  if (unkpitch > -1)
    printf ("UNKNOWN PITCH FOUND E.G. %03X", unkpitch);
  return 0;
}
