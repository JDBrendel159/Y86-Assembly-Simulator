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
#include "Status.h"
#include "Debug.h"
#include <iostream>
#include "Instructions.h"
#include "ConditionCodes.h"
#include "Tools.h"

uint64_t dstE;
uint64_t valE;
bool M_bubble;
uint64_t Cnd;

/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool ExecuteStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    M * mreg = (M *) pregs[MREG];
    E * ereg = (E *) pregs[EREG];
    W * wreg = (W *) pregs[WREG];
    MemoryStage * mem = (MemoryStage *) stages[MSTAGE];

    uint64_t stat = ereg->getstat()->getOutput(), icode = ereg->geticode()->getOutput(),dstM = ereg->getdstM()->getOutput();
    uint64_t valA = 0; 
    M_bubble = needBubble(wreg, mem);
    dstE = ereg->getdstE()->getOutput();
    valA = ereg->getvalA()->getOutput();
    valE = ereg->getvalC()->getOutput();
    int64_t aluA = getAluA(icode, valA, valE);
    int64_t aluB = getAluB(icode, ereg);
    valE = runAlu(aluA, aluB, getalufun(icode, ereg->getifun()->getOutput()), icode, wreg, mem);

    Cnd = cond(icode, ereg->getifun()->getOutput());
    dstE = getdstE(icode, dstE, Cnd);
    setMInput(mreg, stat, icode, Cnd, valE, valA, dstE, dstM);
    return false;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void ExecuteStage::doClockHigh(PipeReg ** pregs)
{
    M * mreg = (M *) pregs[MREG];
    if (M_bubble) {
        mreg->getstat()->bubble(SAOK);
        mreg->geticode()->bubble(INOP);
        mreg->getCnd()->bubble();
        mreg->getvalE()->bubble();
        mreg->getvalA()->bubble();
        mreg->getdstE()->bubble(RNONE);
        mreg->getdstM()->bubble(RNONE);
    } else {
        mreg->getstat()->normal();
        mreg->geticode()->normal();
        mreg->getCnd()->normal();
        mreg->getvalE()->normal();
        mreg->getvalA()->normal();
        mreg->getdstE()->normal();
        mreg->getdstM()->normal();
    }
}

/**
 * Sets the fields of the M register.
 *
 * @param mreg pointer to an M register object
 * @params values/fields to be set
 */
void ExecuteStage::setMInput(M * mreg, uint64_t stat, uint64_t icode, 
                           uint64_t Cnd, uint64_t valE, uint64_t valA,
                           uint64_t dstE, uint64_t dstM)
{
   mreg->getstat()->setInput(stat);
   mreg->geticode()->setInput(icode);
   mreg->getCnd()->setInput(Cnd);
   mreg->getvalE()->setInput(valE);
   mreg->getvalA()->setInput(valA);
   mreg->getdstE()->setInput(dstE);
   mreg->getdstM()->setInput(dstM);
}

/**
 * Determines if the condition for the jump is true/false.
 *
 * @param e_icode is the instruction code
 * @param e_ifun is the instruction function
 * @return true if the condition is met/false otherwise
 */
bool ExecuteStage::cond(uint64_t e_icode, uint64_t e_ifun) {
    ConditionCodes * codes = ConditionCodes::getInstance();
    bool e_ZF;
    bool e_OF;
    bool e_SF;
    bool error;


    if (e_icode == IJXX || e_icode == ICMOVXX) {
        e_ZF = codes->getConditionCode(ZF, error);
        e_OF = codes->getConditionCode(OF, error);
        e_SF = codes->getConditionCode(SF, error);
        if (e_ifun == UNCOND) {
            return 1;        
        } 
        
        else if ((e_ifun == LESSEQ) && ((e_SF ^ e_OF) | e_ZF)) {
            return 1;
        } 
        
        else if ((e_ifun == LESS) && (e_SF ^ e_OF)) {
            return 1;
        } 
        
        else if ((e_ifun == EQUAL) && (e_ZF)) {
            return 1;
        } 
        
        else if ((e_ifun == NOTEQUAL) && !(e_ZF)) {
            return 1;
        } 
        
        else if ((e_ifun == GREATER) && (!(e_SF ^ e_OF) & !(e_ZF))) {
            return 1;
        } 
        
        else if ((e_ifun == GREATEREQ) && !(e_SF ^ e_OF)) {
            return 1;
        } 
        
        else {
            return 0;
        }


    } else {
        return 0;
    }   
}

bool ExecuteStage::needBubble(W * wreg, MemoryStage * mem) {
    uint64_t m_stat = mem->getm_stat();
    uint64_t w_stat = wreg->getstat()->getOutput();
    if (m_stat == SADR || m_stat == SINS || m_stat == SHLT || w_stat == SADR || w_stat == SINS || w_stat == SHLT) {
        return true;
    } else {
        return false;
    }
}

/**
 * Sets the condition codes.
 *
 * @param E_aluA is value A of the operation
 * @param E_aluB is value B of the operation
 * @param result is the result of the operation
 * @param E_aluFun is the ALU function code
 */
