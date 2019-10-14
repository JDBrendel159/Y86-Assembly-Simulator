//Joey Brendel and RJ Witschger [Team 16]
#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "E.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "MemoryStage.h"
#include "ExecuteStage.h"
#include "DecodeStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include <iostream>

uint64_t srcA = RNONE;
uint64_t srcB = RNONE;
bool e_bubble = false;

/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool DecodeStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   D * dreg = (D *) pregs[DREG];
   E * ereg = (E *) pregs[EREG];
   M * mreg = (M *) pregs[MREG];
   W * wreg = (W *) pregs[WREG];
   ExecuteStage * exec = (ExecuteStage *) stages[ESTAGE];
   MemoryStage * mem = (MemoryStage *) stages[MSTAGE];
   uint64_t icode = dreg->geticode()->getOutput(), ifun = dreg->getifun()->getOutput(), stat = dreg->getstat()->getOutput(), valC = dreg->getvalC()->getOutput(); //<----
   uint64_t dstE = RNONE, dstM = RNONE, valA = 0, valB = 0;        //<----
   
   srcA = set_srcA(dreg, icode);
   srcB = set_srcB(dreg, icode);
   dstE = set_dstE(dreg, icode);
   dstM = set_dstM(dreg, icode);
   valA = set_valA(srcA, icode, dreg, mreg, wreg, exec, mem);
   valB = set_valB(srcB, mreg, wreg, exec, mem);
   calcControlSignals(ereg, exec);
   setEInput(ereg, stat, icode, ifun, valA, valB, valC, dstE, dstM, srcA, srcB);
   return false;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void DecodeStage::doClockHigh(PipeReg ** pregs)
{
   if (!e_bubble) {
   E * ereg = (E *) pregs[EREG];
        ereg->getstat()->normal();
        ereg->geticode()->normal();
        ereg->getifun()->normal();
        ereg->getvalC()->normal();
        ereg->getvalB()->normal();
        ereg->getvalA()->normal();
        ereg->getdstE()->normal();
        ereg->getdstM()->normal();
        ereg->getsrcA()->normal();
        ereg->getsrcB()->normal();
   } else {
        bubbleE(pregs);
   }
}

/**
 * Sets the values of the E register.
 * 
 * @param ereg is a pointer to an E register object
 * @params values to be set
 */
void DecodeStage::setEInput(E * ereg, uint64_t stat, uint64_t icode, 
                           uint64_t ifun, uint64_t valA, uint64_t valB,
                           uint64_t valC, uint64_t dstE, uint64_t dstM,
                           uint64_t srcA, uint64_t srcB)
{
   ereg->getstat()->setInput(stat);
   ereg->geticode()->setInput(icode);
   ereg->getifun()->setInput(ifun);
   ereg->getvalC()->setInput(valC);
   ereg->getvalB()->setInput(valB);
   ereg->getvalA()->setInput(valA);
   ereg->getdstE()->setInput(dstE);
   ereg->getsrcA()->setInput(srcA);
   ereg->getsrcB()->setInput(srcB);
   ereg->getdstM()->setInput(dstM);
}

/**
 * Calculates if the E register needs to be bubbled.
 * 
 * @param ereg is a pointer to an E register object
 * @param exec is a pointer to an ExecuteStage object
 */
void DecodeStage::calcControlSignals(E * ereg, ExecuteStage * exec) {
    uint64_t e_icode = ereg->geticode()->getOutput();
    uint64_t e_dstM = ereg->getdstM()->getOutput();
    uint64_t e_Cnd = exec->get_Cnd();

    if ((e_icode == IJXX && !e_Cnd) || ((e_icode == IMRMOVQ || e_icode == IPOPQ) && (e_dstM == srcA 
        || e_dstM == srcB))) {
        e_bubble = true;
    } else {
        e_bubble = false;
    }
}

/**
 * Calculates value for srcA to be set to.
 * 
 * @param dreg is a pointer to a D register object
 * @param D_icode is the instruction code
 * @return the value for srcA
 */
uint64_t DecodeStage::set_srcA(D * dreg, uint64_t D_icode) {
    bool error = false;
    if (D_icode == IRRMOVQ || D_icode == IRMMOVQ || D_icode == IOPQ || D_icode == IPUSHQ) {
        return dreg->getrA()->getOutput();
    } else if ((D_icode == IPOPQ || D_icode == IRET) && error == false) {
        return 4;
    } else {
        return RNONE;
    }
}

/**
 * Calculates value for srcB to be set to.
 * 
 * @param dreg is a pointer to a D register object
 * @param D_icode is the instruction code
 * @return the value for srcB
 */
uint64_t DecodeStage::set_srcB(D * dreg, uint64_t D_icode) {
    bool error = false;
    if (D_icode == IOPQ || D_icode == IRMMOVQ || D_icode == IMRMOVQ) {
        return dreg->getrB()->getOutput();
    } else if ((D_icode == IPUSHQ || D_icode == IPOPQ || D_icode == ICALL || D_icode == IRET) && error == false) {
        return 4;
    } else {
        return RNONE;
    }
}

