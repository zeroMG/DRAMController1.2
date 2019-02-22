#ifndef COMMANDSCHEDULER_FCFS_H
#define COMMANDSCHEDULER_FCFS_H

#include "../CommandScheduler.h"

namespace DRAMController
{
	class CommandScheduler_FCFS: public CommandScheduler
	{
	private:

	public:
		CommandScheduler_FCFS(vector<CommandQueue*>& commandQueues, const map<unsigned int, bool>& requestorTable):
			CommandScheduler(commandQueues, requestorTable)
		{}

		BusPacket* commandSchedule()
		{
			scheduledCommand = NULL;
			checkCommand = NULL;
			for(unsigned int index = 0; index < commandQueue.size(); index++)
			{
				if(commandQueue[index]->getSize(true) > 0)
				{
					checkCommand = commandQueue[index]->getCommand(true);
				}
				if(checkCommand != NULL) 
				{
					if(isIssuable(checkCommand)){
						scheduledCommand = checkCommand;
						sendCommand(scheduledCommand, index);
						break;
					}
				}
			}
			return scheduledCommand;
		}
	};
}

#endif
