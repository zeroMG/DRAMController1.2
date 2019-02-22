/*********************************************************************************
*  Copyright (c) 2018-2019, 
*  Reza Mirosanlou
*  University of Waterloo               
*  All rights reserved.
*********************************************************************************/
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm> 

#include "MemoryController.h"
#include "MemorySystem.h"

using namespace DRAMController;

MemoryController::MemoryController(const string& systemConfigFile, function<void(Request&)> callback):
callback(callback)
{
	// assign values to parameters based on configuration file
	readConfigFile(systemConfigFile);

	//Initialize flow control
	clockCycle = 0;
	incomingRequest = NULL;
	outgoingData = NULL;
	outgoingCmd = NULL;

	// Initialize statistic info
	// ============ Stats Tracker ===================
	stats.totalRequest = 0;
	stats.openRead = 0;
	stats.closeRead = 0;
	stats.openWrite = 0;
	stats.closeWrite = 0;

	stats.readBytes = 0;
	stats.writeBytes = 0;
	stats.close = 0;
	stats.open = 0;

	stats.closeRequest = true;
	stats.open_counter = 0;
	stats.close_counter = 0;
	stats.open_Latency = 0;
	stats.close_Latency = 0;

	myTrace.open ("trace.txt");
	memorySystem = NULL;
}

MemoryController::~MemoryController()
{
	// Delete Schedulers
	delete schedulerRegister;
	// Delete Queues 
	for(auto it=requestQueue.begin(); it!=requestQueue.end(); it++) {
		delete (*it);
	}
	requestQueue.clear();
	for(auto it=commandQueue.begin(); it!=commandQueue.end(); it++) {
		delete (*it);
	}
	commandQueue.clear();

	pendingReadRequest.clear();
	pendingWriteRequest.clear();

	myTrace.close();
}

void MemoryController::setRequestor(unsigned int id, bool criticality) {
	requestorCriticalTable[id] = criticality;
}

void MemoryController::connectMemorySystem(MemorySystem* memSys) {
	// information on memory structure
	memorySystem = memSys;
	memTable[Rank] = memorySystem->get_Rank();
	memTable[BankGroup] = memorySystem->get_BankGroup();
	memTable[Bank] = memorySystem->get_Bank();
	memTable[Row] = memorySystem->get_Row();
	memTable[Column] = memorySystem->get_Column();

	dataBusWidth = memorySystem->get_DataBus();
	// construct the request and command queues
	unsigned int queueNum = queueNumber(reqMap);
	if(queueNum != 0) {
		for(unsigned int index=0; index<queueNum; index++) {
			requestQueue.push_back(new RequestQueue(requestorReqQ, writeQueueEnable));
		}		
	}
	queueNum = queueNumber(cmdMap);
	if(queueNum != 0) {
		for(unsigned int index=0; index<queueNum; index++) {
			commandQueue.push_back(new CommandQueue(requestorCmdQ));
		}
	}

	// Configured Scheduler based on configuration table
	schedulerRegister = new SchedulerRegister(dataBusWidth, requestorCriticalTable, requestQueue, commandQueue);
	addressMapping = new AddressMapping(configTable["AddressMapping"], memTable);
	requestScheduler = schedulerRegister->getRequestScheduler(configTable["RequestScheduler"]);
	commandGenerator = schedulerRegister->getCommandGenerator(configTable["CommandGenerator"]);
	commandScheduler = schedulerRegister->getCommandScheduler(configTable["CommandScheduler"]);

	requestScheduler->connectCommandGenerator(commandGenerator);
	commandScheduler->connectMemorySystem(memSys);
}

bool MemoryController::addRequest(unsigned int requestorID, unsigned long long address, bool R_W, unsigned int size)
{
	string type = "write";
	if(R_W) {type = "read";}
	myTrace<< clockCycle <<","<< type <<","<<address<<","<<"16,"<< requestorID <<"\n";

	RequestType requestType = DATA_READ;
	if(!R_W) {
		requestType = DATA_WRITE;
	}
	incomingRequest = new Request(requestorID, requestType, size, address, NULL);
	incomingRequest->arriveTime = clockCycle;

	//Here the address mapping will occures

	addressMapping->addressMapping(incomingRequest);

	// ============ Stats Tracker ===================
	//Why request ID is checked here?
	if(incomingRequest->requestorID == 0) {
		stats.totalRequest++;
	}
	// Decode the request queue index
	unsigned int queueIndex = decodeQueue(incomingRequest->addressMap, reqMap); //true indicate requestQueue
	if(requestQueue[queueIndex]->insertRequest(incomingRequest))
	{
		if(incomingRequest->requestType == DATA_READ) {
			pendingReadRequest.push_back(incomingRequest);	// pending for return Data
		}
		else {
			pendingWriteRequest.push_back(incomingRequest);	// pending for write acknowledgement
		}		
	}
	incomingRequest = NULL;
	return true;
}

