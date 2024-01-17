//
//  stock.cpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//

#include "util.cpp"
#include "complx.cpp"
#include "matrix.cpp"
#include "option.cpp"
#include "stock.h"
using namespace std;


Stock::Stock(double currentPrice, double dividendYield, double driftRate, double volatility,
             const vector<double>& dynParams, string dynamics, string name){
    this->currentPrice = currentPrice;
    this->dividendYield = dividendYield;
    this->driftRate = driftRate;
    this->volatility = volatility;
    this->dynParams = dynParams;
    this->dynamics = dynamics;
    this->name = name;
    assert(checkParams());
}

Stock::Stock(const Stock& stock){
    this->currentPrice = stock.currentPrice;
    this->dividendYield = stock.dividendYield;
    this->driftRate = stock.driftRate;
    this->volatility = stock.volatility;
    this->dynParams = stock.dynParams;
    this->dynamics = stock.dynamics;
    this->name = stock.name;
}

string Stock::getAsJson() const {
    ostringstream oss;
    oss << "{" <<
    "\"name\":\""         << name             << "\"," <<
    "\"dynamics\":\""     << dynamics         << "\"," <<
    "\"currentPrice\":"   << currentPrice     << "," <<
    "\"dividendYield\":"  << dividendYield    << "," <<
    "\"driftRate\":"      << driftRate        << "," <<
    "\"volatility\":"     << volatility       << "," <<
    "\"dynParams\":"      << dynParams        <<
    "}";
    return oss.str();
}

string Stock::setName(string name){
    this->name = name;
    return name;
}

string Stock::setDynamics(string dynamics){
    this->dynamics = dynamics;
    return dynamics;
}

double Stock::setCurrentPrice(double currentPrice){
    this->currentPrice = currentPrice;
    return currentPrice;
}

double Stock::setDividendYield(double dividendYield){
    this->dividendYield = dividendYield;
    return dividendYield;
}

double Stock::setDriftRate(double driftRate){
    this->driftRate = driftRate;
    return driftRate;
}

double Stock::setVolatility(double volatility){
    this->volatility = volatility;
    return volatility;
}

vector<double> Stock::setDynParams(const vector<double>& dynParams){
    this->dynParams = dynParams;
    return dynParams;
}

matrix Stock::setSimTimeVector(const matrix& simTimeVector){
    this->simTimeVector = simTimeVector;
    return simTimeVector;
}

matrix Stock::setSimPriceMatrix(const matrix& simPriceMatrix){
    this->simPriceMatrix = simPriceMatrix;
    return simPriceMatrix;
}

double Stock::estDriftRateFromPrice(const matrix& priceSeries, double dt, string method){
    if(method=="simple"){
        matrix returnSeries;
        returnSeries =
        (priceSeries.submatrix(1,-1,"col")-priceSeries.submatrix(0,-2,"col"))
        /priceSeries.submatrix(0,-2,"col");
        driftRate = returnSeries.mean()/dt;
    }
    return driftRate;
}

double Stock::estVolatilityFromPrice(const matrix& priceSeries, double dt, string method){
    if(method=="simple"){
        matrix returnSeries;
        returnSeries =
        (priceSeries.submatrix(1,-1,"col")-priceSeries.submatrix(0,-2,"col"))
        /priceSeries.submatrix(0,-2,"col");
        volatility = sqrt(returnSeries.var()/dt);
    }
    return volatility;
}

bool Stock::checkParams() const {
    return currentPrice>=0 && dividendYield>=0 && volatility>=0;
}

double Stock::calcLognormalPrice(double z, double time){
    double S = currentPrice*exp(
                                (driftRate-volatility*volatility/2)*time+volatility*sqrt(time)*z
                                );
    return S;
}

matrix Stock::calcLognormalPriceVector(const matrix& z, double time){
    int n = z.getCols();
    matrix S(1,n);
    for(int i=0; i<n; i++) S.setEntry(0,i,calcLognormalPrice(z.getEntry(0,i),time));
    return S;
}

