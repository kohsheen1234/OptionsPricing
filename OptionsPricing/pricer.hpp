//
//  pricer.hpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#ifndef PRICER_h
#define PRICER_h

#include "option.h"
#include "market.hpp"
#include "simulationConfig.hpp"
#include "backtest.hpp"
#include <stdio.h>

using namespace std;

//const Stock NULL_STOCK;
//const SimulationConfig NULL_CONFIG;

class Pricer{
private:
    Option option, option_orig;
    Market market, market_orig;
    double price;
public:
    vector<double> tmp; // tmp variable log
    /**** constructors ****/
    Pricer(){};
    Pricer(const Option& option, const Market& market);
    /**** accessors ****/
    Option getOption() const {return option;}
    Market getMarket() const {return market;}
    double getPrice() const {return price;}
    string getAsJson() const;
    double getVariable(string var, int i=-1, int j=-1) const;
    /**** mutators ****/
    double setVariable(string var, double v, int i=-1);
    string setStringVariable(string var, string v);
    Pricer setVariablesFromFile(string file);
    Pricer resetOriginal();
    Pricer saveAsOriginal();
    /**** main ****/
    double BlackScholesClosedForm();
    double BinomialTreePricer(const SimulationConfig& config);
    double MonteCarloPricer(const SimulationConfig& config, int numSim, string method="simple");
    double MultiStockMonteCarloPricer(const SimulationConfig& config, int numSim, string method="simple");
    double NumIntegrationPricer(double z=5, double dz=1e-3);
    double BlackScholesPDESolver(const SimulationConfig& config, int numSpace, string method="implicit");
    vector<matrix> BlackScholesPDESolverWithFullCalc(const SimulationConfig& config, int numSpace, string method="implicit");
    vector<double> _FourierInversionPricer(const function<complx(complx)>& charFunc, int numSpace, double rightLim=INF, string method="RN Prob");
    vector<matrix> _fastFourierInversionPricer(const function<complx(complx)>& charFunc, int numSpace, double rightLim=INF);
    double FourierInversionPricer(int numSpace, double rightLim=INF, string method="RN Prob");
    double calcPrice(string method="Closed Form", const SimulationConfig& config=NULL_CONFIG,
                     int numSim=0, int numSpace=0);
    matrix varyPriceWithVariable(string var, const matrix& varVector,
                                 string method="Closed Form", const SimulationConfig& config=NULL_CONFIG, int numSim=0);
    double ClosedFormGreek(string var, int derivOrder=1);
    double FiniteDifferenceGreek(string var, int derivOrder=1, string method="Closed Form",
                                 const SimulationConfig& config=NULL_CONFIG, int numSim=0, double eps=1e-5);
    double calcGreek(string greekName, string greekMethod="Closed Form", string method="Closed Form",
                     const SimulationConfig& config=NULL_CONFIG, int numSim=0, double eps=1e-5);
    matrix varyGreekWithVariable(string var, const matrix& varVector,
                                 string greekName, string greekMethod="Closed Form", string method="Closed Form",
                                 const SimulationConfig& config=NULL_CONFIG, int numSim=0, double eps=1e-5);
    matrix generatePriceSurface(const matrix& stockPriceVector, const matrix& optionTermVector,
                                string method="Closed Form", const SimulationConfig& config=NULL_CONFIG, int numSim=0);
    bool satisfyPriceBounds(double optionMarketPrice);
    double calcImpliedVolatility(double optionMarketPrice, double vol0=5, double eps=1e-5);
    void generateImpliedVolSurfaceFromFile(string input, string file, double vol0=5, double eps=1e-5);
    void generateGreeksFromImpliedVolFile(string input, string file);
    vector<matrix> modelImpliedVolSurface(const SimulationConfig& config, int numSpace,
                                          const function<double(double)>& impVolFunc0, const function<double(double)>& impVolFunc1,
                                          double lambdaT, double eps=1e-5);
    vector<matrix> modelImpliedVolSurfaceFromFile(string input, const SimulationConfig& config, int numSpace);
    Backtest runBacktest(const SimulationConfig& config, int numSim=1,
                         string strategy="simple-delta", int hedgeFreq=1, double mktImpVol=0, double mktPrice=0,
                         const vector<double>& stratParams={}, const vector<Option>& hOptions={}, const vector<matrix>& impVolSurfaceSet={},
                         string simPriceMethod="model", const matrix& stockPriceSeries=NULL_VECTOR);
    /**** operators ****/
    friend ostream& operator<<(ostream& out, const Pricer& pricer);
};

#endif /* pricer_hpp */