bool MemoryController::enqueueCommand(BusPacket* command)
{
	// ============ Stats Tracker ===================

	if(command->requestorID == 0){
		if(command->busPacketType == ACT_R) {stats.closeRequest = true;}
		else if (command->busPacketType == ACT_W) {stats.closeRequest = true;}
		else if (command->busPacketType < ACT_R) {
			if(stats.closeRequest) {
				stats.close_counter = 0;
				if(command->busPacketType == RD) {stats.closeRead++;}
				else {stats.closeWrite++;}
			}	
			else {
				stats.open_counter = 0;
				if(command->busPacketType == RD) {stats.openRead++;}
				else {stats.openWrite++;}
			}
		}
	}



	command->addressMap[Rank] = command->rank;
	command->addressMap[Bank] = command->bank;
	command->addressMap[BankGroup] = 0;//command->bankGroup;
	command->addressMap[SubArray] = 0;//command->subArray;
	command->arriveTime = clockCycle;

	unsigned int queueIndex = decodeQueue(command->addressMap, cmdMap); // false indicate commandQueue
	return commandQueue[queueIndex]->insertCommand(command, requestorCriticalTable[command->requestorID]);
}

void MemoryController::step()
{
	requestScheduler->requestSchedule();
	while(commandGenerator->bufferSize() > 0) {
		if(enqueueCommand(commandGenerator->getCommand())) {
			commandGenerator->removeCommand();	
		}
		else {
			DEBUG("MC : command not inserted to queues");
			abort();
		}
	}

	if(!commandScheduler->refreshing())
	{
		outgoingCmd = commandScheduler->commandSchedule();
		if(outgoingCmd != NULL)
		{
			if(outgoingCmd->requestorID == 0 && outgoingCmd->busPacketType <= ACT_R)
			{
				// DEBUG(clockCycle<<" MC : send REQ"<<outgoingCmd->requestorID<<" CMD"<<outgoingCmd->busPacketType<<" R:"<<outgoingCmd->rank<<" B:"<<outgoingCmd->bank);
			}
			DEBUG(clockCycle<<" MC : send REQ"<<outgoingCmd->requestorID<<" CMD"<<outgoingCmd->busPacketType<<" R:"<<outgoingCmd->rank<<" B:"<<outgoingCmd->bank);
			if (outgoingCmd->busPacketType == WR || outgoingCmd->busPacketType == WRA) {
				sendDataBuffer.push_back(new BusPacket(DATA, outgoingCmd->requestorID, outgoingCmd->address, outgoingCmd->column,
				                                    outgoingCmd->row, outgoingCmd->bank, outgoingCmd->rank, outgoingCmd->data, clockCycle));
				if(outgoingCmd->postCommand) {
					// additive latency (AL) is tRCD - 1
					sendDataCounter.push_back(memorySystem->get_constraints("tRCD") - 1 + memorySystem->get_constraints("tWL")+memorySystem->get_constraints("tBus"));
				}
				else {
					sendDataCounter.push_back(memorySystem->get_constraints("tWL")+memorySystem->get_constraints("tBus"));
				}
			}
			// commandScheduler->commandClear();
		}
		delete outgoingCmd;
		outgoingCmd = NULL;
	}
	else {
		// If there is no more pending data to be sent out, start refreshing
		if(sendDataBuffer.empty() && outgoingData == NULL) {
			commandScheduler->refresh();
			DEBUG("REFRESHING");
			exit(1);
		}
	}

	if (sendDataCounter.size() > 0) {
		if (sendDataCounter.front() == 0) {
			outgoingData = sendDataBuffer.front();
			sendData(outgoingData);
			// *********** -- *************
			delete outgoingData;
			outgoingData=NULL;
			sendDataCounter.erase(sendDataCounter.begin());
			sendDataBuffer.erase(sendDataBuffer.begin());
		}
		for (unsigned int i=0;i<sendDataCounter.size();i++) {
			sendDataCounter[i]--;
		}
	}

	// step through the scheduler components
	// requestScheduler->step();
	commandScheduler->tick();
	clockCycle++;

	// ============ Stats Tracker ===================
	//What the fuck are these for????
	stats.close_counter++;
	stats.open_counter++;
}

