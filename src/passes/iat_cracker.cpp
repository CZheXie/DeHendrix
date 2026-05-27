#include "deobf/iat_cracker.h"
#include <algorithm>
#include <cstring>

namespace deobf {

void IATCracker::add_api(const std::string& name) {
    api_names_.push_back(name);
}

void IATCracker::load_default_kernel_apis() {
    static const char* apis[] = {
        "ExAllocatePool", "ExAllocatePoolWithTag", "ExFreePool", "ExFreePoolWithTag",
        "ExAllocatePool2", "ExAllocatePool3",
        "MmGetSystemRoutineAddress", "MmCopyVirtualMemory", "MmCopyMemory",
        "MmIsAddressValid", "MmMapIoSpace", "MmUnmapIoSpace",
        "MmGetPhysicalAddress", "MmAllocateContiguousMemory",
        "IoCreateDevice", "IoDeleteDevice", "IoCreateSymbolicLink", "IoDeleteSymbolicLink",
        "IoCompleteRequest", "IoAllocateMdl", "IoFreeMdl", "IoGetCurrentProcess",
        "PsCreateSystemThread", "PsTerminateSystemThread",
        "PsSetCreateProcessNotifyRoutine", "PsSetCreateProcessNotifyRoutineEx",
        "PsSetCreateThreadNotifyRoutine", "PsSetLoadImageNotifyRoutine",
        "PsGetCurrentProcess", "PsGetCurrentThread", "PsGetProcessId",
        "PsLookupProcessByProcessId", "PsLookupThreadByThreadId",
        "ObReferenceObjectByHandle", "ObDereferenceObject", "ObOpenObjectByPointer",
        "ObRegisterCallbacks", "ObUnRegisterCallbacks",
        "KeWaitForSingleObject", "KeSetEvent", "KeInitializeEvent",
        "KeAcquireSpinLockRaiseToDpc", "KeReleaseSpinLock",
        "KeEnterCriticalRegion", "KeLeaveCriticalRegion",
        "KeStackAttachProcess", "KeUnstackDetachProcess",
        "KeQueryActiveProcessorCount", "KeGetCurrentIrql",
        "ZwQuerySystemInformation", "ZwQueryInformationProcess",
        "ZwOpenProcess", "ZwClose", "ZwCreateFile", "ZwReadFile", "ZwWriteFile",
        "ZwQueryVirtualMemory", "ZwAllocateVirtualMemory", "ZwFreeVirtualMemory",
        "RtlInitUnicodeString", "RtlCopyUnicodeString", "RtlGetVersion",
        "RtlRandomEx", "RtlInitAnsiString",
        "FltRegisterFilter", "FltUnregisterFilter", "FltStartFiltering",
        "FltGetFileNameInformation", "FltReleaseFileNameInformation",
        "KeIpiGenericCall", "KeSetAffinityThread",
        "KdDisableDebugger", "KdEnableDebugger",
        "ProbeForRead", "ProbeForWrite",
        "IoCreateDeviceSecure", "IoValidateDeviceIoControlAccess",
        "MmMapMemoryDumpMdl", "PsGetProcessSessionId",
        "NtQueryInformationProcess", "NtReadVirtualMemory", "NtWriteVirtualMemory",
        "KeAcquireGuardedMutex", "KeReleaseGuardedMutex",
        "IofCompleteRequest", "IofCallDriver",
    };
    for (auto name : apis) add_api(name);
}

uint64_t IATCracker::fnv1a_64(const std::string& s) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (char c : s) {
        h ^= static_cast<uint8_t>(c);
        h *= 0x100000001b3ULL;
    }
    return h;
}

uint64_t IATCracker::fnv1a_64_lower(const std::string& s) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (char c : s) {
        h ^= static_cast<uint8_t>(c >= 'A' && c <= 'Z' ? c + 32 : c);
        h *= 0x100000001b3ULL;
    }
    return h;
}

uint64_t IATCracker::djb2_64(const std::string& s) {
    uint64_t h = 5381;
    for (char c : s) h = h * 33 + static_cast<uint8_t>(c);
    return h;
}

uint64_t IATCracker::sdbm_64(const std::string& s) {
    uint64_t h = 0;
    for (char c : s) h = static_cast<uint8_t>(c) + (h << 6) + (h << 16) - h;
    return h;
}

uint64_t IATCracker::ror13_add(const std::string& s) {
    uint32_t h = 0;
    for (char c : s) {
        h = ((h >> 13) | (h << 19)) + static_cast<uint8_t>(c);
    }
    return h;
}

uint64_t IATCracker::ror13_add_unicode(const std::string& s) {
    uint32_t h = 0;
    for (char c : s) {
        h = ((h >> 13) | (h << 19)) + static_cast<uint16_t>(c);
    }
    return h;
}

uint64_t IATCracker::crc32_custom(const std::string& s, uint64_t seed) {
    uint32_t crc = static_cast<uint32_t>(seed);
    for (char c : s) {
        crc ^= static_cast<uint8_t>(c);
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return crc;
}

void IATCracker::precompute() {
    hash_to_api_.clear();
    for (auto& name : api_names_) {
        hash_to_api_[fnv1a_64(name)] = {name, "fnv1a"};
        hash_to_api_[fnv1a_64_lower(name)] = {name, "fnv1a_lower"};
        hash_to_api_[djb2_64(name)] = {name, "djb2"};
        hash_to_api_[sdbm_64(name)] = {name, "sdbm"};
        hash_to_api_[ror13_add(name)] = {name, "ror13"};
        hash_to_api_[ror13_add_unicode(name)] = {name, "ror13_unicode"};
        hash_to_api_[crc32_custom(name, 0)] = {name, "crc32_0"};
        hash_to_api_[crc32_custom(name, 0xFFFFFFFF)] = {name, "crc32_ff"};
    }
}

std::vector<IATCracker::HashMatch> IATCracker::scan(
    const uint8_t* data, size_t len, uint64_t base_offset) const {
    std::vector<HashMatch> results;
    if (len < 8) return results;

    for (size_t i = 0; i <= len - 8; i += 8) {
        uint64_t val;
        std::memcpy(&val, data + i, 8);
        if (val == 0) continue;
        auto it = hash_to_api_.find(val);
        if (it != hash_to_api_.end()) {
            results.push_back({
                base_offset + i,
                val,
                it->second.first,
                it->second.second,
            });
        }
    }
    return results;
}

} // namespace deobf
