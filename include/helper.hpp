#pragma once
#include <unistd.h>

ssize_t writen(int fd, const char *buffer, size_t n);
int rand(int low, int high);
void makeDirectory(char* dirname);
bool checkifExisting(char* dirname);