matrix Stock::simulatePrice(const SimulationConfig& config, int numSim, const matrix& randomMatrix){
    vector<matrix> fullCalc = USE_LOOP?
    simulatePriceWithFullCalc_loop(config,numSim,randomMatrix):
    simulatePriceWithFullCalc(config,numSim,randomMatrix);
    return fullCalc[0]; // simPriceMatrix
}

vector<matrix> Stock::simulatePriceWithFullCalc_loop(const SimulationConfig& config, int numSim, const matrix& randomMatrix){
    const int n = config.iters;
    const int m = numSim;
    double dt = config.stepSize;
    double sqrt_dt = sqrt(dt);
    double *simTimeVector_ = new double[n+1];
    double *simPriceMatrix_ = new double[(n+1)*m];
    bool nullInputRandMatrix = randomMatrix.isEmpty();
    simTimeVector_[0] = 0;
    if(dynamics=="lognormal"){
        double mult0 = 1+driftRate*dt;
        double mult1 = volatility*sqrt_dt;
        for(int j=0; j<m; j++) simPriceMatrix_[j] = currentPrice;
        for(int i=1; i<n+1; i++){
            for(int j=0; j<m; j++){
                double r0 = nullInputRandMatrix?normalRand_():randomMatrix.getEntry(i,j);
                double S0 = simPriceMatrix_[(i-1)*m+j];
                double S1 = S0*(mult0+mult1*r0);
                simPriceMatrix_[i*m+j] = S1;
            }
            simTimeVector_[i] = i*dt;
        }
    }else if(dynamics=="Heston"){
        double sig0             = volatility;
        double reversionRate    = dynParams[0];
        double longRunVar       = dynParams[1];
        double volOfVol         = dynParams[2];
        double brownianCor0     = dynParams[3];
        double brownianCor1     = sqrt(1-brownianCor0*brownianCor0);
        double *simVolMatrix_ = new double[(n+1)*m];
        double *simVarMatrix_ = new double[(n+1)*m];
        double mult0 = 1+driftRate*dt, mult1;
        for(int j=0; j<m; j++){
            simPriceMatrix_[j] = currentPrice;
            simVolMatrix_[j] = sig0;
            simVarMatrix_[j] = sig0*sig0;
        }
        // assert(2*reversionRate*longRunVar>volOfVol*volOfVol); // Feller condition
        for(int i=1; i<n+1; i++){
            for(int j=0; j<m; j++){
                double r0 = nullInputRandMatrix?normalRand_():randomMatrix.getEntry(i,j);
                double r1 = normalRand_();
                double S0 = simPriceMatrix_[(i-1)*m+j];
                double currentVar = simVarMatrix_[(i-1)*m+j], currentVol;
                currentVar += reversionRate*(longRunVar-currentVar)*dt+volOfVol*sqrt(currentVar)*sqrt_dt*(brownianCor0*r0+brownianCor1*r1);
                currentVar  = max(currentVar,0.);
                currentVol  = sqrt(currentVar);
                mult1 = currentVol*sqrt_dt;
                double S1 = S0*(mult0+mult1*r0);
                simPriceMatrix_[i*m+j] = S1;
                simVolMatrix_[i*m+j] = currentVol;
                simVarMatrix_[i*m+j] = currentVar;
            }
            simTimeVector_[i] = i*dt;
        }
        simTimeVector = matrix(1,n+1,simTimeVector_);
        simPriceMatrix = matrix(n+1,m,simPriceMatrix_);
        matrix simVolMatrix = matrix(n+1,m,simVolMatrix_);
        matrix simVarMatrix = matrix(n+1,m,simVarMatrix_);
        delete[] simTimeVector_;
        delete[] simPriceMatrix_;
        delete[] simVolMatrix_;
        delete[] simVarMatrix_;
        return {simPriceMatrix,simVolMatrix,simVarMatrix};
    }
    simTimeVector = matrix(1,n+1,simTimeVector_);
    simPriceMatrix = matrix(n+1,m,simPriceMatrix_);
    delete[] simTimeVector_;
    delete[] simPriceMatrix_;
    return {simPriceMatrix};
}

