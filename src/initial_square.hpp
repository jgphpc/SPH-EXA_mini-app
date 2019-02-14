#pragma once
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// #define PI 3.141592653589793

namespace sphexa
{


void generate_initial_square(int tot_size)
{
    const double omega = 5.0;
    const double myPI = std::acos(-1.0);
    const int nx = cbrt(tot_size);
    
    double x, y, z, vx, vy, vz, p_0;
    std::vector<double> x_arr, y_arr, z_arr, vx_arr, vy_arr, vz_arr, p_0_arr;
    x_arr.resize(tot_size);
    y_arr.resize(tot_size);
    z_arr.resize(tot_size);
    vx_arr.resize(tot_size);
    vy_arr.resize(tot_size);
    vz_arr.resize(tot_size);
    p_0_arr.resize(tot_size);

    std::ofstream outputFile("sqpatch.txt");

    for (int i = 0; i < nx; ++i)
    {
        z = -0.5 + 1.0 / (2.0 * nx) + (double)i / (double)nx;

        for (int j = 0; j < nx; ++j)
        {
            x = -0.5 + 1.0 / (2 * nx) + (double)j / (double)nx;

            for (int k = 0; k < nx; ++k)
            {
                y = -0.5 + 1.0 / (2 * nx) + (double)k / (double)nx;
                vx = omega * y;
                vy = -omega * x;
                vz = 0.;
                p_0 = 0.;
                for (int m = 1; m < 39; m+=2)
                    for (int n = 1; n < 39; n+=2)
                        p_0 = p_0 - 32.0 * (omega * omega) / ((double)m * (double)n * (myPI * myPI)) / (((double)m * myPI) * ((double)m * myPI) + ((double)n * myPI) * ((double)n * myPI)) * sin((double)m * myPI * (x + 0.5)) * sin((double)n * myPI * (y + 0.5));

                outputFile << std::scientific << x << ' ' << y << ' ' << z << ' ' << vx << ' ' << vy << ' ' << vz << ' ' << p_0*1000.0 << std::endl;
            }
        }
    }
    outputFile.close();

    FILE *f = fopen("sqpatch.txt", "r");
    
    if(f)
    {
        for(int i=0; i<tot_size; i++)
        {   
            fscanf(f, "%lf %lf %lf %lf %lf %lf %lf\n", &x_arr[i], &y_arr[i], &z_arr[i], &vx_arr[i], &vy_arr[i], &vz_arr[i], &p_0_arr[i]);
        }

        fclose(f);
        if (remove("sqpatch.txt") == 0) 
            printf("Successfully deleted txt file.\n");
        else
            printf("Unable to delete the txt file\n");


        std::ofstream ofs("sqpatch.bin", std::ofstream::out | std::ofstream::binary);
        
        if(ofs)
        {
            ofs.write(reinterpret_cast<const char*>(x_arr.data()), x_arr.size() * sizeof(double));
            ofs.write(reinterpret_cast<const char*>(y_arr.data()), y_arr.size() * sizeof(double));
            ofs.write(reinterpret_cast<const char*>(z_arr.data()), z_arr.size() * sizeof(double));
            ofs.write(reinterpret_cast<const char*>(vx_arr.data()), vx_arr.size() * sizeof(double));
            ofs.write(reinterpret_cast<const char*>(vy_arr.data()), vy_arr.size() * sizeof(double));
            ofs.write(reinterpret_cast<const char*>(vz_arr.data()), vz_arr.size() * sizeof(double));
            ofs.write(reinterpret_cast<const char*>(p_0_arr.data()), p_0_arr.size() * sizeof(double));

            ofs.close();
        }
        else
            printf("Error: couldn't open file for writing.\n");
    }
    else
        printf("Error: couldn't open file for reading.\n");

}

}