void ExecuteStage::setCodes(int64_t E_aluA, int64_t E_aluB, int64_t result, uint64_t E_aluFun) { 
    bool error = false;
    ConditionCodes * codes = ConditionCodes::getInstance();
    bool E_OF = 0;
    bool E_SF = 0;
    bool E_ZF = 0;
    if (E_aluFun == ADDQ) {
        E_OF = Tools::addOverflow(E_aluA, E_aluB);
        if (E_OF) {
            E_SF = (Tools::sign(result));
        } else {
            E_SF = Tools::sign(result);
        }
    } else if (E_aluFun == SUBQ) {
        E_OF = Tools::subOverflow(E_aluA, E_aluB);
        if (E_OF) {
            E_SF = (Tools::sign(result));
        } else {
            E_SF = Tools::sign(result);
        }
    } else {
        E_SF = Tools::sign(result);
    }
    if (result == 0) {
        E_ZF = true;
    }
    codes->setConditionCode(E_ZF, ZF, error);
    codes->setConditionCode(E_SF, SF, error);
    codes->setConditionCode(E_OF, OF, error);
}

/**
 * Runs the ALU operation and returns the result.
 *
 * @param E_aluA is value A of the operation
 * @param E_aluB is value B of the operation
 * @param E_aluFun is the ALU function code
 * @param E_icode is the instruction code
 * @param wreg is a pointer to a W register object
 * @param mem is a pointer to a MemoryStage object
 * @return the result of the operation
 */
uint64_t ExecuteStage::runAlu(int64_t E_aluA, int64_t E_aluB, uint64_t E_aluFun, uint64_t E_icode, W * wreg, MemoryStage * mem) {
    int64_t result = 0;
    if (E_aluFun == ADDQ) {
        result = E_aluA + E_aluB;
    } else if (E_aluFun == SUBQ) {
        result = E_aluB - E_aluA;
    } else if (E_aluFun == XORQ) {
        result = E_aluA ^ E_aluB;
    } else {
        result = E_aluA & E_aluB;
    }
    if (E_icode == IOPQ && set_cc(E_icode, wreg, mem)) {
        setCodes(E_aluA, E_aluB, result, E_aluFun);
    }
    return result;
}

/**
 * Gets the A value for the ALU operation.
 *
 * @param E_icode is the instruction code
 * @param E_valA is the current valA
 * @param E_valC is the current valC
 * @return the value computed for aluA
 */
uint64_t ExecuteStage::getAluA(uint64_t E_icode, uint64_t E_valA, uint64_t E_valC) {
    uint64_t aluA;
    
    if (E_icode == IRRMOVQ || E_icode == IOPQ) {
        aluA = E_valA;
    } else if (E_icode == IIRMOVQ || E_icode == IRMMOVQ || E_icode == IMRMOVQ) {
        aluA = E_valC;
    } else if (E_icode == ICALL || E_icode == IPUSHQ) {
        aluA = -8;
    } else if (E_icode == IRET || E_icode == IPOPQ) {
        aluA = 8;
    } else {
        aluA = 0;
    }
    return aluA;
}

/**
 * Gets the B value for the ALU operation.
 *
 * @param E_icode is the instruction code
 * @param ereg is a pointer to an E register object
 * @return the value computed for aluB
 */
uint64_t ExecuteStage::getAluB(uint64_t E_icode, E * ereg) {
    uint64_t aluB;
    
    if (E_icode == IRMMOVQ || E_icode == IMRMOVQ || E_icode == IOPQ || E_icode == ICALL 
        || E_icode == IPUSHQ || E_icode == IRET || E_icode == IPOPQ) {
        aluB = ereg->getvalB()->getOutput();
    } else if (E_icode == IRRMOVQ || E_icode == IIRMOVQ) {
        aluB = 0;
    } else {
        aluB = 0;
    }
    return aluB;
}

/**
 * Gets the code of the ALU function
 *
 * @param E_icode is the instruction code
 * @param E_ifun is instruction function
 * @return the value computed for aluFun
 */
uint64_t ExecuteStage::getalufun(uint64_t E_icode, uint64_t E_ifun){
    uint64_t alufun;

    if (E_icode == IOPQ) {
        alufun = E_ifun;
    } else {
        alufun = 0;
    }
    return alufun;
}

/**
 * Determines if the condition codes need to be set.
 *
 * @param E_icode is the instruction code
 * @param wreg is a pointer to a W register object
 * @param mem is a pointer to a MemoryStage object
 * @return true if the condition codes need to be set/false otherwise
 */
bool ExecuteStage::set_cc(uint64_t E_icode, W * wreg, MemoryStage * mem) {
    uint64_t w_stat = wreg->getstat()->getOutput();
    uint64_t m_stat = mem->getm_stat();
    if ((E_icode == IOPQ) && (!(m_stat == SADR || m_stat == SINS || m_stat == SHLT)
        && !(w_stat == SADR || w_stat == SINS || w_stat == SHLT))) {
        return true;   
    } else {
        return false;
    }
}

/**
 * Determines the value for dstE.
 *
 * @param E_icode is the instruction code
 * @param E_dstE is the current dstE
 * @param e_Cnd is the current condition state
 * @return the value computed for dstE
 */
uint64_t ExecuteStage::getdstE(uint64_t E_icode, uint64_t E_dstE, uint64_t e_Cnd) {
    uint64_t dstE;
    if ((E_icode == IRRMOVQ) && !(e_Cnd)) {
        dstE = RNONE;
    } else {
        dstE = E_dstE;
    }
    return dstE;
}

/**
 * Gets the value of dstE.
 *
 * @return dstE
 */
uint64_t ExecuteStage::gete_dstE() {
    return dstE;
}

/**
 * Gets the value of valE.
 *
 * @return valE
 */
uint64_t ExecuteStage::gete_valE() {
    return valE;
}

/**
 * Gets the condition state.
 *
 * @return the condition
 */
uint64_t ExecuteStage::get_Cnd() {
    return Cnd;
}

