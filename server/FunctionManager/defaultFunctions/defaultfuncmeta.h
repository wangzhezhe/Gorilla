#ifndef DEFAULTTGFUNCMETA_H
#define DEFAULTTGFUNCMETA_H

#include <vector>
#include <string>
#include "../../../commondata/metadata.h"
#include "../../../client/unimosclient.h"

namespace GORILLA
{
struct FunctionManagerMeta;

std::string defaultCheckGetStep(size_t step, std::string varName, BlockDescriptor &rde);

bool defaultComparisonStep(std::string checkResults, std::vector<std::string> parameters);

//start a new dynamic trigger, the first name is the checking service
/*
void defaultActionSartDt(DynamicTriggerManager*dtm, 
size_t step, 
std::string varName, 
BlockDescriptor& rde,
std::vector<std::string> parameters);
*/

void defaultActionSartDt(
FunctionManagerMeta*fmm,
size_t step, 
std::string varName, 
BlockDescriptor& rde,
std::vector<std::string> parameters);

//std::string defaultRemoteCheck(size_t step, std::string varName, BlockDescriptor& rde);

std::string defaultCheck(size_t step, std::string varName, BlockDescriptor &rde, std::vector<std::string> parameters);

bool defaultComparison(std::string checkResults, 
std::vector<std::string> parameters);

void defaultAction(
FunctionManagerMeta*fmm,
size_t step, 
std::string varName,
std::string triggerMaster,
UniClient* uniclient,
BlockDescriptor& rde,
std::vector<std::string> parameters);

void defaultNotifyAction(
FunctionManagerMeta *fmm, 
size_t step, 
std::string varName,
std::string triggerMaster,
UniClient *uniclient, 
BlockDescriptor &rde, 
std::vector<std::string> parameters);

void defaultPutEvent(
FunctionManagerMeta *fmm, 
size_t step, 
std::string varName,
std::string triggerMaster,
UniClient *uniclient, 
BlockDescriptor &rde, 
std::vector<std::string> parameters);


std::string InsituExpCheck(size_t step, std::string varName, BlockDescriptor &rde, std::vector<std::string> parameters);

bool InsituExpCompare(std::string checkResults, 
std::vector<std::string> parameters);

void InsituExpAction(
FunctionManagerMeta *fmm,    
size_t step, 
std::string varName,
std::string triggerMaster,
UniClient *uniclient, 
BlockDescriptor &rde, 
std::vector<std::string> parameters);


}
#endif