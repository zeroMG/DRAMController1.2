/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#ifndef MEMORYCONTROLLER_H
#define MEMORYCONTROLLER_H

#include <stdint.h>
#include <cstdlib>
#include <string>
#include <map>
#include <functional>

#include "global.h"

#include "Request.h"
#include "BusPacket.h"

#include "RequestQueue.h"
#include "CommandQueue.h"

#include "SchedulerRegister.h"
#include "AddressMapping.h"
#include "RequestScheduler.h"
#include "CommandGenerator.h"
#include "CommandScheduler.h"
// #include "RefreshMachine.h"

namespace DRAMController
{
	class MemorySystem;
	class MemoryController
	{
	public:
		// systemConfigFile includes info on memory channel and memory controller architecture
		MemoryController(const string& systemConfigFile, function<void(Request&)> callback);
		virtual ~MemoryController();
		void displayConfiguration();
		void setRequestor(unsigned int id, bool criticality);
		void connectMemorySystem(MemorySystem* memSys);
		bool addRequest(unsigned int requestorID, unsigned long long address, bool R_W, unsigned int size);
		void receiveData(BusPacket *bpacket);
		void step();

		void printResult();
		void trackError(int id);
		
	private:
		// General Information
		unsigned long clockCycle;
		// information on memory structure
		unsigned int memTable[Column+1];
		// return served requests back to requestors
		function<void(Request&)> callback; 
		// number of pins connect to the memory channel
		unsigned int dataBusWidth;

		bool enqueueCommand(BusPacket* command);

		// Bus Traffic
		Request* incomingRequest;
		BusPacket* outgoingData;
		BusPacket* outgoingCmd;
		void sendData(BusPacket* bpacket);

		// Memory Controller Configuration (scheduler name and memory version)
		std::map<std::string, std::string> configTable; 
		std::map<unsigned int, bool> requestorCriticalTable;
		MemorySystem* memorySystem;
		
		// Hardware Components
		SchedulerRegister* schedulerRegister;
		AddressMapping* addressMapping;
		RequestScheduler* requestScheduler;
		CommandGenerator* commandGenerator;
		CommandScheduler* commandScheduler;

		// Request and Command Queues
		vector<RequestQueue*> requestQueue;
		vector<unsigned int> reqMap;
		bool writeQueueEnable;
		bool requestorReqQ;
		vector<CommandQueue*> commandQueue;
		vector<unsigned int> cmdMap;
		bool requestorCmdQ;
						
		// Request Buffer
		vector< Request* > pendingReadRequest;
		vector< Request* > pendingWriteRequest;
		// Data Buffer
		vector< unsigned > sendDataCounter;
		vector< BusPacket* > sendDataBuffer;

		// Read Value from Configuration File
		void readConfigFile(const string& filename);
		void SetKey(string key, string valueString);
		// Queue Structure
		bool isNumeric(const std::string& input);
		unsigned int convertNumber(const std::string& input);
		void setQueue(const std::string& queueOrg, vector<unsigned int>& queueMap);
		unsigned int queueNumber(vector<unsigned int>& queueMap);
		unsigned int decodeQueue(const unsigned int (&addressMap)[4], vector<unsigned int>& queueMap);

		// Statistics
		struct Statistics{
			unsigned long long totalRequest;
			unsigned long long openRead;
			unsigned long long closeRead;
			unsigned long long openWrite;
			unsigned long long closeWrite;
			
			unsigned long long readBytes;
			unsigned long long writeBytes;
			unsigned long long open;		//total requests
			unsigned long long close;		//close requests
			bool closeRequest;
			unsigned long long open_counter;
			unsigned long long close_counter;
			unsigned long long open_Latency;
			unsigned long long close_Latency;
		} stats;

		// requests traces generator
		ofstream myTrace;
	};
}

#endif

