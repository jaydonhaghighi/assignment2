#include "interrupts_101206884_101211245.hpp"

// utility namespace containing helper functions
namespace utils {
    // trims whitespace from both ends of a string
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (std::string::npos == first) {
            return str;
        }
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }

    // splits a string based on a delimiter
    std::vector<std::string> split_delim(const std::string& str, const std::string& delim) {
        std::vector<std::string> tokens;
        size_t prev = 0, pos = 0;
        do {
            pos = str.find(delim, prev);
            if (pos == std::string::npos) pos = str.length();
            std::string token = str.substr(prev, pos-prev);
            if (!token.empty()) tokens.push_back(token);
            prev = pos + delim.length();
        } while (pos < str.length() && prev < str.length());
        return tokens;
    }

    // formats a uint16_t value as a hexadecimal string
    std::string formatHex(uint16_t value) {
        std::stringstream ss;
        ss << "0x" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << value;
        return ss.str();
    }
}

// constructor for vector table, loads from a file
VectorTable::VectorTable(const std::string& filename) {
    loadFromFile(filename);
}

// loads vector table addresses from a file
bool VectorTable::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        line = utils::trim(line);
        if (line.empty()) continue;

        if (line.substr(0, 2) == "0X" || line.substr(0, 2) == "0x") {
            line = line.substr(2);
        }

        uint16_t address;
        std::istringstream iss(line);
        iss >> std::hex >> address;
        addresses.push_back(address);
    }
    file.close();
    return true;
}

// retrieves the ISR address for a given interrupt number
uint16_t VectorTable::getISRAddress(uint16_t interrupt_num) const {
    return addresses[interrupt_num];
}

// gets the memory position in hexadecimal for a given interrupt number
std::string VectorTable::getMemoryPositionHex(uint16_t interrupt_num) const {
    return utils::formatHex(ADDR_BASE + (interrupt_num * VECTOR_SIZE));
}

// gets the memory position as a uint16_t for a given interrupt number
uint16_t VectorTable::getMemoryPosition(uint16_t interrupt_num) const {
    return ADDR_BASE + (interrupt_num * VECTOR_SIZE);
}


// constructor for OS simulator, initializes variables and memory partitions
OSSimulator::OSSimulator()
    : nextPID(1), currentTime(0),
      rng(std::random_device()()),
      execTimeDistr(1, 10),
      schedulerType("FCFS")
{
    clearOutputFiles();
    initializeMemoryPartitions();
}

// clears the output files by truncating them
void OSSimulator::clearOutputFiles() {
    std::ofstream("execution.txt", std::ios::trunc).close();
    std::ofstream("memory_status.txt", std::ios::trunc).close();
}

// generates a random execution time between 1 and 10
int OSSimulator::getRandomExecutionTime() {
    return execTimeDistr(rng);
}

// checks if a process is a child process based on its pid
bool OSSimulator::isChildProcess(unsigned int pid) {
    return pid != 1;
}

// initializes memory partitions with predefined sizes
void OSSimulator::initializeMemoryPartitions() {
    std::vector<unsigned int> sizes = {40, 25, 15, 10, 8, 2};
    for(unsigned int i = 0; i < sizes.size(); i++) {
        memoryPartitions.push_back({i+1, sizes[i], -1});
    }
}

// finds the best fit partition for a given process size
int OSSimulator::findBestFitPartition(unsigned int size) {
    int bestFit = -1;
    unsigned int minDiff = UINT_MAX;

    for(size_t i = 0; i < memoryPartitions.size(); i++) {
        if(memoryPartitions[i].occupiedBy == -1 && memoryPartitions[i].size >= size) {
            unsigned int diff = memoryPartitions[i].size - size;
            if(diff < minDiff) {
                minDiff = diff;
                bestFit = i;
            }
        }
    }
    return bestFit;
}

