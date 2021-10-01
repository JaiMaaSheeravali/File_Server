#pragma once

#include <string>
#include <vector>

// std::pair<char **, int> roosh_parse(const std::string &line);
std::vector<std::string> ftp_tokenizer(const std::string &line, const char delimiter);