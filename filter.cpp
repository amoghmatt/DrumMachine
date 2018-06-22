//  Created by Amogh Matt on 09/03/2018.
//  Copyright © 2018 Amogh Matt. All rights reserved.
//

#include "filter.h"
#include "stdio.h"
#include "math.h"

void filter::resetFilter()
{
    // Clear the delayed input values
    xLast = 0;
    xSecondLast = 0;
    
    // Clear the delayed output values
    yLast = 0;
    ySecondLast = 0;
}

// Method to calculate the coefficients of the Low Pass Filter
void filter::getCoefficients(double cutoff, double fs)
{
	float wc = 2 * M_PI * cutoff; // cutoff frequency in radians
	float Q = sqrt(2) / 2; // Q of filter

	float wca = tanf(wc * 0.5 / fs); // warped analogue frequency

	// set coeffiecients for butterworth filters high pass):

		a[0] = 1 + wca/Q + pow(wca,2); 

		b[0] = 1 / a[0];
		b[1] = -2 / a[0];
		b[2] = 1 / a[0];

		a[1] = (-2 + 2 * pow(wca,2) ) / a[0];
		a[2] = (1 - wca/Q + pow(wca,2) ) / a[0];

}

// Method to set the filter coefficients calculated earlier
void filter::setCoefficients(const std::vector<double>& bUpdated, const std::vector<double>& aUpdated)
{
    a[0] = aUpdated[0];
    a[1] = aUpdated[1];
    a[2] = aUpdated[2];
    
    b[1] = bUpdated[1];
    b[2] = bUpdated[2];
}

// Method to apply the filter to the signal
float filter::processFilter(float y)
{
    float x = y;
    
    y = a[0]*x + a[1]*xLast + a[2]*xSecondLast - b[1]*yLast - b[2]*ySecondLast;
    
    xSecondLast = xLast;
    xLast = x;
    ySecondLast = yLast;
    yLast = y;
    
    return y;
}