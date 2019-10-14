//Joey Brendel and RJ Witschger [Team 16]
#include <iostream>
#include <fstream>
#include <string.h>
#include <ctype.h>

#include "Loader.h"
#include "Memory.h"

//first column in file is assumed to be 0
#define ADDRBEGIN 2   //starting column of 3 digit hex address 
#define ADDREND 4     //ending column of 3 digit hext address
#define DATABEGIN 7   //starting column of data bytes
#define COMMENT 28    //location of the '|' character
uint64_t preAdd = 0; 

using namespace std;

/**
 * Loader constructor
 * Opens the .yo file named in the command line arguments, reads the contents of the file
 * line by line and loads the program into memory.  If no file is given or the file doesn't
 * exist or the file doesn't end with a .yo extension or the .yo file contains errors then
 * loaded is set to false.  Otherwise loaded is set to true.
 *
 * @param argc is the number of command line arguments passed to the main; should
 *        be 2
 * @param argv[0] is the name of the executable
 *        argv[1] is the name of the .yo file
 */
Loader::Loader(int argc, char * argv[])
{
   bool checkExtension = Loader::isYoExtension(argv[1]);
   if (!checkExtension) {
       loaded = false;
       return;
   }
   string temp;
  
   inf.open(argv[1]);

   int lineNumber = 1;
   int32_t addToAddress = 0;

   if (checkExtension == true)
   {
       //stores next line in temp sting
       while(getline(inf, temp))
       {
           char charLine[temp.capacity() + 1];
           //c_str splits temp into characters and puts chars in charLine
           strncpy(charLine, temp.c_str(), temp.capacity());
           //grabs memory address
           if (hasErrors(charLine)) {
               loaded = false;
               cout << "Error on line " << dec << lineNumber << ": " << temp << endl;
               return;
           }
           if (charLine[DATABEGIN] != ' ') {
               addToAddress = 0;
              
               for (int i = DATABEGIN; i < COMMENT - 1; i+=2) {
                   int64_t byte = convertData(charLine, i, 2);
                   loadLine(charLine, byte, addToAddress);
                   addToAddress++;
               }
           }
           lineNumber++;
       }
       loaded = true;
       inf.close();
       loaded = true;
   }   

}

/**
 * Checks for errors in the line that is being loaded.
 * 
 * @param line[] is the char array of the line
 * @return true if there is an error/false if no error
 */
bool Loader::hasErrors(char line[]) {   
    if (checkElse(line)) {
        return true;
    }
    if (hasData(line)) {
        if (!hasAddress(line)) {
            return true;
        }
    }
    if (!isBlank(line, 0)) {
        if (checkAddress(line, preAdd)) {
            return true;
        }
        if (hasData(line)) {
            if (checkData(line)) {
                return true;   
            }
        }
    }
    return false;
}

/**
 * Checks the address to make sure it's valid.
 *
 * @param line[] is the line itself
 * @param lastAdd the address of the previous line
 * @return true if there is an error in the address/false if no error
 */
bool Loader::checkAddress(char line[], uint64_t lastAdd) {
    if ((line[ADDRBEGIN - 2] != '0') || (line[ADDRBEGIN - 1] != 'x') || (line[ADDREND + 1] != ':')) {
        return true;
    }
    if (checkChar(line[ADDRBEGIN]) || checkChar(line[ADDRBEGIN + 1]) || checkChar(line[ADDREND])) {
        return true;
    }
    uint64_t thisAdd = convertData(line, ADDRBEGIN, 3);
    if (thisAdd < lastAdd) {
        return true;
    }
    return false;
}

/**
 * Checks the data of the line to make sure it's valid.
 *
 * @param line[] is the line
 * @return true if data has an error/false if no error
 */
bool Loader::checkData(char line[]) {
    preAdd = convertData(line, ADDRBEGIN, 3);
    for (int x = DATABEGIN; x < COMMENT - 1; x+=2) {
        if (line[x] == ' ') {
            return !(isBlank(line, x));
        }
        if (checkChar(line[x]) || checkChar(line[x+1])) {
            return true;
        }
        preAdd++;
        if (preAdd > 4096) {
            return true;
        }
    }
    return false;
}

/**
 * Checks the | character and checks the two spaces that should always be present.
 * 
 * @param line [] is the line
 * @return true if any of the three characters contain an error/false if no error
 */
bool Loader::checkElse(char line[]) {
    if (line[DATABEGIN - 1] != ' ' || line[COMMENT - 1] != ' ' || line[COMMENT] != '|') {
        return true;
    }
    return false;
}

/**
 * Checks if the line is blank from the specified character on.
 *
 * @param line[] is the line
 * @param spot is the character at which the check starts at
 * @return true if the line is blank/false otherwise
 */
bool Loader::isBlank(char line[], uint64_t spot) {
    for (int x = spot; x < COMMENT - 1; x++) {
        if (line[x] != ' ') {
            return false;
        }
    }
    return true;
}

/**
 * Checks if the line has an address
 *
 * @param line[] is the line
 * @return true if line has address/false otherwise
 */
bool Loader::hasAddress(char line[]) {
    if (!checkChar(line[ADDRBEGIN])) {
        return true;
    }
    return false;
}

/**
 * Checks if tbe line has data
 *
 * @param line[] is the line
 * @return true if the line has data/false otherwise
 */
bool Loader::hasData(char line[]) {
    for (int x = DATABEGIN; x < COMMENT - 1; x++) {
        if (!checkChar(line[x])) {
            return true;
        }
    }
    return false;
}

/**
 * Checks the given character to see if it's valid.
 *
 * @param symbol is the character to be checked
 * @return true if the character has an error/false if no error
 */
bool Loader::checkChar(char symbol) {  
    if (!(((int)symbol >= 48 && (int)symbol <= 57) || ((int)symbol >= 97 && (int)symbol <= 102))) {
        return true;
    }
    return false;
}

/**
 * Checks the file extension.
 *
 * @param fileName[] is the filename
 * @return true if the file has a .yo extension/false otherwise
 */
bool Loader::isYoExtension(char fileName[]){
 
    ifstream stream;

    uint64_t o = strlen(fileName) - 1;
    uint64_t y = strlen(fileName) - 2;

    stream.open(fileName);

    if (fileName[o] == 'o' && fileName[y] == 'y') {
        if (stream.is_open()  == true) {
            loaded = true;
            return true;
        }
    } else {
        loaded = false;
        return false;
    }
    return false;
}

/**
 * Loads the line into memory.
 *
 * @param line[] is the line
 * @param byte is the byte to be added
 * @param addToAddress is the number to increment the address by
 */
void Loader::loadLine(char line[], int64_t byte, int addToAddress) {
    Memory * mem = Memory::getInstance();
    bool error = false;
    int64_t memAddress = convertData(line, ADDRBEGIN, 3);
    memAddress += addToAddress;
    mem->putByte(byte, memAddress, error);
}

/**
 * Converts the data of the line into a 64bit int.
 *
 * @param line[] is the line
 * @param dataStart is where the data starts on the line
 * @param dataEnd is where the data ends on the line
 * @return the converted data
 */
int64_t Loader::convertData(char line[], int dataStart, int dataEnd) {
    string byte = "00";
    string str(line);
    if (str.substr(dataStart, dataEnd) != "  ")
    {
        byte = str.substr(dataStart, dataEnd);
    }
    return stoul(byte, nullptr, 16);
}

/**
 * isLoaded
 * returns the value of the loaded data member; loaded is set by the constructor
 *
 * @return value of loaded (true or false)
 */
bool Loader::isLoaded()
{
   return loaded;
}
