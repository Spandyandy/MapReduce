#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "map_reduce.h"
#define DEFAULT_DISP_NUM 10


using namespace std;

struct fd_line {
    char* data;
    uint64_t len;
};

struct FLIGHTDATA{
	char* data;
	string fl_date;
	string carrier;
	string origin;
	string dest;


	// necessary functions to use this as a key
    bool operator<(FLIGHTDATA const& other) const {
        return strcmp(data, other.data) < 0;
    }
    bool operator==(FLIGHTDATA const& other) const {
        return strcmp(data, other.data) == 0;
    }
};

struct FLIGHTDELAY{
	string origin_state;
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

	double ave_arr_delay_p, ave_arr_delay_n;

};

struct fd_row_hash
{
    // FNV-1a hash for 64 bits
    size_t operator()(FLIGHTDATA const& key) const
    {
        char* h = key.data;
        uint64_t v = 14695981039346656037ULL;
        while (*h != 0)
            v = (v ^ (size_t)(*(h++))) * 1099511628211ULL;
        return v;
    }
};

class FlightsMR : public MapReduce<FlightsMR, fd_line, FLIGHTDATA, 
	FLIGHTDELAY, hash_container<FLIGHTDATA, FLIGHTDELAY, 
	buffer_combiner, fd_row_hash> >
{
	char* data;
    uint64_t data_size;
    uint64_t chunk_size;
    uint64_t splitter_pos;
public:
    explicit FlightsMR(char* _data, uint64_t length, uint64_t _chunk_size) :
        data(_data), data_size(length), chunk_size(_chunk_size), 
            splitter_pos(0) {}

    void* locate(data_type* str, uint64_t len) const
    {
        return str->data;
    }


	void map(data_type const& s, map_container& out) const
	{
		string number;
		string temp(s.data);
		stringstream ss(temp);
				
		FLIGHTDATA fdt;
		FLIGHTDELAY fdl;


		getline(ss,fdt.fl_date,',');
		getline(ss,fdt.carrier,',');
		getline(ss,fdt.origin,',');
		getline(ss,fdl.origin_state,',');
		getline(ss,fdt.dest,',');
		getline(ss,fdl.dest_state,',');
		
		string tempor = fdt.fl_date+ fdt.carrier + fdt.origin + fdt.dest;
		fdt.data = tempor.c_str();
		
		getline(ss,number,',');
		fdl.dep_delay = strtol(number.c_str(),NULL,10);
		getline(ss,number,',');
		fdl.arr_delay = strtol(number.c_str(),NULL,10);
		getline(ss,fdl.cancel_code,',');
		getline(ss,number,',');
		fdl.carrier_delay = strtol(number.c_str(),NULL,10);
		getline(ss,number,',');
		fdl.weather_delay = strtol(number.c_str(),NULL,10);
		getline(ss,number,',');
		fdl.nas_delay = strtol(number.c_str(),NULL,10);
		getline(ss,number,',');
		fdl.sec_delay = strtol(number.c_str(),NULL,10);
		getline(ss,number,',');
		fdl.late_aircraft_delay = strtol(number.c_str(),NULL,10);
		getline(ss,number,',');
		fdl.total_add_gtime = strtol(number.c_str(),NULL,10);

       
        emit_intermediate(out, fdt, fdl);
    }

    void reduce(key_type const& key, reduce_iterator const& values, std::vector<keyval>& out) const {
        value_type fd;


        int total_arr_delay_p = 0, total_arr_delay_n = 0;
		int posArrData = 0;
		int negArrData = 0;

        while (values.next(fd))
        {
			if(fd.arr_delay > 0){
				total_arr_delay_p += fd.arr_delay;
				posArrData++;
			}
			else if (fd.arr_delay < 0){
				total_arr_delay_n += fd.arr_delay;
				negArrData++;
			}
        }

        if(posArrData == 0)
        	fd.ave_arr_delay_p = 0;
        else
        	fd.ave_arr_delay_p = (total_arr_delay_p / posArrData);

        if(negArrData == 0)
        	fd.ave_arr_delay_n = 0;
        else
			fd.ave_arr_delay_n = (total_arr_delay_n / -negArrData);

        keyval kv = {key, fd};
        out.push_back(kv);
    }
    
	int split(fd_line& out)
    {
        /* End of data reached, return FALSE. */
        if ((uint64_t)splitter_pos >= data_size)
        {
            return 0;
        }

        /* Determine the nominal end point. */
        uint64_t end = std::min(splitter_pos + chunk_size, data_size);

        /* Move end point to next word break */
        while(end < data_size && data[end] != '\n')
            end++;

        /* Set the start of the next data. */
        out.data = data + splitter_pos;
        out.len = end - splitter_pos;
        
        splitter_pos = end;

        /* Return true since the out data is valid. */
        return 1;
    }
};