// loads processes from an input file and initializes PCB table
void OSSimulator::loadProcesses(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while(std::getline(file, line)) {
        line = utils::trim(line);
        if(line.empty()) continue;

        auto parts = utils::split_delim(line, ",");
        if(parts.size() < 6) continue;

        PCB pcb;
        pcb.pid = std::stoi(utils::trim(parts[0]));
        pcb.size = std::stoi(utils::trim(parts[1]));
        pcb.arrivalTime = std::stoi(utils::trim(parts[2]));
        pcb.totalCPUTime = std::stoi(utils::trim(parts[3]));
        pcb.remainingCPUTime = pcb.totalCPUTime;
        pcb.ioFrequency = std::stoi(utils::trim(parts[4]));
        pcb.initialIOFrequency = pcb.ioFrequency;
        pcb.ioDuration = std::stoi(utils::trim(parts[5]));
        pcb.initialIODuration = pcb.ioDuration;
        pcb.nextIOTime = pcb.ioFrequency;
        pcb.state = NEW;
        pcb.partitionNumber = 0;
        pcb.programName = "Program_" + std::to_string(pcb.pid);
        pcb.lastScheduledTime = 0;
        pcb.totalWaitTime = 0;
        pcb.startTime = 0;
        pcb.finishTime = 0;
        pcb.responseTime = 0;
        pcb.hasStarted = false;
        pcb.totalIOTime = 0;
        pcb.numberOfIO = 0;

        if(parts.size() >= 7) {
            pcb.priority = std::stoi(utils::trim(parts[6]));
        } else {
            pcb.priority = 0;
        }

        pcbTable.push_back(pcb);
    }
}

