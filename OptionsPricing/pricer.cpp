//
//  pricer.cpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/16/24.
//




#include <iostream>
#include <sstream>
#include "pricer.hpp"
#include "stock.h"
#include "util.cpp"
#include "complx.cpp"
#include "matrix.cpp"

using namespace std;

inline void logMessage(string msg){if(LOG) cout << getCurrentTime() << " [LOG] " << msg << endl;}



Pricer::Pricer(const Option& option, const Market& market){
    this->option = option; this->option_orig = option;
    this->market = market; this->market_orig = market;
    this->price = NAN;
}

string Pricer::getAsJson() const {
    ostringstream oss;
    oss << "{" <<
    "\"option\":"     << option   << "," <<
    "\"market\":"     << market   << "," <<
    "\"price\":"      << price    <<
    "}";
    return oss.str();
}

double Pricer::getVariable(string var, int i, int j) const {
    double v = NAN;
    if(var=="currentPrice"){
        Stock stock = market.getStock(i);
        v = stock.getCurrentPrice();
    }else if(var=="driftRate"){
        Stock stock = market.getStock(i);
        v = stock.getDriftRate();
    }else if(var=="dividendYield"){
        Stock stock = market.getStock(i);
        v = stock.getDividendYield();
    }else if(var=="volatility"){
        Stock stock = market.getStock(i);
        v = stock.getVolatility();
    }else if(var=="correlation"){
        v = market.getCorMatrix().getEntry(i,j);
    }else if(var=="riskFreeRate"){
        v = market.getRiskFreeRate();
    }else if(var=="strike"){
        v = option.getStrike();
    }else if(var=="maturity"){
        v = option.getMaturity();
    }
    return v;
}

double Pricer::setVariable(string var, double v, int i){
    if(var=="currentPrice"){
        Stock tmpStock = market.getStock(i);
        tmpStock.setCurrentPrice(v);
        market.setStock(tmpStock,i);
    }else if(var=="dividendYield"){
        Stock tmpStock = market.getStock(i);
        tmpStock.setDividendYield(v);
        market.setStock(tmpStock,i);
    }else if(var=="volatility"){
        Stock tmpStock = market.getStock(i);
        tmpStock.setVolatility(v);
        market.setStock(tmpStock,i);
    }else if(var=="riskFreeRate"){
        market.setRiskFreeRate(v);
    }else if(var=="strike"){
        option.setStrike(v);
    }else if(var=="maturity"){
        option.setMaturity(v);
    }
    return v;
}

string Pricer::setStringVariable(string var, string v){
    if(var=="stockName"){
        Stock tmpStock = market.getStock();
        tmpStock.setName(v);
        market.setStock(tmpStock);
    }else if(var=="optionName"){
        option.setName(v);
    }else if(var=="optionType"){
        option.setType(v);
    }
    return v;
}

Pricer Pricer::setVariablesFromFile(string file){
    string var, val;
    ifstream f(file);
    while(getline(f,var,',')){
        getline(f,val);
        double v;
        if(isDouble(val,v)) setVariable(var,v);
        else setStringVariable(var,val);
    }
    f.close();
    return *this;
}

Pricer Pricer::resetOriginal(){
    option = option_orig;
    market = market_orig;
    return *this;
}

Pricer Pricer::saveAsOriginal(){
    option_orig = option;
    market_orig = market;
    return *this;
}

double Pricer::BlackScholesClosedForm(){
    logMessage("starting calculation BlackScholesClosedForm");
    if(option.getType()=="European"){
        double K   = getVariable("strike");
        double T   = getVariable("maturity");
        double r   = getVariable("riskFreeRate");
        double S0  = getVariable("currentPrice");
        double q   = getVariable("dividendYield");
        double sig = getVariable("volatility");
        double d1  = (log(S0/K)+(r-q+sig*sig/2)*T)/(sig*sqrt(T));
        double d2  = d1-sig*sqrt(T);
        if(option.getPutCall()=="Call")
            price = S0*exp(-q*T)*normalCDF(d1)-K*exp(-r*T)*normalCDF(d2);
        else if(option.getPutCall()=="Put")
            price = K*exp(-r*T)*normalCDF(-d2)-S0*exp(-q*T)*normalCDF(-d1);
    }else if(option.getType()=="Margrabe"){
        double T   = getVariable("maturity");
        //        double r   = getVariable("riskFreeRate");
        double S0  = getVariable("currentPrice",0);
        double S1  = getVariable("currentPrice",1);
        double q0  = getVariable("dividendYield",0);
        double q1  = getVariable("dividendYield",1);
        double sig0 = getVariable("volatility",0);
        double sig1 = getVariable("volatility",1);
        double rho = getVariable("correlation",0,1);
        double sig = sqrt(sig0*sig0+sig1*sig1-2*rho*sig0*sig1);
        double d0  = (log(S0/S1)+(q1-q0+sig*sig/2)*T)/(sig*sqrt(T));
        double d1  = d0-sig*sqrt(T);
        if(option.getPutCall()=="Call")
            price = S0*exp(-q0*T)*normalCDF(d0)-S1*exp(-q1*T)*normalCDF(d1);
        else if(option.getPutCall()=="Put")
            price = S1*exp(-q1*T)*normalCDF(d1)-S0*exp(-q0*T)*normalCDF(d0);
    }else if(option.getType()=="Digital"){
        double K   = getVariable("strike");
        double T   = getVariable("maturity");
        double r   = getVariable("riskFreeRate");
        double S0  = getVariable("currentPrice");
        double q   = getVariable("dividendYield");
        double sig = getVariable("volatility");
        vector<string> nature = option.getNature();
        if(nature.size()>0){
            string strikeType = nature[0];
            if(strikeType=="Double"){
                vector<double> params = option.getParams();
                double K0 = params[0];
                double K1 = params[1];
                double d01 = (log(S0/K0)+(r-q+sig*sig/2)*T)/(sig*sqrt(T));
                double d02 = d01-sig*sqrt(T);
                double d11 = (log(S0/K1)+(r-q+sig*sig/2)*T)/(sig*sqrt(T));
                double d12 = d11-sig*sqrt(T);
                if(option.getPutCall()=="Call")
                    price = exp(-r*T)*(normalCDF(d02)-normalCDF(d12));
                else if(option.getPutCall()=="Put")
                    price = exp(-r*T)*(normalCDF(-d02)+normalCDF(d12));
            }
        }
        double d1 = (log(S0/K)+(r-q+sig*sig/2)*T)/(sig*sqrt(T));
        double d2 = d1-sig*sqrt(T);
        if(option.getPutCall()=="Call")
            price = exp(-r*T)*normalCDF(d2);
        else if(option.getPutCall()=="Put")
            price = exp(-r*T)*normalCDF(-d2);
    }else if(option.getType()=="Barrier"){
    }else if(option.getType()=="American"){
        double T = getVariable("maturity");
        if(T==INF){
            if(option.getPutCall()=="Put"){
                double K   = getVariable("strike");
                double r   = getVariable("riskFreeRate");
                double S0  = getVariable("currentPrice");
                //                double q   = getVariable("dividendYield");
                double sig = getVariable("volatility");
                double sig2 = sig*sig;
                double k = sig2/(2*r);
                double s = K/(1+k);
                double B = k*pow(s,1+1/k);
                if(S0<s) price = K-S0; // stopped
                else price = B*pow(S0,-1/k); // continue
            }
        }
    }
    logMessage("ending calculation BlackScholesClosedForm, return "+to_string(price));
    return price;
}

double Pricer::BinomialTreePricer(const SimulationConfig& config){
    logMessage("starting calculation BinomialTreePricer on config "+to_string(config));
    Stock stock = market.getStock();
    int n = config.iters;
    double dt = config.stepSize;
    double sqrt_dt = sqrt(dt);
    double r = getVariable("riskFreeRate");
    double q = getVariable("dividendYield");
    double sig = getVariable("volatility");
    double u = exp(sig*sqrt_dt), d = 1/u;
    double qu = (exp((r-q)*dt)-d)/(u-d), qd = 1-qu;
    stock.setDriftRate(r-q);
    stock.generatePriceTree(config);
    matrix optionBinomialTree(n,n);
    if(!option.isPathDependent()){
        matrix payoffs = option.calcPayoffs(stock.getBinomialPriceTree().getLastRow());
        optionBinomialTree.setRow(n-1,payoffs);
        for(int i=n-2; i>=0; i--){
            for(int j=0; j<i+1; j++)
                optionBinomialTree.setEntry(i,j,max(
                                                    exp(-r*dt)*(qu*optionBinomialTree.getEntry(i+1,j+1)+qd*optionBinomialTree.getEntry(i+1,j)),
                                                    (option.canEarlyExercise())?option.calcPayoff(stock.getBinomialPriceTree().getEntry(i,j)):0.
                                                    ));
        }
        // cout << stock.getBinomialPriceTree().print() << endl;
        // cout << optionBinomialTree.print() << endl;
        price = optionBinomialTree.getEntry(0,0);
    }
    logMessage("ending calculation BinomialTreePricer, return "+to_string(price));
    return price;
}

