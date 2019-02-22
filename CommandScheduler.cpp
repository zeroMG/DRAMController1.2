/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#include <iostream>
#include <algorithm>

#include "CommandScheduler.h"
#include "MemorySystem.h"
#include "global.h"

// using namespace std;
using namespace DRAMController;

CommandScheduler::CommandScheduler(
	vector<CommandQueue*>& commandQueues, const map<unsigned int, bool>& requestorTable):
commandQueue(commandQueues),
requestorCriticalTable(requestorTable)
{
	// if the command queue is used as per memory level
	cmdQueueTimer = std::map<unsigned int, std::map<unsigned int, unsigned int> >();
	// if the command queue is used as per requestor queue
	for(unsigned int index=0; index<commandQueue.size(); index++) {
		reqCmdQueueTimer.push_back(std::map<unsigned int, std::map<unsigned int, unsigned int> >());
	}

	memorySystem = NULL;
	refreshMachine = NULL;
	scheduledCommand = NULL;
	checkCommand = NULL;
	clock = 0;
	ranks = 0;
	banks = 0;
}

CommandScheduler::~CommandScheduler()
{
	cmdQueueTimer.clear();
	// delete refreshMachine;
}

void CommandScheduler::connectMemorySystem(MemorySystem* memSys)
{
	memorySystem = memSys;
	ranks = memorySystem->get_Rank();
	banks = memorySystem->get_Bank();
	refreshMachine = new RefreshMachine(commandQueue, ranks, banks, getTiming("tREFI"), getTiming("tRFC"));
}

unsigned int CommandScheduler::getTiming(const string& param)
{
	return memorySystem->get_constraints(param);
}
// Perform refresh
bool CommandScheduler::refreshing()
{
	return refreshMachine->refreshing();
}
	// Indicator of reaching refresh interval
	//What is this fucking Shit??
void CommandScheduler::refresh()
{
	BusPacket* tempCmd = NULL;
	refreshMachine->refresh(tempCmd);
	if(tempCmd != NULL) {
		if(isIssuable(tempCmd)) {
			memorySystem->receiveFromBus(tempCmd);
			refreshMachine->popCommand();
		}
	}
	tempCmd = NULL;
	delete tempCmd;
}

// each command queue share the same command ready time
bool CommandScheduler::isReady(BusPacket* cmd, unsigned int index)
{
	if(commandQueue[index]->isPerRequestor()) {
		if(reqCmdQueueTimer[index].find(cmd->requestorID) == reqCmdQueueTimer[index].end()) {
			reqCmdQueueTimer[index][cmd->requestorID] = map<unsigned int, unsigned int>();
		}
		if(reqCmdQueueTimer[index][cmd->requestorID].find(cmd->busPacketType) != reqCmdQueueTimer[index][cmd->requestorID].end())
		{
			if(reqCmdQueueTimer[index][cmd->requestorID][cmd->busPacketType] != 0)
			{
				// DEBUG("CMD "<<cmd->busPacketType<<" Count "<<reqCmdQueueTimer[index][cmd->requestorID][cmd->busPacketType]);
				return false;
			}
		}
	}
	else 
	{
		if(cmdQueueTimer.find(index) == cmdQueueTimer.end()) {
			cmdQueueTimer[index] = map<unsigned int, unsigned int>();
		}
		if(cmdQueueTimer[index].find(cmd->busPacketType) != cmdQueueTimer[index].end()) {
			if(cmdQueueTimer[index][cmd->busPacketType] != 0){	
				return false;
			}
		}		
	}
	return true;
}

bool CommandScheduler::isIssuable(BusPacket* cmd)
{
	return memorySystem->command_check(cmd);
}

void CommandScheduler::sendCommand(BusPacket* cmd, unsigned int index)
{
	// Update command counter
	if(commandQueue[index]->isPerRequestor()) {
		for(unsigned int type = RD; type != PDE; type++) {
			reqCmdQueueTimer[index][cmd->requestorID][type] = std::max(reqCmdQueueTimer[index][cmd->requestorID][type], 
			memorySystem->command_timing(cmd, static_cast<BusPacketType>(type)));
			// if(type == ACT) {
			// 	DEBUG("ACT @ "<<reqCmdQueueTimer[index][cmd->requestorID][type]);
			// }
		}
		commandQueue[index]->removeCommand(cmd->requestorID);
	}
	else {
		for(unsigned int type = RD; type != PDE; type++) {
			cmdQueueTimer[index][type] = std::max(cmdQueueTimer[index][type], 
				memorySystem->command_timing(cmd, static_cast<BusPacketType>(type)));
		}
		commandQueue[index]->removeCommand();		
	}
	memorySystem->receiveFromBus(scheduledCommand);
}

void CommandScheduler::commandClear()
{
	// ************* -- *************
	delete scheduledCommand;
	scheduledCommand = NULL;
}

void CommandScheduler::tick()
{
	// countdown command ready timer
	for(unsigned int index = 0; index < commandQueue.size(); index++) {
		if(commandQueue[index]->isPerRequestor()) {
			for(auto &requestor : reqCmdQueueTimer[index]) {
				for(auto &counter : reqCmdQueueTimer[index][requestor.first]) {
					if(counter.second > 0) {
						counter.second--;
						// if(counter.first == ACT) {
						// 	DEBUG("REQ "<<requestor.first<<" ACT @ "<<counter.second);
						// }
					}
				}
			}
		}
		else
		{
			if(!cmdQueueTimer[index].empty()) {
				for(auto &counter : cmdQueueTimer[index]) {
					if(counter.second > 0) {
						counter.second--;
					}
				}
			}			
		}
	}
	refreshMachine->step();
	clock++;
}



