#pragma once
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// #define PI 3.141592653589793

namespace sphexa
{


void generate_initial_square(int tot_size, const Array<T> &x, const Array<T> &y, const Array<T> &z, const Array<T> &vx, const Array<T> &vy, const Array<T> &vz, const Array<T> &p_0)
{
    const double omega = 5.0;
    const double myPI = std::acos(-1.0);
    const int nx = cbrt(tot_size);
    
    double lx, ly, lz, lvx, lvy, lvz, lp_0;
    // std::vector<double> x_arr, y_arr, z_arr, vx_arr, vy_arr, vz_arr, p_0_arr;
    // x_arr.resize(tot_size);
    // y_arr.resize(tot_size);
    // z_arr.resize(tot_size);
    // vx_arr.resize(tot_size);
    // vy_arr.resize(tot_size);
    // vz_arr.resize(tot_size);
    // p_0_arr.resize(tot_size);

    // std::ofstream outputFile("sqpatch.txt");
    int lindex;
    for (int i = 0; i < nx; ++i)
    {
        lz = -0.5 + 1.0 / (2.0 * nx) + (double)i / (double)nx;

        for (int j = 0; j < nx; ++j)
        {
            lx = -0.5 + 1.0 / (2 * nx) + (double)j / (double)nx;

            for (int k = 0; k < nx; ++k)
            {
                ly = -0.5 + 1.0 / (2 * nx) + (double)k / (double)nx;
                
                lvx = omega * y;
                lvy = -omega * x;
                lvz = 0.;
                lp_0 = 0.;

                for (int m = 1; m < 39; m+=2)
                    for (int n = 1; n < 39; n+=2)
                        lp_0 = p_0 - 32.0 * (omega * omega) / ((double)m * (double)n * (myPI * myPI)) / (((double)m * myPI) * ((double)m * myPI) + ((double)n * myPI) * ((double)n * myPI)) * sin((double)m * myPI * (x + 0.5)) * sin((double)n * myPI * (y + 0.5));

                lp_0 *= 1000.0;

                //add to the vectors the current calculated values
                lindex = i*nx*nx + j*nx + k;
                z[lindex] = lz;
                y[lindex] = ly;
                x[lindex] = lx;
                vx[lindex] = lvx;
                vy[lindex] = lvy;
                vz[lindex] = lvz;
                p_0[lindex] = lp_0;
            }
        }
    }


    


}

}
