#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>

#include "string_processing.h"



std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}