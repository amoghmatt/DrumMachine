//
//  filter.h
//  DrumMachine
//
//  Created by Amogh Matt on 09/03/2018.
//  Copyright © 2018 Amogh Matt. All rights reserved.
//

#ifndef filter_h
#define filter_h

#include <vector>

class filter
{
public:
    filter() {};
    ~filter() {};
    
    void resetFilter();
    void getCoefficients(double cutoff, double fs);
    void setCoefficients(const std::vector<double>& b, const std::vector<double>& a);
    float processFilter(float x);
    
    const std::vector<double>& getA() {return a; }
    const std::vector<double>& getB() {return b; }
    
private:
    std::vector<double> a = {1.0, 0.0, 0.0}; // Setting Coefficients for Low Pass as Default
    std::vector<double> b = {1.0, 0.0, 0.0};
    
    double xLast {0.0};
    double xSecondLast {0.0};
    double yLast {0.0};
    double ySecondLast {0.0};
    
};

#endif /* filter_h */

