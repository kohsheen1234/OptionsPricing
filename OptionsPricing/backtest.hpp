//
//  backtest.hpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#ifndef backtest_hpp
#define backtest_hpp

#include <iostream>
#include <string>
#include <vector> 
#include "matrix.cpp"
using namespace std;

class Backtest{
public:
    vector<string> labels;
    vector<matrix> results;
    vector<string> hLabels;
    vector<vector<matrix>> hResults;
    Backtest(
             const vector<matrix>& results,
             const vector<vector<matrix>>& hResults
             );
    void printToCsvFiles(
                         bool perSim=false,
                         string name="backtest"
                         );
};


#endif /* backtest_hpp */
