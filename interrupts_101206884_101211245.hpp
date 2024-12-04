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

constexpr uint16_t ADDR_BASE = 0x00; // base address for memory
constexpr uint16_t VECTOR_SIZE = 2;  // size of each vector entry

namespace utils {
    // trims whitespace from both ends of a string
    std::string trim(const std::string& str);
    
    // splits a string based on a delimiter
    std::vector<std::string> split_delim(const std::string& str, const std::string& delim);
    
    // formats a uint16_t value as a hexadecimal string
    std::string formatHex(uint16_t value);
}

enum ProcessState { NEW, READY, RUNNING, WAITING, TERMINATED }; // possible states of a process

struct VectorEntry {
    uint16_t interrupt_num; // interrupt number
    uint16_t isr_address;   // address of the interrupt service routine
};

struct Partition {
    unsigned int number;   // partition number
    unsigned int size;     // size of the partition
    int occupiedBy;        // pid of the occupying process, -1 if free
};

struct PCB {
    unsigned int pid;               // process identifier
    std::string programName;        // name of the program
    unsigned int arrivalTime;       // time when the process arrives
    unsigned int totalCPUTime;      // total cpu time required
    unsigned int remainingCPUTime;  // remaining cpu time
    unsigned int ioFrequency;       // frequency of io operations
    unsigned int initialIOFrequency; // initial io frequency
    unsigned int ioDuration;        // duration of io operations
    unsigned int initialIODuration; // initial io duration
    unsigned int nextIOTime;        // time until next io
    unsigned int partitionNumber;   // partition number where the process is located
    ProcessState state;             // current state of the process
    unsigned int size;              // memory size required by the process
    unsigned int priority;          // priority of the process for priority scheduling
    unsigned int lastScheduledTime; // last time the process was scheduled
    unsigned int totalWaitTime;     // total time spent in the ready queue
    unsigned int startTime;         // time when the process started execution
    unsigned int finishTime;        // time when the process finished execution
    unsigned int responseTime;      // time from arrival to first execution
    bool hasStarted;                // flag indicating if the process has started execution
    unsigned int totalIOTime;       // total time spent performing io operations
    unsigned int numberOfIO;        // number of io operations performed
};

class VectorTable {
private:
    std::vector<uint16_t> addresses; // list of isr addresses

public:
    // constructor that loads vector table from a file
    VectorTable(const std::string& filename);
    
    // loads vector table addresses from a file
    bool loadFromFile(const std::string& filename);
    
    // retrieves the isr address for a given interrupt number
    uint16_t getISRAddress(uint16_t interrupt_num) const;
    
    // gets the memory position in hexadecimal for a given interrupt number
    std::string getMemoryPositionHex(uint16_t interrupt_num) const;
    
    // gets the memory position as a uint16_t for a given interrupt number
    uint16_t getMemoryPosition(uint16_t interrupt_num) const;
};

class OSSimulator {
private:
    VectorTable* vectorTable;                // pointer to the vector table
    std::vector<Partition> memoryPartitions; // list of memory partitions
    std::vector<PCB> pcbTable;               // table of process control blocks
    unsigned int nextPID;                    // next available process id
    unsigned int currentTime;                // current simulation time
    std::string executionLog;                // log of process state transitions
    std::string memoryStatusLog;             // log of memory status over time
    std::mt19937 rng;                        // random number generator
    std::uniform_int_distribution<> execTimeDistr; // distribution for execution times
    std::string schedulerType;               // type of scheduler being used

    // clears the output files by truncating them
    void clearOutputFiles();
    
    // generates a random execution time
    int getRandomExecutionTime();
    
    // checks if a process is a child process based on its pid
    bool isChildProcess(unsigned int pid);
    
    // initializes memory partitions with predefined sizes
    void initializeMemoryPartitions();
    
    // finds the best fit partition for a given process size
    int findBestFitPartition(unsigned int size);
    
    // logs the state transition of a process
    void logStateTransition(unsigned int time, unsigned int pid, const std::string& oldState, const std::string& newState);
    
    // saves the current memory status to the memory status log
    void saveMemoryStatus(unsigned int time);

public:
    // constructor that initializes the simulator
    OSSimulator();
    
    // loads processes from an input file and initializes the pcb table
    void loadProcesses(const std::string& filename);
    
    // runs the simulation based on the specified scheduler type
    void simulate(const std::string& schedulerType);
    
    // saves the execution log to a file
    void saveExecution();
    
    // saves the memory status log to a file
    void saveMemoryStatus();
    
    // calculates and displays simulation metrics
    void calculateMetrics();
};

#endif