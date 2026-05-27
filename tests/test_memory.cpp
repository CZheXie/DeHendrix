#include "deobf/memory.h"
#include <cassert>
#include <iostream>

using namespace deobf;

static void test_byte_store_load() {
    ByteMemory mem;
    mem.store_u64(0x1000, 0xDEADBEEFCAFEBABE);
    auto val = mem.load_u64(0x1000);
    assert(val && *val == 0xDEADBEEFCAFEBABE);
    std::cout << "  PASS: u64 store/load roundtrip\n";
}

static void test_u32() {
    ByteMemory mem;
    mem.store_u32(0x2000, 0x12345678);
    auto val = mem.load_u32(0x2000);
    assert(val && *val == 0x12345678);
    std::cout << "  PASS: u32 store/load roundtrip\n";
}

static void test_safe_range() {
    ByteMemory mem;
    mem.add_safe_range(0x1000, 0x2000);
    assert(mem.is_safe(0x1000));
    assert(mem.is_safe(0x1500));
    assert(!mem.is_safe(0x2000));
    assert(!mem.is_safe(0x500));
    std::cout << "  PASS: safe range check\n";
}

static void test_missing_load() {
    ByteMemory mem;
    auto val = mem.load_u64(0x9999);
    assert(!val);
    std::cout << "  PASS: load from uninitialized returns nullopt\n";
}

static void test_overlapping_store() {
    ByteMemory mem;
    mem.store_u64(0x3000, 0x1111111122222222);
    mem.store_u32(0x3000, 0xAAAAAAAA);
    auto val = mem.load_u64(0x3000);
    assert(val);
    // low 4 bytes overwritten, high 4 unchanged
    assert((*val & 0xFFFFFFFF) == 0xAAAAAAAA);
    assert((*val >> 32) == 0x11111111);
    std::cout << "  PASS: overlapping store preserves partial\n";
}

int main() {
    std::cout << "=== libdeobf ByteMemory tests ===\n";
    test_byte_store_load();
    test_u32();
    test_safe_range();
    test_missing_load();
    test_overlapping_store();
    std::cout << "\nAll memory tests passed!\n";
    return 0;
}
