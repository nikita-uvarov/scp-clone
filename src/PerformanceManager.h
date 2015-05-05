#ifndef SGE_PERFORMANCE_MANAGER_H
#define SGE_PERFORMANCE_MANAGER_H

#include "Common.h"
#include <string>

namespace sge
{
    
class PerformanceManager
{
    PerformanceManager() {}
    
    ftype gpuUsageRate = 0;
    void updateIntelGpuUsage();
    
    ftype cpuUsageRate = 0;
    void updateCpuUsage();
    
    ftype virtualMemoryBytes = 0;
    void updateVirtualMemoryUsage();
public :
    static PerformanceManager& instance();
    
    std::string formatPerformanceString();
    
    // must be called in 1 second intervals
    void updatePerformanceInformation();
};
    
}

#endif