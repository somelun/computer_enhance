#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/sysctl.h>

uint64_t get_cpu_freq(void)
{
    uint64_t freq = 0;
    size_t size = sizeof(freq);

    if (sysctlbyname("hw.cpufrequency", &freq, &size, NULL, 0) < 0)
    {
        perror("sysctl");
    }
    return freq;
}

int main() {
    uint64_t ad = get_cpu_freq();
    printf("%llu\n", ad);

    return 0;
}

