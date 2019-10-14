//Joey Brendel and RJ Witschger [Team 16]
#include <iostream>
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
#include "FetchStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"
#include "Tools.h"
#include <iostream>

bool f_stall;
bool d_stall;
bool D_bubble;
/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool FetchStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   F * freg = (F *) pregs[FREG];
   D * dreg = (D *) pregs[DREG];
   M * mreg = (M *) pregs[MREG];
   W * wreg = (W *) pregs[WREG];
   E * ereg = (E *) pregs[EREG];
   DecodeStage * dec = (DecodeStage *) stages[DSTAGE];
   ExecuteStage * exec = (ExecuteStage *) stages[ESTAGE];
   uint64_t f_pc = 0, icode = 0, ifun = 0, valC = 0, valP = 0;
   uint64_t rA = RNONE, rB = RNONE, stat = SAOK;
   
    
   calcControlSignals(ereg, dreg, mreg, dec, exec);
   f_pc = selectPC(freg, mreg, wreg);
   Memory * mem = Memory::getInstance();
   bool error = false;
   uint64_t instruction = mem->getByte(f_pc, error);
   if (error) {
       icode = INOP;
       ifun = FNONE;
   } else {
       icode = Tools::getBits(instruction, 4, 7);
       ifun = Tools::getBits(instruction, 0, 3);
   }
   bool valid = instrValid(icode);
   stat = getStat(icode, valid, error);
   bool needvalC = need_valC(icode);
   bool needregids = need_regids(icode);
   if (need_valC(icode)) {
       valC = buildValC(f_pc, icode);
   }
   if (need_regids(icode)) {
       getRegIds(f_pc, rA, rB);
   }
   valP = PCincrement(f_pc, needregids, needvalC);
   uint64_t predpc = predictPC(icode, valC, valP);

   freg->getpredPC()->setInput(predpc);   

   setDInput(dreg, stat, icode, ifun, rA, rB, valC, valP);
   return false;
}

/**
 * Gives the proper control signals to the D register fields (and one F reg field).
 * 
 * @param pregs is a pointer to the array of registers
 */
void FetchStage::doClockHigh(PipeReg ** pregs)
{
    F * freg = (F *) pregs[FREG];
    D * dreg = (D *) pregs[DREG];
    
    if (!f_stall) {
        freg->getpredPC()->normal();
    }
    if (D_bubble) {
        bubbleD(dreg);
    } else if (!d_stall) {
        dreg->getstat()->normal();
        dreg->geticode()->normal();
        dreg->getifun()->normal();
        dreg->getrA()->normal();
        dreg->getrB()->normal();
        dreg->getvalC()->normal();
        dreg->getvalP()->normal();
    }
}

/**
 * Bubbles the D register fields.
 *
 * @param dreg is a pointer to a D register object
 */
void FetchStage::bubbleD(D * dreg) {
    dreg->getstat()->bubble(SAOK);
    dreg->geticode()->bubble(INOP);
    dreg->getifun()->bubble();
    dreg->getrA()->bubble(RNONE);
    dreg->getrB()->bubble(RNONE);
    dreg->getvalC()->bubble();
    dreg->getvalP()->bubble();
}



/* setDInput
 * provides the input to potentially be stored in the D register
 * during doClockHigh
 *
 * @param: dreg - pointer to the D register instance
 * @param: stat - value to be stored in the stat pipeline register within D
 * @param: icode - value to be stored in the icode pipeline register within D
 * @param: ifun - value to be stored in the ifun pipeline register within D
 * @param: rA - value to be stored in the rA pipeline register within D
 * @param: rB - value to be stored in the rB pipeline register within D
 * @param: valC - value to be stored in the valC pipeline register within D
 * @param: valP - value to be stored in the valP pipeline register within D
*/
void FetchStage::setDInput(D * dreg, uint64_t stat, uint64_t icode, 
                           uint64_t ifun, uint64_t rA, uint64_t rB,
                           uint64_t valC, uint64_t valP)
{
    dreg->getstat()->setInput(stat);
    dreg->geticode()->setInput(icode);
    dreg->getifun()->setInput(ifun);
    dreg->getrA()->setInput(rA);
    dreg->getrB()->setInput(rB);
    dreg->getvalC()->setInput(valC);
    dreg->getvalP()->setInput(valP);
}

/**
 * Calculates if the F/D registers need to be stalled/bubbled.
 *
 * @param ereg is a pointer to an E register object
 * @param dreg is a pointer to a D register object
 * @param mreg is a pointer to an M register object
 * @param dec is a pointer to a DecodeStage object
 * @param exec is a pointer to an ExecuteStage object
 */
