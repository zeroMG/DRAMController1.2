/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*	This is the memory system and we have add Ramulator into this platform
*********************************************************************************/

#ifndef MEMORYSYSTEM_H
#define MEMORYSYSTEM_H

#include <string>
#include <map>
#include <vector>
#include <queue>
#include <fstream>

#include "BusPacket.h"

namespace DRAMController
{
	class MemoryController;
	class MemorySystem
	{
	public:
		MemorySystem(unsigned int ranks);
		virtual ~MemorySystem();
		void connectMemoryController(MemoryController* memCtlr);


		void update();
		unsigned int get_DataBus();
		unsigned int get_Rank();
		unsigned int get_BankGroup();
		unsigned int get_Bank();
		unsigned long get_Row();
		unsigned long get_Column();
		// These functions are coming from Ramulator
		virtual float get_constraints(const std::string& parameter) = 0;
		virtual long command_timing(BusPacket* command) = 0;
		virtual unsigned int command_timing(BusPacket* command, int type) = 0;
		virtual bool command_check(BusPacket* command) = 0;
		virtual void receiveFromBus(BusPacket* busPacket) = 0;

	protected:
		unsigned int clockCycle;
		unsigned int ranks;
		unsigned int bankGroups;
		unsigned int banks;
		unsigned int subArrays;
		unsigned long rows;
		unsigned long columns;

		unsigned int dataBusWidth;
		MemoryController* memoryController;

		//what is the role of the post buffer and pendingReadData???
		std::vector<unsigned int> dataCycles;
		std::vector<BusPacket*> pendingReadData;
		
		typedef std::map<unsigned int, std::map<unsigned int, long> > dataArray;
		std::vector< std::vector< dataArray > > memoryArray;

		std::vector<BusPacket*> postBuffer;
		std::vector<int> postCounter;
		
		void* readDataArray(BusPacket* load);
		void updateDataArray(BusPacket* store);

		void generateData(BusPacket* cmd);

		std::ofstream commandTrace;
	};
}

#endif