/**
 * Calculates value for dstE to be set to.
 * 
 * @param dreg is a pointer to a D register object
 * @param D_icode is the instruction code
 * @return the value for dstE
 */
uint64_t DecodeStage::set_dstE(D * dreg, uint64_t D_icode) {
    bool error = false;
    if (D_icode == IRRMOVQ || D_icode == IIRMOVQ || D_icode == IOPQ) {
        return dreg->getrB()->getOutput();
    } else if ((D_icode == IPUSHQ || D_icode == IPOPQ || D_icode == ICALL || D_icode == IRET) && error == false) {
        return 4;
    } else {
        return RNONE;
    }
}

/**
 * Calculates value for dstM to be set to.
 * 
 * @param dreg is a pointer to a D register object
 * @param D_icode is the instruction code
 * @return the value for dstM
 */
uint64_t DecodeStage::set_dstM(D * dreg, uint64_t D_icode) {

    if (D_icode == IMRMOVQ || D_icode == IPOPQ) {
        return dreg->getrA()->getOutput();
    } else {
        return RNONE;
    }
}

/**
 * Calculates value for valA to be set to.
 * 
 * @param sourceA is the value of srcA
 * @param d_icode is the instruction code
 * @param dreg is a pointer to a D register object
 * @param mreg is a pointer to an M register object
 * @param wreg is a pointer to a W register object
 * @param exec is a pointer to an ExecuteStage object
 * @param mem is a pointer to a MemoryStage object
 * @return the value for valA
 */
uint64_t DecodeStage::set_valA(uint64_t sourceA, uint64_t d_icode, D * dreg, M * mreg, W * wreg, ExecuteStage * exec, MemoryStage * mem) {
    bool error = false;
    RegisterFile * reg = RegisterFile::getInstance();
    if (d_icode == ICALL || d_icode == IJXX) {
        return dreg->getvalP()->getOutput();
    } else if (sourceA == RNONE) {
        return 0;
    } else if (sourceA == exec->gete_dstE()) {
        return exec->gete_valE();
    } else if (sourceA == mreg->getdstM()->getOutput()) {
        return mem->get_valM();
    } else if (sourceA == mreg->getdstE()->getOutput()) {
        return mreg->getvalE()->getOutput();
    } else if (sourceA == wreg->getdstM()->getOutput()) {
        return wreg->getvalM()->getOutput();
    } else if (sourceA == wreg->getdstE()->getOutput()) {
        return wreg->getvalE()->getOutput();
    } else {
        return reg->readRegister(sourceA, error);
    }
}

/**
 * Calculates value for valB to be set to.
 * 
 * @param sourceB is the value of srcB
 * @param mreg is a pointer to an M register object
 * @param wreg is a pointer to a W register object
 * @param exec is a pointer to an ExecuteStage object
 * @param mem is a pointer to a MemoryStage object
 * @return the value for valB
 */
uint64_t DecodeStage::set_valB(uint64_t sourceB, M * mreg, W * wreg, ExecuteStage * exec, MemoryStage * mem) {
    bool error = false;
    RegisterFile * reg = RegisterFile::getInstance();
    if (sourceB == RNONE) {
        return 0;
    } else if (sourceB == exec->gete_dstE()) {
        return exec->gete_valE();
    } else if (sourceB == mreg->getdstM()->getOutput()) {
        return mem->get_valM();
    } else if (sourceB == mreg->getdstE()->getOutput()) {
        return mreg->getvalE()->getOutput();
    } else if (sourceB == wreg->getdstM()->getOutput()) {
        return wreg->getvalM()->getOutput();
    } else if (sourceB == wreg->getdstE()->getOutput()) {
        return wreg->getvalE()->getOutput();
    } else {
        return reg->readRegister(sourceB, error);
    }
}

/**
 * Gets the value of srcA.
 *
 * @return value of srcA
 */
uint64_t DecodeStage::get_srcA() {
    return srcA;
}

/**
 * Gets the value of srcB.
 *
 * @return value of srcB
 */
uint64_t DecodeStage::get_srcB() {
    return srcB;
}

/**
 * Bubbles the fields of the E register
 *
 * @param pregs pointer to the array of registers
 */
void DecodeStage::bubbleE(PipeReg ** pregs) {
    E * ereg = (E *) pregs [EREG];

    ereg->getstat()->bubble(SAOK);
    ereg->geticode()->bubble(INOP);
    ereg->getifun()->bubble();
    ereg->getvalC()->bubble();
    ereg->getvalB()->bubble();
    ereg->getvalA()->bubble();
    ereg->getdstE()->bubble(RNONE);
    ereg->getdstM()->bubble(RNONE);
    ereg->getsrcA()->bubble(RNONE);
    ereg->getsrcB()->bubble(RNONE);
}