void FetchStage::calcControlSignals(E * ereg, D * dreg, M * mreg, DecodeStage * dec, ExecuteStage * exec) {
    if (needf_stall(ereg, dreg, mreg, dec)) {
        f_stall = true;
    } else {
        f_stall = false;
    }

    if (needd_stall(ereg, dec)) {
        d_stall = true;
    } else {
        d_stall = false;
    }

    if (needd_bubble(ereg, dreg, mreg, dec, exec)) {
        D_bubble = true;
    } else {
        D_bubble = false;
    }
}

/**
 * Determines if the D register needs to be bubbled.
 *
 * @param ereg is a pointer to an E register object
 * @param dreg is a pointer to a D register object
 * @param mreg is a pointer to an M register object
 * @param dec is a pointer to a DecodeStage object
 * @param exec is a pointer to an ExecuteStage object
 * @return true if the register must be bubbled/false otherwise
 */
bool FetchStage::needd_bubble(E * ereg, D * dreg, M * mreg, DecodeStage * dec, ExecuteStage * exec) {
    uint64_t e_Cnd = exec->get_Cnd();
    uint64_t e_icode = ereg->geticode()->getOutput();
    uint64_t m_icode = mreg->geticode()->getOutput();
    uint64_t d_icode = dreg->geticode()->getOutput();
    uint64_t d_srcA = dec->get_srcA();
    uint64_t d_srcB = dec->get_srcB();
    uint64_t e_dstM = ereg->getdstM()->getOutput();

    if ((e_icode == IJXX && !e_Cnd) || (!((e_icode == IMRMOVQ || e_icode == IPOPQ) 
        && (e_dstM == d_srcA || e_dstM == d_srcB)) && (d_icode == IRET || e_icode == IRET
        || m_icode == IRET))) {
        return true;
    } else {
        return false;
    }
}

/**
 * Determines if the F register needs to be stalled.
 *
 * @param ereg is a pointer to an E register object
 * @param dreg is a pointer to a D register object
 * @param mreg is a pointer to an M register object
 * @param dec is a pointer to a DecodeStage object
 * @return true if the register must be stalled/false otherwise
 */
bool FetchStage::needf_stall(E * ereg, D * dreg, M * mreg, DecodeStage * dec) {
    uint64_t m_icode = mreg->geticode()->getOutput();
    uint64_t d_icode = dreg->geticode()->getOutput();
    uint64_t e_icode = ereg->geticode()->getOutput();
    uint64_t e_dstM = ereg->getdstM()->getOutput();
    uint64_t d_srcA = dec->get_srcA();
    uint64_t d_srcB = dec->get_srcB();

    if (((e_icode == IMRMOVQ || e_icode == IPOPQ) && (e_dstM == d_srcA || e_dstM == d_srcB)) 
        || (e_icode == IRET || d_icode == IRET || m_icode == IRET)) {
        return true;
    } else {
        return false;
    }
} 

/**
 * Determines if the D register must be stalled.
 * 
 * @param ereg is a pointer to an E register object
 * @param dec is a pointer to a DecodeStage object
 * @return true if the stall is needed/false otherwise
 */
bool FetchStage::needd_stall(E * ereg, DecodeStage * dec) {
    uint64_t e_icode = ereg->geticode()->getOutput();
    uint64_t e_dstM = ereg->getdstM()->getOutput();
    uint64_t d_srcA = dec->get_srcA();
    uint64_t d_srcB = dec->get_srcB();

    if ((e_icode == IMRMOVQ || e_icode == IPOPQ) && (e_dstM == d_srcA || e_dstM == d_srcB)) {
        return true;
    } else {
        return false;
    }
}

/**
 * Selects the value for the PC.
 *
 * @param freg is a pointer to an F register object
 * @param mreg is a pointer to an M register object
 * @param wreg is a pointer to a W register object
 * @return the computed PC
 */
uint64_t FetchStage::selectPC(F * freg, M * mreg, W * wreg) {

    uint64_t m_icode = mreg->geticode()->getOutput();
    uint64_t w_icode = wreg->geticode()->getOutput();
    uint64_t w_valm = wreg->geticode()->getOutput();
    uint64_t m_cnd = mreg->getCnd()->getOutput();
    uint64_t m_valA = mreg->getvalA()->getOutput();
    uint64_t predPC = freg->getpredPC()->getOutput();
    uint64_t f_pc;

    if (m_icode == IJXX && !(m_cnd)) {
        f_pc = m_valA;
    } else if (w_icode == IRET) {
        f_pc = w_valm;   
    }
    else {
        f_pc = predPC;
    }
    return f_pc;
}

