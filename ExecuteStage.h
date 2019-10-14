//class to perform the combinational logic of
//the Execute stage
class ExecuteStage: public Stage
{
   private:
      void setMInput(M * mreg, uint64_t stat, uint64_t icode, uint64_t Cnd, 
                     uint64_t valE, uint64_t valA, uint64_t dstE, uint64_t dstM);
      void setCodes (int64_t E_aluA, int64_t E_aluB, int64_t result, uint64_t E_aluFun);
      uint64_t runAlu (int64_t E_aluA, int64_t E_aluB, uint64_t E_aluFun, uint64_t E_icode, W * wreg, MemoryStage * mem);
      uint64_t getAluA (uint64_t E_icode, uint64_t E_valA, uint64_t E_valC);
      uint64_t getAluB (uint64_t E_icode, E * ereg);
      uint64_t getalufun (uint64_t E_icode, uint64_t E_ifun);
      bool cond (uint64_t e_icode, uint64_t e_ifun);
      uint64_t getdstE (uint64_t E_icode, uint64_t E_dstE, uint64_t e_Cnd);
   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);
      bool needBubble (W * wreg, MemoryStage * mem);
      bool set_cc (uint64_t E_icode, W * wreg, MemoryStage * mem);
      uint64_t gete_dstE ();
      uint64_t gete_valE ();
      uint64_t get_Cnd();

};
