#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace deobf {

/// ACE-style encrypted IAT hash cracker.
/// Scans PE .data/.rdata for 64-bit values matching known API name hashes
/// under multiple hash algorithm/seed variants.
class IATCracker {
public:
    struct HashMatch {
        uint64_t file_offset;
        uint64_t hash_value;
        std::string api_name;
        std::string algorithm;
    };

    /// Add an API name to the known database.
    void add_api(const std::string& name);

    /// Load default Windows kernel API names (~200).
    void load_default_kernel_apis();

    /// Precompute all hashes for all registered APIs under all algorithms.
    void precompute();

    /// Scan a data region for hash matches.
    std::vector<HashMatch> scan(const uint8_t* data, size_t len,
                                uint64_t base_offset = 0) const;

    size_t api_count() const { return api_names_.size(); }
    size_t hash_count() const { return hash_to_api_.size(); }

private:
    std::vector<std::string> api_names_;
    std::unordered_map<uint64_t, std::pair<std::string, std::string>> hash_to_api_;

    static uint64_t fnv1a_64(const std::string& s);
    static uint64_t fnv1a_64_lower(const std::string& s);
    static uint64_t djb2_64(const std::string& s);
    static uint64_t sdbm_64(const std::string& s);
    static uint64_t crc32_custom(const std::string& s, uint64_t seed);
    static uint64_t ror13_add(const std::string& s);
    static uint64_t ror13_add_unicode(const std::string& s);
};

} // namespace deobf
