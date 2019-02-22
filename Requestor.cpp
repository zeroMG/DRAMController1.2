/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#include "Requestor.h"
#include "MemoryController.h"
#include <sstream>
using namespace DRAMController;

Requestor::Requestor(int id, bool inOrder, const string& traceFile):
	requestorID(id),
	inOrder(inOrder) 
{
	transFile.open(traceFile.c_str());
	localCache = new Cache();
	sim_done = false;
	currentClockCycle = 0;

	prevArrive = 0;
	prevComplete = 0;
	wcLatency = 0;
	compTime = 0;
	oppcheck = 0;

	completeRequest = 0;
	requestRequest = 0;
	latency = 0;
	waitingRequest = false;

	memoryController = NULL;
	pendingRequest = NULL;

	hitRatioCtr = 0;
	string addressStr = "0x2a8b6ca0";
	istringstream iniHit(addressStr.substr(2));//gets rid of 0x
	iniHit>>hex>>rowHitAddr;
	addressStr = "0x23588c0";
	istringstream iniMiss(addressStr.substr(2));//gets rid of 0x
	iniMiss>>hex>>rowMissAddr;
	currAddr = rowHitAddr;
}

Requestor::~Requestor()
{
	corePendingData.clear();
	// delete localCache;
	transFile.close();
}

void Requestor::connectMemoryController(MemoryController* memCtrl) {
	memoryController = memCtrl;
}

void Requestor::setMemoryClock(float clk) {
	memoryClock = clk;
}

void Requestor::sendRequest(Request* request) {
	bool R_W = true;
	if(request->requestType == DATA_WRITE) {
		R_W = false;
	}
	
	// /*********HIT RATIO CONTROL FUNCTION*/
	// if(requestorID > 0)
	// {
	// 	if(hitRatioCtr%2 != 0)
	// 	{
	// 		if(currAddr == rowHitAddr) {
	// 			//R_W = true;
	// 			currAddr = rowMissAddr;
	// 		}
	// 		else {
	// 			//R_W = false;
	// 			currAddr = rowHitAddr;
	// 		}
	// 	}
	// 	request->address = currAddr;
	// 	hitRatioCtr++;
	// }


	//What is the function of this add request?? what it actually returns?
	if(memoryController->addRequest(request->requestorID, request->address, R_W, request->requestSize))
	{
		requestRequest++;
		latency = 0;
		waitingRequest = true;
		request->arriveTime = currentClockCycle;
		corePendingData.push_back(request);	
	}
}

void Requestor::returnData(Request* returnTrans)
{
	if(!corePendingData.empty()) {
		
		for(unsigned index = 0; index < corePendingData.size(); index++) {
			if(returnTrans->address == corePendingData[index]->address) {
				if(currentClockCycle - corePendingData[index]->arriveTime > wcLatency) {
					wcLatency = currentClockCycle - corePendingData[index]->arriveTime;
				}
				// DEBUG(requestorID<<" RECIEVE ACK Arv: "<<corePendingData[index]->arriveTime
				// 	<<" Comp "<<currentClockCycle - corePendingData[index]->arriveTime);
				prevComplete = currentClockCycle;
				completeRequest++;
				latency = 0;
				waitingRequest = false;
				// *** Deallocation
				delete corePendingData[index];
				corePendingData.erase(corePendingData.begin() + index);
				break;
			}
		}
		if(latency != 0) {
			DEBUG("REQUEST NOT FOUND");
			abort();
		}
	}	
}

void Requestor::update()
{
	latency++;
	if(wcLatency >= 1000 && requestorID == 0) {
		DEBUG("worst case @ "<<currentClockCycle<<" REQ"<<requestorID<<" == "<<wcLatency);
		abort();
	}
	sim_done = readingTraceFile();
	if(pendingRequest != NULL) {
		
		if(pendingRequest->arriveTime <= currentClockCycle) {

			sendRequest(pendingRequest);
			
			pendingRequest = NULL;
		}
	}
	if(!corePendingData.empty()){
		sim_done = false;
	}
	currentClockCycle++;
}

bool Requestor::sim_end(){
	if(sim_done) {
		DEBUG("REQ"<<requestorID<<" Complete");
		transFile.close();
	}
	return sim_done;
}

void Requestor::printResult() 
{
	PRINT(requestorID<<" WC "<<wcLatency-1<<" done "<<completeRequest<<" request "<<requestRequest
		<<" compTime: "<<compTime<<" @ "<<memoryClock);
}

bool Requestor::readingTraceFile()
{
	string transLine;
	uint64_t addr;
	uint64_t clockCycle;
	uint64_t compDelay;
	enum RequestType requestType;

	if(inOrder && !corePendingData.empty()) {
		return false;
	}
	if(pendingRequest == NULL) {
		if(!transFile.eof()) {
			getline(transFile, transLine);
			if (transLine.size() > 0) {
				parseTraceFileLine(transLine, addr, requestType, compDelay, clockCycle);
				pendingRequest = new Request(requestorID, requestType, RequestSize, addr, NULL);
				pendingRequest->arriveTime = clockCycle;
			}			
		}
		else {
			return true;
		}
	}
	return false;
}

void Requestor::parseTraceFileLine(string &line, uint64_t &addr, enum RequestType &requestType, uint64_t &compDelay, uint64_t &clockCycle)
{
	size_t previousIndex=0;
	size_t spaceIndex=0;
	string addressStr="", cmdStr="", dataStr="", ccStr="", idStr="", delayStr="";
	spaceIndex = line.find_first_of(" ", 0);

	//Address Decoding
	spaceIndex = line.find_first_not_of(" ", previousIndex);
	addressStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);
	istringstream b(addressStr.substr(2));//gets rid of 0x
	b>>hex>>addr;
	previousIndex = line.find_first_of(" ", spaceIndex);

	//Command Decoding
	spaceIndex = line.find_first_not_of(" ", previousIndex);
	cmdStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);

	if (cmdStr.compare("WRITE")==0) {requestType = DATA_WRITE;}
	else if (cmdStr.compare("READ")==0) {requestType = DATA_READ;}
	else {exit(0); }
	previousIndex = line.find_first_of(" ", spaceIndex);

	//Arrival Time Decoding
	spaceIndex = line.find_first_not_of(" ", previousIndex);
	ccStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);
	istringstream c(ccStr);

	c>>compDelay;
	// Important for the traces
	compDelay = (int)(compDelay/memoryClock);
	
	if(inOrder) {
		compTime = compTime + compDelay;
		clockCycle = compDelay + prevComplete;	
	}
	else {
		clockCycle = compDelay + prevArrive;
		prevArrive = clockCycle;
	}
}


