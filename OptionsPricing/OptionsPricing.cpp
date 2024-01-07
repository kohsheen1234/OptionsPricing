//
//  OptionsPricing.cpp
//  OptionsPricing
//
//  Created by Kohsheen Tiku on 1/7/24.
//

#include <iostream>
#include "OptionsPricing.hpp"
#include "OptionsPricingPriv.hpp"

void OptionsPricing::HelloWorld(const char * s)
{
    OptionsPricingPriv *theObj = new OptionsPricingPriv;
    theObj->HelloWorldPriv(s);
    delete theObj;
};

void OptionsPricingPriv::HelloWorldPriv(const char * s) 
{
    std::cout << s << std::endl;
};