int main(int argc, char *argv[]){
	int fdt;
    char * fdata;
    unsigned int disp_num;
    struct stat finfo;
    char * fname, * disp_num_str;
    struct timespec begin, end;

//	double ave_dep_delay_p, ave_dep_delay_n, ave_arr_delay_p, ave_arr_delay_n, 
//			ave_carrier_delay, ave_weather_delay, ave_nas_delay, ave_sec_delay, 
//			ave_late_aircraft_delay, ave_total_add_gtime;

    get_time(begin);

	if (argv[1] == NULL || argv[2] == NULL)
	{
		printf("USAGE: %s <filename> [Top # of results to display]\n", argv[0]);
		exit(1);
	}
	fname = argv[1];  
	disp_num_str = argv[2];        

    printf("Reading: Running...\n");

    // Read in the file
    CHECK_ERROR((fdt = open(fname, O_RDONLY)) < 0);
    // Get the file info (for file length)
    CHECK_ERROR(fstat(fdt, &finfo) < 0);

    uint64_t r = 0;

    fdata = (char *)malloc (finfo.st_size);
    CHECK_ERROR (fdata == NULL);
    while(r < (uint64_t)finfo.st_size)
        r += pread (fdt, fdata + r, finfo.st_size, r);
    CHECK_ERROR (r != (uint64_t)finfo.st_size);
    CHECK_ERROR((disp_num = (disp_num_str == NULL) ? 
      DEFAULT_DISP_NUM : atoi(disp_num_str)) <= 0);
	get_time(end);

	print_time("initialize", begin, end);
	printf("\n\n");

	    get_time(begin);

    std::vector<FlightsMR::keyval> result;    
    FlightsMR mapReduce(fdata, finfo.st_size, 1024*1024);
    CHECK_ERROR( mapReduce.run(result) < 0);

    get_time(end);
	print_time("MAPREDUCE", begin, end);


    unsigned int dn = std::min(disp_num, (unsigned int)result.size());
    printf("\nWordcount: Results (TOP %d of %lu):\n", dn, result.size());
    uint64_t total = 0;
    for (size_t i = 0; i < dn; i++)
    {
        	cout << "Average arrival delay: " << result[i].val.ave_arr_delay_p << " min late\n";
    }

/*
	string firstrow, end;
	

	int totalNumData = 0;
	int posDepData = 0;
	int negDepData = 0;
	int posArrData = 0;
	int negArrData = 0;
	int posCarData = 0;
	int posWeaData = 0;
	int posNasData = 0;
	int posSecData = 0;
	int posLadData = 0;
	int posTagData = 0;
	int disp_counter = 1;

	while(ip.good()){
		totalNumData++;

		getline(ip,fd.fl_date,',');
		getline(ip,fd.carrier,',');
		getline(ip,fd.origin,',');
		getline(ip,fd.origin_state,',');
		getline(ip,fd.dest,',');
		getline(ip,fd.dest_state,',');
		string temp = fd.fl_date+ fd.carrier + fd.origin + fd.dest;
		fd.data = temp.c_str();
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
		if(fd.carrier_delay > 0){
			ave_carrier_delay += fd.carrier_delay;
			posCarData++;
		}
		if(fd.weather_delay > 0){
			ave_weather_delay += fd.weather_delay;
			posWeaData++;
		} 
		if(fd.nas_delay > 0){
			ave_nas_delay += fd.nas_delay;
			posNasData++;
		} 
		if(fd.sec_delay > 0){
			ave_sec_delay += fd.sec_delay;
			posSecData++;
		} 
		if(fd.late_aircraft_delay > 0){
			ave_late_aircraft_delay += fd.late_aircraft_delay;
			posLadData++;
		} 
		if(fd.total_add_gtime > 0){
			ave_total_add_gtime += fd.total_add_gtime;
			posTagData++;
		} 


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
	cout << "Average arrival delay: " << (ave_arr_delay_p / posArrData) << " min late\n";
	cout << "Average early arrival: " << (ave_arr_delay_n / -negArrData) << " min early\n";
	cout << "Average carrier delay: " << (ave_carrier_delay / posCarData) << " min late\n";
	cout << "Average weather delay: " << (ave_weather_delay / posWeaData) << " min late\n";	
	cout << "Average nas delay: " << (ave_nas_delay / posNasData) << " min late\n";	
	cout << "Average security delay: " << (ave_sec_delay / posSecData) << " min late\n";
	cout << "Average late aircraft delay: " << (ave_late_aircraft_delay / posLadData) << " min late\n";
	cout << "Average total add gtime: " << (ave_total_add_gtime / posTagData) << '\n';				
	cout << "How many data read: " << totalNumData << '\n';
*/
//	ip.close();
}