double Pricer::MonteCarloPricer(const SimulationConfig& config, int numSim, string method){
    logMessage("starting calculation MonteCarloPricer on config "+to_string(config)+", numSim "+to_string(numSim));
    int n = config.iters;
    Stock stock = market.getStock();
    double r = getVariable("riskFreeRate");
    double q = getVariable("dividendYield");
    double T = getVariable("maturity");
    double err = NAN;
    matrix simPriceMatrix, simTimeVector;
    stock.setDriftRate(r-q);
    // handle exceptions ================
    string dynamics = stock.getDynamics();
    string optionType = option.getType();
    if(optionType=="Chooser"){
        double K = option.getStrike();
        double t = option.getParams()[0];
        option.setDiscStrike(K*exp(-r*(T-t)));
    }
    if(dynamics=="jump-diffusion"){
        vector<double> params = stock.getDynParams();
        double lamJ = params[0];
        double muJ = params[1];
        stock.setDriftRate(r-q-lamJ*muJ);
    }
    // ==================================
    if(method=="simple"){
        simPriceMatrix = stock.simulatePrice(config,numSim);
        simTimeVector = stock.getSimTimeVector();
        if(!option.canEarlyExercise()){
            matrix payoffs = option.calcPayoffs(NULL_VECTOR,simPriceMatrix,{},simTimeVector);
            price = exp(-r*T)*payoffs.mean();
            err = exp(-r*T)*payoffs.stdev()/sqrt(numSim);
        }else{
            // Longstaff-Schwartz algorithm
        }
    }else if(method=="antithetic variates"){
        matrix simPriceMatrix0, simPriceMatrix1;
        matrix randomMatrix0(n+1,numSim), randomMatrix1(n+1,numSim);
        randomMatrix0.setNormalRand(); randomMatrix1 = -randomMatrix0;
        simPriceMatrix0 = stock.simulatePrice(config,numSim,randomMatrix0);
        simPriceMatrix1 = stock.simulatePrice(config,numSim,randomMatrix1);
        simTimeVector = stock.getSimTimeVector();
        if(!option.canEarlyExercise()){
            matrix payoffs0 = option.calcPayoffs(NULL_VECTOR,simPriceMatrix0,{},simTimeVector);
            matrix payoffs1 = option.calcPayoffs(NULL_VECTOR,simPriceMatrix1,{},simTimeVector);
            matrix payoffs = (payoffs0+payoffs1)/2;
            price = exp(-r*T)*payoffs.mean();
            err = exp(-r*T)*payoffs.stdev()/sqrt(numSim);
        }
    }else if(method=="control variates"){
        simPriceMatrix = stock.simulatePrice(config,numSim);
        simTimeVector = stock.getSimTimeVector();
        if(!option.canEarlyExercise()){
            matrix payoffs = option.calcPayoffs(NULL_VECTOR,simPriceMatrix,{},simTimeVector);
            price = exp(-r*T)*payoffs.mean();
            err = exp(-r*T)*payoffs.stdev()/sqrt(numSim);
        }
        Pricer refPricer(option,market);
        refPricer.setStringVariable("optionType","European");
        double refPriceMonte = refPricer.MonteCarloPricer(config,numSim);
        double refPriceClosed = refPricer.BlackScholesClosedForm();
        double refErr = refPricer.tmp[0];
        price += refPriceClosed-refPriceMonte;
        err = sqrt(err*err+refErr*refErr); // assume uncorrelated MC estimates
    }
    tmp = {err};
    logMessage("ending calculation MonteCarloPricer, return "+to_string(price)+" with error "+to_string(err));
    return price;
}

double Pricer::MultiStockMonteCarloPricer(const SimulationConfig& config, int numSim, string method){
    logMessage("starting calculation MonteCarloPricer on config "+to_string(config)+", numSim "+to_string(numSim));
    //    int n = config.iters;
    double r = getVariable("riskFreeRate");
    double T = getVariable("maturity");
    Market rnMarket(market); // risk-neutral market
    vector<Stock> stocks = rnMarket.getStocks();
    for(auto& stock:stocks){
        double q = stock.getDividendYield();
        stock.setDriftRate(r-q);
    }
    rnMarket.setStocks(stocks);
    double err = NAN;
    vector<matrix> simPriceMatrixSet;
    if(method=="simple"){
        simPriceMatrixSet = rnMarket.simulateCorrelatedPrices(config,numSim);
        if(!option.canEarlyExercise()){
            matrix payoffs = option.calcPayoffs(NULL_VECTOR,NULL_MATRIX,simPriceMatrixSet);
            price = exp(-r*T)*payoffs.mean();
            err = exp(-r*T)*payoffs.stdev()/sqrt(numSim);
        }
    }else if(method=="antithetic variates"){} // TO DO
    else if(method=="control variates"){} // TO DO
    tmp = {err};
    logMessage("ending calculation MonteCarloPricer, return "+to_string(price)+" with error "+to_string(err));
    return price;
}

double Pricer::NumIntegrationPricer(double z, double dz){
    logMessage("starting calculation NumIntegrationPricer on z "+to_string(z)+", dz "+to_string(dz));
    Stock stock = market.getStock();
    double r = getVariable("riskFreeRate");
    double q = getVariable("dividendYield");
    double T = getVariable("maturity");
    stock.setDriftRate(r-q);
    int n = static_cast<int>(z/dz);
    matrix z0; z0.setRange(-z,z,2*n);
    matrix S = stock.calcLognormalPriceVector(z0,T);
    if(!option.canEarlyExercise() && !option.isPathDependent()){
        matrix payoffs = option.calcPayoffs(S);
        matrix probs = z0.apply(stdNormalPDF)*dz;
        price = exp(-r*T)*(probs*payoffs).sum();
    }
    logMessage("ending calculation NumIntegrationPricer, return "+to_string(price));
    return price;
}

double Pricer::BlackScholesPDESolver(const SimulationConfig& config, int numSpace, string method){
    logMessage("starting calculation BlackScholesPDESolver on config "+to_string(config)+
               ", numSpace "+to_string(numSpace)+", method "+method);
    double S0 = getVariable("currentPrice");
    double x = log(S0);
    vector<matrix> fullCalc = BlackScholesPDESolverWithFullCalc(config,numSpace,method);
    matrix spaceGrids = fullCalc[0];
    matrix priceMatrix = fullCalc[2];
    vector<int> idx = spaceGrids.find(x,"closest");
    price = priceMatrix.getEntry(idx);
    logMessage("ending calculation BlackScholesPDESolver, return "+to_string(price));
    return price;
}

vector<matrix> Pricer::BlackScholesPDESolverWithFullCalc(const SimulationConfig& config, int numSpace, string method){
    Stock stock = market.getStock();
    double K = getVariable("strike");
    double T = getVariable("maturity");
    double r = getVariable("riskFreeRate");
    //    double S0 = getVariable("currentPrice");
    double q = getVariable("dividendYield");
    double sig = getVariable("volatility");
    int n = config.iters;
    int m = numSpace;
    double dt = config.stepSize;
    double x0, x1;
    double dx, dx2;
    double sig2 = sig*sig;
    matrix priceMatrix(n+1,m+1);
    matrix spaceGrids, timeGrids; timeGrids.setRange(0,T,n,true);
    matrix payoffs, bdryCondition0(1,n+1), bdryCondition1(1,n+1), v, u(m-1,1);
    if(option.getType()=="European" || option.getType()=="American"){
        x0 = log(K/3); x1 = log(3*K);
        if(option.getPutCall()=="Call"){
            bdryCondition1 = exp(x1)*exp(-q*(T-timeGrids))-K*exp(-r*(T-timeGrids));
        }else if(option.getPutCall()=="Put"){
            bdryCondition0 = K*exp(-r*(T-timeGrids));
        }
    }else if(option.getType()=="Digital"){
        x0 = log(K/3); x1 = log(3*K);
        if(option.getPutCall()=="Call"){
            bdryCondition1 = exp(-r*(T-timeGrids));
        }else if(option.getPutCall()=="Put"){
            bdryCondition0 = exp(-r*(T-timeGrids));
        }
    }else if(option.getType()=="Barrier"){
        vector<string> nature = option.getNature();
        vector<double> params = option.getParams();
        double barrier = params[0];
        double rebate = params[1];
        string barrierType = nature[0];
        if(barrierType=="Up-and-In"){ // TO DO: concat vnla result
            x0 = log(K/3); x1 = log(barrier);
            Option vnlaOption(option); vnlaOption.setType("European");
            Pricer vnlaPricer(vnlaOption,market);
            vector<matrix> vnlaFullCalc = vnlaPricer.BlackScholesPDESolverWithFullCalc(config,numSpace,method);
            matrix vnlaSpaceGrids = vnlaFullCalc[0];
            matrix vnlaPriceMatrix = vnlaFullCalc[2];
            int bdry1Idx = vnlaSpaceGrids.find(log(barrier),"closest")[1];
            bdryCondition1 = vnlaPriceMatrix.getCol(bdry1Idx);
        }else if(barrierType=="Up-and-Out"){
            x0 = log(K/3); x1 = log(barrier);
            bdryCondition1 = rebate*exp(-r*(T-timeGrids));
            if(option.getPutCall()=="Put"){
                bdryCondition0 = K*exp(-r*(T-timeGrids));
            }
        }else if(barrierType=="Down-and-In"){ // TO DO: concat vnla result
            x0 = log(barrier); x1 = log(3*K);
            Option vnlaOption(option); vnlaOption.setType("European");
            Pricer vnlaPricer(vnlaOption,market);
            vector<matrix> vnlaFullCalc = vnlaPricer.BlackScholesPDESolverWithFullCalc(config,numSpace,method);
            matrix vnlaSpaceGrids = vnlaFullCalc[0];
            matrix vnlaPriceMatrix = vnlaFullCalc[2];
            int bdry0Idx = vnlaSpaceGrids.find(log(barrier),"closest")[1];
            bdryCondition0 = vnlaPriceMatrix.getCol(bdry0Idx);
        }else if(barrierType=="Down-and-Out"){
            x0 = log(barrier); x1 = log(3*K);
            bdryCondition0 = rebate*exp(-r*(T-timeGrids));
            if(option.getPutCall()=="Call"){
                bdryCondition1 = exp(x1)*exp(-q*(T-timeGrids))-K*exp(-r*(T-timeGrids));
            }
        }
    }else return {};
    dx = (x1-x0)/m; dx2 = dx*dx;
    spaceGrids.setRange(x0,x1,m,true);
    payoffs = option.calcPayoffs(exp(spaceGrids));
    priceMatrix.setRow(n,payoffs);
    priceMatrix.setCol(0,bdryCondition0);
    priceMatrix.setCol(m,bdryCondition1);
    // cout << priceMatrix.print() << endl;
    v = priceMatrix.submatrix(n,n+1,1,m).T();
    if(method=="implicit"){
        double a = +(r-q-sig2/2)*dt/(2*dx)-sig2/2*dt/dx2;
        double b = 1+r*dt+sig2*dt/dx2;
        double c = -(r-q-sig2/2)*dt/(2*dx)-sig2/2*dt/dx2;
        matrix D(m-1,m-1);
        D.setDiags({a,b,c},{-1,0,1});
        D = D.inverse(); // TO DO: eff sparse inverse
        for(int i=n-1; i>=0; i--){
            double u0 = a*priceMatrix.getEntry(i,0);
            double u1 = c*priceMatrix.getEntry(i,m);
            u.setEntry(0,0,u0);
            u.setEntry(m-2,0,u1);
            v = D.dot(v-u);
            if(option.canEarlyExercise()) v = max(payoffs.submatrix(1,-2,"col").T(),v);
            priceMatrix.setSubmatrix(i,i+1,1,m,v.T());
        }
    }else if(method=="explicit"){
        double a = -(r-q-sig2/2)*dt/(2*dx)+sig2/2*dt/dx2;
        double b = 1-r*dt-sig2*dt/dx2;
        double c = +(r-q-sig2/2)*dt/(2*dx)+sig2/2*dt/dx2;
        matrix D(m-1,m-1);
        D.setDiags({a,b,c},{-1,0,1});
        for(int i=n-1; i>=0; i--){
            double u0 = a*priceMatrix.getEntry(i+1,0);
            double u1 = c*priceMatrix.getEntry(i+1,m);
            u.setEntry(0,0,u0);
            u.setEntry(m-2,0,u1);
            v = D.dot(v)+u;
            if(option.canEarlyExercise()) v = max(payoffs.submatrix(1,-2,"col").T(),v);
            priceMatrix.setSubmatrix(i,i+1,1,m,v.T());
        }
    }
    // cout << priceMatrix.print() << endl;
    return {spaceGrids, timeGrids, priceMatrix};
}

