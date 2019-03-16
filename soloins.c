/* soloins */

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

int main (int argc, char *argv[])
{
  FILE *fp;
  unsigned char *mod, *pl, *insdef;
  int ins, msize, maxp = 0, n, sampdata, s;
  //
  if (argc != 4)
  {
    puts ("Usage: soloins in out ins");
    return 0;
  }
  //
  ins = (int) strtol (argv[3], NULL, 16);
  //
  fp = fopen (argv[1], "rb");
  if (fp == NULL)
  {
    puts ("Unable to open input file");
    return 0;
  }
  //
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
  //
  fread (mod, msize, 1, fp);
  fclose (fp);
  //
  pl = mod + 0x3b8;
  for (n = 0; n < 128; n++)
    if (pl[n] > maxp)
      maxp = pl[n];
  //
  sampdata = 0x43c + (maxp + 1) * 0x400;
  printf ("maxp = %d\nsampdata @ 0x%08X\n", maxp, sampdata);
  //
  insdef = mod + 20;
  for (n = 1; n <= 31; n++)
  {
printf("%02x\n",insdef[25]);
    int inslen;
    inslen = (((int) insdef[22]) << 9) + (((int) insdef[23]) << 1);
//    if (inslen > 0)
    {
      int x, z = 1;
      printf
      (
        "INS %02X @ 0x%08X LEN 0x%08X ",
        n,
        sampdata,
        inslen
      );
      for (x = 0; x < 22; x++)
        if (insdef[x] == 0)
        {
          z = 0;
          putchar (' ');
        }
        else
        {
          if (z == 0)
            putchar (' ');
          else
            putchar (insdef[x]);
        }
      if (n == ins)
      {
        printf (" SOLO\n");
      }
      else
      {
        printf ("\n");
        for (s = sampdata; s < (sampdata + inslen); s++)
          mod[s] = 0;
      }
      sampdata += inslen;
    }
    insdef += 30;
  }
  //
  fp = fopen (argv[2], "wb");
  if (fp == NULL)
  {
    puts ("Unable to open output file");
    return 0;
  }
  fwrite (mod, msize, 1, fp);
  fclose (fp);
  return 0;
}
