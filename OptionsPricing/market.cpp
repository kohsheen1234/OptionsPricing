//
//  market.cpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#include "market.hpp"

#ifndef MARKET
#define MARKET

Market::Market(double riskFreeRate, const Stock& stock, const vector<Stock>& stocks, const matrix& corMatrix){
    this->riskFreeRate = riskFreeRate;
    this->stock = stock;
    this->stocks = stocks;
    this->corMatrix = corMatrix;
}

Market::Market(const Market& market){
    this->riskFreeRate = market.riskFreeRate;
    this->stock = market.stock;
    this->stocks = market.stocks;
    this->corMatrix = market.corMatrix;
}

string Market::getAsJson() const {
    ostringstream oss;
    oss << "{" <<
    "\"riskFreeRate\":"   << riskFreeRate << "," <<
    "\"stock\":"          << stock        << "," <<
    "\"stocks\":"         << stocks       << "," <<
    "\"corMatrix\":"      << corMatrix    <<
    "}";
    return oss.str();
}

double Market::setRiskFreeRate(double riskFreeRate){
    this->riskFreeRate = riskFreeRate;
    return riskFreeRate;
}

Stock Market::setStock(const Stock& stock, int i){
    if(i<0) this->stock = stock;
    else this->stocks[i] = stock;
    return stock;
}

vector<Stock> Market::setStocks(const vector<Stock>& stocks){
    this->stocks = stocks;
    return stocks;
}

matrix Market::setCorMatrix(const matrix& corMatrix){
    this->corMatrix = corMatrix;
    return corMatrix;
}

vector<matrix> Market::simulateCorrelatedPrices(const SimulationConfig& config, int numSim, const vector<matrix>& randomMatrixSet){
    vector<vector<matrix>> fullCalc = simulateCorrelatedPricesWithFullCalc(config,numSim,randomMatrixSet);
    return fullCalc[0]; // simPriceMatrixSet
}

vector<vector<matrix>> Market::simulateCorrelatedPricesWithFullCalc(const SimulationConfig& config, int numSim, const vector<matrix>& randomMatrixSet){
    int n = config.iters;
    int m = (int)stocks.size();
    double dt = config.stepSize;
    double sqrt_dt = sqrt(dt);
    matrix simTimeVector(1,n+1);
    vector<matrix> randomVectorSet;
    vector<matrix> simPriceVectorSet;
    vector<matrix> simPriceMatrixSet;
    simTimeVector.setEntry(0,0,0);
    for(auto stock:stocks){
        double S = stock.getCurrentPrice();
        matrix randomVector(1,numSim);
        matrix simPriceVector(1,numSim,S);
        matrix simPriceMatrix(n+1,numSim);
        simPriceMatrix.setRow(0,simPriceVector);
        randomVectorSet.push_back(randomVector);
        simPriceVectorSet.push_back(simPriceVector);
        simPriceMatrixSet.push_back(simPriceMatrix);
    }
    matrix corFactor = corMatrix.chol(); // Choleskey decomposition
    string dynamics = stocks[0].getDynamics();
    if(dynamics=="lognormal"){
        for(int i=1; i<n+1; i++){
            matrix iidRandomMatrix(m,numSim);
            if(randomMatrixSet.empty()) iidRandomMatrix.setNormalRand();
            else for(int j=0; j<m; j++) iidRandomMatrix.setRow(j,randomMatrixSet[j].getRow(i));
            matrix corRandomMatrix = corFactor.dot(iidRandomMatrix);
            for(int j=0; j<m; j++){
                double driftRate = stocks[j].getDriftRate();
                double volatility = stocks[j].getVolatility();
                randomVectorSet[j] = corRandomMatrix.getRow(j);
                simPriceVectorSet[j] += simPriceVectorSet[j]*(driftRate*dt+volatility*sqrt_dt*randomVectorSet[j]);
                simPriceMatrixSet[j].setRow(i,simPriceVectorSet[j]);
            }
            simTimeVector.setEntry(0,i,i*dt);
        }
        for(int j=0; j<m; j++){
            stocks[j].setSimTimeVector(simTimeVector);
            stocks[j].setSimPriceMatrix(simPriceMatrixSet[j]);
        }
    }else if(dynamics=="jump-diffusion"){} // TO DO
    else if(dynamics=="Heston"){} // TO DO
    return {simPriceMatrixSet};
}

#endif
