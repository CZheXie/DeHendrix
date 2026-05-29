#include "deobf/ir.h"
#include "deobf/passes.h"
#include "deobf/evaluator.h"
#include "deobf/pe_loader.h"
#include "deobf/llvm_emitter.h"
#include "deobf/segment_eval.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace deobf;

static void usage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " devirt --image <dump.bin> --base 0x140000000 "
              << "--entry 0x14132C758 [--vpc-reg r11] [--max-steps 50000] "
              << "[--max-vm-insns 500] [--verbose]\n"
              << "  " << prog << " cfg --image <file> [--base 0x140000000] "
              << "[--entry 0x...] [--emit-llvm] [--llvm-out out.ll]\n"
              << "      (native multi-block CFG recovery; --entry defaults to PE entry)\n";
}

static uint64_t parse_hex(const char* s) {
    return std::strtoull(s, nullptr, 0);
}

int main(int argc, char* argv[]) {
    if (argc < 2) { usage(argv[0]); return 1; }

    std::string cmd = argv[1];
    if (cmd != "devirt" && cmd != "cfg") {
        std::cerr << "Unknown command: " << cmd << "\n";
        usage(argv[0]);
        return 1;
    }

    std::string image_path;
    uint64_t base = 0x140000000;
    uint64_t entry = 0;
    std::string vpc_reg = "r11";
    int max_steps = 50000;
    int max_vm_insns = 500;
    bool verbose = false;
    bool emit_llvm = false;
    std::string llvm_output;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--image" && i + 1 < argc) image_path = argv[++i];
        else if (arg == "--base" && i + 1 < argc) base = parse_hex(argv[++i]);
        else if (arg == "--entry" && i + 1 < argc) entry = parse_hex(argv[++i]);
        else if (arg == "--vpc-reg" && i + 1 < argc) vpc_reg = argv[++i];
        else if (arg == "--max-steps" && i + 1 < argc) max_steps = std::atoi(argv[++i]);
        else if (arg == "--max-vm-insns" && i + 1 < argc) max_vm_insns = std::atoi(argv[++i]);
        else if (arg == "--verbose") verbose = true;
        else if (arg == "--emit-llvm") emit_llvm = true;
        else if (arg == "--llvm-out" && i + 1 < argc) llvm_output = argv[++i];
    }

    if (image_path.empty()) {
        std::cerr << "Error: --image is required\n";
        usage(argv[0]);
        return 1;
    }
    if (cmd == "devirt" && entry == 0) {
        std::cerr << "Error: --entry is required for devirt\n";
        usage(argv[0]);
        return 1;
    }

    std::ifstream file(image_path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error: cannot open " << image_path << "\n";
        return 1;
    }
    auto size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> raw_data(size);
    file.read(reinterpret_cast<char*>(raw_data.data()), size);

    std::vector<uint8_t> image;
    auto pe_opt = PELoader::parse(raw_data.data(), raw_data.size());
    if (pe_opt) {
        image = PELoader::load_image(raw_data.data(), raw_data.size(), *pe_opt);
        base = pe_opt->image_base;
        std::cout << "=== dehex Guided Symbolic Evaluation (C++) ===\n";
        std::cout << "Image: " << image_path << " (PE, " << size << " raw, "
                  << image.size() << " mapped)\n";
        std::cout << "ImageBase: 0x" << std::hex << base << "  Sections: "
                  << std::dec << pe_opt->sections.size() << "\n";
    } else {
        image = std::move(raw_data);
        std::cout << "=== dehex Guided Symbolic Evaluation (C++) ===\n";
        std::cout << "Image: " << image_path << " (raw dump, " << size << " bytes)\n";
    }
    if (cmd == "cfg") {
        if (entry == 0 && pe_opt) entry = base + pe_opt->entry_point_rva;
        std::cout << "Mode: native multi-block CFG recovery\n";
        std::cout << "Entry: 0x" << std::hex << entry << "\n\n";
        if (entry == 0) {
            std::cerr << "Error: no entry (provide --entry or a PE with an entry point)\n";
            return 1;
        }
        CFGFunction cfg = recover_native_cfg(image.data(), image.size(), base, entry);
        std::cout << "Recovered " << std::dec << cfg.blocks.size() << " blocks, "
                  << cfg.edges.size() << " edges, " << cfg.total_instrs() << " IR instrs\n\n";
        std::cout << cfg.dump() << "\n";
        if (emit_llvm) {
            std::string ll = LLVMEmitter::emit_cfg_function(cfg, "devirt_cfg");
            if (llvm_output.empty()) {
                std::cout << "\n=== LLVM IR ===\n" << ll;
            } else {
                std::ofstream ofs(llvm_output);
                ofs << ll;
                std::cout << "\nLLVM IR written to: " << llvm_output << "\n";
            }
        }
        return 0;
    }

    std::cout << "Entry: 0x" << std::hex << entry << "  VPC reg: " << vpc_reg << "\n";
    std::cout << "Limits: " << std::dec << max_steps << " steps, " << max_vm_insns << " VM insns\n\n";

    GuidedEvaluator ev(image.data(), image.size(), base, vpc_reg);

    if (pe_opt) {
        for (auto& sec : pe_opt->sections) {
            if (sec.name.find("tvm") != std::string::npos ||
                sec.name.find("ytr") != std::string::npos ||
                sec.name.find("vmp") != std::string::npos) {
                uint64_t start = base + sec.virtual_address;
                uint64_t end = start + sec.virtual_size;
                ev.add_safe_range(start, end);
                if (verbose)
                    std::cout << "Safe range: [0x" << std::hex << start
                              << ", 0x" << end << ") (" << sec.name << ")\n";
            }
        }
    }
    auto report = ev.run(entry, max_steps, max_vm_insns);

    std::cout << "Result: " << eval_result_name(report.result) << "\n";
    std::cout << "Stop VA: 0x" << std::hex << report.stop_va << "\n";
    std::cout << "Steps: " << std::dec << report.steps << "\n";
    std::cout << "VM insns dispatched: " << report.vm_insn_count << "\n";
    std::cout << "IR instructions: " << report.ir_count << "\n";

    if (report.dispatcher_va)
        std::cout << "Dispatcher VA: 0x" << std::hex << *report.dispatcher_va << "\n";
    if (report.final_vpc)
        std::cout << "Final VPC: 0x" << std::hex << *report.final_vpc << "\n";

    auto& trace = ev.jmp_trace();
    std::cout << "\nJMP trace (" << trace.size() << " blocks):\n";
    for (size_t i = 0; i < std::min(trace.size(), size_t(50)); ++i)
        std::cout << "  [" << i << "] 0x" << std::hex << trace[i] << "\n";
    if (trace.size() > 50)
        std::cout << "  ... " << trace.size() - 50 << " more\n";

    if (!ev.handlers().empty())
        std::cout << "\n" << ev.dump_handler_catalog();

    if (!ev.vpc_trace().empty())
        std::cout << "\n" << ev.dump_vpc_trace();

    if (verbose) {
        auto& f = ev.func();
        std::cout << "\nFinal IR (" << f.instrs.size() << " instructions):\n";
        size_t start = f.instrs.size() > 30 ? f.instrs.size() - 30 : 0;
        for (size_t i = start; i < f.instrs.size(); ++i)
            std::cout << "  " << f.instrs[i].to_string() << "\n";
    }

    if (emit_llvm) {
        std::string ll = LLVMEmitter::emit_function(ev.func(), "devirt");
        if (llvm_output.empty()) {
            std::cout << "\n=== LLVM IR ===\n" << ll;
        } else {
            std::ofstream ofs(llvm_output);
            ofs << ll;
            std::cout << "\nLLVM IR written to: " << llvm_output << "\n";
        }
    }

    return 0;
}
