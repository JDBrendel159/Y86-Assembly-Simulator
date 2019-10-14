//class to perform the combinational logic of
//the Fetch stage
class FetchStage: public Stage
{
   private:
      void calcControlSignals(E * ereg, D * dreg, M * mreg, DecodeStage * dec, ExecuteStage * exec);
      void setDInput(D * dreg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                     uint64_t rA, uint64_t rB,
                     uint64_t valC, uint64_t valP);
      bool need_regids(uint64_t f_icode);
      bool need_valC(uint64_t f_icode);
      void getRegIds(uint64_t f_pc, uint64_t &rA, uint64_t &rB);
      uint64_t buildValC(uint64_t f_pc, uint64_t f_icode);
      uint64_t predictPC(uint64_t icode, uint64_t valC, uint64_t valP);
      uint64_t selectPC(F * freg, M * mreg, W * wreg);     
      uint64_t PCincrement(uint64_t f_pc, bool needregidBool, bool needvalcBool);
      bool instrValid(uint64_t f_icode);
      uint64_t getStat(uint64_t f_icode, bool isValid, bool mem_error);
      bool needd_bubble (E * ereg, D * dreg, M * mreg, DecodeStage * dec, ExecuteStage * exec);
      bool needd_stall (E * ereg, DecodeStage * dec);
      bool needf_stall (E * ereg, D * dreg, M * mreg, DecodeStage * dec);
      void bubbleD (D * dreg);
   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);

};
