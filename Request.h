/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#ifndef REQUEST_H
#define REQUEST_H

#include <stdint.h>
#include <iostream>
#include <map>

// #include "BusPacket.h"

namespace DRAMController
{
	enum RequestType
	{
		DATA_READ,
		DATA_WRITE,
		RETURN_DATA
	};

	class Request
	{
	public:
		Request(unsigned int id, RequestType requestType, unsigned int size, unsigned long long addr, void *data):
			requestType(requestType),
			requestorID(id),
			requestSize(size),
			address(addr),
			data(data),
			returnTime(0)
		{}
		//fields
		RequestType requestType;
		unsigned int requestorID;
		unsigned int requestSize;
		unsigned long long address;
		void *data;
		unsigned int arriveTime;
		unsigned int returnTime;
		unsigned int rank;
		unsigned int bankGroup;
		unsigned int bank;
		unsigned int subArray;
		unsigned int row;
		unsigned int col;

		// Rank, BankGroup, Bank, SubArray
		unsigned int addressMap[4];
	};

}

#endif

