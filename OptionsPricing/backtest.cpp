//
//  backtest.cpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#include "backtest.hpp"


Backtest::Backtest(const vector<matrix>& results, const vector<vector<matrix>>& hResults):
results(results),hResults(hResults){
    labels = {
        "simPrice",
        "stratCash",
        "stratNStock",
        "stratModPrice",
        "stratModValue",
        "stratGrkDelta",
        "stratGrkGamma",
        "stratGrkVega",
        "stratGrkRho",
        "stratGrkTheta"
    };
    hLabels = {
        "stratNOption",
        "stratHModPrice"
    };
}

void Backtest::printToCsvFiles(bool perSim, string name){
    int a = (int)results.size();
    int b = (int)hResults.size();
    int n = (int)hResults[0].size();
    if(perSim){
        int iters = results[0].getRows();
        int numSim = results[0].getCols();
        string header = joinStr(labels);
        for(int i=0; i<b; i++)
            for(int j=0; j<n; j++)
                header += ","+hLabels[i]+"-"+to_string(j);
        for(int k=0; k<numSim; k++){
            matrix result(iters,a+b*n);
            for(int i=0; i<a; i++)
                result.setCol(i,results[i].getCol(k));
            for(int i=0; i<b; i++)
                for(int j=0; j<n; j++)
                    result.setCol(a+i*n+j,hResults[i][j].getCol(k));
            result.printToCsvFile(
                                  name+"-"+to_string(k)+".csv",
                                  header
                                  );
        }
    }else{
        for(int i=0; i<a; i++)
            results[i].printToCsvFile(
                                      name+"-"+labels[i]+".csv"
                                      );
        for(int i=0; i<b; i++)
            for(int j=0; j<n; j++)
                hResults[i][j].printToCsvFile(
                                              name+"-"+hLabels[i]+"-"+to_string(j)+".csv"
                                              );
    }
}