vector<double> Pricer::_FourierInversionPricer(const function<complx(complx)>& charFunc, int numSpace, double rightLim, string method){
    int m = numSpace;
    double x1 = rightLim;
    double K = getVariable("strike");
    double T = getVariable("maturity");
    double r = getVariable("riskFreeRate");
    double S0 = getVariable("currentPrice");
    double q = getVariable("dividendYield");
    double k = log(S0/K)+(r-q)*T; // forward log moneyness
    double x0 = 1e-5;
    double du = (x1-x0)/m;
    if(method=="RN Prob"){
        double Q0,Q1;
        auto f0 = [k,charFunc](double u){return (exp(i*u*k)*charFunc(u)/(i*u)).getReal();};
        auto f1 = [k,charFunc](double u){return (exp(i*u*k)*charFunc(u-i)/(i*u)).getReal();};
        if(USE_LOOP){
            double I0 = 0, I1 = 0;
            for(int n=0; n<m; n++){
                I0 += f0(x0+n*du)*((n==0||n==m-1)?1:(n%2?4:2));
                I1 += f1(x0+n*du)*((n==0||n==m-1)?1:(n%2?4:2));
            }
            Q0 = .5+1/M_PI*I0/3*du; // cash numeraire ITM prob
            Q1 = .5+1/M_PI*I1/3*du; // stock numeraire ITM prob
        }else{
            matrix spaceGrids; spaceGrids.setRange(x0,x1,m,true);
            matrix w; w.setRange(0,m+1); w = 3+pow(-1,w+1);
            w.setEntry(0,0,1); w.setEntry(0,m,1); w /= 3;
            Q0 = .5+1/M_PI*spaceGrids.apply(f0).sum(w)*du; // cash numeraire ITM prob
            Q1 = .5+1/M_PI*spaceGrids.apply(f1).sum(w)*du; // stock numeraire ITM prob
        }
        return {Q0,Q1};
    }else if(method=="Lewis"){
        auto f = [k,charFunc](double u){return (exp(i*u*k)*charFunc(u-i/2)).getReal()/(u*u+.25);};
        double lwCall;
        if(USE_LOOP){
            double I = 0;
            for(int n=0; n<m; n++) I += f(x0+n*du)*((n==0||n==m-1)?1:(n%2?4:2));
            lwCall = S0*exp(-q*T)-sqrt(S0*K)*exp(-(r+q)*T/2)/M_PI*I/3*du;
        }else{
            matrix spaceGrids; spaceGrids.setRange(x0,x1,m,true);
            matrix w; w.setRange(0,m+1); w = 3+pow(-1,w+1);
            w.setEntry(0,0,1); w.setEntry(0,m,1); w /= 3;
            lwCall = S0*exp(-q*T)-sqrt(S0*K)*exp(-(r+q)*T/2)/M_PI*spaceGrids.apply(f).sum(w)*du;
        }
        return {lwCall};
    }else if(method=="FFT"){
        vector<matrix> fftCalc = _fastFourierInversionPricer(charFunc,numSpace,rightLim);
        matrix kGrids = fftCalc[0];
        matrix lwCalls = fftCalc[1];
        double lwCall = interp({k},{kGrids},lwCalls,"linear");
        // vector<int> idx = kGrids.find(k,"closest");
        // double lwCall = lwCalls.getEntry(idx);
        return {lwCall};
    }
    return {};
}

vector<matrix> Pricer::_fastFourierInversionPricer(const function<complx(complx)>& charFunc, int numSpace, double rightLim){
    int m = pow(2,ceil(log(numSpace)/log(2)));
    double x1 = rightLim;
//    double K = getVariable("strike");
    double T = getVariable("maturity");
//    double r = getVariable("riskFreeRate");
    double S0 = getVariable("currentPrice");
    double q = getVariable("dividendYield");
    double du = x1/m;
    double dk = 2*M_PI/x1;
    double b = m*dk/2;
    matrix kGrids, lwCalls; kGrids.setRange(-b,b,m);
    vector<complx> F(m);
    for(int n=0; n<m; n++){
        double u = n*du;
        double w = (n==0||n==m-1)?1:(n%2?4:2);
        F[n] = w/3*exp(-i*b*n*du)*charFunc(u-i/2)/(u*u+.25);
    }
    fft(F);
    if(USE_LOOP){
        lwCalls = matrix(1,m);
        double mult0 = S0*exp(-q*T);
        double mult1 = du/M_PI;
        double b_2 = b/2, dk_2 = dk/2;
        for(int n=0; n<m; n++)
            lwCalls.setEntry(0,n,mult0*(1-mult1*exp(b_2-n*dk_2)*F[n].getReal()));
    }else{
        function<double(complx)> f = [](complx c){return c.getReal();};
        lwCalls = S0*exp(-q*T)*(1-exp(-kGrids/2)/M_PI*matrix(apply(f,F))*du);
    }
    return {kGrids,lwCalls};
}

double Pricer::FourierInversionPricer(int numSpace, double rightLim, string method){
    logMessage("starting calculation FourierInversionPricer on config numSpace "+
               to_string(numSpace)+", rightLim "+to_string(rightLim)+", method "+method);
    Stock stock = market.getStock();
    double K = getVariable("strike");
    double T = getVariable("maturity");
    double r = getVariable("riskFreeRate");
    double S0 = getVariable("currentPrice");
    double q = getVariable("dividendYield");
    stock.setDriftRate(r-q);
    function<complx(complx)> charFunc;
    string dynamics = stock.getDynamics();
    if(dynamics=="lognormal"){
        double sig = stock.getVolatility();
        double sig2 = sig*sig;
        double Mu = -sig2/2*T;
        double Sig2 = sig2*T;
        charFunc = [Mu,Sig2](complx u){return exp(i*Mu*u-Sig2*u*u/2);};
    }else if(dynamics=="jump-diffusion"){
        vector<double> dynParams = stock.getDynParams();
        double sig = stock.getVolatility();
        double sig2 = sig*sig;
        double lamJ = dynParams[0]; // jump intesity
        double muJ = dynParams[1];  // jump mean
        double sigJ = dynParams[2]; // jump s.d.
        double sigJ2 = sigJ*sigJ;
        double Mu = (-sig2/2-lamJ*(exp(muJ+sigJ2/2)-1))*T;
        double Sig2 = sig2*T;
        double LamJ = lamJ*T;
        charFunc = [Mu,Sig2,LamJ,muJ,sigJ2](complx u){return exp(i*Mu*u-Sig2*u*u/2+LamJ*(exp(i*muJ*u-sigJ2*u*u/2)-1));};
    }else if(dynamics=="variance-gamma"){}
    else if(dynamics=="Heston"){
        vector<double> dynParams = stock.getDynParams();
        double sig = stock.getVolatility();
        double sig2 = sig*sig;
        double kappa = dynParams[0]; // reversion rate
        double theta = dynParams[1]; // long run var
        double sigma = dynParams[2]; // vol of vol
        double rho = dynParams[3];   // Brownian cor
        double sigma2 = sigma*sigma;
        charFunc = [sig2,kappa,theta,sigma,sigma2,rho,T](complx u){
            complx xi = kappa-i*sigma*rho*u;
            complx d = sqrt(xi*xi+sigma2*(u*u+i*u));
            complx g1 = (xi+d)/(xi-d), g2 = 1/g1;
            return exp(kappa*theta/sigma2*((xi-d)*T-2*log((1-g2*exp(-d*T))/(1-g2)))+sig2/sigma2*(xi-d)*(1-exp(-d*T))/(1-g2*exp(-d*T)));
        };
    }
    vector<double> fiCalc = _FourierInversionPricer(charFunc,numSpace,rightLim,method);
    if(method=="RN Prob"){
        double Q0 = fiCalc[0];
        double Q1 = fiCalc[1];
        if(option.getType()=="European"){
            if(option.getPutCall()=="Call") price = S0*exp(-q*T)*Q1-K*exp(-r*T)*Q0;
            else if(option.getPutCall()=="Put") price = K*exp(-r*T)*(1-Q0)-S0*exp(-q*T)*(1-Q1);
        }else if(option.getType()=="Digital"){
            if(option.getPutCall()=="Call") price = exp(-r*T)*Q0;
            else if(option.getPutCall()=="Put") price = exp(-r*T)*(1-Q0);
        }
    }else if(method=="Lewis" || method=="FFT"){
        double lwCall = fiCalc[0];
        if(option.getType()=="European"){
            if(option.getPutCall()=="Call") price = lwCall;
            else if(option.getPutCall()=="Put") price = lwCall-S0*exp(-q*T)+K*exp(-r*T);
        }
    }
    logMessage("ending calculation FourierInversionPricer, return "+to_string(price));
    return price;
}

double Pricer::calcPrice(string method, const SimulationConfig& config, int numSim, int numSpace){
    if(method=="Closed Form"){
        price = BlackScholesClosedForm();
    }else if(method=="Binomial Tree"){
        price = BinomialTreePricer(config);
    }else if(method=="Monte Carlo"){
        price = MonteCarloPricer(config,numSim);
    }else if(method=="Num Integration"){
        price = NumIntegrationPricer();
    }else if(method=="PDE Solver"){
        price = BlackScholesPDESolver(config,numSpace);
    }else if(method=="Fourier Inversion"){
        price = FourierInversionPricer(numSpace);
    }
    return price;
}

