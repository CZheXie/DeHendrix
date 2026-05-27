#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace deobf {

struct PESection {
    std::string name;
    uint64_t virtual_address;
    uint64_t virtual_size;
    uint64_t raw_offset;
    uint64_t raw_size;
    uint32_t characteristics;

    bool is_executable() const { return characteristics & 0x20000000; }
    bool is_writable() const  { return characteristics & 0x80000000; }
    bool is_readable() const  { return characteristics & 0x40000000; }
};

struct PEImport {
    std::string dll_name;
    std::string func_name;
    uint16_t ordinal;
    uint64_t iat_rva;
};

struct PEExport {
    std::string name;
    uint32_t ordinal;
    uint64_t rva;
};

struct PEInfo {
    uint64_t image_base;
    uint64_t entry_point_rva;
    uint32_t size_of_image;
    bool is_64bit;
    std::vector<PESection> sections;
    std::vector<PEImport> imports;
    std::vector<PEExport> exports;
};

class PELoader {
public:
    static std::optional<PEInfo> parse(const uint8_t* data, size_t len);

    static const PESection* find_section(const PEInfo& pe, const std::string& name);
    static const PESection* section_for_rva(const PEInfo& pe, uint64_t rva);

    /// Load full PE into a flat image buffer at image_base.
    /// Returns (image_buffer, image_base).
    static std::vector<uint8_t> load_image(const uint8_t* raw_data, size_t raw_len,
                                           const PEInfo& pe);

    /// Get IAT slot VA → API name mapping.
    static std::vector<std::pair<uint64_t, std::string>>
        build_iat_map(const PEInfo& pe);
};

} // namespace deobf
