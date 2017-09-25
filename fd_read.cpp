#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <iostream>

#define DEFAULT_DISP_NUM 10


int main(int argc, std::string argv[]) {/*
	int fd;
	std::string fdata;
	std::string fname, disp_num_str;
  
	if (argv[1] == NULL)
	{
		printf("USAGE: %s <filename> [Top # of results to display]\n", argv[0]);
		exit(1);
	}

	fname = argv[1];
	disp_num_str = argv[2];
	CHECK_ERROR((fd = open(fname, O_RDONLY)) < 0);
	CHECK_ERROR(fstat(fd, &finfo) < 0);  

	fdata = (std::string)malloc (finfo.st_size);
	CHECK_ERROR (fdata == NULL);
	while(r < (uint64_t)finfo.st_size)
        r += pread (fd, fdata + r, finfo.st_size, r);
    CHECK_ERROR (r != (uint64_t)finfo.st_size);

    CHECK_ERROR((disp_num = (disp_num_str == NULL) ? 
      DEFAULT_DISP_NUM : atoi(disp_num_str)) <= 0);
	*/

	std::string text,result;

	std::getline(std::cin, text, ',');

	  std::remove_copy_if(text.begin(), text.end(),
                      std::back_inserter(result), //Store output                                                                                  
              std::ptr_fun<int, int>(&std::ispunct)
                      );
	std::cout << result << std::endl;
	std::cout << text << std::endl;

}
