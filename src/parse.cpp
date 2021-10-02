#include "../include/parse.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static inline std::string &ltrim(std::string &s)
{
    s.erase(
        s.begin(),
        std::find_if(
            s.begin(),
            s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(
                s.rbegin(),
                s.rend(),
                std::not1(std::ptr_fun<int, int>(std::isspace)))
                .base(),
            s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s)
{
    return ltrim(rtrim(s));
}

// split the input string into list of words depending on delimiter
std::vector<std::string> ftp_tokenizer(const std::string &input, const char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;

    // input stream used to split string around delimiter
    std::istringstream iss(input);

    // Tokenizing w.r.t. specified delimiter
    while (getline(iss, token, delimiter))
    {
        token = trim(token);
        if (!token.empty())
        {
            tokens.push_back(token);
        }
    }

    return tokens;
}