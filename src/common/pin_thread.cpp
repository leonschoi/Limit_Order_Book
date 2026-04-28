#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <pthread.h>
#include <vector>

#include "common/pin_thread.h"

/*
Check CPU and Core IDs to get a list of logical CPUs that correspond to 
physical cores.

This is important for pinning threads in a way that minimizes contention 
on shared resources like caches.

$ lscpu -e
CPU NODE SOCKET CORE L1d:L1i:L2:L3 ONLINE    MAXMHZ   MINMHZ       MHZ
  0    0      0    0 0:0:0:0          yes 5053.3770 426.1890 4231.1460
  1    0      0    1 1:1:1:0          yes 5053.3770 426.1890 2983.3191
  2    0      0    2 2:2:2:0          yes 5053.3770 426.1890 4838.3350
  3    0      0    3 3:3:3:0          yes 5053.3770 426.1890 4141.2920
  4    0      0    4 4:4:4:0          yes 5053.3770 426.1890 4787.7671
  5    0      0    5 5:5:5:0          yes 5053.3770 426.1890 4671.4282
  6    0      0    6 6:6:6:0          yes 5053.3770 426.1890 3541.8601
  7    0      0    7 7:7:7:0          yes 5053.3770 426.1890 4835.9028
  8    0      0    0 0:0:0:0          yes 5053.3770 426.1890 2983.3191
  9    0      0    1 1:1:1:0          yes 5053.3770 426.1890 3252.7319
 10    0      0    2 2:2:2:0          yes 5053.3770 426.1890 3573.5320
 11    0      0    3 3:3:3:0          yes 5053.3770 426.1890 3413.6431
 12    0      0    4 4:4:4:0          yes 5053.3770 426.1890 2983.3191
 13    0      0    5 5:5:5:0          yes 5053.3770 426.1890 2983.3191
 14    0      0    6 6:6:6:0          yes 5053.3770 426.1890 4026.2959
 15    0      0    7 7:7:7:0          yes 5053.3770 426.1890 4293.3960

*/

std::vector<int> get_physical_core_cpus() {
    std::ifstream file("/proc/cpuinfo");

    std::map<std::pair<int,int>, std::vector<int>> core_map;
    std::string line;

    int processor = -1, core_id = -1, physical_id = -1;

    while (std::getline(file, line)) {
        if (line.find("processor") != std::string::npos) {
            processor = std::stoi(line.substr(line.find(":") + 1));
        } 
        else if (line.find("core id") != std::string::npos) {
            core_id = std::stoi(line.substr(line.find(":") + 1));
        } 
        else if (line.find("physical id") != std::string::npos) {
            physical_id = std::stoi(line.substr(line.find(":") + 1));
        }

        if (processor != -1 && core_id != -1 && physical_id != -1) {
            core_map[{physical_id, core_id}].push_back(processor);
            processor = core_id = physical_id = -1;
        }
    }

    std::vector<int> result;

    // pick ONE logical CPU per physical core
    for (auto& [key, cpus] : core_map) {
        result.push_back(cpus[0]); // first sibling only
    }

    return result;
}


void pin_thread_physical(int thread_id, const std::vector<int>& cpu_map) {
    if (cpu_map.empty()) {
        std::cerr << "CPU map is empty, cannot pin thread " << thread_id << "\n";
        return;
    }

    int cpu = cpu_map[thread_id % cpu_map.size()];

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Affinity failed: " << std::strerror(rc) << "\n";
    }
}