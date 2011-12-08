#include <iostream>
#include <cassert>
#include "feature.h"

#define sqrt2 1.414213562373095

using namespace std;

// Feature constructor
Feature::Feature(FILE *fd, char type) {
  Init();
  if(type == 'a') {
    LoadFromAscii(fd);
  } else if(type == 'b') {
    LoadFromBin(fd);
  }
}

Feature::Feature(string filename) {
  Init();
  LoadFile(filename);
}

void Feature::Init() {
  LT = 0;
  LF = 0;
  dT = 0.0;
  dF = 0.0;
  data = NULL;
  LT_size = 0;
  LF_size = 0;
  data_size = 0;
}

bool Feature::ResetFeature() {
  mem_op<double>::delete_2d_array(data);
  Init();
  return true;
}

bool Feature::LoadFile(string filename) {
  /* Check for extension of the file */
  size_t found = filename.find_last_of(".");
  string ext = filename.substr(found+1);

  FILE *fd = FOPEN(filename.c_str(),"r");
  if ( ext == "mfc" || ext == "fbank" ){
    LoadFromHTK(fd);
  }
  else if ( ext == "spec" ){
    LoadFromBin(fd);
  }
  else{
    ErrorExit(__FILE__, __LINE__, 1,"Unknown extension `%s'\n", ext.c_str());
  }
  fclose(fd);
  fname = filename;
  return true;

}


// Feature I/O
bool Feature::WriteToBin(FILE *fo_bin) {
  assert( data != NULL);
  static int sizeInt = sizeof(int);
  static int sizeDouble = sizeof(double);
  assert( fwrite(&LT, sizeInt, 1, fo_bin) == 1 );
  assert( fwrite(&LF, sizeInt, 1, fo_bin) == 1 );
  assert( fwrite(&dT, sizeDouble, 1, fo_bin) == 1 );
  assert( fwrite(&dF, sizeDouble, 1, fo_bin) == 1 );
  assert( fwrite(&window, sizeDouble, 1, fo_bin) == 1 );
  assert( fwrite(data[0], sizeDouble, LT*LF, fo_bin) == unsigned(LT*LF) );
  return true;
}

bool Feature::LoadFromBin(FILE *fi_bin) {
  static int sizeInt = sizeof(int);
  static int sizeDouble = sizeof(double);
  assert( fread(&LT, sizeInt, 1, fi_bin) == 1 );
  assert( fread(&LF, sizeInt, 1, fi_bin) == 1 );
  assert( fread(&dT, sizeDouble, 1, fi_bin) == 1 );
  assert( fread(&dF, sizeDouble, 1, fi_bin) == 1 );
  assert( fread(&window, sizeDouble, 1, fi_bin) == 1 );
  mem_op<double>::reallocate_2d_array(data, LT, LF, LT_size, LF_size, data_size);
  assert( fread(data[0], sizeDouble, LT*LF, fi_bin) == unsigned(LT*LF) );
  return true;
}

bool Feature::WriteToAscii(FILE *fo_ascii) {
  assert( data != NULL);
  bool ret = true;
  if(fprintf(fo_ascii, "LT=%d, LF=%d, dT=%f, dF=%f, window=%f\n", LT, LF, dT, dF, window) < 0) ret = false;
  for(int t=0; t<LT; t++)
  {
    if(fprintf(fo_ascii, "%.2f", data[t][0]) < 0) ret = false;
    for(int f=1;f<LF;f++) if(fprintf(fo_ascii, "\t%.2f", data[t][f]) < 0) ret = false;
    if(fprintf(fo_ascii, "\n") < 0) ret = false;
  }
  return ret;
}

bool Feature::LoadFromAscii(FILE *fi_ascii) {
  bool ret = true;
  if(fscanf(fi_ascii, "LT=%d, LF=%d, dT=%lf, dF=%lf, window=%lf\n",
            &LT, &LF, &dT, &dF, &window) < 0)
    ret = false;

  mem_op<double>::reallocate_2d_array(data, LT, LF, LT_size, LF_size, data_size);
  for(int t=0; t<LT; t++)
  {
    if(fscanf(fi_ascii, "%lf", &data[t][0]) != 1) ret = false;
    for(int f=1;f<LF;f++) if(fscanf(fi_ascii, "\t%lf", &data[t][f]) != 1) ret = false;
    if(fscanf(fi_ascii, "\n" ) != 0) ret = false;
  }
  return ret;
}

bool Feature::LoadFromHTK(FILE *fi_htk) {
  static int sizeInt = sizeof(int);
  static int sizeDouble = sizeof(double);

  int sampPeriod = 0, sampSize = 0, parmKind = 0;

  assert( fread(&LT, 4, 1, fi_htk) == 1 );
  assert( fread(&sampPeriod, 4, 1, fi_htk) == 1 );
  assert( fread(&sampSize, 2, 1, fi_htk) == 1 );
  assert( fread(&parmKind, 2, 1, fi_htk) == 1 );

  if((02000 & parmKind) > 0){
    LF = sampSize/2;
    sampSize = 2;
  }else{
    LF = sampSize/4;
    sampSize = 4;
  }

  dT = double(sampPeriod) * 1e-7;
  dF = window = 0.0;

  mem_op<double>::reallocate_2d_array(data, LT, LF, LT_size, LF_size, data_size);
  /* Uncompressed type */
  if(sampSize == 4){
    if(sizeDouble == sampSize) {
      assert( fread(data[0], sampSize, LT*LF, fi_htk) == unsigned(LT*LF) );
    } else {
      unsigned totnum = LT * LF;
      float *ptr = new float [totnum];
      assert( fread(ptr, sampSize, totnum, fi_htk) == totnum );
      for(unsigned int i = 0; i < totnum; i++)
        data[0][i] = static_cast<double>(ptr[i]);
      delete [] ptr;
    }
  }
  /* Compressed type */
  else if(sampSize == 2){
    if(sizeInt == sampSize){
      int **ptr = mem_op<int>::new_2d_array(LT, LF);
      assert( fread(data[0], sizeInt, LT*LF, fi_htk) == unsigned(LT*LF) );
      for(int t = 0; t < LT; t++){
        for(int d = 0; d < LF; d++)
          data[t][d] = static_cast<double>(ptr[t][d]);
      }
      mem_op<int>::delete_2d_array(ptr);
    }
    else {
      // Not implement yet.
      ErrorExit(__FILE__, __LINE__, 1,"Int size =/= 2, not implement yet\n");
    }
  }
  //cout << "LF = " << LF << ", LT = " << LT << endl;
  return true;
}

void Feature::DumpData() const {
  printf("DATA:\n");
  printf("(LT, LF) = (%d, %d)\n", LT, LF);
  printf("(dT, dF) = (%gs, %gHz)\n", dT, dF);
  printf("window = %gs\n", window);
  printf("PRIVATE:\n");
  printf("(LT_size, LF_size, data_size) = (%d, %d, %d)\n", LT_size, LF_size, data_size);
  for (int t = 0; t < LT; t++){
    printf("%d: ", t);
    for (int f = 0; f < LF; f++){
      printf("%.2f ", data[t][f]);
    }
    printf("\n");
  }
}
