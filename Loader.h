
class Loader
{
   private:
      bool loaded;        //set to true if a file is successfully loaded into memory
      std::ifstream inf;  //input file handle
   public:
      Loader(int argc, char * argv[]);
      bool isYoExtension(char fileName[]);
      int64_t convertAddress(char line[], int addressStart, int addressEnd);
      int64_t convertData(char line[], int dataStart, int dataEnd);
      void loadLine(char line[], int64_t operation, int addToAddress);
      bool checkData(char line[]);
      bool checkElse(char line[]);
      bool hasData(char line[]);
      bool hasAddress(char line[]);
      bool isBlank(char line[], uint64_t spot);
      bool checkAddress(char line[], uint64_t lastAdd);
      bool checkChar(char symbol);
      bool isLoaded();
      bool hasErrors(char line[]);
};
