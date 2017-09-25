/* Copyright (c) 2007-2011, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the names of its 
*       contributors may be used to endorse or promote products derived from 
*       this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ 

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <iostream>

#include "map_reduce.h"
#define DEFAULT_DISP_NUM 10

// a passage from the text. The input data to the Map-Reduce
struct fd_line {
    char* data;
    uint64_t len;
};

// a single null-terminated word
struct fd_row {
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
    
    // necessary functions to use this as a key
    bool operator<(fd_row const& other) const {
        return strcmp(origin, other.origin) < 0;
    }
    bool operator==(fd_row const& other) const {
        return strcmp(data, other.data) == 0;
    }
};


// a hash for the word
struct fd_row_hash
{
    // FNV-1a hash for 64 bits
    size_t operator()(fd_row const& key) const
    {
        char* h = key.data;
        uint64_t v = 14695981039346656037ULL;
        while (*h != 0)
            v = (v ^ (size_t)(*(h++))) * 1099511628211ULL;
        return v;
    }
};


class FlightsMR : public MapReduce<FlightsMR, fd_line, fd_row, uint64_t, hash_container<fd_row, uint64_t, sum_combiner, fd_row_hash> >
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
        for (uint64_t i = 0; i < s.len; i++)
        {
            s.data[i] = toupper(s.data[i]);
        }
        uint64_t i = 0;
        while(i < s.len)
        {            
            while(i < s.len && (s.data[i] < '0' || (s.data[i] > '9' && s.data[i] < 'A') || (s.data[i] > 'Z')))k
                i++;
            uint64_t start = i;
            while(i < s.len && ((s.data[i] >= 'A' && s.data[i] <= 'Z') || s.data[i] == '.' || s.data[i] == '-' || s.data[i] == '_' || (s.data[i] >= '0' && s.data[i] <= '9')))
                i++;
            if(i>start)
            {
                s.data[i] = 0;
                fd_row word = { s.data+start };
                emit_intermediate(out, word, 1);
            }
        }
    }

    /** wordcount split()
     *  Memory map the file and divide file on a word border i.e. a space.
     */
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
        while(end < data_size && data[end] != '\r' &&
            data[end] != '\n')
            end++;

        /* Set the start of the next data. */
        out.data = data + splitter_pos;
        out.len = end - splitter_pos;
        
	while(end < data_size &&  data[end] == '\r' &&
            data[end] == '\n')
	    end++;
        splitter_pos = end;

        /* Return true since the out data is valid. */
        return 1;
    }
};



int main(int argc, char *argv[]) 
{
    int fd;
    char * fdata;
    unsigned int disp_num;
    struct stat finfo;
    char * fname, * disp_num_str;
    struct timespec begin, end;

    get_time (begin);

    // Make sure a filename is specified
    if (argv[1] == NULL)
    {
        printf("USAGE: %s <filename> [Top # of results to display]\n", argv[0]);
        exit(1);
    }

    fname = argv[1];
    disp_num_str = argv[2];

    printf("Reading: Running...\n");

    // Read in the file
    CHECK_ERROR((fd = open(fname, O_RDONLY)) < 0);
    // Get the file info (for file length)
    CHECK_ERROR(fstat(fd, &finfo) < 0);

    uint64_t r = 0;

    fdata = (char *)malloc (finfo.st_size);
    CHECK_ERROR (fdata == NULL);
    while(r < (uint64_t)finfo.st_size)
        r += pread (fd, fdata + r, finfo.st_size, r);
    CHECK_ERROR (r != (uint64_t)finfo.st_size);

    
    // Get the number of results to display
    CHECK_ERROR((disp_num = (disp_num_str == NULL) ? 
      DEFAULT_DISP_NUM : atoi(disp_num_str)) <= 0);




    get_time (end);

    #ifdef TIMING
    print_time("initialize", begin, end);
    #endif

    printf("Reading: Calling MapReduce Scheduler Wordcount\n");
    get_time (begin);
    std::vector<FlightsMR::keyval> result;    
    FlightsMR mapReduce(fdata, finfo.st_size, 1024*1024);
    CHECK_ERROR(mapReduce.run(result) < 0);
    get_time (end);

    #ifdef TIMING
    print_time("library", begin, end);
    #endif
    printf("Reading: MapReduce Completed\n");

    get_time (begin);

    unsigned int dn = std::min(disp_num, (unsigned int)result.size());
    printf("\Reading: Results \n", dn, result.size());
    uint64_t total = 0;

    for(size_t i = 0; i < result.size(); i++)
    {
	printf("%s  ", result[i]);
	if(i % 15 == 14)
	    printf("\n");
        total += result[i].val;
    }

    printf("Total: %lu\n", total);

    free (fdata);

    CHECK_ERROR(close(fd) < 0);

    get_time (end);

    #ifdef TIMING
    print_time("finalize", begin, end);
    #endif

    return 0;
}

// vim: ts=8 sw=4 sts=4 smarttab smartindent
