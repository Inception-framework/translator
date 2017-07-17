#ifndef SUB_LIFTER_H
#define SUB_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class SubtractLifter : public ARMLifter{
  public :

  void registerLifter();

  SubtractLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~SubtractLifter(){};
};

#endif
