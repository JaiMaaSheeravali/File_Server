#include "../include/helper.hpp"

#include <unistd.h>
#include <errno.h>
#include <random>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>


ssize_t writen(int fd, const char *buffer, size_t n)
{
    ssize_t numWritten; /* # of bytes written by last write() */
    size_t totWritten;  /* Total # of bytes written so far */
    const char *buf = buffer;
    for (totWritten = 0; totWritten < n;)
    {
        numWritten = write(fd, buf, n - totWritten);

        if (numWritten <= 0)
        {
            if (numWritten == -1 && errno == EINTR)
                continue; /* Interrupted --> restart write() */
            else
                return -1; /* Some other error */
        }
        totWritten += numWritten;
    }
    return totWritten; /* Must be 'n' bytes if we get here */
}

int rand(int low, int high)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(low, high); // distribution in range [low, high]

    return dist(rng);
}

void makeDirectory(char* dirname){
	DIR* dir = opendir(dirname);

	if(dir){
		// do nothing
	}
	else if(ENOENT == errno){
		if(mkdir(dirname, 0777) == -1){
			std::cerr << "Error Creating Storage Directory.\n";
		}
	} else {
		std::cerr << "Error opening Storage Directory\n";
	}
}

bool checkifExisting(char* dirname){
    DIR* dir = opendir(dirname);
    if(dir)
        return true;
    else
        return false;
}