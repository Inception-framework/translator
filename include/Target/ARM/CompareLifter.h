#ifndef COMPARE_LIFTER_H
#define COMPARE_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class CompareLifter : public ARMLifter{
  public :

  void registerLifter();

  CompareLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~CompareLifter(){};
};

#endif
