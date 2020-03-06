
#ifndef DEFAULTFUNCRAW_H
#define DEFAULTFUNCRAW_H

#include "../../../commondata/metadata.h"
#include "../functionManager.h"
#include <vector>
#include <string>


struct FunctionManagerRaw;

std::string test(
FunctionManagerRaw* fmr,
const BlockSummary &bs, 
void *inputData,
const std::vector<std::string>& parameters);

std::string testvtk(
FunctionManagerRaw*fmr,
const BlockSummary &bs, 
void *inputData,
const std::vector<std::string>& parameters);

std::string valueRange(
FunctionManagerRaw*fmr,
const BlockSummary &bs, 
void *inputData, 
const std::vector<std::string>& parameters);

std::string adiosWrite(
FunctionManagerRaw*fmr,
const BlockSummary &bs, 
void *inputData, 
const std::vector<std::string>& parameters);



#endif