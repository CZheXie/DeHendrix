#include "deobf/target.h"

namespace deobf {

std::unique_ptr<VMTarget> create_target(const std::string& name) {
    if (name == "vmp3" || name == "vmp" || name == "vmprotect")
        return std::make_unique<VMP3Target>();
    if (name == "themida" || name == "codevirtualizer")
        return std::make_unique<ThemidaTarget>();
    if (name == "ace")
        return std::make_unique<ACETarget>();
    return std::make_unique<GenericTarget>();
}

} // namespace deobf
