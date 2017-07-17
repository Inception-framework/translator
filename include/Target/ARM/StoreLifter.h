#ifndef STORE_LIFTER_H
#define STORE_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class StoreLifter : public ARMLifter{
  public :

  void registerLifter();

  StoreLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~StoreLifter(){};
};

#endif
