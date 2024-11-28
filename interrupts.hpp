#ifndef INTERRUPTS_HPP
#define INTERRUPTS_HPP

#include <string>
#include <vector>
#include <random>

constexpr uint16_t ADDR_BASE = 0x00;
constexpr uint16_t VECTOR_SIZE = 2;

namespace utils {
    std::string trim(const std::string& str);
    std::vector<std::string> split_delim(const std::string& str, const std::string& delim);
    std::string formatHex(uint16_t value);
}

struct VectorEntry {
    uint16_t interrupt_num;
    uint16_t isr_address;
};

struct Partition {
    unsigned int number;
    unsigned int size;
    std::string code;
    bool occupied;
};

struct PCB {
    unsigned int pid;
    std::string programName;
    unsigned int partitionNumber;
    unsigned int size;
    unsigned int remainingCPUTime;
    bool isReady;
    unsigned int parentPID;
};

struct ExternalFile {
    std::string programName;
    unsigned int size;
};

struct ProgramTrace {
    std::string command;
    int duration;
};

class VectorTable {
private:
    std::vector<uint16_t> addresses;

public:
    VectorTable(const std::string& filename);
    bool loadFromFile(const std::string& filename);
    uint16_t getISRAddress(uint16_t interrupt_num) const;
    std::string getMemoryPositionHex(uint16_t interrupt_num) const;
    uint16_t getMemoryPosition(uint16_t interrupt_num) const;
};

class OSSimulator {
private:
    VectorTable* vectorTable;
    std::vector<Partition> memoryPartitions;
    std::vector<PCB> pcbTable;
    std::vector<ExternalFile> externalFiles;
    unsigned int nextPID;
    int currentTime;
    std::string executionLog;
    std::mt19937 rng;
    std::uniform_int_distribution<> execTimeDistr;

    void clearOutputFiles();
    int getRandomExecutionTime();
    bool isChildProcess(unsigned int pid);
    void initializeMemoryPartitions();
    void initializePCB0();
    int findBestFitPartition(unsigned int size);
    void loadExternalFiles(const std::string& filename);
    std::vector<ProgramTrace> loadProgramTrace(const std::string& programName);
    unsigned int getProgramSize(const std::string& programName);
    void logActivity(int duration, const std::string& activity);

public:
    OSSimulator();
    void setVectorTable(VectorTable* vt);
    void executeProgram(const std::string& programName);
    void processTrace(const std::string& command, int duration);
    void handleFork(unsigned int parentPID);
    void handleExec(const std::string& programName);
    void handleSyscall(int syscallNum, int duration);
    void handleEndIO(int ioNum, int duration);
    void saveSystemStatus();
    void saveExecution();
};

#endif