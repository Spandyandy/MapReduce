#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <fstream>

using namespace std;

struct FLIGHTDATA{
	string fl_date;
	string carrier;
	string origin;
	string origin_state;
	string dest;
	string dest_state;
	int dep_delay;
	int arr_delay;
	string cancel_code;
	int carrier_delay;
	int weather_delay;
	int nas_delay;
	int sec_delay;
	int late_aircraft_delay;
	int total_add_gtime;
};

int main(int argc, char *argv[]){
	struct FLIGHTDATA fd;


	string number;

	char *fdata;
	unsigned int disp_num;
	struct stat finfo;
	char *fname, *disp_num_str;
	double ave_dep_delay_p, ave_dep_delay_n, ave_arr_delay_p, ave_arr_delay_n, ave_carrier_delay, ave_weather_delay, 
			ave_nas_delay, ave_sec_delay, ave_late_aircraft_delay, 
			ave_total_add_gtime;

	disp_num = 0;

	if (argv[1] == NULL || argv[2] == NULL)
	{
		printf("USAGE: %s <filename> [Top # of results to display]\n", argv[0]);
		exit(1);
	}
	fname = argv[1];  
	disp_num_str = argv[2];    
	disp_num = atoi(disp_num_str);

	ifstream ip(fname);

	if(!ip.is_open()) std::cout << "ERROR: File Open" << '\n';

	string firstrow, end;
	getline(ip,firstrow,'\n');

	int totalNumData = 0;
	int posDepData = 0;
	int negDepData = 0;
	int posArrData = 0;
	int negArrData = 0;
	int disp_counter = 1;

	while(ip.good()){
		totalNumData++;

		getline(ip,fd.fl_date,',');
		getline(ip,fd.carrier,',');
		getline(ip,fd.origin,',');
		getline(ip,fd.origin_state,',');
		getline(ip,fd.dest,',');
		getline(ip,fd.dest_state,',');
		getline(ip,number,',');
		fd.dep_delay = strtol(number.c_str(),NULL,10);
		getline(ip,number,',');
		fd.arr_delay = strtol(number.c_str(),NULL,10);
		getline(ip,fd.cancel_code,',');
		getline(ip,number,',');
		fd.carrier_delay = strtol(number.c_str(),NULL,10);
		getline(ip,number,',');
		fd.weather_delay = strtol(number.c_str(),NULL,10);
		getline(ip,number,',');
		fd.nas_delay = strtol(number.c_str(),NULL,10);
		getline(ip,number,',');
		fd.sec_delay = strtol(number.c_str(),NULL,10);
		getline(ip,number,',');
		fd.late_aircraft_delay = strtol(number.c_str(),NULL,10);
		getline(ip,number,',');
		fd.total_add_gtime = strtol(number.c_str(),NULL,10);
		getline(ip,end, '\n');

		if(fd.dep_delay > 0){
			ave_dep_delay_p += fd.dep_delay;
			posDepData++;
		}
		else if (fd.dep_delay < 0){
			ave_dep_delay_n += fd.dep_delay;
			negDepData++;
		}

		if(fd.arr_delay > 0){
			ave_arr_delay_p += fd.arr_delay;
			posArrData++;
		}
		else if (fd.arr_delay < 0){
			ave_arr_delay_n += fd.arr_delay;
			negArrData++;
		}

		ave_carrier_delay += fd.carrier_delay;
		ave_weather_delay += fd.weather_delay;
		ave_nas_delay += fd.nas_delay;
		ave_sec_delay += fd.sec_delay;
		ave_late_aircraft_delay += fd.late_aircraft_delay;
		ave_total_add_gtime += fd.total_add_gtime;


		if(disp_num > 0){
			std::cout << "-------------------------------" << '\n';
			cout << disp_counter << "\n\n";
			disp_counter++;
			cout << "Flight Date: "<<fd.fl_date<< '\n';
			cout << "Carrier: "<<fd.carrier << '\n';
			cout << "Origin: "<<fd.origin << '\n';
			cout << "Origin State: "<<fd.origin_state << '\n';
			cout << "Destination: "<<fd.dest << '\n';
			cout << "Destination State: "<<fd.dest_state << '\n';
			cout << "Departure delay: "<<fd.dep_delay << '\n';
			cout << "Arrival delay: "<<fd.arr_delay << '\n';
			cout << "Cancel Code: "<<fd.cancel_code << '\n';
			cout << "Carrier delay: "<<fd.carrier_delay << '\n';
			cout << "Weather delay: "<<fd.weather_delay << '\n';
			cout << "NAS delay: "<<fd.nas_delay << '\n';
			cout << "Security delay: "<<fd.sec_delay << '\n';
			cout << "Late aircraft delay: "<<fd.late_aircraft_delay << '\n';
			cout << "Total Add Gtime: "<<fd.total_add_gtime << "\n\n";
			disp_num -= 1;
		}
	}

	cout << "Average departure delay: " << (ave_dep_delay_p / posDepData) << " min late\n";
	cout << "Average early departure: " << (ave_dep_delay_n / -negDepData) << " min early\n";
	cout << "Average departure delay: " << (ave_arr_delay_p / posArrData) << " min late\n";
	cout << "Average early departure: " << (ave_arr_delay_n / -negArrData) << " min early\n";
	cout << "Average carrier delay: " << (ave_carrier_delay / totalNumData) << " min \n";
	cout << "Average weather delay: " << (ave_weather_delay / totalNumData) << " min \n";			
	cout << "Average nas delay: " << (ave_nas_delay / totalNumData) << " min \n";
	cout << "Average security delay: " << (ave_sec_delay / totalNumData) << " min \n";
	cout << "Average late aircraft delay: " << (ave_late_aircraft_delay / totalNumData) << " min \n";
	cout << "Average total add gtime: " << (ave_total_add_gtime / totalNumData) << '\n';				
	cout << "How many data read: " << totalNumData << '\n';

	ip.close();
}
