#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <iomanip>
#include "../timing.h"
#include "../ispc_malloc.h"
#include "mergeSort_ispc.h"

/* progress bar by Ross Hemsley;
 * http://www.rosshemsley.co.uk/2011/02/creating-a-progress-bar-in-c-or-any-other-console-app/ */
static inline void progressbar (unsigned int x, unsigned int n, unsigned int w = 50)
{
  if (n < 100)
  {
    x *= 100/n;
    n = 100;
  }

  if ((x != n) && (x % (n/100) != 0)) return;

  using namespace std;
  float ratio  =  x/(float)n;
  int c =  ratio * w;

  cout << setw(3) << (int)(ratio*100) << "% [";
  for (int x=0; x<c; x++) cout << "=";
  for (int x=c; x<w; x++) cout << " ";
  cout << "]\r" << flush;
}

#include "keyType.h"
struct Key
{
  Key_t key;
  Val_t val;
};


int main (int argc, char *argv[])
{
  int i, j, n = argc == 1 ? 1024*1024: atoi(argv[1]), m = n < 100 ? 1 : 50, l = n < 100 ? n : RAND_MAX;
  double tISPC1 = 0.0, tISPC2 = 0.0, tSerial = 0.0;

  Key *keys = new Key[n];
  srand48(rtc()*65536);
#pragma omp parallel for
  for (int i = 0; i < n; i++)
  {
    keys[i].key = i; //((int)(drand48() * (1<<30)));
    keys[i].val = i;
  }
  std::random_shuffle(keys, keys + n);

  Key_t *keysSrc = new Key_t[n];
  Val_t *valsSrc = new Val_t[n];
  Key_t *keysBuf = new Key_t[n];
  Val_t *valsBuf = new Val_t[n];
  Key_t *keysDst = new Key_t[n];
  Val_t *valsDst = new Val_t[n];
  Key_t *keysGld = new Key_t[n];
  Val_t *valsGld = new Val_t[n];
#pragma omp parallel for
  for (int i = 0; i < n; i++)
  {
    keysSrc[i] = keys[i].key;
    valsSrc[i] = keys[i].val;

    keysGld[i] = keysSrc[i];
    valsGld[i] = valsSrc[i];
  }
  delete keys;
    
  ispcSetMallocHeapLimit(1024*1024*1024);

  ispc::openMergeSort();

  tISPC2 = 1e30;
  for (i = 0; i < m; i ++)
  {
    ispcMemcpy(keysSrc, keysGld, n*sizeof(Key_t));
    ispcMemcpy(valsSrc, valsGld, n*sizeof(Val_t));

    reset_and_start_timer();
    ispc::mergeSort(keysDst, valsDst, keysBuf, valsBuf, keysSrc, valsSrc, n);
    tISPC2 = std::min(tISPC2, get_elapsed_msec());

    if (argc != 3)
        progressbar (i, m);
  }

  ispc::closeMergeSort();

  printf("[sort ispc + tasks]:\t[%.3f] msec [%.3f Mpair/s]\n", tISPC2, 1.0e-3*n/tISPC2);

#if 0
  printf("\n---\n");
  for (int i = 0; i < 128; i++)
  {
    if ((i%32) == 0) printf("\n");
    printf("%d ", (int)keysSrc[i]);
  }
  printf("\n---\n");
  for (int i = 0; i < 128; i++)
  {
    if ((i%32) == 0) printf("\n");
    printf("%d ", (int)keysBuf[i]);
  }
  printf("\n---\n");
  for (int i = 0; i < 128; i++)
  {
    if ((i%32) == 0) printf("\n");
    printf("%d ", (int)keysDst[i]);
  }
  printf("\n---\n");
#endif
    


  std::sort(keysGld, keysGld + n);
  for (int i = 0; i < n; i++)
    assert(keysDst[i] == keysGld[i]);

  delete keysSrc;
  delete valsSrc;
  delete keysDst;
  delete valsDst;
  delete keysBuf;
  delete valsBuf;
  delete keysGld;
  delete valsGld;

  return 0;
}
