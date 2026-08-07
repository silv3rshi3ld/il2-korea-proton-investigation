/*
 * Minimal Win64 probe for Wine's NUMA topology and IL-2's bundled OpenMP DLL.
 *
 * Build with:
 *   x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -o numa-openmp-probe.exe \
 *       probes/numa-openmp-probe.c
 */

#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*omp_int_fn)(void);

static void print_group_affinity(const GROUP_AFFINITY *affinity)
{
    printf("mask=0x%llx group=%u reserved=%u,%u,%u\n",
            (unsigned long long)affinity->Mask, affinity->Group,
            affinity->Reserved[0], affinity->Reserved[1], affinity->Reserved[2]);
}

static void print_numa_topology(void)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, *entry;
    DWORD length = 0, offset = 0, error;

    SetLastError(ERROR_SUCCESS);
    if (GetLogicalProcessorInformationEx(RelationNumaNode, NULL, &length) ||
            (error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
    {
        printf("GetLogicalProcessorInformationEx(size): ret=unexpected error=%lu length=%lu\n",
                GetLastError(), length);
        return;
    }

    if (!(buffer = malloc(length)))
    {
        printf("GetLogicalProcessorInformationEx: allocation failed\n");
        return;
    }

    SetLastError(ERROR_SUCCESS);
    if (!GetLogicalProcessorInformationEx(RelationNumaNode, buffer, &length))
    {
        printf("GetLogicalProcessorInformationEx(data): ret=0 error=%lu length=%lu\n",
                GetLastError(), length);
        free(buffer);
        return;
    }

    while (offset < length)
    {
        entry = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)buffer + offset);
        if (!entry->Size || offset + entry->Size > length)
        {
            printf("GetLogicalProcessorInformationEx: malformed entry at offset=%lu\n", offset);
            break;
        }
        printf("RelationNumaNode: node=%lu ", entry->NumaNode.NodeNumber);
        print_group_affinity(&entry->NumaNode.GroupMask);
        offset += entry->Size;
    }

    free(buffer);
}

int main(int argc, char **argv)
{
    GROUP_AFFINITY affinity;
    ULONG highest = 0xdeadbeef;
    HMODULE module;
    FARPROC proc;
    omp_int_fn omp_get_max_threads, omp_get_num_procs;
    BOOL ret;

    printf("active_groups=%u active_group0=%lu\n",
            GetActiveProcessorGroupCount(), GetActiveProcessorCount(0));

    SetLastError(0xdeadbeef);
    ret = GetNumaHighestNodeNumber(&highest);
    printf("GetNumaHighestNodeNumber: ret=%d error=%lu highest=%lu\n",
            ret, GetLastError(), highest);

    memset(&affinity, 0xa5, sizeof(affinity));
    SetLastError(0xdeadbeef);
    ret = GetNumaNodeProcessorMaskEx(0, &affinity);
    printf("GetNumaNodeProcessorMaskEx(0): ret=%d error=%lu ", ret, GetLastError());
    print_group_affinity(&affinity);
    print_numa_topology();

    if (argc < 2)
    {
        printf("OpenMP DLL not supplied; NUMA-only probe complete.\n");
        return 0;
    }

    SetLastError(ERROR_SUCCESS);
    module = LoadLibraryA(argv[1]);
    if (!module)
    {
        printf("LoadLibraryA(%s): error=%lu\n", argv[1], GetLastError());
        return 2;
    }

    proc = GetProcAddress(module, "omp_get_max_threads");
    memcpy(&omp_get_max_threads, &proc, sizeof(omp_get_max_threads));
    proc = GetProcAddress(module, "omp_get_num_procs");
    memcpy(&omp_get_num_procs, &proc, sizeof(omp_get_num_procs));
    if (!omp_get_max_threads || !omp_get_num_procs)
    {
        printf("OpenMP exports missing: max=%s procs=%s error=%lu\n",
                omp_get_max_threads ? "yes" : "no", omp_get_num_procs ? "yes" : "no",
                GetLastError());
        FreeLibrary(module);
        return 3;
    }

    printf("OpenMP: num_procs=%d max_threads=%d\n",
            omp_get_num_procs(), omp_get_max_threads());
    FreeLibrary(module);
    return 0;
}
