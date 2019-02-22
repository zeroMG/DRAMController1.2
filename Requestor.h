
/**********************************
*	2016-1-14
*	Core characteristics
*	MG
***********************************/

#ifndef REQUESTOR_H
#define REQUESTOR_H

#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>

#include "Request.h"

using namespace std;

namespace DRAMController
{
	class MemoryController;

	class Cache
	{
	public:
		void add(unsigned long address, unsigned int size, void* data)
		{
			cache = std::make_pair(address, data);
			lowAddr = address;
			highAddr = address + size;
		}
		bool find(unsigned long address)
		{
			// if(address > lowAddr && address < highAddr) {return true;}
			// else {return false;}
			return false;
		}
	private:
		unsigned long lowAddr;
		unsigned long highAddr;
		std::pair<unsigned long, void*> cache;
	};

	class Requestor
	{
	public:
		Requestor(int id, bool inOrder, const string& traceFile);
		virtual ~Requestor();

		void connectMemoryController(MemoryController* memCtrl);
		void setMemoryClock(float clk);
		void sendRequest(Request* request);
		void returnData(Request* returnData);
		bool sim_end();
		void update();

		void printResult();

		float memoryClock;
		unsigned RequestSize;

	private:
		unsigned requestorID;
		bool inOrder;
		MemoryController* memoryController;
		Cache* localCache;
		unsigned long wcLatency;
		unsigned long compTime;
		int oppcheck;


		ifstream transFile;
		unsigned currentClockCycle;
		unsigned prevArrive;
		unsigned prevComplete;

		unsigned long requestRequest;
		unsigned long completeRequest;
		unsigned long latency;
		bool waitingRequest;
		ofstream outTrace;

		Request* pendingRequest;
		std::vector< Request* > corePendingData;
			
		bool sim_done;
		bool readingTraceFile();
		void parseTraceFileLine(string &line, uint64_t &addr, enum RequestType &requestType, 
			uint64_t &compDelay, uint64_t &clockCycle);

		unsigned int hitRatioCtr;
		unsigned long rowHitAddr;
		unsigned long rowMissAddr;
		unsigned long currAddr;
	};
}

#endif




