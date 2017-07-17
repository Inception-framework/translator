#ifndef SUB_LIFTER_H
#define SUB_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class ShiftLifter : public ARMLifter{
  public :

  void registerLifter();

  ShiftLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~ShiftLifter(){};
};

#endif
