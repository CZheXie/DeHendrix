#pragma once
#include "deobf/ir.h"
#include "deobf/cfg.h"
#include "deobf/pe_loader.h"
#include "deobf/evaluator.h"
#include "deobf/target.h"
#include "deobf/iat_cracker.h"
#include <memory>
#include <string>
#include <vector>

namespace deobf {

/// End-to-end devirtualization pipeline.
/// PE load → safe range setup → guided eval → optimize → report.
struct PipelineConfig {
    std::string image_path;
    uint64_t entry_va = 0;
    std::string target_name = "generic";
    int max_steps = 50000;
    int max_vm_insns = 500;
    bool scan_iat_hashes = true;
    bool emit_llvm = false;
    std::string output_dir = ".";
    std::vector<std::pair<uint64_t, uint64_t>> extra_safe_ranges;
};

struct PipelineResult {
    bool success = false;
    std::string error;

    EvalReport eval_report;
    std::vector<HandlerRecord> handlers;
    std::vector<std::optional<uint64_t>> vpc_trace;
    std::vector<IATCracker::HashMatch> iat_matches;

    std::string markdown_report;
    std::string llvm_ir;
};

class Pipeline {
public:
    static PipelineResult run(const PipelineConfig& config);

    /// Generate a Markdown devirt report.
    static std::string generate_report(const PipelineConfig& config,
                                       const PipelineResult& result,
                                       const PEInfo* pe = nullptr);
};

} // namespace deobf
