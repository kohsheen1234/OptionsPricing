//
//  simulationConfig.hpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#ifndef simulationConfig_hpp
#define simulationConfig_hpp

#include <stdio.h>
#include <iostream>
using namespace std;

class SimulationConfig{
public:
    int iters;
    double endTime, stepSize;
    SimulationConfig(double t=0, int n=1):endTime(t),iters(n),stepSize(t/n){}
    bool isEmpty() const {return endTime==0;}
    string getAsJson() const;
    friend ostream& operator<<(ostream& out, const SimulationConfig& config);
};


#endif /* simulationConfig_hpp */
