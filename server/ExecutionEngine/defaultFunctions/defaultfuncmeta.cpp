#include <iostream>
#include "defaultfuncmeta.h"

std::string defaultCheck(std::vector<std::string> parameters)
{
    std::cout << "execute default check" << std::endl;
    std::cout << "parameters\n";
    int size = parameters.size();
    for (int i = 0; i < size; i++)
    {
        std::cout << "index " << i << "," << parameters[i] << std::endl;
    }
    return "check results";
}

bool defaultComparison(std::string checkResults, std::vector<std::string> parameters)
{
    std::cout << "execute default defaultComparison" << std::endl;

    std::cout << "parameters\n";
    std::cout << checkResults << std::endl;
    int size = parameters.size();
    for (int i = 0; i < size; i++)
    {
        std::cout << "index " << i << "," << parameters[i] << std::endl;
    }
    return true;
}

void defaultAction(std::vector<std::string> parameters)
{
    std::cout << "execute default defaultAction" << std::endl;
    std::cout << "parameters\n";
    int size = parameters.size();
    for (int i = 0; i < size; i++)
    {
        std::cout << "index " << i << "," << parameters[i] << std::endl;
    }
    return;
}