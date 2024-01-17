//
//  option.cpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/7/24.
//

#ifndef OPTION
#define OPTION
#include "util.cpp"
#include "option.h"
#include "complx.cpp"
#include "matrix.cpp"
#include "stock.h"
#include "pricer.hpp"
#include "simulationConfig.hpp"
#include "market.hpp"
using namespace std;

#define GUI true
#define LOG true
#define INF 1e3
#define USE_LOOP true



/**** global variables ********************************************************/

const set<string> OPTION_TYPES{
    "European",   // European options can only be exercised at expiration
    "Digital",    // Digital options pay a fixed amount if the underlying asset is above (call) or below (put) a certain level at expiration
    "American",   // American options can be exercised at any time before expiration
    "Bermudan",   // Bermudan options can be exercised at specific times before expiration
    "Asian",      // Asian options have payoffs linked to the average price of the underlying asset over a period
    "Barrier",    // Barrier options become active or inactive if the underlying asset reaches a certain price
    "Lookback",   // Lookback options allow exercising at the most favorable price of the underlying asset over the life of the option
    "Margrabe",   // Margrabe options are exchange options between two assets, without any cash flow
    "Basket",     // Basket options have payoffs based on a group (basket) of underlying assets
    "Rainbow",    // Rainbow options have payoffs depending on multiple correlated assets and their performance
    "Chooser",    // Chooser options allow the holder to decide whether it's a call or put option at a later date
    "Shout"       // Shout options allow the holder to lock in a profit while retaining the potential for further gains
};


const set<string> EARLY_EXERCISE_OPTION_TYPES{
    "American", "Bermudan"
};

const set<string> PATH_DEPENDENT_OPTION_TYPES{
    "Asian",    // Asian options have a payoff based on the average price of the underlying asset over a certain period
    "Barrier",  // Barrier options become active or inactive based on the underlying asset reaching a certain price level
    "Lookback", // Lookback options allow the holder to 'look back' at the asset's price history to determine the exercise price
    "Chooser",  // Chooser options allow the holder to choose whether it's a call or put option after a certain time
    "Shout"     // Shout options allow the holder to lock in a certain profit while retaining the option to benefit from further price movements
};


const set<string> PUT_CALL{
    "Put", "Call", ""
};




Option::Option(string type, string putCall, double strike, double maturity, const vector<double>& params, const vector<string>& nature, string name){
    this->type = type;
    this->putCall = putCall;
    this->strike = strike;
    this->maturity = maturity;
    this->params = params;
    this->nature = nature;
    this->name = name;
    assert(checkParams());
}

Option::Option(const Option& option){
    this->type = option.type;
    this->putCall = option.putCall;
    this->strike = option.strike;
    this->maturity = option.maturity;
    this->params = option.params;
    this->nature = option.nature;
    this->name = option.name;
}

bool Option::canEarlyExercise() const {
    return EARLY_EXERCISE_OPTION_TYPES.find(type)!=EARLY_EXERCISE_OPTION_TYPES.end();
}

bool Option::isPathDependent() const {
    return PATH_DEPENDENT_OPTION_TYPES.find(type)!=PATH_DEPENDENT_OPTION_TYPES.end();
    return PATH_DEPENDENT_OPTION_TYPES.find(type)!=PATH_DEPENDENT_OPTION_TYPES.end();
}

string Option::getAsJson() const {
    ostringstream oss;
    oss << "{" <<
    "\"name\":\""      << name     << "\"," <<
    "\"type\":\""      << type     << "\"," <<
    "\"nature\":"      << nature   << ","   <<
    "\"putCall\":\""   << putCall  << "\"," <<
    "\"strike\":"      << strike   << ","   <<
    "\"maturity\":"    << maturity << ","   <<
    "\"params\":"      << params   <<
    "}";
    return oss.str();
}

string Option::setName(string name){
    this->name = name;
    return name;
}

string Option::setType(string type){
    this->type = type;
    return type;
}

double Option::setStrike(double strike){
    this->strike = strike;
    return strike;
}

double Option::setDiscStrike(double discStrike){
    this->discStrike = discStrike;
    return discStrike;
}

double Option::setMaturity(double maturity){
    this->maturity = maturity;
    return maturity;
}

vector<double> Option::setParams(const vector<double>& params){
    this->params = params;
    return params;
}

