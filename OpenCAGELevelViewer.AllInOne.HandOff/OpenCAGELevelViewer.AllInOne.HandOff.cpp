// OpenCAGELevelViewer.AllInOne.HandOff.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <handoff.h>
#include <iostream>

int main(char **argv, int argc)
{
    int exit = handoff(argv, argc);

    std::cout << exit << std::endl;

    return exit;
}