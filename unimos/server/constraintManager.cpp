#include "constraintManager.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <thread>


//put as separate file that includes the functions

bool defaultStepFilter(size_t step)
{
    std::cout << "default StepFilter" << std::endl;

    return true;
}

bool defaultBlockFilter(size_t blockID)
{
    std::cout << "default BlockFilter" << std::endl;
    return true;
}

bool defaultContentFilter(void *data)
{
    std::cout << "default ContentFilter" << std::endl;
    return true;
}

constraintManager::constraintManager(std::string stepConstriaintsName,
                                     std::string blockConstraintsName,
                                     std::string contentConstrains)
{
    if (stepConstriaintsName.compare("default") == 0)
    {
        this->stepConstraintsPtr = &defaultStepFilter;
    }
    else
    {
        std::cout << "unsuported filter type " << stepConstriaintsName << " for step fliter" << std::endl;
    }

    if (blockConstraintsName.compare("default") == 0)
    {
        this->blockConstraintsPtr = &defaultBlockFilter;
    }
    else
    {
        std::cout << "unsuported filter type " << blockConstraintsName << " for block" << std::endl;
    }

    if (contentConstrains.compare("default") == 0)
    {
        this->contentConstraintPtr = &defaultContentFilter;
    }
    else
    {
        std::cout << "unsuported filter type " << contentConstrains << " for content filter" << std::endl;
    }

    return;
}

bool constraintManager::execute(size_t step, size_t blockID, void *data)
{

    //std::this_thread::sleep_for(std::chrono::seconds(2));

    bool stepConstraint = this->stepConstraintsPtr(step);

    if (stepConstraint == false)
    {
        return false;
    }

    bool blockConstraint = this->blockConstraintsPtr(blockID);

    if (blockConstraint == false)
    {
        return false;
    }

    bool contentConstraint = this->contentConstraintPtr(data);

    return contentConstraint;
}

