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
#include "FetchStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"
uint64_t valM;
uint64_t stat;


/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool MemoryStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];


    stat = mreg->getstat()->getOutput();
    uint64_t icode = mreg->geticode()->getOutput();
    uint64_t valE = mreg->getvalE()->getOutput();
    valM = 0;
    uint64_t dstE = mreg->getdstE()->getOutput();
    uint64_t dstM = mreg->getdstM()->getOutput();
     
    uint64_t address = mem_addr(mreg);
    bool read = mem_read(icode);
    bool write = mem_write(icode);
    bool error = false;

    Memory * mem = Memory::getInstance();
    if (read == true) {
        valM = mem->getLong(address, error);
    }
    if (write == true) {
        mem->putLong(mreg->getvalA()->getOutput(), address, error);
        valM = 0;
    }
    if (error) {
        stat = SADR;
    } else {
        stat = mreg->getstat()->getOutput();
    }
    setWInput(wreg, stat, icode, valE, valM, dstE, dstM);
    return false;
}


/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void MemoryStage::doClockHigh(PipeReg ** pregs)
{
    W * wreg = (W *) pregs[WREG];
    wreg->getstat()->normal();
    wreg->geticode()->normal();
    wreg->getvalE()->normal();
    wreg->getvalM()->normal();
    wreg->getdstE()->normal();
    wreg->getdstM()->normal();
}


/* setWInput
 *
 * Provides the input to potentially be stored in the W registe during doClockHigh
 *
 * @param: wreg - pointer to the W register instance
 * @param: stat - value to be stored in the stat pipeline register within W
 * @param: icode - value to be stored in the icode pipeline register within W
 * @param: valE - value to be stored in the valE pipeline register within W
 * @param: valM - value to be stored in the valM pipeline register within W
 * @param: dstE - value to be stored in the dstE pipeline register within W
 * @param: dstM - value to be stored in the dstM pipeline register within W
*/
void MemoryStage::setWInput(W * wreg, uint64_t stat, uint64_t icode,
                            uint64_t valE, uint64_t valM,
                            uint64_t dstE, uint64_t dstM)
{
    wreg->getstat()->setInput(stat);
    wreg->geticode()->setInput(icode);
    wreg->getvalE()->setInput(valE);
    wreg->getvalM()->setInput(valM);
    wreg->getdstE()->setInput(dstE);
    wreg->getdstM()->setInput(dstM);

}

/**
 * mem_addr
 *
 * Obtains a memory address if the given instruction (dependent on the icode) needs one.
 *
 * @param mreg is a pointer to an M register object
 *
 * @return memory address
 */
uint64_t MemoryStage::mem_addr(M * mreg) {
    uint64_t addr;
    uint64_t m_icode = mreg->geticode()->getOutput();
    
    if (m_icode == IRMMOVQ || m_icode == IPUSHQ || m_icode == ICALL || m_icode == IMRMOVQ) {
        addr = mreg->getvalE()->getOutput();
        return addr;
    } else if (m_icode == IPOPQ || m_icode == IRET) {
        addr = mreg->getvalA()->getOutput();
        return addr;
    } else {
        return 0;
    }
}


/**
 * mem_read
 *
 * Determines whether or not the instruction is an IMRMOVQ, IPOPQ, or IRET
 *
 * @param: m_icode - icode of the current instruction in the memory stage
 *
 * @return boolean value if read from memory is required
 */
bool MemoryStage::mem_read(uint64_t m_icode) {
    if (m_icode == IMRMOVQ || m_icode == IPOPQ || m_icode == IRET) {
        return true;
    } else {
        return false;
    }
}

/**
 * mem_write
 *
 * Determines whether or not the instruction is an IRMMOVQ, IPUSHQ, or ICALL
 *
 * @param: m_icode - icode of the current instruction in the memory stage
 *
 * @return boolean value if write to memory is required
 */
bool MemoryStage::mem_write(uint64_t m_icode) {
    if (m_icode == IRMMOVQ || m_icode == IPUSHQ || m_icode == ICALL) {
        return true;
    } else {
        return false;
    }
}

/**
 * Returns valM from Memory
 */
uint64_t MemoryStage::get_valM() {
    return valM;
}

/**
 * Returns stat from Memory
 */
uint64_t MemoryStage::getm_stat() {
    return stat;
}



