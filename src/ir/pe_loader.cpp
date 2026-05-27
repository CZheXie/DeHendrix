#include "deobf/pe_loader.h"
#include <cstring>
#include <algorithm>

namespace deobf {

// Minimal PE parsing — just enough for devirtualization use cases.
// For production use, replace with a proper PE library (LIEF, pe-parse, etc.)

#pragma pack(push, 1)
struct DOS_HEADER {
    uint16_t e_magic;
    uint16_t e_cblp, e_cp, e_crlc, e_cparhdr, e_minalloc, e_maxalloc;
    uint16_t e_ss, e_sp, e_csum, e_ip, e_cs, e_lfarlc, e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid, e_oeminfo;
    uint16_t e_res2[10];
    int32_t e_lfanew;
};

struct FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

struct OPTIONAL_HEADER_64 {
    uint16_t Magic;
    uint8_t MajorLinkerVersion, MinorLinkerVersion;
    uint32_t SizeOfCode, SizeOfInitializedData, SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment, FileAlignment;
    uint16_t MajorOSVersion, MinorOSVersion;
    uint16_t MajorImageVersion, MinorImageVersion;
    uint16_t MajorSubsystemVersion, MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem, DllCharacteristics;
    uint64_t SizeOfStackReserve, SizeOfStackCommit;
    uint64_t SizeOfHeapReserve, SizeOfHeapCommit;
    uint32_t LoaderFlags, NumberOfRvaAndSizes;
};

struct DATA_DIRECTORY {
    uint32_t VirtualAddress;
    uint32_t Size;
};

struct SECTION_HEADER {
    char Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
};
#pragma pack(pop)

std::optional<PEInfo> PELoader::parse(const uint8_t* data, size_t len) {
    if (len < sizeof(DOS_HEADER)) return std::nullopt;

    auto* dos = reinterpret_cast<const DOS_HEADER*>(data);
    if (dos->e_magic != 0x5A4D) return std::nullopt; // "MZ"

    uint32_t pe_offset = dos->e_lfanew;
    if (pe_offset + 4 > len) return std::nullopt;

    uint32_t pe_sig;
    std::memcpy(&pe_sig, data + pe_offset, 4);
    if (pe_sig != 0x00004550) return std::nullopt; // "PE\0\0"

    auto* fh = reinterpret_cast<const FILE_HEADER*>(data + pe_offset + 4);

    PEInfo pe{};
    pe.is_64bit = (fh->Machine == 0x8664);

    size_t opt_offset = pe_offset + 4 + sizeof(FILE_HEADER);
    if (!pe.is_64bit) return std::nullopt; // only x64 supported for now

    if (opt_offset + sizeof(OPTIONAL_HEADER_64) > len) return std::nullopt;
    auto* opt = reinterpret_cast<const OPTIONAL_HEADER_64*>(data + opt_offset);

    pe.image_base = opt->ImageBase;
    pe.entry_point_rva = opt->AddressOfEntryPoint;
    pe.size_of_image = opt->SizeOfImage;

    // Parse sections
    size_t sec_offset = opt_offset + fh->SizeOfOptionalHeader;
    for (uint16_t i = 0; i < fh->NumberOfSections; ++i) {
        if (sec_offset + sizeof(SECTION_HEADER) > len) break;
        auto* sh = reinterpret_cast<const SECTION_HEADER*>(data + sec_offset);

        PESection sec;
        sec.name = std::string(sh->Name, strnlen(sh->Name, 8));
        sec.virtual_address = sh->VirtualAddress;
        sec.virtual_size = sh->VirtualSize;
        sec.raw_offset = sh->PointerToRawData;
        sec.raw_size = sh->SizeOfRawData;
        sec.characteristics = sh->Characteristics;
        pe.sections.push_back(sec);

        sec_offset += sizeof(SECTION_HEADER);
    }

    // Parse import directory (simplified)
    if (opt->NumberOfRvaAndSizes > 1) {
        auto* dirs = reinterpret_cast<const DATA_DIRECTORY*>(
            data + opt_offset + sizeof(OPTIONAL_HEADER_64));
        uint32_t import_rva = dirs[1].VirtualAddress;
        uint32_t import_size = dirs[1].Size;

        // Import parsing requires mapping RVA → file offset through sections
        // This is a simplified version that works for most PE files
        if (import_rva > 0 && import_size > 0) {
            // Find section containing import directory
            for (auto& sec : pe.sections) {
                if (import_rva >= sec.virtual_address &&
                    import_rva < sec.virtual_address + sec.virtual_size) {
                    uint64_t file_offset = sec.raw_offset + (import_rva - sec.virtual_address);
                    // Import descriptor parsing would go here
                    // Skipped for brevity — use LIEF or pe-parse for full import support
                    break;
                }
            }
        }
    }

    return pe;
}

const PESection* PELoader::find_section(const PEInfo& pe, const std::string& name) {
    for (auto& s : pe.sections)
        if (s.name == name) return &s;
    return nullptr;
}

const PESection* PELoader::section_for_rva(const PEInfo& pe, uint64_t rva) {
    for (auto& s : pe.sections)
        if (rva >= s.virtual_address && rva < s.virtual_address + s.virtual_size)
            return &s;
    return nullptr;
}

std::vector<uint8_t> PELoader::load_image(const uint8_t* raw_data, size_t raw_len,
                                           const PEInfo& pe) {
    std::vector<uint8_t> image(pe.size_of_image, 0);

    // Copy headers
    size_t header_size = std::min(static_cast<size_t>(pe.size_of_image), raw_len);
    if (!pe.sections.empty())
        header_size = std::min(header_size, static_cast<size_t>(pe.sections[0].raw_offset));
    std::memcpy(image.data(), raw_data, std::min(header_size, raw_len));

    // Map sections
    for (auto& sec : pe.sections) {
        if (sec.raw_offset + sec.raw_size > raw_len) continue;
        if (sec.virtual_address + sec.raw_size > pe.size_of_image) continue;
        std::memcpy(image.data() + sec.virtual_address,
                    raw_data + sec.raw_offset,
                    std::min(static_cast<size_t>(sec.raw_size),
                             static_cast<size_t>(sec.virtual_size)));
    }

    return image;
}

std::vector<std::pair<uint64_t, std::string>> PELoader::build_iat_map(const PEInfo& pe) {
    std::vector<std::pair<uint64_t, std::string>> result;
    for (auto& imp : pe.imports) {
        result.push_back({pe.image_base + imp.iat_rva, imp.func_name});
    }
    return result;
}

} // namespace deobf
