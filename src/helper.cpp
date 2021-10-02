#include "../include/helper.hpp"

#include <unistd.h>
#include <errno.h>

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