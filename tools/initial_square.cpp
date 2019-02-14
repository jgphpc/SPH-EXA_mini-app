#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// #define PI 3.141592653589793

int main(){

	const double omega = 5.0;
	const double PI = std::acos(-1.0);
	const int nx = 100;
	const int tot_size = nx*nx*nx;
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
						p_0 = p_0 - 32.0 * (omega * omega) / ((double)m * (double)n * (PI * PI)) / (((double)m * PI) * ((double)m * PI) + ((double)n * PI) * ((double)n * PI)) * sin((double)m * PI * (x + 0.5)) * sin((double)n * PI * (y + 0.5));

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
  //       if (remove("sqpatch.txt") == 0) 
		// 	printf("Deleted successfully"); 
		// else
		// 	printf("Unable to delete the file");

        std::ofstream ofs("sqpatchTEST.bin", std::ofstream::out | std::ofstream::binary);
        
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

	exit(0);
}