vector<matrix> Stock::simulatePriceWithFullCalc(const SimulationConfig& config, int numSim, const matrix& randomMatrix){
    int n = config.iters;
    double dt = config.stepSize;
    double sqrt_dt = sqrt(dt);
    matrix randomVector(1,numSim);
    matrix simPriceVector(1,numSim,currentPrice);
    simTimeVector = matrix(1,n+1);
    simPriceMatrix = matrix(n+1,numSim);
    simPriceMatrix.setRow(0,simPriceVector);
    simTimeVector.setEntry(0,0,0);
    if(dynamics=="lognormal"){
        for(int i=1; i<n+1; i++){
            if(randomMatrix.isEmpty()) randomVector.setNormalRand();
            else randomVector = randomMatrix.getRow(i);
            simPriceVector += simPriceVector*(driftRate*dt+volatility*sqrt_dt*randomVector);
            simPriceMatrix.setRow(i,simPriceVector);
            simTimeVector.setEntry(0,i,i*dt);
        }
    }else if(dynamics=="jump-diffusion"){ // Merton
        double sig0 = volatility;
        double lamJ = dynParams[0];
        double muJ  = dynParams[1];
        double sigJ = dynParams[2];
        matrix poiRandomVector(1,numSim), jmpRandomVector(1,numSim);
        matrix simPoiMatrix(n+1,numSim), simJmpMatrix(n+1,numSim);
        for(int i=1; i<n+1; i++){
            if(randomMatrix.isEmpty()) randomVector.setNormalRand();
            else randomVector = randomMatrix.getRow(i);
            poiRandomVector.setPoissonRand(lamJ*dt);
            for(int j=0; j<numSim; j++) jmpRandomVector.setEntry(0,j,
                                                                 matrix(1,poiRandomVector.getEntry(0,j),"normal rand",{muJ*dt,sigJ*sqrt_dt}).sum());
            simPriceVector += simPriceVector*(driftRate*dt+volatility*sqrt_dt*randomVector+jmpRandomVector);
            simPriceMatrix.setRow(i,simPriceVector);
            simPoiMatrix.setRow(i,poiRandomVector);
            simJmpMatrix.setRow(i,jmpRandomVector);
            simTimeVector.setEntry(0,i,i*dt);
        }
        return {simPriceMatrix,simPoiMatrix,simJmpMatrix};
    }else if(dynamics=="Heston"){
        double sig0             = volatility;
        double reversionRate    = dynParams[0];
        double longRunVar       = dynParams[1];
        double volOfVol         = dynParams[2];
        double brownianCor0     = dynParams[3];
        double brownianCor1     = sqrt(1-brownianCor0*brownianCor0);
        matrix volRandomVector(1,numSim);
        matrix currentVol(1,numSim,sig0), currentVar(1,numSim,sig0*sig0);
        matrix simVolMatrix(n+1,numSim), simVarMatrix(n+1,numSim);
        simVolMatrix.setRow(0,currentVol);
        simVarMatrix.setRow(0,currentVar);
        // assert(2*reversionRate*longRunVar>volOfVol*volOfVol); // Feller condition
        for(int i=1; i<n+1; i++){
            if(randomMatrix.isEmpty()) randomVector.setNormalRand();
            else randomVector = randomMatrix.getRow(i);
            volRandomVector.setNormalRand();
            volRandomVector = brownianCor0*randomVector+brownianCor1*volRandomVector;
            // currentVar += reversionRate*(longRunVar-max(currentVar,0.))*dt+volOfVol*sqrt(max(currentVar,0.))*sqrt_dt*volRandomVector;
            currentVar += reversionRate*(longRunVar-currentVar)*dt+volOfVol*sqrt(currentVar)*sqrt_dt*volRandomVector;
            currentVar  = max(currentVar,0.);
            currentVol  = sqrt(currentVar);
            simPriceVector += simPriceVector*(driftRate*dt+currentVol*sqrt_dt*randomVector);
            simPriceMatrix.setRow(i,simPriceVector);
            simVolMatrix.setRow(i,currentVol);
            simVarMatrix.setRow(i,currentVar);
            simTimeVector.setEntry(0,i,i*dt);
        }
        return {simPriceMatrix,simVolMatrix,simVarMatrix};
    }else if(dynamics=="GARCH"){
        double sig0             = volatility;
        double reversionRate    = dynParams[0];
        double longRunVar       = dynParams[1];
        double volOfVol         = dynParams[2];
        double brownianCor0     = dynParams[3];
        double brownianCor1     = sqrt(1-brownianCor0*brownianCor0);
        matrix volRandomVector(1,numSim);
        matrix currentVol(1,numSim,sig0), currentVar(1,numSim,sig0*sig0);
        matrix simVolMatrix(n+1,numSim), simVarMatrix(n+1,numSim);
        simVolMatrix.setRow(0,currentVol);
        simVarMatrix.setRow(0,currentVar);
        for(int i=1; i<n+1; i++){
            if(randomMatrix.isEmpty()) randomVector.setNormalRand();
            else randomVector = randomMatrix.getRow(i);
            volRandomVector.setNormalRand();
            volRandomVector = brownianCor0*randomVector+brownianCor1*volRandomVector;
            currentVar += reversionRate*(longRunVar-currentVar)*dt+volOfVol*currentVar*sqrt_dt*volRandomVector;
            currentVar  = max(currentVar,0.);
            currentVol  = sqrt(currentVar);
            simPriceVector += simPriceVector*(driftRate*dt+currentVol*sqrt_dt*randomVector);
            simPriceMatrix.setRow(i,simPriceVector);
            simVolMatrix.setRow(i,currentVol);
            simVarMatrix.setRow(i,currentVar);
            simTimeVector.setEntry(0,i,i*dt);
        }
        return {simPriceMatrix,simVolMatrix,simVarMatrix};
    }else if(dynamics=="CEV"){
        double gamma = dynParams[0];
        for(int i=1; i<n+1; i++){
            if(randomMatrix.isEmpty()) randomVector.setNormalRand();
            else randomVector = randomMatrix.getRow(i);
            simPriceVector += simPriceVector*driftRate*dt+pow(simPriceVector,gamma)*volatility*sqrt_dt*randomVector;
            simPriceMatrix.setRow(i,simPriceVector);
            simTimeVector.setEntry(0,i,i*dt);
        }
    }
    return {simPriceMatrix};
}

