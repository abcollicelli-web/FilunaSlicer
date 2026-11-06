// Standalone C++17 verification of the Bambu Studio speed-mapping port.
// This checks only the math; it never launches the slicer or printer.
#include "../../modules/bambu-a1/BambuSpeedMapping.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using namespace Slic3r::Filuna::BambuA1;
    // Source formula: speed = coefficient * distance * distance.
    assert(mapping_speed(-1.,80.)==0.);
    assert(mapping_speed(0.,80.)==0.);
    assert(mapping_speed(2.,80.)==320.);
    assert(mapping_speed(3.,80.)==720.);
    assert(inverse_speed_distance(320.,80.)==2.);
    assert(inverse_speed_distance(720.,80.)==3.);
    assert(std::isnan(inverse_speed_distance(12.,0.)));
    assert(std::isnan(inverse_speed_distance(-1.,80.)));
    std::cout << "Bambu A1 speed mapping PORT_CANDIDATE_MATH_PASS (not integrated)\n";
}
