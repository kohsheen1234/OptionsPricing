//
//  stock.h
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#ifndef stock_h
#define stock_h

#include "util.cpp"
#include "complx.cpp"
#include "matrix.cpp"

#include "simulationConfig.hpp"
using namespace std;



class Stock{
private:
    string name, dynamics;
    double currentPrice, dividendYield, driftRate, volatility;
    matrix simTimeVector, simPriceMatrix, binomialPriceTree;
    vector<double> dynParams;
public:
    /**** constructors ****/
    Stock(){};
    Stock(double currentPrice, double dividendYield, double driftRate, double volatility,
          const vector<double>& dynParams={}, string dynamics="lognormal", string name="unnamed");
    Stock(const Stock& stock);
    /**** accessors ****/
    string getName() const {return name;}
    string getDynamics() const {return dynamics;}
    double getCurrentPrice() const {return currentPrice;}
    double getDividendYield() const {return dividendYield;}
    double getDriftRate() const {return driftRate;}
    double getVolatility() const {return volatility;}
    matrix getSimTimeVector() const {return simTimeVector;}
    matrix getSimPriceMatrix() const {return simPriceMatrix;}
    matrix getBinomialPriceTree() const {return binomialPriceTree;}
    vector<double> getDynParams() const {return dynParams;}
    string getAsJson() const;
    /**** mutators ****/
    string setName(string name);
    string setDynamics(string dynamics);
    double setCurrentPrice(double currentPrice);
    double setDividendYield(double dividendYield);
    double setDriftRate(double driftRate);
    double setVolatility(double volatility);
    vector<double> setDynParams(const vector<double>& dynParams);
    matrix setSimTimeVector(const matrix& simTimeVector);
    matrix setSimPriceMatrix(const matrix& simPriceMatrix);
    double estDriftRateFromPrice(const matrix& priceSeries, double dt, string method="simple");
    double estVolatilityFromPrice(const matrix& priceSeries, double dt, string method="simple");
    /**** main ****/
    bool checkParams() const;
    double calcLognormalPrice(double z, double time);
    matrix calcLognormalPriceVector(const matrix& z, double time);
    matrix simulatePrice(const SimulationConfig& config, int numSim=1, const matrix& randomMatrix=NULL_MATRIX);
    vector<matrix> simulatePriceWithFullCalc_loop(const SimulationConfig& config, int numSim=1, const matrix& randomMatrix=NULL_MATRIX);
    vector<matrix> simulatePriceWithFullCalc(const SimulationConfig& config, int numSim=1, const matrix& randomMatrix=NULL_MATRIX);
    matrix bootstrapPrice(const matrix& priceSeries, const SimulationConfig& config, int numSim=1);
    matrix generatePriceTree(const SimulationConfig& config);
    matrix generatePriceMatrixFromTree();
    /**** operators ****/
    friend ostream& operator<<(ostream& out, const Stock& stock);
};


#endif /* stock_h */