matrix Pricer::varyPriceWithVariable(string var, const matrix& varVector,
                                     string method, const SimulationConfig& config, int numSim){
    saveAsOriginal();
    int n = varVector.getCols();
    matrix optionPriceVector(1,n);
    for(int i=0; i<n; i++){
        double v = varVector.getEntry(0,i);
        setVariable(var,v);
        price = calcPrice(method,config,numSim);
        optionPriceVector.setEntry(0,i,price);
    }
    resetOriginal();
    return optionPriceVector;
}

double Pricer::ClosedFormGreek(string var, int derivOrder){
    logMessage("starting calculation ClosedFormGreek on var "+var+", derivOrder "+to_string(derivOrder));
    saveAsOriginal();
    double greek = NAN;
    if(option.getType()=="European"){
        if(getVariable("maturity")==0)
            setVariable("maturity",1e-5);
        double K   = getVariable("strike");
        double T   = getVariable("maturity");
        double r   = getVariable("riskFreeRate");
        double S0  = getVariable("currentPrice");
        double q   = getVariable("dividendYield");
        double sig = getVariable("volatility");
        double sqrt_T = sqrt(T);
        double d1  = (log(S0/K)+(r-q+sig*sig/2)*T)/(sig*sqrt_T);
        double d2  = d1-sig*sqrt_T;
        if(option.getPutCall()=="Call"){
            if(var=="currentPrice" && derivOrder==1)
                greek = normalCDF(d1);
            else if(var=="currentPrice" && derivOrder==2)
                greek = normalPDF(d1)/(S0*sig*sqrt_T);
            else if(var=="volatility" && derivOrder==1)
                greek = S0*normalPDF(d1)*sqrt_T;
            else if(var=="riskFreeRate" && derivOrder==1)
                greek = K*T*exp(-r*T)*normalCDF(d2);
            else if(var=="time" && derivOrder==1)
                greek = -S0*normalPDF(d1)*sig/(2*sqrt_T)-r*K*exp(-r*T)*normalCDF(d2);
        }else if(option.getPutCall()=="Put"){
            if(var=="currentPrice" && derivOrder==1)
                greek = normalCDF(d1)-1;
            else if(var=="currentPrice" && derivOrder==2)
                greek = normalPDF(d1)/(S0*sig*sqrt_T);
            else if(var=="volatility" && derivOrder==1)
                greek = S0*normalPDF(d1)*sqrt_T;
            else if(var=="riskFreeRate" && derivOrder==1)
                greek = -K*T*exp(-r*T)*normalCDF(-d2);
            else if(var=="time" && derivOrder==1)
                greek = -S0*normalPDF(d1)*sig/(2*sqrt_T)+r*K*exp(-r*T)*normalCDF(-d2);
        }
    }else if(option.getType()=="Digital"){}
    resetOriginal();
    logMessage("ending calculation ClosedFormGreek, return "+to_string(greek));
    return greek;
}

double Pricer::FiniteDifferenceGreek(string var, int derivOrder, string method,
                                     const SimulationConfig& config, int numSim, double eps){
    logMessage("starting calculation FiniteDifferenceGreek on var "+var+", derivOrder "+to_string(derivOrder)+
               ", method "+method+", config "+to_string(config)+", numSim "+to_string(numSim)+", eps "+to_string(eps));
    saveAsOriginal();
    double greek = NAN;
    double v,dv,v_pos,v_neg,price_pos,price_neg;
    v = getVariable(var);
    dv = v*eps;
    v_pos = v+dv;
    v_neg = v-dv;
    price = calcPrice(method,config,numSim);
    setVariable(var,v_pos);
    price_pos = calcPrice(method,config,numSim);
    setVariable(var,v_neg);
    price_neg = calcPrice(method,config,numSim);
    switch(derivOrder){
        case 1: greek = (price_pos-price_neg)/(2*dv); break;
        case 2: greek = (price_pos-2*price+price_neg)/(dv*dv); break;
    }
    resetOriginal();
    logMessage("ending calculation FiniteDifferenceGreek, return "+to_string(greek));
    return greek;
}

double Pricer::calcGreek(string greekName, string greekMethod, string method,
                         const SimulationConfig& config, int numSim, double eps){
    double greek = NAN;
    string var; int derivOrder;
    if(greekName=="Delta"){
        var = "currentPrice";
        derivOrder = 1;
    }else if(greekName=="Gamma"){
        var = "currentPrice";
        derivOrder = 2;
    }else if(greekName=="Vega"){
        var = "volatility";
        derivOrder = 1;
    }else if(greekName=="Rho"){
        var = "riskFreeRate";
        derivOrder = 1;
    }else if(greekName=="Theta"){
        var = "time";
        derivOrder = 1;
    }
    if(greekMethod=="Closed Form")
        greek = ClosedFormGreek(var,derivOrder);
    else if(greekMethod=="Finite Difference")
        greek = FiniteDifferenceGreek(var,derivOrder,method,config,numSim,eps);
    return greek;
}

matrix Pricer::varyGreekWithVariable(string var, const matrix& varVector, string greekName,
                                     string greekMethod, string method, const SimulationConfig& config, int numSim, double eps){
    saveAsOriginal();
    int n = varVector.getCols();
    double greek;
    matrix optionGreekVector(1,n);
    for(int i=0; i<n; i++){
        double v = varVector.getEntry(0,i);
        setVariable(var,v);
        greek = calcGreek(greekName,greekMethod,method,config,numSim,eps);
        optionGreekVector.setEntry(0,i,greek);
    }
    resetOriginal();
    return optionGreekVector;
}

matrix Pricer::generatePriceSurface(const matrix& stockPriceVector, const matrix& optionTermVector,
                                    string method, const SimulationConfig& config, int numSim){
    saveAsOriginal();
    int m = optionTermVector.getCols();
    int n = stockPriceVector.getCols();
    matrix priceSurface(m,n);
    for(int i=0; i<m; i++){
        double term = optionTermVector.getEntry(0,i);
        setVariable("maturity",term);
        priceSurface.setRow(i,
                            varyPriceWithVariable("currentPrice",stockPriceVector,method,config,numSim)
                            );
    }
    resetOriginal();
    return priceSurface;
}

bool Pricer::satisfyPriceBounds(double optionMarketPrice){
    if(option.getType()=="European"){
        double K   = getVariable("strike");
        double T   = getVariable("maturity");
        double r   = getVariable("riskFreeRate");
        double S0  = getVariable("currentPrice");
        double q   = getVariable("dividendYield");
        if(option.getPutCall()=="Call")
            return (optionMarketPrice<S0*exp(-q*T)) &&
            (optionMarketPrice>max(S0*exp(-q*T)-K*exp(-r*T),0.));
        else if(option.getPutCall()=="Put")
            return (optionMarketPrice<K*exp(-r*T)) &&
            (optionMarketPrice>max(K*exp(-r*T)-S0*exp(-q*T),0.));
    }
    return false;
}

double Pricer::calcImpliedVolatility(double optionMarketPrice, double vol0, double eps){
    saveAsOriginal();
    double impliedVol = NAN;
    double impliedVol0, impliedVol1;
    double err = 1;
    if(option.getType()=="European"){
        impliedVol0 = 0;
        impliedVol1 = vol0;
        if(satisfyPriceBounds(optionMarketPrice)){
            while(err>eps){
                impliedVol = (impliedVol0+impliedVol1)/2;
                setVariable("volatility",impliedVol);
                price = calcPrice("Closed Form");
                if(price>optionMarketPrice) impliedVol1 = impliedVol;
                else if(price<optionMarketPrice) impliedVol0 = impliedVol;
                err = abs(price-optionMarketPrice)/optionMarketPrice;
                if(impliedVol==vol0) break;
            }
        }
    }
    resetOriginal();
    return impliedVol;
}

void Pricer::generateImpliedVolSurfaceFromFile(string input, string file, double vol0, double eps){
    saveAsOriginal();
    string name, type, putCall;
    string strikeStr, maturityStr, optionMarketPriceStr;
    double strike, maturity, optionMarketPrice;
    double impliedVol;
    ifstream fi(input);
    ofstream fo; fo.open(file);
    while(getline(fi,name,',')){
        getline(fi,type,',');
        getline(fi,putCall,',');
        getline(fi,strikeStr,',');
        getline(fi,maturityStr,',');
        getline(fi,optionMarketPriceStr);
        strike = stod(strikeStr);
        maturity = stod(maturityStr);
        optionMarketPrice = stod(optionMarketPriceStr);
        option = Option(type,putCall,strike,maturity,{},{},name);
        // cout << option << endl;
        impliedVol = calcImpliedVolatility(optionMarketPrice,vol0,eps);
        if(!isnan(impliedVol)) fo << name << "," << type << "," << putCall << "," <<
            strike << "," << maturity << "," << impliedVol << endl;
    }
    fi.close();
    fo.close();
    resetOriginal();
}

void Pricer::generateGreeksFromImpliedVolFile(string input, string file){
    saveAsOriginal();
    string name, type, putCall;
    string strikeStr, maturityStr, impliedVolStr;
    double strike, maturity, impliedVol;
    double Delta, Gamma, Vega, Rho, Theta;
    ifstream fi(input);
    ofstream fo; fo.open(file);
    while(getline(fi,name,',')){
        getline(fi,type,',');
        getline(fi,putCall,',');
        getline(fi,strikeStr,',');
        getline(fi,maturityStr,',');
        getline(fi,impliedVolStr);
        strike = stod(strikeStr);
        maturity = stod(maturityStr);
        impliedVol = stod(impliedVolStr);
        setVariable("volatility",impliedVol);
        option = Option(type,putCall,strike,maturity,{},{},name);
        // cout << option << endl;
        Delta = calcGreek("Delta");
        Gamma = calcGreek("Gamma");
        Vega = calcGreek("Vega");
        Rho = calcGreek("Rho");
        Theta = calcGreek("Theta");
        fo << name << "," << type << "," << putCall << "," <<
        strike << "," << maturity << "," << impliedVol << "," << Delta << "," <<
        Gamma << "," << Vega << "," << Rho << "," << Theta << endl;
    }
    fi.close();
    fo.close();
    resetOriginal();
}

