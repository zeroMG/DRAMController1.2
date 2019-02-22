/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#include "global.h"
#include "CommandGenerator.h"

#include <algorithm>    // std::swap

using namespace DRAMController;

CommandGenerator::CommandGenerator(unsigned int dataBus):
	dataBusSize(dataBus)
{}

CommandGenerator::~CommandGenerator()
{
	lookupTable.clear();
	// std::queue<BusPacket*> empty;
	// std::swap(commandBuffer, empty);
}

BusPacket* CommandGenerator::getCommand() 
{
	return commandBuffer.front();
}

void CommandGenerator::removeCommand()
{
	commandBuffer.pop();
}

unsigned CommandGenerator::bufferSize() 
{
	return commandBuffer.size();
}




