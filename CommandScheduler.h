/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#ifndef _COMMANDSCHEDULER_H
#define _COMMANDSCHEDULER_H

#include "BusPacket.h"
#include "CommandQueue.h"
#include "RefreshMachine.h"

#include <map>
#include <utility>
#include <vector>

namespace DRAMController
{
	class MemorySystem;
	class CommandScheduler
	{
	public:
		CommandScheduler(vector<CommandQueue*>& commandQueues, const std::map<unsigned int, bool>& requestorTable);
		virtual ~CommandScheduler();
		void connectMemorySystem(MemorySystem* memSys);
		// Scheduling Algoirthm
		virtual BusPacket* commandSchedule() = 0;
		void commandClear();
		// Indicator of reaching refresh interval
		void refresh();
		// Perform refresh
		bool refreshing();
		// next clk cycle
		void tick();
		
	protected:
		unsigned long clock;
		MemorySystem* memorySystem;
		RefreshMachine* refreshMachine;
		
		std::vector<CommandQueue*>& commandQueue;
		std::map<unsigned int, std::map<unsigned int, unsigned int> > cmdQueueTimer;
		std::vector<std::map<unsigned int, std::map<unsigned int, unsigned int>> > reqCmdQueueTimer;
		const std::map<unsigned int, bool>& requestorCriticalTable;	

		// get timing constraint from memory system interface
		unsigned int getTiming(const string& param);
		// check if a command satisfy the timing constraint on its own queue
		bool isReady(BusPacket* cmd, unsigned int index);
		// check if a command can be issued
		bool isIssuable(BusPacket* cmd);
		// issue the command
		void sendCommand(BusPacket* cmd, unsigned int index);

		// compute ready cycles
		// unsigned int getReady(BusPacket* command, int type);
		
		BusPacket* scheduledCommand;
		BusPacket* checkCommand;
		BusPacket* checkCommand_1;
		bool count_ACT;
		bool count_CAS;
		//BusPacket* checkCommand1;
		bool continiue;
		bool skipCAS;
		unsigned int ranks;
		unsigned int banks;

	};
}

#endif
