//class to perform the combinational logic of
//the Decode stage
class DecodeStage: public Stage
{
   private:
      void setEInput(E * ereg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                     uint64_t valA, uint64_t valB, uint64_t valC, 
                     uint64_t dstE, uint64_t dstM, 
                     uint64_t srcA, uint64_t srcB);
   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void calcControlSignals(E * ereg, ExecuteStage * exec);
      void bubbleE(PipeReg ** pregs);
      uint64_t set_srcA(D * dreg, uint64_t D_icode);
      uint64_t set_srcB(D * dreg, uint64_t D_icode);
      uint64_t set_dstE(D * dreg, uint64_t D_icode);
      uint64_t set_dstM(D * dreg, uint64_t D_icode);
      uint64_t set_valA(uint64_t sourceA, uint64_t d_icode, D * dreg, M * mreg, W * wreg, ExecuteStage * exec, MemoryStage * mem);
      uint64_t set_valB(uint64_t sourceB, M * mreg, W * wreg, ExecuteStage * exec, MemoryStage * mem);
      uint64_t get_srcA();
      uint64_t get_srcB();
      void doClockHigh(PipeReg ** pregs);

};