void MemoryController::receiveData(BusPacket *bpacket)
{
	stats.readBytes+=dataBusWidth;
	cout<<"the command is ******************************************  "<<bpacket->busPacketType<<endl;
	// Checking with pending request
	for(unsigned int index=0; index < pendingReadRequest.size(); index++) {
		if(bpacket->requestorID == pendingReadRequest[index]->requestorID && 
			bpacket->address == pendingReadRequest[index]->address)
		{
			if(pendingReadRequest[index]->requestSize == dataBusWidth) {
				if(bpacket->requestorID == 0) {
					// DEBUG("	MC : Receive Data $"<<bpacket->requestorID<<" @ "<<clockCycle);
				}
				
				callback(*pendingReadRequest[index]);

				// ============ Stats Tracker ===================
				if(bpacket->requestorID == 0) {
					if(stats.closeRequest) {
						if(stats.close_Latency < stats.close_counter) {
							stats.close_Latency = stats.close_counter;
							
						}
						stats.closeRequest = false;
					}
					else {
						if(stats.open_Latency < stats.open_counter) {
							stats.open_Latency = stats.open_counter;
						}
					}
				}

				// *** Deallocation
				delete pendingReadRequest[index];
				pendingReadRequest.erase(pendingReadRequest.begin()+index);
			}
			else {
				pendingReadRequest[index]->requestSize -= dataBusWidth;
			}
			break;			
		}
	}
}

void MemoryController::sendData(BusPacket* bpacket) 
{
	DEBUG(clockCycle<<" MC : send REQ"<<bpacket->requestorID<<" DATA"<<bpacket->busPacketType);//<<" R:"<<bpacket->rank<<" B:"<<bpacket->bank);
	memorySystem->receiveFromBus(bpacket);
	stats.writeBytes += dataBusWidth;

	for(unsigned int index=0; index < pendingWriteRequest.size(); index++) {
		if(bpacket->requestorID == pendingWriteRequest[index]->requestorID && 
			bpacket->address == pendingWriteRequest[index]->address) 
		{
			if(pendingWriteRequest[index]->requestSize == dataBusWidth) {
				callback(*pendingWriteRequest[index]);

				// ============ Stats Tracker ===================
				if(bpacket->requestorID == 0) {
					if(stats.closeRequest) {
						if(stats.close_Latency < stats.close_counter) {
							stats.close_Latency = stats.close_counter;
						}
						stats.closeRequest = false;
					}
					else {
						if(stats.open_Latency < stats.open_counter) {
							stats.open_Latency = stats.open_counter;
						}
					}
				}				
				// **** Deallocation
				delete pendingWriteRequest[index];
				pendingWriteRequest.erase(pendingWriteRequest.begin()+index);
			}
			else {
				pendingWriteRequest[index]->requestSize -= dataBusWidth;
			}
			break;
		}
	}
}

void MemoryController::trackError(int id)
{
	DEBUG("RQ: "<<requestQueue[id]->getSize(true,0)<<" CQ: "<<commandQueue[id]->getSize(true));
}

void MemoryController::printResult()
{
	PRINT("REQ0: Open Request Ratio = "<<(float)(stats.openRead+stats.openWrite)/(stats.openRead+stats.openWrite+stats.closeRead+stats.closeWrite));
	PRINT("REQ0: WC Open "<<stats.open_Latency<<"	"<<"WC Close "<<stats.close_Latency);
	PRINT("Summary: Bandwidth = "<<(float)(stats.readBytes+stats.writeBytes)/clockCycle*1000<<" MB/s");
	PRINT("Summary: WC OpenRead "<<stats.openRead<<"	"<<"CloseRead "<<stats.closeRead<<" OpenWrite "<<stats.openWrite<<"	"<<"CloseWrite "<<stats.closeWrite);
}

void MemoryController::displayConfiguration()
{
	DEBUG("Configuration Summary");
	DEBUG("Rank "<<memTable[Rank]);
	DEBUG("Bank "<<memTable[Bank]);
	DEBUG("Row "<<memTable[Row]);
	DEBUG("Col "<<memTable[Column]);
	DEBUG("	Address Mapping 	: "<<configTable["AddressMapping"]);
	DEBUG("	Request Queue 		: "<<requestQueue.size()<<" perREQ "<<requestorReqQ<<" WBuffer "<<writeQueueEnable);
	DEBUG("	Request Scheduler 	: "<<configTable["RequestScheduler"]);
	DEBUG("	Command Generator 	: "<<configTable["CommandGenerator"]);
	DEBUG("	Command Queue 		: "<<commandQueue.size()<<" perREQ "<<requestorCmdQ);
	DEBUG("	Command Scheduelr 	: "<<configTable["CommandScheduler"]);
}

