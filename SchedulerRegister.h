/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#include "RequestScheduler.h"
#include "CommandGenerator.h"
#include "CommandScheduler.h"

// General Scheduler
#include "CommonScheduler/RequestScheduler_DIRECT.h"
#include "CommonScheduler/RequestScheduler_RR.h"

#include "CommonScheduler/CommandGenerator_OPEN.h"
//#include "CommonScheduler/CommandGenerator_CLOSE.h"

#include "CommonScheduler/CommandScheduler_FCFS.h"

//Round

#include "system/Round/CommandScheduler_Round.h"

// AMC
// RTMem
//#include "system/RTMem/RequestScheduler_RTMem.h"
//#include "system/RTMem/CommandScheduler_RTMem.h"

// PMC
//#include "system/PMC/RequestScheduler_PMC.h"
// #include "system/PMC/CommandGenerator_PMC.h"

// ORP
//#include "system/ORP/CommandScheduler_ORP.h"

// // MEDUSA
// #include "system/MEDUSA/RequestScheduler_MEDUSA.h"
// #include "system/MEDUSA/CommandGenerator_MEDUSA.h"

// // DCmc
//#include "system/DCmc/CommandScheduler_DCmc.h"

// MAG
//#include "system/MAG/CommandScheduler_MAG.h"

// ReOrder
//#include "system/ReOrder/CommandScheduler_ReOrder.h"

// ROC
//#include "system/ROC/CommandScheduler_ROC.h"

// MCMC
//#include "system/MCMC/RequestScheduler_MCMC.h"
//#include "system/MCMC/CommandGenerator_MCMC.h"

// RankReOrder
//#include "system/RankReOrder/CommandScheduler_rankReOrder.h"

// RankReOrder
// #include "system/PipeCAS/RequestScheduler_PipeCAS.h"
//#include "system/PipeCAS/CommandGenerator_PipeCAS.h"
//#include "system/PipeCAS/CommandScheduler_PIPECAS.h"

#include "BusPacket.h"

namespace DRAMController
{
	class SchedulerRegister
	{
	public:
		SchedulerRegister(unsigned int dataBus, const map<unsigned int, bool>& requestorTable, 
			vector<RequestQueue*>& requestQueues, vector<CommandQueue*>& commandQueues)
		{
			// User define the initialized scheduler and components for easy access
			requestSchedulerTable["DIRECT"] = new RequestScheduler_Direct(requestQueues, commandQueues, requestorTable);
			requestSchedulerTable["RR"] = new RequestScheduler_RR(requestQueues, commandQueues, requestorTable, dataBus);
			commandGeneratorTable["OPEN"] = new CommandGenerator_Open(dataBus);
			//commandGeneratorTable["CLOSE"] = new CommandGenerator_Close(dataBus);
			commandSchedulerTable["FCFS"] = new CommandScheduler_FCFS(commandQueues, requestorTable);

			//requestSchedulerTable["RTMem"] = new RequestScheduler_RTMem(requestQueues, commandQueues, requestorTable, dataBus);
			//commandSchedulerTable["RTMem"] = new CommandScheduler_RTMem(commandQueues, requestorTable);

			//requestSchedulerTable["PMC"] = new RequestScheduler_PMC(requestQueues, commandQueues, requestorTable);
			// commandGeneratorTable["PMC"] = new CommandGenerator_PMC(dataBus, commandQueues);
			
			commandSchedulerTable["Round"] = new CommandScheduler_Round(commandQueues, requestorTable);
			//commandSchedulerTable["ORP"] = new CommandScheduler_ORP(commandQueues, requestorTable);
			//commandSchedulerTable["DCmc"] = new CommandScheduler_DCmc(commandQueues, requestorTable);
			//commandSchedulerTable["MAG"] = new CommandScheduler_MAG(commandQueues, requestorTable);
			//commandSchedulerTable["ReOrder"] = new CommandScheduler_ReOrder(commandQueues, requestorTable);
			//commandSchedulerTable["ROC"] = new CommandScheduler_ROC(commandQueues, requestorTable);

			// requestSchedulerTable["MEDUSA"] = new RequestScheduler_MEDUSA(requestQueues, requestorTable);
			// commandGeneratorTable["MEDUSA"] = new CommandGenerator_MEDUSA(dataBus, commandQueues);

			//requestSchedulerTable["MCMC"] = new RequestScheduler_MCMC(requestQueues, commandQueues, requestorTable, dataBus);
			//commandGeneratorTable["MCMC"] = new CommandGenerator_MCMC(dataBus);

			//commandGeneratorTable["PipeCAS"] = new CommandGenerator_PipeCAS(dataBus, requestorTable);
			//commandSchedulerTable["PipeCAS"] = new CommandScheduler_PIPECAS(commandQueues, requestorTable, 0);

			//commandSchedulerTable["rankReOrder"] = new CommandScheduler_rankReOrder(commandQueues, requestorTable);
		}

		~SchedulerRegister()
		{
			for(auto it=requestSchedulerTable.begin(); it!=requestSchedulerTable.end(); it++) {
				// DEBUG("RS Deleting "<<it->first);
				delete it->second;
			}
			requestSchedulerTable.clear();
			for(auto it=commandSchedulerTable.begin(); it!=commandSchedulerTable.end(); it++) {
				// DEBUG("CS Deleting "<<it->first);
				delete it->second;
			}
			commandSchedulerTable.clear();
			for(auto it=commandGeneratorTable.begin(); it!=commandGeneratorTable.end(); it++) {
				// DEBUG("CG Deleting "<<it->first);
				delete it->second;
			}
			commandGeneratorTable.clear();
		}

		RequestScheduler* getRequestScheduler(string schedulerName) {
			if(requestSchedulerTable.find(schedulerName) == requestSchedulerTable.end()) {
				DEBUG("No Such REQScheduler "<<schedulerName);
				abort();
			}
			return requestSchedulerTable[schedulerName];
		}
		CommandGenerator* getCommandGenerator(string schedulerName) {
			if(commandGeneratorTable.find(schedulerName) == commandGeneratorTable.end()) {
				DEBUG("No Such CMDgen");
				abort();
			}			
			return commandGeneratorTable[schedulerName];
		}	
		CommandScheduler* getCommandScheduler(string schedulerName) {
			if(commandSchedulerTable.find(schedulerName) == commandSchedulerTable.end()) {
				DEBUG("No Such CMDScheduler");
				abort();
			}			
			return commandSchedulerTable[schedulerName];
		}
		
	private:
		std::map<std::string, RequestScheduler*> requestSchedulerTable;
		std::map<std::string, CommandGenerator*> commandGeneratorTable;
		std::map<std::string, CommandScheduler*> commandSchedulerTable;
	};
}


