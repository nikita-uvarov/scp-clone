#include "PerformanceManager.h"

#include <cstdio>
#include <cstdlib>

using namespace sge;
using namespace std;

void sge::PerformanceManager::updatePerformanceInformation()
{
    updateIntelGpuUsage();
    updateCpuUsage();
    updateVirtualMemoryUsage();
}

sge::PerformanceManager& sge::PerformanceManager::instance()
{
    static PerformanceManager theInstance;
    return theInstance;
}

void sge::PerformanceManager::updateIntelGpuUsage()
{
    FILE* tempFile = fopen("/tmp/tmp-intel_gpu_time", "r");
    if (tempFile)
    {
        fscanf(tempFile, "%*[^G]GPU: %lf", &gpuUsageRate);
        gpuUsageRate /= 100.0;
        
        fclose(tempFile);
    }
    
    int errorCode = system("intel_gpu_time sleep 0.5 >/tmp/tmp-intel_gpu_time &");
    if (errorCode != 0)
        fprintf(stderr, "warning: intel_gpu_time error code: %d\n", errorCode);
}
    
#include "sys/times.h"
#include "sys/vtimes.h"

// returns current cpu usage rate (sum over all processors, >1 possible)
ftype getCurrentCpuUsageRate()
{
    static clock_t lastCPU, lastSysCPU, lastUserCPU;
    
    tms timeSample;
    ftype rate;

    clock_t now = times(&timeSample);
    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
        timeSample.tms_utime < lastUserCPU)
    {
        rate = -1.0;
    }
    else
    {
        rate = (ftype)((timeSample.tms_stime - lastSysCPU) + (timeSample.tms_utime - lastUserCPU));
        rate /= (ftype)(now - lastCPU);
    }
    
    lastCPU = now;
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    return rate;
}
    
void PerformanceManager::updateCpuUsage()
{
    cpuUsageRate = getCurrentCpuUsageRate();
}

int getProcSelfStatusVmRSS()
{
    FILE* file = fopen("/proc/self/status", "r");
    int vmRss = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL)
    {
        if (sscanf(line, "VmRSS: %d", &vmRss) == 1)
            break;
    }
    
    fclose(file);
    return vmRss;
}

void PerformanceManager::updateVirtualMemoryUsage()
{
    virtualMemoryBytes = getProcSelfStatusVmRSS() * 1024;
}

string PerformanceManager::formatPerformanceString()
{
    return "CPU: " + to_string((int)(cpuUsageRate * 100)) +
        "%, GPU: " + to_string((int)(gpuUsageRate * 100)) +
        "%, mem: " + to_string((int)(virtualMemoryBytes / 1e6)) + " Mb";
}
