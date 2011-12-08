#ifndef _FEATURE_H_
#define _FEATURE_H_

#include "ugoc_utility.h"

using namespace std;

class Feature {
 public:
  // Methods
  Feature() { Init(); }
  Feature(FILE *fd, char type);
  Feature(string filename);
  Feature(double *wav, int len);
  string getFname() { return fname; }
  void DumpData() const;
  bool LoadFile(string filename);
  bool WriteToBin(FILE *fo_bin);
  bool WriteToAscii(FILE *fo_ascii);
  bool ResetFeature();
  ~Feature() {
    ResetFeature();
  }
  // Data
  string fname;
  int LT;
  int LF;
  double dT;
  double dF;
  double **data;
  double window;
  int LT_size;
  int LF_size;
  int data_size;
 private:
  bool LoadFromBin(FILE *fi_bin);
  bool LoadFromHTK(FILE *fi_htk);
  bool LoadFromAscii(FILE *fi_ascii);
  void Init();
};


#endif /*_FEATURE_H_ */