// runs the simulation based on the specified scheduler type
void OSSimulator::simulate(const std::string& schedulerType) {
    this->schedulerType = schedulerType;

    std::vector<PCB*> readyQueue;
    std::vector<PCB*> waitingQueue;
    PCB* runningProcess = nullptr;

    unsigned int timeQuantum = 100;
    unsigned int timeSliceRemaining = 0;

    std::vector<PCB*> memoryWaitQueue;

    // initialize execution log header
    executionLog += "+--------------------+-----+-------------+------------+\n";
    executionLog += "| Time of Transition | PID |  Old State  | New State  |\n";
    executionLog += "+--------------------+-----+-------------+------------+\n";

    while (true) {
        // check if all processes are terminated
        bool allTerminated = true;
        for (auto& pcb : pcbTable) {
            if (pcb.state != TERMINATED) {
                allTerminated = false;
                break;
            }
        }
        if (allTerminated) break;

        // handle process arrivals
        for (auto& pcb : pcbTable) {
            if (pcb.arrivalTime == currentTime && pcb.state == NEW) {
                int partitionIndex = findBestFitPartition(pcb.size);
                if (partitionIndex >= 0) {
                    memoryPartitions[partitionIndex].occupiedBy = pcb.pid;
                    pcb.partitionNumber = memoryPartitions[partitionIndex].number;
                    pcb.state = READY;
                    pcb.startTime = currentTime;
                    saveMemoryStatus(currentTime);
                    logStateTransition(currentTime, pcb.pid, "NEW", "READY");
                    readyQueue.push_back(&pcb);
                } else {
                    memoryWaitQueue.push_back(&pcb);
                }
            }
        }

        // allocate memory to waiting processes
        for (auto it = memoryWaitQueue.begin(); it != memoryWaitQueue.end();) {
            PCB* pcb = *it;
            int partitionIndex = findBestFitPartition(pcb->size);
            if (partitionIndex >= 0) {
                memoryPartitions[partitionIndex].occupiedBy = pcb->pid;
                pcb->partitionNumber = memoryPartitions[partitionIndex].number;
                pcb->state = READY;
                pcb->startTime = currentTime;
                saveMemoryStatus(currentTime);
                logStateTransition(currentTime, pcb->pid, "NEW", "READY");
                readyQueue.push_back(pcb);
                it = memoryWaitQueue.erase(it);
            } else {
                ++it;
            }
        }

        // update waiting processes for I/O completion
        for (auto it = waitingQueue.begin(); it != waitingQueue.end();) {
            PCB* pcb = *it;
            pcb->ioDuration--;
            pcb->totalIOTime++;

            if (pcb->ioDuration <= 0) {
                pcb->state = READY;
                pcb->ioDuration = pcb->initialIODuration;
                pcb->nextIOTime = pcb->initialIOFrequency;
                logStateTransition(currentTime, pcb->pid, "WAITING", "READY");
                readyQueue.push_back(pcb);
                it = waitingQueue.erase(it);
            } else {
                ++it;
            }
        }

        // handle time slice expiration for round robin scheduler
        if (schedulerType == "RR") {
            if (runningProcess != nullptr) {
                timeSliceRemaining--;
                if (timeSliceRemaining <= 0) {
                    runningProcess->state = READY;
                    logStateTransition(currentTime, runningProcess->pid, "RUNNING", "READY");
                    readyQueue.push_back(runningProcess);
                    runningProcess = nullptr;
                }
            }
        }

        // schedule the next process if no process is currently running
        if (runningProcess == nullptr && !readyQueue.empty()) {
            PCB* nextProcess = nullptr;
            if (schedulerType == "FCFS") {
                nextProcess = readyQueue.front();
                readyQueue.erase(readyQueue.begin());
            } else if (schedulerType == "EP") {
                auto it = std::min_element(readyQueue.begin(), readyQueue.end(), [](PCB* a, PCB* b) {
                    return a->priority < b->priority;
                });
                nextProcess = *it;
                readyQueue.erase(it);
            } else if (schedulerType == "RR") {
                nextProcess = readyQueue.front();
                readyQueue.erase(readyQueue.begin());
                timeSliceRemaining = timeQuantum;
            }
            if (nextProcess != nullptr) {
                runningProcess = nextProcess;
                runningProcess->state = RUNNING;

                // set response time if process is starting for the first time
                if (!runningProcess->hasStarted) {
                    runningProcess->responseTime = currentTime - runningProcess->arrivalTime;
                    runningProcess->hasStarted = true;
                }

                runningProcess->lastScheduledTime = currentTime;
                logStateTransition(currentTime, runningProcess->pid, "READY", "RUNNING");
            }
        }

        // simulate the running process
        if (runningProcess != nullptr) {
            runningProcess->remainingCPUTime--;
            runningProcess->nextIOTime--;
            if (runningProcess->remainingCPUTime <= 0) {
                runningProcess->state = TERMINATED;
                runningProcess->finishTime = currentTime;

                // release memory occupied by the process
                for (auto& partition : memoryPartitions) {
                    if (partition.occupiedBy == runningProcess->pid) {
                        partition.occupiedBy = -1;
                        break;
                    }
                }
                saveMemoryStatus(currentTime);
                logStateTransition(currentTime, runningProcess->pid, "RUNNING", "TERMINATED");
                runningProcess = nullptr;
            } else if (runningProcess->nextIOTime <= 0) {
                runningProcess->state = WAITING;
                runningProcess->nextIOTime = runningProcess->initialIOFrequency;
                runningProcess->ioDuration = runningProcess->initialIODuration;
                runningProcess->numberOfIO++;
                logStateTransition(currentTime, runningProcess->pid, "RUNNING", "WAITING");
                waitingQueue.push_back(runningProcess);
                runningProcess = nullptr;
            }
        }

        // update wait times for all processes in the ready queue
        for (auto& pcb : readyQueue) {
            pcb->totalWaitTime++;
        }

        currentTime++;
    }
}

// logs the state transition of a process
void OSSimulator::logStateTransition(unsigned int time, unsigned int pid, const std::string& oldState, const std::string& newState) {
    std::stringstream ss;

    ss << "| " << std::setw(18) << std::left << time << " | "
       << std::setw(3) << std::left << pid << " | "
       << std::setw(11) << std::left << oldState << " | "
       << std::setw(10) << std::left << newState << " |\n";
    executionLog += ss.str();
}

