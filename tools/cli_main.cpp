#include "deobf/ir.h"
#include "deobf/passes.h"
#include "deobf/evaluator.h"

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
              << "[--max-vm-insns 500] [--verbose]\n";
}

static uint64_t parse_hex(const char* s) {
    return std::strtoull(s, nullptr, 0);
}

int main(int argc, char* argv[]) {
    if (argc < 2) { usage(argv[0]); return 1; }

    std::string cmd = argv[1];
    if (cmd != "devirt") {
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

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--image" && i + 1 < argc) image_path = argv[++i];
        else if (arg == "--base" && i + 1 < argc) base = parse_hex(argv[++i]);
        else if (arg == "--entry" && i + 1 < argc) entry = parse_hex(argv[++i]);
        else if (arg == "--vpc-reg" && i + 1 < argc) vpc_reg = argv[++i];
        else if (arg == "--max-steps" && i + 1 < argc) max_steps = std::atoi(argv[++i]);
        else if (arg == "--max-vm-insns" && i + 1 < argc) max_vm_insns = std::atoi(argv[++i]);
        else if (arg == "--verbose") verbose = true;
    }

    if (image_path.empty() || entry == 0) {
        std::cerr << "Error: --image and --entry are required\n";
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
    std::vector<uint8_t> image(size);
    file.read(reinterpret_cast<char*>(image.data()), size);

    std::cout << "=== dehex Guided Symbolic Evaluation (C++) ===\n";
    std::cout << "Image: " << image_path << " (" << size << " bytes)\n";
    std::cout << "Entry: 0x" << std::hex << entry << "  VPC reg: " << vpc_reg << "\n";
    std::cout << "Limits: " << std::dec << max_steps << " steps, " << max_vm_insns << " VM insns\n\n";

    GuidedEvaluator ev(image.data(), image.size(), base, vpc_reg);
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

    return 0;
}