// Read Value from Configuration File
void MemoryController::readConfigFile(const string& filename)
{
	std::ifstream configFile;
	string line, key, valueString;
	size_t commentIndex, equalsIndex;
	
	configFile.open(filename.c_str());
	size_t lineNumber = 0;
	if(configFile.is_open()) {
		while (!configFile.eof()) {
			lineNumber++;
			getline(configFile, line);
			// skip zero-length lines
			if (line.size() == 0) { continue; }
			//search for a comment char
			if ((commentIndex = line.find_first_of(";")) != string::npos) {
				//if the comment char is the first char, ignore the whole line
				if (commentIndex == 0) { continue; }
				//truncate the line at first comment before going on
				line = line.substr(0,commentIndex);
			}
			// trim off the end spaces that might have been between the value and comment char
			size_t whiteSpaceEndIndex;
			if ((whiteSpaceEndIndex = line.find_last_not_of(" \t")) != string::npos) {
				line = line.substr(0,whiteSpaceEndIndex+1);}
			// a line has to have an equals sign
			if ((equalsIndex = line.find_first_of("=")) == string::npos) {
				ERROR("Malformed Line "<<lineNumber<<" (missing equals)");
				abort();
			}
			size_t strlen = line.size();
			// all characters before the equals are the key
			key = line.substr(0, equalsIndex);
			// all characters after the equals are the value
			valueString = line.substr(equalsIndex+1,strlen-equalsIndex);
			SetKey(key, valueString);
		}
	}
	configFile.close();
}

void MemoryController::SetKey(string key, string valueString)
{
	// Request Queue Structure (if perRequestor or writeBuffer enabled)
	if(key == "RequestQueue") { setQueue(valueString, reqMap);}
	else if(key == "WriteQueue") { writeQueueEnable = convertNumber(valueString); } // 0 is false, 1 is true
	else if(key == "ReqPerREQ") { requestorReqQ = convertNumber(valueString); }

	// Command Queue Structure (if perRequestor enabled)
	else if(key == "CommandQueue"){ setQueue(valueString, cmdMap);}
	else if(key == "CmdPerREQ") { requestorCmdQ = convertNumber(valueString); }

	// Memory Controller Scheduler Identification
	else { configTable[key] = valueString;}	
}

unsigned int MemoryController::convertNumber(const std::string& input)
{
	unsigned int intValue = 0;
	std::istringstream ss(input);
	if(isNumeric(input)) { ss >> intValue;}
	else { cout<<"Wrong Operation"<<endl; }
	return intValue;
}

/*
unsigned int MemoryController::convertNumber(const std::string& input)
{
	unsigned int intValue = 0;
	std::istringstream ss(input);
	ss >> intValue;
	return intValue;
}
*/

bool MemoryController::isNumeric(const std::string& input)
{
	return std::all_of(input.begin(), input.end(), ::isdigit);
}

void MemoryController::setQueue(const std::string& queueOrg, vector<unsigned int>& queueMap)
{
	for(unsigned index=0; index<queueOrg.size(); index++) {
		if(queueOrg[index] == '1') { 
			queueMap.push_back(1); 
		}
		else { queueMap.push_back(0); }
	}
}

unsigned int MemoryController::queueNumber(vector<unsigned int>& queueMap)
{
	unsigned int queueCount = 1;
	for(unsigned int index=0; index<queueMap.size(); index++) {
		if(queueMap[index] == 1) {
			for(unsigned int n=0; n<=index; n++) {
				queueCount = queueCount * memTable[n]; // Each sub-level is a multiple of upper level
			}
			break;
		}
	}
	return queueCount;
}

unsigned int MemoryController::decodeQueue(const unsigned int (&addressMap)[4], vector<unsigned int>& queueMap)
{
	unsigned int queueIndex = 0;
	for(unsigned int index = 0; index < queueMap.size(); index++) {
		if(queueMap[index] == 1) {
			queueIndex = addressMap[index];
			unsigned int level = 0;
			for(unsigned int n=0; n< index; n++) {
				if(addressMap[n] > 0) {
					if(level == 0) {
						level = addressMap[n];
					}
					else {
						level = level * addressMap[n];
					}
				}
			}
			queueIndex = queueIndex + level*memTable[index];
			break;
		}
	}
	return queueIndex;	
}
