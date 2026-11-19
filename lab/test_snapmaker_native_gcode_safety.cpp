// Standalone C++17 QA executable for the exact native safety header.
#include "../src/slic3r/Utils/FilunaGCodeSafety.hpp"
#include <cassert>
#include <iostream>
#include <string>

using namespace Slic3r::Filuna::Safety;

static const char* status(Report::Decision v)
{
    switch (v) {
    case Report::Decision::Pass: return "PASS";
    case Report::Decision::Warn: return "WARN";
    case Report::Decision::Block: return "BLOCK";
    case Report::Decision::Review: return "REVIEW";
    }
    return "UNKNOWN";
}

int main(int argc, char** argv)
{
    if (argc > 1) {
        Policy p;
        if (argc > 2 && std::string(argv[2]) == "GEOMETRY_ONLY")
            p.bounds_known = false;
        const Report r=inspect_file(argv[1],p);
        std::cout << "NATIVE_STATUS=" << status(r.decision)
                  << " ERRORS=" << r.collisions.size()
                  << " WARNINGS=" << r.warnings.size()
                  << " OBJECT_EXTRUSIONS=" << r.object_extrusions
                  << " TOWER_EXTRUSIONS=" << r.tower_extrusions
                  << " REASON=" << r.explanation << "\n";
        for (const Risk& event : r.collisions)
            std::cout << "NATIVE_COLLISION line=" << event.line
                      << " layer=" << event.layer
                      << " x=" << event.xyz[0] << " y=" << event.xyz[1]
                      << " z=" << event.xyz[2] << " printed_z=" << event.printed_z << "\n";
        for (const Risk& event : r.warnings)
            std::cout << "NATIVE_WARNING line=" << event.line
                      << " type=" << event.reason << "\n";
        // Nonzero for blocked/review. CI tests expected blocked original separately.
        return r.decision==Report::Decision::Block ? 2 :
               r.decision==Report::Decision::Review ? 3 : 0;
    }
    const std::string part =
        "G90\nM83\nG1 X10 Y10 Z1\n"
        "G1 X20 Y10 E1\nG1 X20 Y20 E1\n"
        "G1 X10 Y20 E1\nG1 X10 Y10 E1\n"
        "G1 X9 Y15\n; stop printing object cube id:0\n"
        "; FILUNA_FINAL_PURGE\n";
    const Report unsafe=inspect(part + "G1 X21 Y15 F24000\n");
    assert(unsafe.decision == Report::Decision::Block);
    assert(!unsafe.collisions.empty());
    assert(unsafe.collisions[0].line > 0);
    const Report safe=inspect(part + "G1 Z2\nG1 X21 Y15 F24000\n");
    assert(safe.decision == Report::Decision::Pass);
    const std::string spiral =
        "G90\nG1 X260 Y260 Z1\nM83\n"
        "G1 X262 Y260 E1\n;FILUNA_SPIRAL_HOP\n"
        "G3 X272 Y262 Z1.5 I6 J1 F3000\n";
    const Report warn=inspect(spiral);
    assert(warn.decision == Report::Decision::Warn);
    assert(!warn.warnings.empty());
    assert(std::string(warn.warnings[0].reason) == "AUTO_SPIRAL_OUT_OF_VOLUME");
    std::cout << "NATIVE_CPP_SYNTHETIC_3_OF_3_PASS\n";
    return 0;
}
