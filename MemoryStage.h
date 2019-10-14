//class to perform the combinational logic of
//the Memory stage
class MemoryStage: public Stage
{
   private:
      void setWInput(W * wreg, uint64_t stat, uint64_t icode, 
                     uint64_t valE, uint64_t valM,
                     uint64_t dstE, uint64_t dstM);
      uint64_t mem_addr (M * mreg);
      bool mem_read (uint64_t m_icode);
      bool mem_write (uint64_t m_icode);
   public:
      uint64_t get_valM ();
      uint64_t getm_stat ();
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);


};
