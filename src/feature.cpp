#include <iostream>
#include <cassert>
#include "feature.h"

#define sqrt2 1.414213562373095

using namespace std;

const int sizeInt = sizeof(int);
const int sizeFloat = sizeof(float);

// Feature constructor
Feature::Feature(FILE *fd, char type) {
  Init();
  if (type == 'a') {
    LoadFromAscii(fd);
  } else if (type == 'b') {
    LoadFromBin(fd);
  }
}

Feature::Feature(string filename) {
  Init();
  LoadFile(filename);
}

void Feature::Init() {
  fname_.clear();
  dT_ = 0.0;
  dF_ = 0.0;
  window_ = 0.0;
}

bool Feature::ResetFeature() {
  Init();
  return true;
}

bool Feature::LoadFile(const string& filename) {
  /* Check for extension of the file */
  size_t found = filename.find_last_of(".");
  string ext = filename.substr(found + 1);

  FILE *fd = FOPEN(filename.c_str(), "r");
  if (ext == "mfc" || ext == "fbank" || ext == "plp") {
    LoadFromHTK(fd);
  }
  else if (ext == "spec") {
    LoadFromBin(fd);
  }
  else{
    ErrorExit(__FILE__, __LINE__, 1,"Unknown extension `%s'\n", ext.c_str());
  }
  fclose(fd);
  fname_ = filename;
  return true;
}


// Feature I/O
bool Feature::WriteToBin(FILE *fo_bin) {
  assert(data_[0] != NULL);
  size_t s_write;
  int ival;

  ival = LT();
  s_write = fwrite(&ival, sizeInt, 1, fo_bin);
  assert(s_write == 1);
  ival = LF();
  s_write = fwrite(&ival, sizeInt, 1, fo_bin);
  assert(s_write == 1);
  s_write = fwrite(&dT_, sizeFloat, 1, fo_bin);
  assert(s_write == 1);
  s_write = fwrite(&dF_, sizeFloat, 1, fo_bin);
  assert(s_write == 1);
  s_write = fwrite(&window_, sizeFloat, 1, fo_bin);
  assert(s_write == 1);
  s_write = fwrite(data_[0], sizeFloat, LT() * LF(), fo_bin);
  assert(static_cast<int>(s_write) == LT() * LF());
  return true;
}

bool Feature::LoadFromBin(FILE *fi_bin) {
  size_t s_read;
  int len_t, len_f;
  s_read = fread(&len_t, sizeInt, 1, fi_bin);
  assert(s_read == 1);
  s_read = fread(&len_f, sizeInt, 1, fi_bin);
  assert(s_read == 1);
  s_read = fread(&dT_, sizeFloat, 1, fi_bin);
  assert(s_read == 1);
  s_read = fread(&dF_, sizeFloat, 1, fi_bin);
  assert(s_read == 1);
  s_read = fread(&window_, sizeFloat, 1, fi_bin);
  assert(s_read == 1);
  data_.Resize(len_t, len_f);
  s_read = fread(data_[0], sizeFloat, LT() * LF(), fi_bin);
  assert(static_cast<int>(s_read) == LT() * LF());
  return true;
}

bool Feature::WriteToAscii(FILE *fo_ascii) {
  assert(data_[0] != NULL);
  bool ret = true;
  if (fprintf(fo_ascii, "LT=%d, LF=%d, dT=%f, dF=%f, window=%f\n",
              LT(), LF(), dT_, dF_, window_) < 0) ret = false;
  for (int t = 0; t < LT(); ++t) {
    if (fprintf(fo_ascii, "%.2f", data_(t, 0)) < 0)
      ret = false;
    for (int f = 1; f < LF(); ++f) {
      if (fprintf(fo_ascii, "\t%.2f", data_(t, f)) < 0) ret = false;
    }
    if (fprintf(fo_ascii, "\n") < 0) ret = false;
  }
  return ret;
}

bool Feature::LoadFromAscii(FILE *fi_ascii) {
  bool ret = true;
  int len_t, len_f;
  if (fscanf(fi_ascii, "LT=%d, LF=%d, dT=%f, dF=%f, window=%f\n",
             &len_t, &len_f, &dT_, &dF_, &window_) < 0) {
    ret = false;
  }

  data_.Resize(len_t, len_f);
  for (int t = 0; t < LT(); ++t) {
    if (fscanf(fi_ascii, "%f", &data_[t][0]) != 1)
      ret = false;
    for (int f = 1; f < LF(); ++f) {
      if (fscanf(fi_ascii, "\t%f", &data_[t][f]) != 1) {
        ret = false;
      }
    }
    if (fscanf(fi_ascii, "\n") != 0) ret = false;
  }
  return ret;
}

bool Feature::LoadFromHTK(FILE *fi_htk) {

  int sampPeriod = 0, sampSize = 0, parmKind = 0;
  int len_t, len_f;
  size_t s_read;

  s_read = fread(&len_t, 4, 1, fi_htk);
  assert(s_read == 1);
  s_read = fread(&sampPeriod, 4, 1, fi_htk);
  assert(s_read == 1);
  s_read = fread(&sampSize, 2, 1, fi_htk);
  assert(s_read == 1);
  s_read = fread(&parmKind, 2, 1, fi_htk);
  assert(s_read == 1);

  if ((02000 & parmKind) > 0) {
    len_f = sampSize / 2;
    sampSize = 2;
  } else {
    len_f = sampSize / 4;
    sampSize = 4;
  }

  dT_ = static_cast<float>(sampPeriod) * 1e-7;
  dF_ = window_ = 0.0;

  data_.Resize(len_t, len_f);
  /* Uncompressed type */
  if (sampSize == 4) {
    if (sizeFloat == sampSize) {
      s_read = fread(data_[0], sampSize, len_t * len_f, fi_htk);
      assert(static_cast<int>(s_read) == len_t * len_f);
    } else {
      unsigned totnum = len_t * len_f;
      float *ptr = new float [totnum];
      assert(fread(ptr, sampSize, totnum, fi_htk) == totnum);
      for (unsigned int i = 0; i < totnum; i++) {
        data_(0, i) = static_cast<float>(ptr[i]);
      }
      delete [] ptr;
    }
  }
  /* Compressed type */
  else if (sampSize == 2) {
    if (sizeInt == sampSize) {
      int **ptr = mem_op<int>::new_2d_array(len_t, len_f);
      s_read = fread(data_[0], sizeInt, len_t*len_f, fi_htk);
      assert(static_cast<int>(s_read) == len_t * len_f);
      for (int t = 0; t < len_t; t++) {
        for (int f = 0; f < len_f; f++) {
          data_(t, f) = static_cast<float>(ptr[t][f]);
        }
      }
      mem_op<int>::delete_2d_array(&ptr);
    }
    else {
      // Not implement yet.
      ErrorExit(__FILE__, __LINE__, 1,"Int size =/= 2, not implement yet\n");
    }
  }
  return true;
}

void Feature::DumpData() const {
  printf("DATA:\n");
  printf("(dT, dF) = (%gs, %gHz)\n", dT_, dF_);
  printf("window = %gs\n", window_);
  printf("PRIVATE:\n");
  cout << data_;
}
