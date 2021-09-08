#include "../include/random.hpp"

#include <random>

int rand(int low, int high)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(low, high); // distribution in range [low, high]

    return dist(rng);
}