vector<string> Option::setNature(const vector<string>& nature){
    this->nature = nature;
    return nature;
}

bool Option::checkParams() const {
    return
    OPTION_TYPES.find(type)!=OPTION_TYPES.end() &&
    PUT_CALL.find(putCall)!=PUT_CALL.end() &&
    strike>=0 && maturity>=0;
}

double Option::calcPayoff(double stockPrice, const matrix& priceSeries, const vector<matrix>& priceSeriesSet, const matrix& timeVector){
    // case by case
    double S;
    if(type=="European" || type=="American"){
        if(priceSeries.isEmpty()) S = stockPrice;
        else S = priceSeries.getLastEntry();
        if(putCall=="Put") return max(strike-S,0.);
        else if(putCall=="Call") return max(S-strike,0.);
    }else if(type=="Digital"){
        if(priceSeries.isEmpty()) S = stockPrice;
        else S = priceSeries.getLastEntry();
        if(nature.size()>0){
            string strikeType = nature[0];
            if(strikeType=="Double"){
                double strike0 = params[0];
                double strike1 = params[1];
                if(putCall=="Put") return (S<strike0||S>strike1);
                else if(putCall=="Call") return (S>strike0&&S<strike1);
            }
        }
        if(putCall=="Put") return (S<strike);
        else if(putCall=="Call") return (S>strike);
    }else if(type=="Asian"){
        string avgType = nature[0];
        if(priceSeries.isEmpty()) return NAN;
        else S = priceSeries.getRow(0).mean(avgType);
        if(nature.size()>1){
            string strikeType = nature[1];
            if(strikeType=="Float"){
                double fltStrk = S;
                S = priceSeries.getLastEntry();
                if(putCall=="Put") return max(fltStrk-S,0.);
                else if(putCall=="Call") return max(S-fltStrk,0.);
            }
        }
        if(putCall=="Put") return max(strike-S,0.);
        else if(putCall=="Call") return max(S-strike,0.);
    }else if(type=="Barrier"){
        if(priceSeries.isEmpty()) return NAN;
        else S = priceSeries.getLastEntry();
        string barrierType = nature[0];
        double barrier = params[0];
        double rebate = params[1];
        bool triggered =
        (barrierType=="Up-and-In" && max(priceSeries)>barrier) ||
        (barrierType=="Up-and-Out" && max(priceSeries)<barrier) ||
        (barrierType=="Down-and-In" && min(priceSeries)<barrier) ||
        (barrierType=="Down-and-Out" && min(priceSeries)>barrier);
        if(triggered){
            if(putCall=="Put") return max(strike-S,0.);
            else if(putCall=="Call") return max(S-strike,0.);
        }else return rebate;
    }else if(type=="Lookback"){
        if(priceSeries.isEmpty()) return NAN;
        else S = priceSeries.getLastEntry();
        if(putCall=="Put"){
            double Smax = max(priceSeries);
            return max(Smax-S,0.);
        }else if(putCall=="Call"){
            double Smin = min(priceSeries);
            return max(S-Smin,0.);
        }
    }else if(type=="Margrabe"){
        if(priceSeriesSet.empty()) return NAN;
        double S0 = priceSeriesSet[0].getLastEntry();
        double S1 = priceSeriesSet[1].getLastEntry();
        if(putCall=="Put") return max(S1-S0,0.);
        else if(putCall=="Call") return max(S0-S1,0.);
    }else if(type=="Basket"){
        if(priceSeriesSet.empty()) return NAN;
        int n = (int)priceSeriesSet.size();
        matrix Sset(1,n);
        for(int i=0; i<n; i++) Sset.setEntry(0,i,priceSeriesSet[i].getLastEntry());
        double S;
        if(params.size()) S = Sset.wmean(params); // weighted average
        else S = Sset.mean(); // simple average
        if(putCall=="Put") return max(strike-S,0.);
        else if(putCall=="Call") return max(S-strike,0.);
    }else if(type=="Rainbow"){
        if(priceSeriesSet.empty()) return NAN;
        int n = (int)priceSeriesSet.size();
        matrix Sset(1,n);
        for(int i=0; i<n; i++) Sset.setEntry(0,i,priceSeriesSet[i].getLastEntry());
        double S;
        string rainbowType = nature[0];
        if(rainbowType=="Best"){
            S = max(Sset);
            return max(S,strike); // Best of assets or cash
        }else if(rainbowType=="Max") S = max(Sset); // Put/Call on max
        else if(rainbowType=="Min") S = min(Sset); // Put/Call on min
        if(putCall=="Put") return max(strike-S,0.);
        else if(putCall=="Call") return max(S-strike,0.);
    }else if(type=="Chooser"){
        if(priceSeries.isEmpty()) return NAN;
        else S = priceSeries.getLastEntry();
        double chTime = params[0];
        vector<int> chTimeIdx = timeVector.find(chTime,"closest");
        string chPutCall;
        if(priceSeries.getEntry(chTimeIdx)<discStrike) chPutCall = "Put";
        else chPutCall = "Call";
        if(chPutCall=="Put") return max(strike-S,0.);
        else if(chPutCall=="Call") return max(S-strike,0.);
    }
    return NAN;
}

