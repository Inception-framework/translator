
/*
    This file is part of Inception translator.

    Inception translator is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Inception translator.  If not, see <https://www.gnu.org/licenses/>.

    Copyright (c) 2017 Maxim Integrated, Inc.
    Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>

    Copyright (c) 2017 EURECOM, Inc.
    Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
*/

#include "Utils/IContext.h"

using namespace llvm;

IContext* IContext::m_instance = NULL;

DenseMap<const SDNode*, Value*> IContext::VisitMap;

llvm::Module* IContext::Mod;

const TargetRegisterInfo* IContext::RegisterInfo;

ARMLifterManager *IContext::alm = new ARMLifterManager();

LLVMContext* IContext::getContext() { return &(IContext::Mod->getContext()); }

LLVMContext& IContext::getContextRef() { return IContext::Mod->getContext(); }
