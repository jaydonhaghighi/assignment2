#ifndef INTERRUPTS_HPP
#define INTERRUPTS_HPP

#include <string>
#include <vector>
#include <random>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <climits>
#include <iostream>

constexpr uint16_t ADDR_BASE = 0x00;
constexpr uint16_t VECTOR_SIZE = 2;

namespace utils {
    std::string trim(const std::string& str);
    std::vector<std::string> split_delim(const std::string& str, const std::string& delim);
    std::string formatHex(uint16_t value);
}

enum ProcessState { NEW, READY, RUNNING, WAITING, TERMINATED };

struct VectorEntry {
    uint16_t interrupt_num;
    uint16_t isr_address;
};

struct Partition {
    unsigned int number;
    unsigned int size;
    int occupiedBy;
};

struct PCB {
    unsigned int pid;
    std::string programName;
    unsigned int arrivalTime;
    unsigned int totalCPUTime;
    unsigned int remainingCPUTime;
    unsigned int ioFrequency;
    unsigned int ioDuration;
    unsigned int nextIOTime;
    unsigned int partitionNumber;
    ProcessState state;
    unsigned int size;
    unsigned int priority;
    unsigned int lastScheduledTime;
    unsigned int totalWaitTime;
    unsigned int startTime;
    unsigned int finishTime;
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
    unsigned int nextPID;
    unsigned int currentTime;
    std::string executionLog;
    std::string memoryStatusLog;
    std::mt19937 rng;
    std::uniform_int_distribution<> execTimeDistr;

    void clearOutputFiles();
    int getRandomExecutionTime();
    bool isChildProcess(unsigned int pid);
    void initializeMemoryPartitions();
    int findBestFitPartition(unsigned int size);
    void logActivity(int duration, const std::string& activity);
    void logStateTransition(unsigned int time, unsigned int pid, const std::string& oldState, const std::string& newState);
    void saveMemoryStatus(unsigned int time);

public:
    OSSimulator();
    void setVectorTable(VectorTable* vt);
    void loadProcesses(const std::string& filename);
    void simulate(const std::string& schedulerType);
    void saveExecution();
    void saveMemoryStatus();
};

#endif
