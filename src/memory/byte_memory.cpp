#include "deobf/memory.h"
#include <cstring>

namespace deobf {

void ByteMemory::add_safe_range(uint64_t start, uint64_t end) {
    safe_ranges_.push_back({start, end});
}

bool ByteMemory::is_safe(uint64_t addr) const {
    for (auto& r : safe_ranges_) {
        if (r.contains(addr)) return true;
    }
    return false;
}

void ByteMemory::store(uint64_t addr, const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); ++i)
        bytes_[addr + i] = data[i];
}

std::optional<std::vector<uint8_t>> ByteMemory::load(uint64_t addr, size_t size) const {
    std::vector<uint8_t> result(size);
    for (size_t i = 0; i < size; ++i) {
        auto it = bytes_.find(addr + i);
        if (it == bytes_.end()) return std::nullopt;
        result[i] = it->second;
    }
    return result;
}

std::optional<uint64_t> ByteMemory::load_u64(uint64_t addr) const {
    auto data = load(addr, 8);
    if (!data) return std::nullopt;
    uint64_t val;
    std::memcpy(&val, data->data(), 8);
    return val;
}

std::optional<uint32_t> ByteMemory::load_u32(uint64_t addr) const {
    auto data = load(addr, 4);
    if (!data) return std::nullopt;
    uint32_t val;
    std::memcpy(&val, data->data(), 4);
    return val;
}

void ByteMemory::store_u64(uint64_t addr, uint64_t val) {
    uint8_t buf[8];
    std::memcpy(buf, &val, 8);
    for (int i = 0; i < 8; ++i) bytes_[addr + i] = buf[i];
}

void ByteMemory::store_u32(uint64_t addr, uint32_t val) {
    uint8_t buf[4];
    std::memcpy(buf, &val, 4);
    for (int i = 0; i < 4; ++i) bytes_[addr + i] = buf[i];
}

} // namespace deobf
