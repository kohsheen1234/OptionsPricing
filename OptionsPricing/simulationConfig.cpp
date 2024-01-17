//
//  simulationConfig.cpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#include "simulationConfig.hpp"
#include <iostream>
#include <sstream> 
using namespace std;

string SimulationConfig::getAsJson() const {
    ostringstream oss;
    oss << "{" <<
    "\"iters\":"    << iters    << "," <<
    "\"endTime\":"  << endTime  << "," <<
    "\"stepSize\":" << stepSize <<
    "}";
    return oss.str();
}
