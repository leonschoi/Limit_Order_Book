#pragma once
#include <pthread.h>
#include <vector>
#include <thread>
#include <iostream>
#include <cstring>


void pin_thread_physical(int thread_id, const std::vector<int>& cpu_map);
std::vector<int> get_physical_core_cpus();