matrix Stock::bootstrapPrice(const matrix& priceSeries, const SimulationConfig& config, int numSim){
    int n = config.iters;
    double dt = config.stepSize;
    matrix simPriceVector(1,numSim,currentPrice);
    matrix returnSeries, bootReturnSeries;
    simTimeVector = matrix(1,n+1);
    simPriceMatrix = matrix(n+1,numSim);
    simPriceMatrix.setRow(0,simPriceVector);
    simTimeVector.setEntry(0,0,0);
    returnSeries =
    (priceSeries.submatrix(1,-1,"col")-priceSeries.submatrix(0,-2,"col"))
    /priceSeries.submatrix(0,-2,"col");
    for(int i=1; i<n+1; i++){
        bootReturnSeries = returnSeries.sample(numSim,true);
        simPriceVector += simPriceVector*bootReturnSeries;
        simPriceMatrix.setRow(i,simPriceVector);
        simTimeVector.setEntry(0,i,i*dt);
    }
    return simPriceMatrix;
}

matrix Stock::generatePriceTree(const SimulationConfig& config){
    int n = config.iters;
    double dt = config.stepSize;
    double sqrt_dt = sqrt(dt);
    double u = exp(volatility*sqrt_dt), d = 1/u;
    simTimeVector = matrix(1,n);
    binomialPriceTree = matrix(n,n);
    binomialPriceTree.setEntry(0,0,currentPrice);
    simTimeVector.setEntry(0,0,0);
    for(int i=1; i<n; i++){
        for(int j=0; j<i; j++)
            binomialPriceTree.setEntry(i,j,binomialPriceTree.getEntry(i-1,j)*d);
        binomialPriceTree.setEntry(i,i,binomialPriceTree.getEntry(i-1,i-1)*u);
        simTimeVector.setEntry(0,i,i*dt);
    }
    return binomialPriceTree;
}



