/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#ifndef COMMANDGENERATOR_H
#define COMMANDGENERATOR_H

#include <queue>

#include "Request.h"
#include "BusPacket.h"
#include "CommandQueue.h"

namespace DRAMController
{
	class CommandGenerator
	{
	public:
		CommandGenerator(unsigned int dataBus);
		virtual ~CommandGenerator();

		virtual bool commandGenerate(Request* request, bool open) = 0;

		BusPacket* getCommand();
		void removeCommand();
		unsigned int bufferSize();

	protected:
		unsigned int dataBusSize;
		// size => [BI, BC]
		std::map<unsigned int, std::pair<unsigned int, unsigned int>>lookupTable;

		std::queue< BusPacket* > commandBuffer;
	};
}

#endif
