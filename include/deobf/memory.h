#pragma once
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace deobf {

struct MemRange {
    uint64_t start;
    uint64_t end;
    bool contains(uint64_t addr) const { return addr >= start && addr < end; }
};

class ByteMemory {
public:
    void add_safe_range(uint64_t start, uint64_t end);
    bool is_safe(uint64_t addr) const;

    void store(uint64_t addr, const std::vector<uint8_t>& data);
    std::optional<std::vector<uint8_t>> load(uint64_t addr, size_t size) const;

    std::optional<uint64_t> load_u64(uint64_t addr) const;
    std::optional<uint32_t> load_u32(uint64_t addr) const;

    void store_u64(uint64_t addr, uint64_t val);
    void store_u32(uint64_t addr, uint32_t val);

private:
    std::vector<MemRange> safe_ranges_;
    std::unordered_map<uint64_t, uint8_t> bytes_;
};

} // namespace deobf