vector<matrix> Pricer::modelImpliedVolSurface(const SimulationConfig& config, int numSpace,
                                              const function<double(double)>& impVolFunc0, const function<double(double)>& impVolFunc1,
                                              double lambdaT, double eps){
    double K = getVariable("strike");
    double T = config.endTime;
    int n = config.iters;
    int m = numSpace;
    double dt = config.stepSize;
    double dt2 = dt*dt;
    double x0 = log(K/3), x1 = log(3*K);
    double dx = (x1-x0)/m;
    double dx2 = dx*dx;
    matrix impVolSurface(n+1,m+1);
    matrix timeGrids; timeGrids.setRange(0,T,n,true);
    matrix spaceGrids; spaceGrids.setRange(x0,x1,m,true);
    matrix initCondition, termCondition, bdryCondition0, bdryCondition1;
    double sig0, sig1, expFactor = exp(-T/lambdaT);
    initCondition = spaceGrids.apply(impVolFunc0);
    termCondition = spaceGrids.apply(impVolFunc1);
    sig0 = initCondition.getFirstEntry();
    sig1 = termCondition.getFirstEntry();
    bdryCondition0 = timeGrids.apply([sig0,sig1,expFactor,lambdaT](double x){
        return mathFunc(x,"exponential",{
            (sig1-sig0*expFactor)/(1-expFactor),(sig0-sig1)/(1-expFactor),lambdaT
        });
    });
    sig0 = initCondition.getLastEntry();
    sig1 = termCondition.getLastEntry();
    bdryCondition1 = timeGrids.apply([sig0,sig1,expFactor,lambdaT](double x){
        return mathFunc(x,"exponential",{
            (sig1-sig0*expFactor)/(1-expFactor),(sig0-sig1)/(1-expFactor),lambdaT
        });
    });
    impVolSurface.setRow(0,initCondition);
    impVolSurface.setRow(n,termCondition);
    impVolSurface.setCol(0,bdryCondition0);
    impVolSurface.setCol(m,bdryCondition1);
    double err = 1;
    while(err>eps){
        matrix impVolSurfacePrev = impVolSurface;
        for(int i=1; i<n; i++)
            for(int j=1; j<m; j++){
                double avg =
                (impVolSurfacePrev.getEntry(i-1,j)+
                 impVolSurfacePrev.getEntry(i+1,j)+
                 impVolSurfacePrev.getEntry(i,j-1)+
                 impVolSurfacePrev.getEntry(i,j+1))/4;
                impVolSurface.setEntry(i,j,avg);
            }
        err = (impVolSurface-impVolSurfacePrev).sum()/impVolSurfacePrev.sum();
    }
    // cout << impVolSurface.print() << endl;
    vector<matrix> impVolSurfaceSet{
        spaceGrids,
        timeGrids,
        impVolSurface
    };
    return impVolSurfaceSet;
}

