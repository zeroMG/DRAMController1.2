/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/


#include <math.h>

#include "RequestScheduler.h"
#include "CommandGenerator.h"
#include "global.h"

using namespace std;
using namespace DRAMController;

RequestScheduler::RequestScheduler(std::vector<RequestQueue*>&requestQueues, std::vector<CommandQueue*>& commandQueues, const std::map<unsigned int, bool>& requestorTable):
	requestorCriticalTable(requestorTable),
	requestQueue(requestQueues),
	commandQueue(commandQueues)
{
	commandGenerator = NULL;
	scheduledRequest = NULL;
	clockCycle = 0;
}

RequestScheduler::~RequestScheduler()
{
	bankTable.clear();
}

void RequestScheduler::connectCommandGenerator(CommandGenerator *generator)
{
	commandGenerator = generator;
}

bool RequestScheduler::isRowHit(Request* request)
{
	bool isHit = false;
	// if(bankTable.find(request->rank) != bankTable.end()) {
	// 	if(bankTable[request->rank].find(request->bank) != bankTable[request->rank].end()) {
	// 		if(bankTable[request->rank][request->bank] == request->row){ isHit = true; }
	// 	}	
	// }
	//What the fuck are the following lines?
	if(bankTable.find(request->addressMap[Rank]) != bankTable.end()) {
		if(bankTable[request->rank].find(request->addressMap[Bank]) != bankTable[request->addressMap[Rank]].end()) {
			if(bankTable[request->addressMap[Rank]][request->addressMap[Bank]] == request->row){ 
				isHit = true; 
			}
		}	
	}
	return isHit;
}

Request* RequestScheduler::scheduleFR(unsigned int qIndex)
{
	//What the fuck are the following lines?
	if(requestQueue[qIndex]->getSize(false, 0) > 0) {
		for(unsigned int index=0; index < requestQueue[qIndex]->getSize(false, 0); index++) {
			if(isRowHit(requestQueue[qIndex]->getRequest(index))) {
				return requestQueue[qIndex]->getRequest(index);
			}
		}
		return requestQueue[qIndex]->getRequest(0);
	}
	else {
		return NULL;
	}
}

bool RequestScheduler::isSchedulable(Request* request, bool open)
{
	return commandGenerator->commandGenerate(request, open);
}

void RequestScheduler::updateRowTable(unsigned rank, unsigned bank, unsigned row)
{
	bankTable[rank][bank] = row;
}

// void RequestScheduler::step()
// {
// 	clockCycle++;
// }