matrix Option::calcPayoffs(const matrix& stockPriceVector, const matrix& priceMatrix, const vector<matrix>& priceMatrixSet, const matrix& timeVector){
    matrix S;
    if(type=="European" || type=="American"){
        if(priceMatrix.isEmpty()) S = stockPriceVector;
        else S = priceMatrix.getLastRow();
        if(putCall=="Put") return max(strike-S,0.);
        else if(putCall=="Call") return max(S-strike,0.);
    }else if(type=="Digital"){
        if(priceMatrix.isEmpty()) S = stockPriceVector;
        else S = priceMatrix.getLastRow();
        if(nature.size()>0){
            string strikeType = nature[0];
            if(strikeType=="Double"){
                double strike0 = params[0];
                double strike1 = params[1];
                if(putCall=="Put") return (S<strike0)||(S>strike1);
                else if(putCall=="Call") return (S>strike0)&&(S<strike1);
            }
        }
        if(putCall=="Put") return (S<strike);
        else if(putCall=="Call") return (S>strike);
    }else if(type=="Asian"){
        string avgType = nature[0];
        if(priceMatrix.isEmpty()) return NULL_VECTOR;
        else S = priceMatrix.mean(2,avgType);
        if(nature.size()>1){
            string strikeType = nature[1];
            if(strikeType=="Float"){
                matrix fltStrk = S;
                S = priceMatrix.getLastRow();
                if(putCall=="Put") return max(fltStrk-S,0.);
                else if(putCall=="Call") return max(S-fltStrk,0.);
            }
        }
        if(putCall=="Put") return max(strike-S,0.);
        else if(putCall=="Call") return max(S-strike,0.);
    }else if(type=="Barrier" || type=="Lookback" || type=="Chooser"){ // generic single-stock
        matrix _priceMatrix = priceMatrix;
        if(priceMatrix.isEmpty()){
            if(type=="Barrier"){
                _priceMatrix = stockPriceVector;
            }else return NULL_VECTOR;
        }
        int n = _priceMatrix.getCols();
        matrix V(1,n);
        for(int i=0; i<n; i++) V.setEntry(0,i,calcPayoff(0,_priceMatrix.getCol(i),{},timeVector));
        return V;
    }else if(type=="Margrabe" || type=="Basket" || type=="Rainbow"){ // generic multi-stock
        if(priceMatrixSet.empty()) return NULL_VECTOR;
        int n = priceMatrixSet[0].getCols();
        int m = (int)priceMatrixSet.size();
        matrix V(1,n);
        for(int i=0; i<n; i++){
            vector<matrix> priceSeriesSet;
            for(int j=0; j<m; j++) priceSeriesSet.push_back(priceMatrixSet[j].getCol(i));
            V.setEntry(0,i,calcPayoff(0,NULL_VECTOR,priceSeriesSet));
        }
        return V;
    }
    return NULL_VECTOR;
}




//### operators ################################################################

ostream& operator<<(ostream& out, const SimulationConfig& config){
    out << config.getAsJson();
    return out;
}

ostream& operator<<(ostream& out, const Option& option){
    out << option.getAsJson();
    return out;
}

ostream& operator<<(ostream& out, const Stock& stock){
    out << stock.getAsJson();
    return out;
}

ostream& operator<<(ostream& out, const Market& market){
    out << market.getAsJson();
    return out;
}

ostream& operator<<(ostream& out, const Pricer& pricer){
    out << pricer.getAsJson();
    return out;
}

#endif
