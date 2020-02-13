
#ifndef DEFAULTFUNCRAW_H
#define DEFAULTFUNCRAW_H

#include "../../../commondata/metadata.h"
#include <vector>
#include <string>

std::string test(const BlockSummary &bs, 
void *inputData,
const std::vector<std::string>& parameters);

std::string testvtk(const BlockSummary &bs, 
void *inputData,
const std::vector<std::string>& parameters);

std::string valueRange(const BlockSummary &bs, 
void *inputData, 
const std::vector<std::string>& parameters);

#endif