// saves the current memory status to the memory status log
void OSSimulator::saveMemoryStatus(unsigned int time) {
    std::stringstream ss;
    unsigned int memoryUsed = 0;
    unsigned int totalFreeMemory = 0;
    unsigned int usableFreeMemory = 0;
    std::vector<int> partitionsState;
    for (auto& partition : memoryPartitions) {
        if (partition.occupiedBy != -1) {
            memoryUsed += partition.size;
            partitionsState.push_back(partition.occupiedBy);
        } else {
            totalFreeMemory += partition.size;
            usableFreeMemory += partition.size;
            partitionsState.push_back(-1);
        }
    }

    if (memoryStatusLog.empty()) {
        memoryStatusLog += "+------------+------------+---------------------------+-------------------+-------------------+\n";
        memoryStatusLog += "| Time Event | Memory Used|   Partitions State        | Total Free Memory | Usable Free Memory|\n";
        memoryStatusLog += "+------------+------------+---------------------------+-------------------+-------------------+\n";
    }

    ss << "| " << std::setw(10) << std::left << time << " | "
       << std::setw(10) << std::left << memoryUsed << " | ";

    std::stringstream ps;
    for (size_t i = 0; i < partitionsState.size(); i++) {
        ps << partitionsState[i];
        if (i != partitionsState.size() -1) ps << ", ";
    }
    ss << std::setw(25) << std::left << ps.str() << " | "
       << std::setw(17) << std::left << totalFreeMemory << " | "
       << std::setw(17) << std::left << usableFreeMemory << " |\n";

    memoryStatusLog += ss.str();
}

// saves the execution log to a file
void OSSimulator::saveExecution() {
    executionLog += "+--------------------+-----+-------------+------------+\n";
    std::ofstream file("execution.txt");
    file << executionLog;
}

// saves the memory status log to a file
void OSSimulator::saveMemoryStatus() {
    memoryStatusLog += "+------------+------------+---------------------------+-------------------+-------------------+\n";
    std::ofstream file("memory_status.txt");
    file << memoryStatusLog;
}

// calculates and displays simulation metrics
void OSSimulator::calculateMetrics() {
    unsigned int totalTurnaroundTime = 0;
    unsigned int totalWaitTime = 0;
    unsigned int totalResponseTime = 0;
    unsigned int totalProcesses = pcbTable.size();
    unsigned int processesCompleted = 0;
    unsigned int totalIOTime = 0;

    for (const auto& pcb : pcbTable) {
        if (pcb.state == TERMINATED) {
            unsigned int turnaroundTime = pcb.finishTime - pcb.arrivalTime;
            totalTurnaroundTime += turnaroundTime;
            totalWaitTime += pcb.totalWaitTime;
            totalResponseTime += pcb.responseTime;
            totalIOTime += pcb.totalIOTime;
            processesCompleted++;
        }
    }

    double averageTurnaroundTime = processesCompleted ? static_cast<double>(totalTurnaroundTime) / processesCompleted : 0;
    double averageWaitTime = processesCompleted ? static_cast<double>(totalWaitTime) / processesCompleted : 0;
    double averageResponseTime = processesCompleted ? static_cast<double>(totalResponseTime) / processesCompleted : 0;
    double throughput = currentTime ? static_cast<double>(processesCompleted) / currentTime : 0;
    double averageIOTime = processesCompleted ? static_cast<double>(totalIOTime) / processesCompleted : 0;

    // output the metrics
    std::cout << "\nSimulation Metrics:\n";
    std::cout << "Scheduler Type: " << schedulerType << "\n";
    std::cout << "Total Simulation Time: " << currentTime << " ms\n";
    std::cout << "Processes Completed: " << processesCompleted << "\n";
    std::cout << "Throughput: " << throughput << " processes/ms\n";
    std::cout << "Average Turnaround Time: " << averageTurnaroundTime << " ms\n";
    std::cout << "Average Wait Time: " << averageWaitTime << " ms\n";
    std::cout << "Average Response Time: " << averageResponseTime << " ms\n";
    std::cout << "Average I/O Time: " << averageIOTime << " ms\n";
}

// main function to run the OS simulator
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_data.txt> [scheduler]\n";
        return 1;
    }

    OSSimulator simulator;
    simulator.loadProcesses(argv[1]);

    std::string scheduler = "FCFS";
    if (argc >= 3) {
        scheduler = argv[2];
    }

    simulator.simulate(scheduler);
    simulator.saveExecution();
    simulator.saveMemoryStatus();
    simulator.calculateMetrics();

    std::cout << "Simulation completed successfully\n";
    return 0;
}