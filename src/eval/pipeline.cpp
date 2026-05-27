#include "deobf/pipeline.h"
#include "deobf/passes.h"
#include "deobf/llvm_emitter.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace deobf {

PipelineResult Pipeline::run(const PipelineConfig& config) {
    PipelineResult result;
    auto t_start = std::chrono::steady_clock::now();

    // 1. Load PE / raw image
    std::ifstream file(config.image_path, std::ios::binary | std::ios::ate);
    if (!file) {
        result.error = "Cannot open file: " + config.image_path;
        return result;
    }
    auto file_size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> raw_data(file_size);
    file.read(reinterpret_cast<char*>(raw_data.data()), file_size);

    // Try PE parse
    auto pe_opt = PELoader::parse(raw_data.data(), raw_data.size());
    std::vector<uint8_t> image;
    uint64_t image_base;

    if (pe_opt) {
        image = PELoader::load_image(raw_data.data(), raw_data.size(), *pe_opt);
        image_base = pe_opt->image_base;
    } else {
        image = std::move(raw_data);
        image_base = 0x140000000;
    }

    // 2. Create target
    auto target = create_target(config.target_name);

    // 3. Run guided evaluation
    GuidedEvaluator ev(image.data(), image.size(), image_base, target->vpc_reg());

    // Add safe ranges from target
    for (auto& [start, end] : target->safe_ranges())
        ev.add_safe_range(start, end);

    // Add extra safe ranges
    for (auto& [start, end] : config.extra_safe_ranges)
        ev.add_safe_range(start, end);

    // If PE parsed, add bytecode sections as safe ranges
    if (pe_opt) {
        for (auto& sec : pe_opt->sections) {
            if (!sec.is_executable() && !sec.is_writable()) continue;
            if (sec.name.find("tvm") != std::string::npos ||
                sec.name.find("ytr") != std::string::npos ||
                sec.name.find("vmp") != std::string::npos) {
                uint64_t start = image_base + sec.virtual_address;
                uint64_t end = start + sec.virtual_size;
                ev.add_safe_range(start, end);
            }
        }
    }

    result.eval_report = ev.run(config.entry_va, config.max_steps, config.max_vm_insns);
    result.handlers.assign(ev.handlers().begin(), ev.handlers().end());
    result.vpc_trace.assign(ev.vpc_trace().begin(), ev.vpc_trace().end());

    // 4. IAT hash scanning
    if (config.scan_iat_hashes && pe_opt) {
        IATCracker cracker;
        cracker.load_default_kernel_apis();
        cracker.precompute();

        for (auto& sec : pe_opt->sections) {
            if (sec.name == ".data" || sec.name == ".rdata") {
                uint64_t rva = sec.virtual_address;
                if (rva + sec.virtual_size <= image.size()) {
                    auto matches = cracker.scan(
                        image.data() + rva, sec.virtual_size,
                        image_base + rva);
                    result.iat_matches.insert(result.iat_matches.end(),
                                             matches.begin(), matches.end());
                }
            }
        }
    }

    // 5. LLVM IR export (optional)
    if (config.emit_llvm) {
        result.llvm_ir = LLVMEmitter::emit_function(ev.func(), "devirt");
    }

    auto t_end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

    // 6. Generate report
    result.markdown_report = generate_report(config, result,
                                              pe_opt ? &(*pe_opt) : nullptr);

    result.success = true;
    return result;
}

std::string Pipeline::generate_report(const PipelineConfig& config,
                                       const PipelineResult& result,
                                       const PEInfo* pe) {
    std::ostringstream md;
    md << "# DeHendrix Devirtualization Report\n\n";

    md << "## Configuration\n\n";
    md << "| Parameter | Value |\n|---|---|\n";
    md << "| Image | `" << config.image_path << "` |\n";
    md << "| Entry VA | `0x" << std::hex << config.entry_va << "` |\n";
    md << "| Target | " << config.target_name << " |\n";
    md << "| Max steps | " << std::dec << config.max_steps << " |\n";
    md << "| Max VM insns | " << config.max_vm_insns << " |\n\n";

    if (pe) {
        md << "## PE Info\n\n";
        md << "| Field | Value |\n|---|---|\n";
        md << "| ImageBase | `0x" << std::hex << pe->image_base << "` |\n";
        md << "| EntryPoint RVA | `0x" << pe->entry_point_rva << "` |\n";
        md << "| Sections | " << std::dec << pe->sections.size() << " |\n\n";

        md << "### Sections\n\n";
        md << "| Name | VirtAddr | VirtSize | Characteristics |\n|---|---|---|---|\n";
        for (auto& sec : pe->sections) {
            md << "| `" << sec.name << "` | `0x" << std::hex << sec.virtual_address
               << "` | `0x" << sec.virtual_size << "` | `0x" << sec.characteristics
               << "` |\n";
        }
        md << "\n";
    }

    md << "## Evaluation Result\n\n";
    md << "| Metric | Value |\n|---|---|\n";
    md << "| Result | **" << eval_result_name(result.eval_report.result) << "** |\n";
    md << "| Stop VA | `0x" << std::hex << result.eval_report.stop_va << "` |\n";
    md << "| Steps | " << std::dec << result.eval_report.steps << " |\n";
    md << "| VM instructions | " << result.eval_report.vm_insn_count << " |\n";
    md << "| IR instructions | " << result.eval_report.ir_count << " |\n\n";

    if (!result.handlers.empty()) {
        md << "## Handler Catalog (" << result.handlers.size() << " dispatches)\n\n";
        std::unordered_map<uint64_t, int> freq;
        for (auto& h : result.handlers) freq[h.handler_va]++;

        std::vector<std::pair<uint64_t, int>> sorted(freq.begin(), freq.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](auto& a, auto& b) { return a.second > b.second; });

        md << "| Handler VA | Count |\n|---|---|\n";
        for (auto& [va, cnt] : sorted) {
            md << "| `0x" << std::hex << va << "` | " << std::dec << cnt << " |\n";
        }
        md << "\n";
    }

    if (!result.vpc_trace.empty()) {
        md << "## VPC Trace (" << result.vpc_trace.size() << " entries)\n\n";
        md << "```\n";
        size_t limit = std::min(result.vpc_trace.size(), size_t(50));
        for (size_t i = 0; i < limit; ++i) {
            md << "[" << i << "] ";
            if (result.vpc_trace[i])
                md << "0x" << std::hex << *result.vpc_trace[i];
            else
                md << "symbolic";
            md << "\n";
        }
        if (result.vpc_trace.size() > 50)
            md << "... " << result.vpc_trace.size() - 50 << " more\n";
        md << "```\n\n";
    }

    if (!result.iat_matches.empty()) {
        md << "## IAT Hash Matches (" << result.iat_matches.size() << ")\n\n";
        md << "| Offset | Hash | API | Algorithm |\n|---|---|---|---|\n";
        for (auto& m : result.iat_matches) {
            md << "| `0x" << std::hex << m.file_offset
               << "` | `0x" << m.hash_value
               << "` | `" << m.api_name
               << "` | " << m.algorithm << " |\n";
        }
        md << "\n";
    }

    return md.str();
}

} // namespace deobf
