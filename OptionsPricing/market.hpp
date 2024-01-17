//
//  market.hpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#ifndef market_hpp
#define market_hpp


#include "matrix.cpp"
#include "stock.h"


#include <stdio.h>


class Market{
private:
    double riskFreeRate;
    Stock stock;
    vector<Stock> stocks;
    matrix corMatrix;
public:
    /**** constructors ****/
    Market(){};
    Market(double riskFreeRate, const Stock& stock, const vector<Stock>& stocks={}, const matrix& corMatrix=NULL_MATRIX);
    Market(const Market& market);
    /**** accessors ****/
    double getRiskFreeRate() const {return riskFreeRate;}
    Stock getStock(int i=-1) const {return i<0?stock:stocks[i];}
    vector<Stock> getStocks() const {return stocks;}
    matrix getCorMatrix() const {return corMatrix;}
    string getAsJson() const;
    /**** mutators ****/
    double setRiskFreeRate(double riskFreeRate);
    Stock setStock(const Stock& stock, int i=-1);
    vector<Stock> setStocks(const vector<Stock>& stocks);
    matrix setCorMatrix(const matrix& corMatrix);
    /**** main ****/
    vector<matrix> simulateCorrelatedPrices(const SimulationConfig& config, int numSim=1, const vector<matrix>& randomMatrixSet={});
    vector<vector<matrix>> simulateCorrelatedPricesWithFullCalc(const SimulationConfig& config, int numSim=1, const vector<matrix>& randomMatrixSet={});
    /**** operators ****/
    friend ostream& operator<<(ostream& out, const Market& market);
};

#endif /* market_hpp */
