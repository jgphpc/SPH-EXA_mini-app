#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include "mytimer.hpp"


#define REPORT_TIME(expr, name) \
    timer::report_time([&](){ expr; }, name); \


double powerd (double x, int y)
{
    double temp;
    if (y == 0)
    return 1;
    temp = powerd (x, y / 2);
    if ((y % 2) == 0) {
        return temp * temp;
    } else {
        if (y > 0)
            return x * temp * temp;
        else
            return (temp * temp) / x;
    }
}

int main(){
    double x = 1e6;
    int y = 7;

    REPORT_TIME(std::pow(x, y), "POWER NORMAL");
    std::cout << std::pow(x, y) << std::endl;
    REPORT_TIME(powerd(x, y), "POWER MOD");
    std::cout << powerd(x, y) << std::endl;
    

    exit(0);
}