Backtest Pricer::runBacktest(const SimulationConfig& config, int numSim,
                             string strategy, int hedgeFreq, double mktImpVol, double mktPrice,
                             const vector<double>& stratParams, const vector<Option>& hOptions, const vector<matrix>& impVolSurfaceSet,
                             string simPriceMethod, const matrix& stockPriceSeries){
    if(GUI) cout << "running backtest for strategy: " << strategy << endl;
    Stock stock = market.getStock();
    //    double K = getVariable("strike");
    double T = getVariable("maturity");
    double r = getVariable("riskFreeRate");
    double S0 = getVariable("currentPrice");
    double q = getVariable("dividendYield");
    double sig = getVariable("volatility");
    double sig2 = sig*sig;
    double dt = config.stepSize;
    double riskFreeRateFactor = exp(r*dt);
    double dividendYieldFactor = exp(q*dt);
    double Delta, Gamma, Vega, Rho, Theta;
    bool flatImpVolSurface = impVolSurfaceSet.size()==0;
    if(simPriceMethod=="bootstrap")
        stock.bootstrapPrice(stockPriceSeries,config,numSim);
    else stock.simulatePrice(config,numSim);
    int n = config.iters;
    matrix
    stratCashMatrix(n+1,numSim),
    stratNStockMatrix(n+1,numSim),
    stratModPriceMatrix(n+1,numSim),
    stratModValueMatrix(n+1,numSim),
    stratGrkDelta(n+1,numSim),
    stratGrkGamma(n+1,numSim),
    stratGrkVega(n+1,numSim),
    stratGrkRho(n+1,numSim),
    stratGrkTheta(n+1,numSim);
    vector<matrix>
    stratNOptions,
    stratHModPrices;
    matrix simPriceMatrix = stock.getSimPriceMatrix();
    if(strategy=="simple-delta" || strategy=="mkt-delta"){
        if(strategy=="mkt-delta"){
            double sigImp;
            if(mktImpVol>0) sigImp = mktImpVol;
            else sigImp = calcImpliedVolatility(mktPrice);
            setVariable("volatility",sigImp);
        }
        for(int i=0; i<numSim; i++){
            setVariable("currentPrice",S0);
            setVariable("maturity",T);
            double modPrice = calcPrice("Closed Form");
            double nStock = calcGreek("Delta");
            double cash = modPrice;
            cash -= nStock*S0;
            double value = cash+nStock*S0-modPrice;
            stratCashMatrix.setEntry(0,i,cash);
            stratNStockMatrix.setEntry(0,i,nStock);
            stratModPriceMatrix.setEntry(0,i,modPrice);
            stratModValueMatrix.setEntry(0,i,value);
            Delta = calcGreek("Delta");
            Gamma = calcGreek("Gamma");
            Vega = calcGreek("Vega");
            Rho = calcGreek("Rho");
            Theta = calcGreek("Theta");
            stratGrkDelta.setEntry(0,i,nStock-Delta);
            stratGrkGamma.setEntry(0,i,-Gamma);
            stratGrkVega.setEntry(0,i,-Vega);
            stratGrkRho.setEntry(0,i,-Rho);
            stratGrkTheta.setEntry(0,i,r*cash+q*nStock*S0-Theta-sig2*S0*S0/2*Gamma);
            for(int t=1; t<n; t++){
                double S = simPriceMatrix.getEntry(t,i);
                double nStockPrev = nStock;
                setVariable("currentPrice",S);
                setVariable("maturity",T-t*dt);
                modPrice = calcPrice("Closed Form");
                if(t%hedgeFreq==0){
                    nStock = calcGreek("Delta");
                    cash = cash*riskFreeRateFactor
                    +nStock*S*(dividendYieldFactor-1)
                    -(nStock-nStockPrev)*S;
                }else{
                    cash = cash*riskFreeRateFactor
                    +nStockPrev*S*(dividendYieldFactor-1);
                }
                value = cash+nStock*S-modPrice;
                stratCashMatrix.setEntry(t,i,cash);
                stratNStockMatrix.setEntry(t,i,nStock);
                stratModPriceMatrix.setEntry(t,i,modPrice);
                stratModValueMatrix.setEntry(t,i,value);
                Delta = calcGreek("Delta");
                Gamma = calcGreek("Gamma");
                Vega = calcGreek("Vega");
                Rho = calcGreek("Rho");
                Theta = calcGreek("Theta");
                stratGrkDelta.setEntry(t,i,nStock-Delta);
                stratGrkGamma.setEntry(t,i,-Gamma);
                stratGrkVega.setEntry(t,i,-Vega);
                stratGrkRho.setEntry(t,i,-Rho);
                stratGrkTheta.setEntry(t,i,r*cash+q*nStock*S-Theta-sig2*S*S/2*Gamma);
            }
            double S1 = simPriceMatrix.getEntry(n,i);
            double nStockPrev = nStock;
            setVariable("currentPrice",S1);
            setVariable("maturity",0);
            modPrice = option.calcPayoff(S1);
            nStock = 0;
            cash = cash*riskFreeRateFactor+nStockPrev*S1;
            value = cash-modPrice;
            stratCashMatrix.setEntry(n,i,cash);
            stratNStockMatrix.setEntry(n,i,nStock);
            stratModPriceMatrix.setEntry(n,i,modPrice);
            stratModValueMatrix.setEntry(n,i,value);
            stratGrkDelta.setEntry(n,i,0);
            stratGrkGamma.setEntry(n,i,0);
            stratGrkVega.setEntry(n,i,0);
            stratGrkRho.setEntry(n,i,0);
            stratGrkTheta.setEntry(n,i,r*cash);
            if(GUI) cout << "[" << getCurrentTime() << "] simulation " <<
                i+1 << "/" << numSim << " completes" << endl;
        }
        // cout << stratModValueMatrix.print() << endl;
    }else if(strategy=="mkt-delta-hedgingVol"){
        double hedgingVol = stratParams[0];
        Pricer hPricer(*this);
        hPricer.setVariable("volatility",hedgingVol);
        double sigImp;
        if(mktImpVol>0) sigImp = mktImpVol;
        else sigImp = calcImpliedVolatility(mktPrice);
        setVariable("volatility",sigImp);
        for(int i=0; i<numSim; i++){
            setVariable("currentPrice",S0);
            setVariable("maturity",T);
            hPricer.setVariable("currentPrice",S0);
            hPricer.setVariable("maturity",T);
            double modPrice = calcPrice("Closed Form");
            double nStock = hPricer.calcGreek("Delta");
            double cash = modPrice;
            cash -= nStock*S0;
            double value = cash+nStock*S0-modPrice;
            stratCashMatrix.setEntry(0,i,cash);
            stratNStockMatrix.setEntry(0,i,nStock);
            stratModPriceMatrix.setEntry(0,i,modPrice);
            stratModValueMatrix.setEntry(0,i,value);
            Delta = calcGreek("Delta");
            Gamma = calcGreek("Gamma");
            Vega = calcGreek("Vega");
            Rho = calcGreek("Rho");
            Theta = calcGreek("Theta");
            stratGrkDelta.setEntry(0,i,nStock-Delta);
            stratGrkGamma.setEntry(0,i,-Gamma);
            stratGrkVega.setEntry(0,i,-Vega);
            stratGrkRho.setEntry(0,i,-Rho);
            stratGrkTheta.setEntry(0,i,r*cash+q*nStock*S0-Theta-sig2*S0*S0/2*Gamma);
            for(int t=1; t<n; t++){
                double S = simPriceMatrix.getEntry(t,i);
                double nStockPrev = nStock;
                setVariable("currentPrice",S);
                setVariable("maturity",T-t*dt);
                hPricer.setVariable("currentPrice",S);
                hPricer.setVariable("maturity",T-t*dt);
                modPrice = calcPrice("Closed Form");
                if(t%hedgeFreq==0){
                    nStock = hPricer.calcGreek("Delta");
                    cash = cash*riskFreeRateFactor
                    +nStock*S*(dividendYieldFactor-1)
                    -(nStock-nStockPrev)*S;
                }else{
                    cash = cash*riskFreeRateFactor
                    +nStockPrev*S*(dividendYieldFactor-1);
                }
                value = cash+nStock*S-modPrice;
                stratCashMatrix.setEntry(t,i,cash);
                stratNStockMatrix.setEntry(t,i,nStock);
                stratModPriceMatrix.setEntry(t,i,modPrice);
                stratModValueMatrix.setEntry(t,i,value);
                Delta = calcGreek("Delta");
                Gamma = calcGreek("Gamma");
                Vega = calcGreek("Vega");
                Rho = calcGreek("Rho");
                Theta = calcGreek("Theta");
                stratGrkDelta.setEntry(t,i,nStock-Delta);
                stratGrkGamma.setEntry(t,i,-Gamma);
                stratGrkVega.setEntry(t,i,-Vega);
                stratGrkRho.setEntry(t,i,-Rho);
                stratGrkTheta.setEntry(t,i,r*cash+q*nStock*S-Theta-sig2*S*S/2*Gamma);
            }
            double S1 = simPriceMatrix.getEntry(n,i);
            double nStockPrev = nStock;
            setVariable("currentPrice",S1);
            setVariable("maturity",0);
            hPricer.setVariable("currentPrice",S1);
            hPricer.setVariable("maturity",0);
            modPrice = option.calcPayoff(S1);
            nStock = 0;
            cash = cash*riskFreeRateFactor+nStockPrev*S1;
            value = cash-modPrice;
            stratCashMatrix.setEntry(n,i,cash);
            stratNStockMatrix.setEntry(n,i,nStock);
            stratModPriceMatrix.setEntry(n,i,modPrice);
            stratModValueMatrix.setEntry(n,i,value);
            stratGrkDelta.setEntry(n,i,0);
            stratGrkGamma.setEntry(n,i,0);
            stratGrkVega.setEntry(n,i,0);
            stratGrkRho.setEntry(n,i,0);
            stratGrkTheta.setEntry(n,i,r*cash);
            if(GUI) cout << "[" << getCurrentTime() << "] simulation " <<
                i+1 << "/" << numSim << " completes" << endl;
        }
    }else if(strategy=="simple-delta-gamma" || strategy=="mkt-delta-gamma"){
        double hDelta, hGamma, hVega, hRho, hTheta;
        stratNOptions.push_back(matrix(n+1,numSim));
        stratHModPrices.push_back(matrix(n+1,numSim));
        Option hOption = hOptions[0];
        Pricer hPricer(hOption,market);
        double Th = hPricer.getVariable("maturity");
        double O0 = hPricer.calcPrice("Closed Form");
        if(strategy=="mkt-delta-gamma"){
            double sigImp;
            if(mktImpVol>0) sigImp = mktImpVol;
            else sigImp = calcImpliedVolatility(mktPrice);
            setVariable("volatility",sigImp);
            hPricer.setVariable("volatility",sigImp);
        }
        for(int i=0; i<numSim; i++){
            setVariable("currentPrice",S0);
            setVariable("maturity",T);
            hPricer.setVariable("currentPrice",S0);
            hPricer.setVariable("maturity",Th);
            double modPrice = calcPrice("Closed Form");
            double nOption = calcGreek("Gamma")
            /hPricer.calcGreek("Gamma");
            double nStock = calcGreek("Delta")
            -nOption*hPricer.calcGreek("Delta");
            double cash = modPrice;
            cash -= nStock*S0+nOption*O0;
            double value = cash+nStock*S0+nOption*O0-modPrice;
            stratCashMatrix.setEntry(0,i,cash);
            stratNStockMatrix.setEntry(0,i,nStock);
            stratModPriceMatrix.setEntry(0,i,modPrice);
            stratModValueMatrix.setEntry(0,i,value);
            Delta = calcGreek("Delta"); hDelta = hPricer.calcGreek("Delta");
            Gamma = calcGreek("Gamma"); hGamma = hPricer.calcGreek("Gamma");
            Vega = calcGreek("Vega"); hVega = hPricer.calcGreek("Vega");
            Rho = calcGreek("Rho"); hRho = hPricer.calcGreek("Rho");
            Theta = calcGreek("Theta"); hTheta = hPricer.calcGreek("Theta");
            stratGrkDelta.setEntry(0,i,nStock+nOption*hDelta-Delta);
            stratGrkGamma.setEntry(0,i,nOption*hGamma-Gamma);
            stratGrkVega.setEntry(0,i,nOption*hVega-Vega);
            stratGrkRho.setEntry(0,i,nOption*hRho-Rho);
            stratGrkTheta.setEntry(0,i,r*cash+q*nStock*S0+nOption*hTheta-Theta);
            stratNOptions[0].setEntry(0,i,nOption);
            stratHModPrices[0].setEntry(0,i,O0);
            for(int t=1; t<n; t++){
                double S = simPriceMatrix.getEntry(t,i);
                double nStockPrev = nStock;
                double nOptionPrev = nOption;
                setVariable("currentPrice",S);
                setVariable("maturity",T-t*dt);
                hPricer.setVariable("currentPrice",S);
                hPricer.setVariable("maturity",Th-t*dt);
                double O = hPricer.calcPrice("Closed Form");
                modPrice = calcPrice("Closed Form");
                if(t%hedgeFreq==0){
                    nOption = calcGreek("Gamma")
                    /hPricer.calcGreek("Gamma");
                    nStock = calcGreek("Delta")
                    -nOption*hPricer.calcGreek("Delta");
                    cash = cash*riskFreeRateFactor
                    +nStock*S*(dividendYieldFactor-1)
                    -(nStock-nStockPrev)*S
                    -(nOption-nOptionPrev)*O;
                }else{
                    cash = cash*riskFreeRateFactor
                    +nStockPrev*S*(dividendYieldFactor-1);
                }
                value = cash+nStock*S+nOption*O-modPrice;
                stratCashMatrix.setEntry(t,i,cash);
                stratNStockMatrix.setEntry(t,i,nStock);
                stratModPriceMatrix.setEntry(t,i,modPrice);
                stratModValueMatrix.setEntry(t,i,value);
                Delta = calcGreek("Delta"); hDelta = hPricer.calcGreek("Delta");
                Gamma = calcGreek("Gamma"); hGamma = hPricer.calcGreek("Gamma");
                Vega = calcGreek("Vega"); hVega = hPricer.calcGreek("Vega");
                Rho = calcGreek("Rho"); hRho = hPricer.calcGreek("Rho");
                Theta = calcGreek("Theta"); hTheta = hPricer.calcGreek("Theta");
                stratGrkDelta.setEntry(t,i,nStock+nOption*hDelta-Delta);
                stratGrkGamma.setEntry(t,i,nOption*hGamma-Gamma);
                stratGrkVega.setEntry(t,i,nOption*hVega-Vega);
                stratGrkRho.setEntry(t,i,nOption*hRho-Rho);
                stratGrkTheta.setEntry(t,i,r*cash+q*nStock*S+nOption*hTheta-Theta);
                stratNOptions[0].setEntry(t,i,nOption);
                stratHModPrices[0].setEntry(t,i,O);
            }
            double S1 = simPriceMatrix.getEntry(n,i);
            double nStockPrev = nStock;
            double nOptionPrev = nOption;
            setVariable("currentPrice",S1);
            setVariable("maturity",0);
            hPricer.setVariable("currentPrice",S1);
            hPricer.setVariable("maturity",Th-n*dt);
            double O1 = hPricer.calcPrice("Closed Form");
            modPrice = option.calcPayoff(S1);
            nStock = 0;
            nOption = 0;
            cash = cash*riskFreeRateFactor+nStockPrev*S1+nOptionPrev*O1;
            value = cash-modPrice;
            stratCashMatrix.setEntry(n,i,cash);
            stratNStockMatrix.setEntry(n,i,nStock);
            stratModPriceMatrix.setEntry(n,i,modPrice);
            stratModValueMatrix.setEntry(n,i,value);
            stratGrkDelta.setEntry(n,i,0);
            stratGrkGamma.setEntry(n,i,0);
            stratGrkVega.setEntry(n,i,0);
            stratGrkRho.setEntry(n,i,0);
            stratGrkTheta.setEntry(n,i,r*cash);
            stratNOptions[0].setEntry(n,i,nOption);
            stratHModPrices[0].setEntry(n,i,O1);
            if(GUI) cout << "[" << getCurrentTime() << "] simulation " <<
                i+1 << "/" << numSim << " completes" << endl;
        }
    }else if(strategy=="simple-delta-gamma-theta" || strategy=="mkt-delta-gamma-theta"){
        double hDelta0, hGamma0, hVega0, hRho0, hTheta0;
        double hDelta1, hGamma1, hVega1, hRho1, hTheta1;
        for(int i=0; i<2; i++){
            stratNOptions.push_back(matrix(n+1,numSim));
            stratHModPrices.push_back(matrix(n+1,numSim));
        }
        Option hOption0 = hOptions[0], hOption1 = hOptions[1];
        Pricer hPricer0(hOption0,market), hPricer1(hOption1,market);
        double Th0 = hPricer0.getVariable("maturity");
        double Th1 = hPricer1.getVariable("maturity");
        double O00 = hPricer0.calcPrice("Closed Form");
        double O10 = hPricer1.calcPrice("Closed Form");
        if(strategy=="mkt-delta-gamma-theta"){
            double sigImp;
            if(mktImpVol>0) sigImp = mktImpVol;
            else sigImp = calcImpliedVolatility(mktPrice);
            setVariable("volatility",sigImp);
            hPricer0.setVariable("volatility",sigImp);
            hPricer1.setVariable("volatility",sigImp);
        }
        for(int i=0; i<numSim; i++){
            setVariable("currentPrice",S0);
            setVariable("maturity",T);
            hPricer0.setVariable("currentPrice",S0);
            hPricer0.setVariable("maturity",Th0);
            hPricer1.setVariable("currentPrice",S0);
            hPricer1.setVariable("maturity",Th1);
            double modPrice = calcPrice("Closed Form");
            double tmpM[2][2]
            = {{hPricer0.calcGreek("Theta"),hPricer1.calcGreek("Theta")},
                hPricer0.calcGreek("Gamma"),hPricer1.calcGreek("Gamma")};
            double tmpV[2] = {calcGreek("Theta"),calcGreek("Gamma")};
            matrix nOptions = matrix(tmpM).inverse().dot(matrix(tmpV).T());
            double nOption0 = nOptions.getEntry(0,0),
            nOption1 = nOptions.getEntry(1,0);
            double nStock = calcGreek("Delta")
            -nOption0*hPricer0.calcGreek("Delta")
            -nOption1*hPricer1.calcGreek("Delta");
            double cash = modPrice;
            cash -= nStock*S0+nOption0*O00+nOption1*O10;
            double value = cash+nStock*S0+nOption0*O00+nOption1*O10-modPrice;
            stratCashMatrix.setEntry(0,i,cash);
            stratNStockMatrix.setEntry(0,i,nStock);
            stratModPriceMatrix.setEntry(0,i,modPrice);
            stratModValueMatrix.setEntry(0,i,value);
            Delta = calcGreek("Delta"); hDelta0 = hPricer0.calcGreek("Delta"); hDelta1 = hPricer1.calcGreek("Delta");
            Gamma = calcGreek("Gamma"); hGamma0 = hPricer0.calcGreek("Gamma"); hGamma1 = hPricer1.calcGreek("Gamma");
            Vega = calcGreek("Vega"); hVega0 = hPricer0.calcGreek("Vega"); hVega1 = hPricer1.calcGreek("Vega");
            Rho = calcGreek("Rho"); hRho0 = hPricer0.calcGreek("Rho"); hRho1 = hPricer1.calcGreek("Rho");
            Theta = calcGreek("Theta"); hTheta0 = hPricer0.calcGreek("Theta"); hTheta1 = hPricer1.calcGreek("Theta");
            stratGrkDelta.setEntry(0,i,nStock+nOption0*hDelta0+nOption1*hDelta1-Delta);
            stratGrkGamma.setEntry(0,i,nOption0*hGamma0+nOption1*hGamma1-Gamma);
            stratGrkVega.setEntry(0,i,nOption0*hVega0+nOption1*hVega1-Vega);
            stratGrkRho.setEntry(0,i,nOption0*hRho0+nOption1*hRho1-Rho);
            stratGrkTheta.setEntry(0,i,r*cash+q*nStock*S0);
            stratNOptions[0].setEntry(0,i,nOption0);
            stratNOptions[1].setEntry(0,i,nOption1);
            stratHModPrices[0].setEntry(0,i,O00);
            stratHModPrices[1].setEntry(0,i,O10);
            for(int t=1; t<n; t++){
                double S = simPriceMatrix.getEntry(t,i);
                double nStockPrev = nStock;
                double nOptionPrev0 = nOption0;
                double nOptionPrev1 = nOption1;
                setVariable("currentPrice",S);
                setVariable("maturity",T-t*dt);
                hPricer0.setVariable("currentPrice",S);
                hPricer0.setVariable("maturity",Th0-t*dt);
                hPricer1.setVariable("currentPrice",S);
                hPricer1.setVariable("maturity",Th1-t*dt);
                double O0 = hPricer0.calcPrice("Closed Form");
                double O1 = hPricer1.calcPrice("Closed Form");
                modPrice = calcPrice("Closed Form");
                if(t%hedgeFreq==0){
                    double tmpM[2][2]
                    = {{hPricer0.calcGreek("Theta"),hPricer1.calcGreek("Theta")},
                        hPricer0.calcGreek("Gamma"),hPricer1.calcGreek("Gamma")};
                    double tmpV[2] = {calcGreek("Theta"),calcGreek("Gamma")};
                    nOptions = matrix(tmpM).inverse().dot(matrix(tmpV).T());
                    nOption0 = nOptions.getEntry(0,0),
                    nOption1 = nOptions.getEntry(1,0);
                    nStock = calcGreek("Delta")
                    -nOption0*hPricer0.calcGreek("Delta")
                    -nOption1*hPricer1.calcGreek("Delta");
                    cash = cash*riskFreeRateFactor
                    +nStock*S*(dividendYieldFactor-1)
                    -(nStock-nStockPrev)*S
                    -(nOption0-nOptionPrev0)*O0
                    -(nOption1-nOptionPrev1)*O1;
                }else{
                    cash = cash*riskFreeRateFactor
                    +nStockPrev*S*(dividendYieldFactor-1);
                }
                value = cash+nStock*S+nOption0*O0+nOption1*O1-modPrice;
                stratCashMatrix.setEntry(t,i,cash);
                stratNStockMatrix.setEntry(t,i,nStock);
                stratModPriceMatrix.setEntry(t,i,modPrice);
                stratModValueMatrix.setEntry(t,i,value);
                Delta = calcGreek("Delta"); hDelta0 = hPricer0.calcGreek("Delta"); hDelta1 = hPricer1.calcGreek("Delta");
                Gamma = calcGreek("Gamma"); hGamma0 = hPricer0.calcGreek("Gamma"); hGamma1 = hPricer1.calcGreek("Gamma");
                Vega = calcGreek("Vega"); hVega0 = hPricer0.calcGreek("Vega"); hVega1 = hPricer1.calcGreek("Vega");
                Rho = calcGreek("Rho"); hRho0 = hPricer0.calcGreek("Rho"); hRho1 = hPricer1.calcGreek("Rho");
                Theta = calcGreek("Theta"); hTheta0 = hPricer0.calcGreek("Theta"); hTheta1 = hPricer1.calcGreek("Theta");
                stratGrkDelta.setEntry(t,i,nStock+nOption0*hDelta0+nOption1*hDelta1-Delta);
                stratGrkGamma.setEntry(t,i,nOption0*hGamma0+nOption1*hGamma1-Gamma);
                stratGrkVega.setEntry(t,i,nOption0*hVega0+nOption1*hVega1-Vega);
                stratGrkRho.setEntry(t,i,nOption0*hRho0+nOption1*hRho1-Rho);
                stratGrkTheta.setEntry(t,i,r*cash+q*nStock*S);
                stratNOptions[0].setEntry(t,i,nOption0);
                stratNOptions[1].setEntry(t,i,nOption1);
                stratHModPrices[0].setEntry(t,i,O0);
                stratHModPrices[1].setEntry(t,i,O1);
            }
            double S1 = simPriceMatrix.getEntry(n,i);
            double nStockPrev = nStock;
            double nOptionPrev0 = nOption0;
            double nOptionPrev1 = nOption1;
            setVariable("currentPrice",S1);
            setVariable("maturity",0);
            hPricer0.setVariable("currentPrice",S1);
            hPricer0.setVariable("maturity",Th0-n*dt);
            hPricer1.setVariable("currentPrice",S1);
            hPricer1.setVariable("maturity",Th1-n*dt);
            double O01 = hPricer0.calcPrice("Closed Form");
            double O11 = hPricer1.calcPrice("Closed Form");
            modPrice = option.calcPayoff(S1);
            nStock = 0;
            nOption0 = 0;
            nOption1 = 0;
            cash = cash*riskFreeRateFactor+nStockPrev*S1+nOptionPrev0*O01+nOptionPrev1*O11;
            value = cash-modPrice;
            stratCashMatrix.setEntry(n,i,cash);
            stratNStockMatrix.setEntry(n,i,nStock);
            stratModPriceMatrix.setEntry(n,i,modPrice);
            stratModValueMatrix.setEntry(n,i,value);
            stratGrkDelta.setEntry(n,i,0);
            stratGrkGamma.setEntry(n,i,0);
            stratGrkVega.setEntry(n,i,0);
            stratGrkRho.setEntry(n,i,0);
            stratGrkTheta.setEntry(n,i,r*cash);
            stratNOptions[0].setEntry(n,i,nOption0);
            stratNOptions[1].setEntry(n,i,nOption1);
            stratHModPrices[0].setEntry(n,i,O01);
            stratHModPrices[1].setEntry(n,i,O11);
            if(GUI) cout << "[" << getCurrentTime() << "] simulation " <<
                i+1 << "/" << numSim << " completes" << endl;
        }
    }else if(strategy=="vol-delta-gamma-theta"){
        double hDelta0, hGamma0, hVega0, hRho0, hTheta0;
        double hDelta1, hGamma1, hVega1, hRho1, hTheta1;
        for(int i=0; i<2; i++){
            stratNOptions.push_back(matrix(n+1,numSim));
            stratHModPrices.push_back(matrix(n+1,numSim));
        }
        Option hOption0 = hOptions[0], hOption1 = hOptions[1];
        Pricer hPricer0(hOption0,market), hPricer1(hOption1,market);
        double Th0 = hPricer0.getVariable("maturity");
        double Th1 = hPricer1.getVariable("maturity");
        double O00 = hPricer0.calcPrice("Closed Form");
        double O10 = hPricer1.calcPrice("Closed Form");
        int idxK, idxK0, idxK1, idxT, idxT0, idxT1;
        if(!flatImpVolSurface){
            idxK = impVolSurfaceSet[0].find(log(getVariable("strike")),"closest")[1];
            idxK0 = impVolSurfaceSet[0].find(log(hPricer0.getVariable("strike")),"closest")[1];
            idxK1 = impVolSurfaceSet[0].find(log(hPricer1.getVariable("strike")),"closest")[1];
        }
        for(int i=0; i<numSim; i++){
            setVariable("currentPrice",S0);
            setVariable("maturity",T);
            hPricer0.setVariable("currentPrice",S0);
            hPricer0.setVariable("maturity",Th0);
            hPricer1.setVariable("currentPrice",S0);
            hPricer1.setVariable("maturity",Th1);
            if(!flatImpVolSurface){
                idxT = impVolSurfaceSet[1].find(log(getVariable("maturity")),"closest")[1];
                idxT0 = impVolSurfaceSet[1].find(log(hPricer0.getVariable("maturity")),"closest")[1];
                idxT1 = impVolSurfaceSet[1].find(log(hPricer1.getVariable("maturity")),"closest")[1];
                setVariable("volatility",impVolSurfaceSet[2].getEntry(idxT,idxK));
                hPricer0.setVariable("volatility",impVolSurfaceSet[2].getEntry(idxT0,idxK0));
                hPricer1.setVariable("volatility",impVolSurfaceSet[2].getEntry(idxT1,idxK1));
            }
            double modPrice = calcPrice("Closed Form");
            double tmpM[2][2]
            = {{hPricer0.calcGreek("Theta"),hPricer1.calcGreek("Theta")},
                hPricer0.calcGreek("Gamma"),hPricer1.calcGreek("Gamma")};
            double tmpV[2] = {calcGreek("Theta"),calcGreek("Gamma")};
            matrix nOptions = matrix(tmpM).inverse().dot(matrix(tmpV).T());
            double nOption0 = nOptions.getEntry(0,0),
            nOption1 = nOptions.getEntry(1,0);
            double nStock = calcGreek("Delta")
            -nOption0*hPricer0.calcGreek("Delta")
            -nOption1*hPricer1.calcGreek("Delta");
            double cash = modPrice;
            cash -= nStock*S0+nOption0*O00+nOption1*O10;
            double value = cash+nStock*S0+nOption0*O00+nOption1*O10-modPrice;
            stratCashMatrix.setEntry(0,i,cash);
            stratNStockMatrix.setEntry(0,i,nStock);
            stratModPriceMatrix.setEntry(0,i,modPrice);
            stratModValueMatrix.setEntry(0,i,value);
            Delta = calcGreek("Delta"); hDelta0 = hPricer0.calcGreek("Delta"); hDelta1 = hPricer1.calcGreek("Delta");
            Gamma = calcGreek("Gamma"); hGamma0 = hPricer0.calcGreek("Gamma"); hGamma1 = hPricer1.calcGreek("Gamma");
            Vega = calcGreek("Vega"); hVega0 = hPricer0.calcGreek("Vega"); hVega1 = hPricer1.calcGreek("Vega");
            Rho = calcGreek("Rho"); hRho0 = hPricer0.calcGreek("Rho"); hRho1 = hPricer1.calcGreek("Rho");
            Theta = calcGreek("Theta"); hTheta0 = hPricer0.calcGreek("Theta"); hTheta1 = hPricer1.calcGreek("Theta");
            stratGrkDelta.setEntry(0,i,nStock+nOption0*hDelta0+nOption1*hDelta1-Delta);
            stratGrkGamma.setEntry(0,i,nOption0*hGamma0+nOption1*hGamma1-Gamma);
            stratGrkVega.setEntry(0,i,nOption0*hVega0+nOption1*hVega1-Vega);
            stratGrkRho.setEntry(0,i,nOption0*hRho0+nOption1*hRho1-Rho);
            stratGrkTheta.setEntry(0,i,r*cash+q*nStock*S0);
            stratNOptions[0].setEntry(0,i,nOption0);
            stratNOptions[1].setEntry(0,i,nOption1);
            stratHModPrices[0].setEntry(0,i,O00);
            stratHModPrices[1].setEntry(0,i,O10);
            for(int t=1; t<n; t++){
                double S = simPriceMatrix.getEntry(t,i);
                double nStockPrev = nStock;
                double nOptionPrev0 = nOption0;
                double nOptionPrev1 = nOption1;
                setVariable("currentPrice",S);
                setVariable("maturity",T-t*dt);
                hPricer0.setVariable("currentPrice",S);
                hPricer0.setVariable("maturity",Th0-t*dt);
                hPricer1.setVariable("currentPrice",S);
                hPricer1.setVariable("maturity",Th1-t*dt);
                if(!flatImpVolSurface){
                    idxT = impVolSurfaceSet[1].find(log(getVariable("maturity")),"closest")[1];
                    idxT0 = impVolSurfaceSet[1].find(log(hPricer0.getVariable("maturity")),"closest")[1];
                    idxT1 = impVolSurfaceSet[1].find(log(hPricer1.getVariable("maturity")),"closest")[1];
                    setVariable("volatility",impVolSurfaceSet[2].getEntry(idxT,idxK));
                    hPricer0.setVariable("volatility",impVolSurfaceSet[2].getEntry(idxT0,idxK0));
                    hPricer1.setVariable("volatility",impVolSurfaceSet[2].getEntry(idxT1,idxK1));
                }
                double O0 = hPricer0.calcPrice("Closed Form");
                double O1 = hPricer1.calcPrice("Closed Form");
                modPrice = calcPrice("Closed Form");
                if(t%hedgeFreq==0){
                    double tmpM[2][2]
                    = {{hPricer0.calcGreek("Theta"),hPricer1.calcGreek("Theta")},
                        hPricer0.calcGreek("Gamma"),hPricer1.calcGreek("Gamma")};
                    double tmpV[2] = {calcGreek("Theta"),calcGreek("Gamma")};
                    nOptions = matrix(tmpM).inverse().dot(matrix(tmpV).T());
                    nOption0 = nOptions.getEntry(0,0),
                    nOption1 = nOptions.getEntry(1,0);
                    nStock = calcGreek("Delta")
                    -nOption0*hPricer0.calcGreek("Delta")
                    -nOption1*hPricer1.calcGreek("Delta");
                    cash = cash*riskFreeRateFactor
                    +nStock*S*(dividendYieldFactor-1)
                    -(nStock-nStockPrev)*S
                    -(nOption0-nOptionPrev0)*O0
                    -(nOption1-nOptionPrev1)*O1;
                }else{
                    cash = cash*riskFreeRateFactor
                    +nStockPrev*S*(dividendYieldFactor-1);
                }
                value = cash+nStock*S+nOption0*O0+nOption1*O1-modPrice;
                stratCashMatrix.setEntry(t,i,cash);
                stratNStockMatrix.setEntry(t,i,nStock);
                stratModPriceMatrix.setEntry(t,i,modPrice);
                stratModValueMatrix.setEntry(t,i,value);
                Delta = calcGreek("Delta"); hDelta0 = hPricer0.calcGreek("Delta"); hDelta1 = hPricer1.calcGreek("Delta");
                Gamma = calcGreek("Gamma"); hGamma0 = hPricer0.calcGreek("Gamma"); hGamma1 = hPricer1.calcGreek("Gamma");
                Vega = calcGreek("Vega"); hVega0 = hPricer0.calcGreek("Vega"); hVega1 = hPricer1.calcGreek("Vega");
                Rho = calcGreek("Rho"); hRho0 = hPricer0.calcGreek("Rho"); hRho1 = hPricer1.calcGreek("Rho");
                Theta = calcGreek("Theta"); hTheta0 = hPricer0.calcGreek("Theta"); hTheta1 = hPricer1.calcGreek("Theta");
                stratGrkDelta.setEntry(t,i,nStock+nOption0*hDelta0+nOption1*hDelta1-Delta);
                stratGrkGamma.setEntry(t,i,nOption0*hGamma0+nOption1*hGamma1-Gamma);
                stratGrkVega.setEntry(t,i,nOption0*hVega0+nOption1*hVega1-Vega);
                stratGrkRho.setEntry(t,i,nOption0*hRho0+nOption1*hRho1-Rho);
                stratGrkTheta.setEntry(t,i,r*cash+q*nStock*S);
                stratNOptions[0].setEntry(t,i,nOption0);
                stratNOptions[1].setEntry(t,i,nOption1);
                stratHModPrices[0].setEntry(t,i,O0);
                stratHModPrices[1].setEntry(t,i,O1);
            }
            double S1 = simPriceMatrix.getEntry(n,i);
            double nStockPrev = nStock;
            double nOptionPrev0 = nOption0;
            double nOptionPrev1 = nOption1;
            setVariable("currentPrice",S1);
            setVariable("maturity",0);
            hPricer0.setVariable("currentPrice",S1);
            hPricer0.setVariable("maturity",Th0-n*dt);
            hPricer1.setVariable("currentPrice",S1);
            hPricer1.setVariable("maturity",Th1-n*dt);
            if(!flatImpVolSurface){
                idxT = impVolSurfaceSet[1].find(log(getVariable("maturity")),"closest")[1];
                idxT0 = impVolSurfaceSet[1].find(log(hPricer0.getVariable("maturity")),"closest")[1];
                idxT1 = impVolSurfaceSet[1].find(log(hPricer1.getVariable("maturity")),"closest")[1];
                setVariable("volatility",impVolSurfaceSet[2].getEntry(idxT,idxK));
                hPricer0.setVariable("volatility",impVolSurfaceSet[2].getEntry(idxT0,idxK0));
                hPricer1.setVariable("volatility",impVolSurfaceSet[2].getEntry(idxT1,idxK1));
            }
            double O01 = hPricer0.calcPrice("Closed Form");
            double O11 = hPricer1.calcPrice("Closed Form");
            modPrice = option.calcPayoff(S1);
            nStock = 0;
            nOption0 = 0;
            nOption1 = 0;
            cash = cash*riskFreeRateFactor+nStockPrev*S1+nOptionPrev0*O01+nOptionPrev1*O11;
            value = cash-modPrice;
            stratCashMatrix.setEntry(n,i,cash);
            stratNStockMatrix.setEntry(n,i,nStock);
            stratModPriceMatrix.setEntry(n,i,modPrice);
            stratModValueMatrix.setEntry(n,i,value);
            stratGrkDelta.setEntry(n,i,0);
            stratGrkGamma.setEntry(n,i,0);
            stratGrkVega.setEntry(n,i,0);
            stratGrkRho.setEntry(n,i,0);
            stratGrkTheta.setEntry(n,i,r*cash);
            stratNOptions[0].setEntry(n,i,nOption0);
            stratNOptions[1].setEntry(n,i,nOption1);
            stratHModPrices[0].setEntry(n,i,O01);
            stratHModPrices[1].setEntry(n,i,O11);
            if(GUI) cout << "[" << getCurrentTime() << "] simulation " <<
                i+1 << "/" << numSim << " completes" << endl;
        }
    }
    vector<matrix> results{
        simPriceMatrix,
        stratCashMatrix,
        stratNStockMatrix,
        stratModPriceMatrix,
        stratModValueMatrix,
        stratGrkDelta,
        stratGrkGamma,
        stratGrkVega,
        stratGrkRho,
        stratGrkTheta
    };
    vector<vector<matrix>> hResults{
        stratNOptions,
        stratHModPrices
    };
    Backtest backtest(results,hResults);
    return backtest;
    
}
