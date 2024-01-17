//
//  option.h
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#ifndef option_h
#define option_h

#include <iostream>
#include <sstream>

#include "stock.h"
#include "simulationConfig.hpp"
#include "matrix.cpp"

#define GUI true
#define LOG true
#define INF 1e3
#define USE_LOOP true


const Stock NULL_STOCK;
const SimulationConfig NULL_CONFIG;


using namespace std;

class Option{
private:
    string name, type, putCall;
    double strike, discStrike, maturity;
    vector<double> params;
    vector<string> nature;
public:
    /**** constructors ****/
    Option(){};
    Option(string type, string putCall, double strike, double maturity,
           const vector<double>& params={}, const vector<string>& nature={}, string name="unnamed");
    Option(const Option& option);
    /**** accessors ****/
    bool canEarlyExercise() const;
    bool isPathDependent() const;
    string getName() const {return name;}
    string getType() const {return type;}
    string getPutCall() const {return putCall;}
    double getStrike() const {return strike;}
    double getDiscStrike() const {return discStrike;}
    double getMaturity() const {return maturity;}
    vector<double> getParams() const {return params;}
    vector<string> getNature() const {return nature;}
    string getAsJson() const;
    /**** mutators ****/
    string setName(string name);
    string setType(string type);
    double setStrike(double strike);
    double setDiscStrike(double discStrike);
    double setMaturity(double maturity);
    vector<double> setParams(const vector<double>& params);
    vector<string> setNature(const vector<string>& nature);
    /**** main ****/
    bool checkParams() const;
    double calcPayoff(double stockPrice=0, const matrix& priceSeries=NULL_VECTOR,
                      const vector<matrix>& priceSeriesSet={}, const matrix& timeVector=NULL_VECTOR);
    matrix calcPayoffs(const matrix& stockPriceVector=NULL_VECTOR, const matrix& priceMatrix=NULL_MATRIX,
                       const vector<matrix>& priceMatrixSet={}, const matrix& timeVector=NULL_VECTOR);
    /**** operators ****/
    friend ostream& operator<<(ostream& out, const Option& option);
};

#endif /* option_h */