/**
 * Determines if the register IDs are needed from the instruction.
 *
 * @param f_icode is the instruction code
 * @return true if the register IDs are needed/false otherwise
 */
bool FetchStage::need_regids(uint64_t f_icode) {
    if (f_icode == IRRMOVQ || f_icode == IOPQ || f_icode == IPUSHQ || f_icode == IPOPQ 
        || f_icode == IIRMOVQ || f_icode == IRMMOVQ || f_icode == IMRMOVQ) {
        return true;     
    } else {
        return false;
    }
}

/**
 * Determines if the valC is needed from the instruction.
 *
 * @param f_icode is the instruction code
 * @return true if valC is needed/false otherwise
 */
bool FetchStage::need_valC(uint64_t f_icode) {
    if (f_icode == IIRMOVQ || f_icode == IRMMOVQ || f_icode == IMRMOVQ || f_icode == IJXX || f_icode == ICALL) {
        return true;
    } else {
        return false;
    }
}

/**
 * Increments the PC accordingly.
 *
 * @param f_pc is the current PC
 * @param needRegIds is if the register IDs are needed or not
 * @param needValC is if valC is needed or not
 * @return the new PC
 */
uint64_t FetchStage::PCincrement(uint64_t f_pc, bool needRegIds, bool needValC) {
    uint64_t inc = f_pc + 1;
    if (needRegIds == true) {
        //std::cout << "reg" << std::endl;
        inc++;
    }
    if (needValC == true) {
        //std::cout << "valc" << std::endl;
        inc += 8;
    }
    return inc;            
}

/**
 * Determines if the instruction is valid.
 *
 * @param f_icode is the instruction code
 * @return true if the intruction is valid/false otherwise
 */
bool FetchStage::instrValid(uint64_t f_icode) {
    if (f_icode == INOP || f_icode == IHALT || f_icode == IRRMOVQ || f_icode == IIRMOVQ
        || f_icode == IRMMOVQ || f_icode == IMRMOVQ || f_icode == IOPQ || f_icode == IJXX
        || f_icode == ICALL || f_icode == IRET || f_icode == IPUSHQ || f_icode == IPOPQ) {
        return true;
    } else {
        return false;
    }
}

/**
 * Determines the stat of the instruction.
 *
 * @param f_icode is the instruction code
 * @param isValid is if the instruction is valid or not
 * @param mem_error is if there is a memory error or not
 * @return the new stat value
 */
uint64_t FetchStage::getStat(uint64_t f_icode, bool isValid, bool mem_error) {
   if (mem_error) {
       return SADR;
   } else if (!isValid) {
       return SINS;
   } else if (f_icode == IHALT) {
       return SHLT;
   } else {
       return SAOK;
   }
}

/**
 * Predicts the value of the next PC
 *
 * @param icode is the instruction code
 * @param valC is the value of valC
 * @param valP is the value of valP
 * @return the computed PC
 */
uint64_t FetchStage::predictPC (uint64_t icode, uint64_t valC, uint64_t valP)
{
    uint64_t f_predPC;
    if (icode == IJXX || icode == ICALL) {
        f_predPC = valC;
    }
    else {
        f_predPC = valP;
    }
    return f_predPC;
}

/**
 * Sets rA and rB to the register IDs of the instruction. 
 *
 * @param f_pc is the PC
 * @param rA is the rA value of the instruction
 * @param rB is the rB value of the instruction
 */
void FetchStage::getRegIds(uint64_t f_pc, uint64_t &rA, uint64_t &rB) {
    Memory * mem = Memory::getInstance();
    bool error = false;
    uint64_t regByte = mem->getByte(f_pc+1, error);
    if (error == false) {
         rA = Tools::getBits(regByte, 4, 7);
         rB = Tools::getBits(regByte, 0, 3);
    }
}

/**
 * Constructs valC of the instruction
 *
 * @param f_pc is the PC
 * @param f_icode is the instruction code
 * @return the finished valC
 */
uint64_t FetchStage::buildValC(uint64_t f_pc, uint64_t f_icode) {
    bool error = false;
    Memory * mem = Memory::getInstance();
    uint8_t val[8];
    int8_t x = 0;
    uint8_t y = 1;
    if (need_regids(f_icode)) {
        y = 2;
    }
    while (x < 8) {
        uint8_t byte = mem->getByte((f_pc+y), error);
        val[x] = byte;
        x++;
        y++;
    }
    return Tools::buildLong(val);
}
