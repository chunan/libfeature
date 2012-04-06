#ifndef _FEATURE_H_
#define _FEATURE_H_

#include <string>
#include "ugoc_utility.h"

using namespace std;

class Feature {
 public:
  // Methods
  Feature() { Init(); }
  Feature(FILE *fd, char type);
  Feature(string filename);
  Feature(float *wav, int len);
  string getFname() { return fname_; }
  void DumpData() const;
  bool LoadFile(const string& filename);
  bool WriteToBin(FILE *fo_bin);
  bool WriteToAscii(FILE *fo_ascii);
  bool ResetFeature();
  ~Feature() {
    ResetFeature();
  }
  /* accessors */
  int LT() const { return data_.R(); }
  int LF() const { return data_.C(); }
  float dT() const { return dT_; }
  float dF() const { return dF_; }
  const TwoDimArray<float>& Data() const { return data_; }
  const float operator()(int t, int f) const {
    return data_.Entry(t, f);
  }
  const float* operator[](int t) const { return data_.Vec(t); }

 private:
  bool LoadFromBin(FILE *fi_bin);
  bool LoadFromHTK(FILE *fi_htk);
  bool LoadFromAscii(FILE *fi_ascii);
  void Init();
  // Data
  string fname_;
  float dT_;
  float dF_;
  float window_;
  TwoDimArray<float> data_;
};


#endif /*_FEATURE_H_ */
