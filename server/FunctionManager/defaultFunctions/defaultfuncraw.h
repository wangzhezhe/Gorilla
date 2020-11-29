
#ifndef DEFAULTFUNCRAW_H
#define DEFAULTFUNCRAW_H

#include "../../../commondata/metadata.h"
#include "../functionManagerRaw.h"
#include <vector>
#include <string>

namespace GORILLA
{

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

}

#endif