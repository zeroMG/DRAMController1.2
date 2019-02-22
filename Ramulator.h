/*********************************************************************************
*  Copyright (c) 2018-2019, 
*  Reza Mirosanlou
*  University of Waterloo               
*  All rights reserved.
*********************************************************************************/

#ifndef RAMULATOR_H
#define RAMULATOR_H

#include "DRAM/DRAM.h"
#include "DRAM/DDR3.h"
#include "DRAM/DDR4.h"
// #include "DRAM/DSARP.h"
// #include "DRAM/GDDR5.h"
// #include "DRAM/LPDDR3.h"
// #include "DRAM/LPDDR4.h"
// #include "DRAM/WideIO.h"
// #include "DRAM/WideIO2.h"
// #include "DRAM/HBM.h"
// #include "DRAM/SALP.h"
// #include "DRAM/ALDRAM.h"
// #include "DRAM/TLDRAM.h"

#include <queue>
#include "MemorySystem.h"

using namespace ramulator;
namespace DRAMController
{
	template <typename T>
	class Ramulator: public MemorySystem
	{	
	public:
		Ramulator(T* spec, unsigned int ranks): MemorySystem(ranks)
		{
			spec->set_rank_number(ranks);
			channel = new DRAM<T>(spec, T::Level::Channel);
      		// channel->regStats("");
			for(int index = 0; index < int(T::Level::MAX); index++) {
				addr_vec.push_back(-1);
			}

			if(int(T::Level::Bank) != int(T::Level::Rank)+1) {
				bankGroups = channel->spec->org_entry.count[int(T::Level::Bank)-1];
			}

			banks = channel->spec->org_entry.count[int(T::Level::Bank)];

			if(int(T::Level::Row) != int(T::Level::Bank)+1) {
				subArrays = channel->spec->org_entry.count[int(T::Level::Row)-1];
			}

			rows = channel->spec->org_entry.count[int(T::Level::Row)];
			columns = channel->spec->org_entry.count[int(T::Level::Column)];
			
			dataBusWidth = channel->spec->org_entry.dq;
		}

		float get_constraints(const string& parameter) 
		{
			float param = 0;
			if(parameter == "tCK") {
				param = channel->spec->speed_entry.tCK;
			}
			else if(parameter == "tCCD") {
				param = channel->spec->speed_entry.nCCD;
			}
			else if(parameter == "tRCD") {
				param = channel->spec->speed_entry.nRCD;
			}
			else if(parameter == "tWL") {
				param = channel->spec->speed_entry.nCWL;
			}
			else if(parameter == "tRL") {
				param = channel->spec->speed_entry.nCL;
			}
			else if(parameter == "tWTR") {
				param = channel->spec->speed_entry.nWTR; /// for DDR3 DEVICE
				// param = channel->spec->speed_entry.nWTRS;
			}
			else if(parameter == "tRTW") {
				param = channel->spec->speed_entry.nCL + channel->spec->speed_entry.nBL + 2 - channel->spec->speed_entry.nCWL;
			}
			else if (parameter == "tBus") {
				param = channel->spec->speed_entry.nBL;
			}
			else if (parameter == "tREFI") {
				param = channel->spec->speed_entry.nREFI;
			}
			else if (parameter == "tRFC") {
				param = channel->spec->speed_entry.nRFC;
			}
			else if (parameter == "tRRD") {
				param = channel->spec->speed_entry.nRRD;
			}
			else if (parameter == "tFAW") {
				param = channel->spec->speed_entry.nFAW;
			}
			else {
				param = 0;
			}
			return param;
		}

		unsigned int command_timing(BusPacket* command, int type) 
		{
			int time = 0;
			switch(command->busPacketType) 
			{
				case PRE:
					if(type == ACT_R) {
						time = channel->spec->speed_entry.nRP;
					}
					else if(type == ACT_W) {
						time = channel->spec->speed_entry.nRP;
					}
					break;
				case ACT_R:
					switch(type) {
						case ACT_R:
							time = channel->spec->speed_entry.nRC;
							break;
						case ACT_W:
							time = channel->spec->speed_entry.nRC;
							break;	
						case PRE:
							time = channel->spec->speed_entry.nRAS;
							break;
						case RD:
						case RDA:
						case WR:
						case WRA:
							time = channel->spec->speed_entry.nRCD;
							break;
						default:
							time = 1;
							break;
					}
					break;
				case ACT_W:
					switch(type) {
						case ACT_R:
							time = channel->spec->speed_entry.nRC;
							break;
						case ACT_W:
							time = channel->spec->speed_entry.nRC;
							break;	
						case PRE:
							time = channel->spec->speed_entry.nRAS;
							break;
						case RD:
						case RDA:
						case WR:
						case WRA:
							time = channel->spec->speed_entry.nRCD;
							break;
						default:
							time = 1;
							break;
					}
					break;
				case RD:
					switch(type) {
						case RD:
						case RDA:
							time = 4;
							break;
						case WR:
						case WRA:
							time = channel->spec->speed_entry.nCL + channel->spec->speed_entry.nBL 
							+ 2 - channel->spec->speed_entry.nCWL;
							break;
						case PRE:
							time = channel->spec->speed_entry.nRTP;
						default:
							time = 1;
							break;
					}	
					break;				
				case RDA:
					switch(type) {
						case ACT_R:
							time = channel->spec->speed_entry.nRTP + channel->spec->speed_entry.nRP;
							break;
						case ACT_W:
							time = channel->spec->speed_entry.nRTP + channel->spec->speed_entry.nRP;
							break;	
						default:
							time = 1;
							break;
					}
					break;
				case WR:
					switch(type) {
						case RD:
						case RDA:
							time = channel->spec->speed_entry.nCWL + 4 + 6;
							break;
						case WR:
						case WRA:
							time = 4;
							break;
						case PRE:
							time = channel->spec->speed_entry.nCWL + 4 + channel->spec->speed_entry.nWR;
						default:
							time = 1;
							break;
					}
					break;
				case WRA:
					switch(type) {
						case ACT_R:
							time = channel->spec->speed_entry.nCWL + 4 + channel->spec->speed_entry.nWR + 
							channel->spec->speed_entry.nRP;
							break;
						case ACT_W:
							time = channel->spec->speed_entry.nCWL + 4 + channel->spec->speed_entry.nWR + 
							channel->spec->speed_entry.nRP;
							break;							
						default:
							time = 1;
							break;
					}
					break;
				default:
					time = 1;
					break;
			}
			return time;
		}

		long command_timing(BusPacket* command) 
		{
			convert_addr(command);
			return channel->get_next(convert_cmd(command), addr_vec.data()) - clockCycle;
		}

		bool command_check(BusPacket* command) 
		{
			if(command->postCommand) {
				return true;
			}
			else {
				convert_addr(command);
				return channel->check(convert_cmd(command), addr_vec.data(), clockCycle);				
			}

		}

		void receiveFromBus(BusPacket* busPacket) 
		{
			if(busPacket->postCommand) {
				postBuffer.push_back(new BusPacket(busPacket->busPacketType, busPacket->requestorID, busPacket->address, 
					busPacket->column, busPacket->row, busPacket->bank, busPacket->rank, busPacket->data, busPacket->arriveTime));
				postCounter.push_back(get_constraints("tRCD"));
				return;
			}
			
			if(busPacket->busPacketType != DATA) {
				// DEBUG("== Ramulator B"<<busPacket->bank<<": receive "<<busPacket->busPacketType<<
				// 	" $"<<busPacket->requestorID<<" @ "<<clockCycle);				
				commandTrace<<clockCycle<<" B"<<busPacket->bank<<" "<<busPacket->busPacketType<<"\n";
				convert_addr(busPacket);
				channel->update(convert_cmd(busPacket), addr_vec.data(), clockCycle);
			}
			generateData(busPacket);
		}

	private:
		DRAM<T>* channel;
		vector<int> addr_vec;
		typename T::Command convert_cmd(BusPacket* command) 
		{
			switch(command->busPacketType) 
			{
			case RD:
				return T::Command::RD;
			case WR:
				return T::Command::WR;
			case RDA:
				return T::Command::RDA;
			case WRA:
				return T::Command::WRA;
			case ACT_R:
				return T::Command::ACT_R;
			case ACT_W:
				return T::Command::ACT_W;	
			case PRE:
				return T::Command::PRE;
			case PREA:
				return T::Command::PREA;
			case REF:
				return T::Command::REF;
			default:
				abort();
			}			
		}
		void convert_addr(BusPacket* command) 
		{
			addr_vec[int(T::Level::Channel)] = 0;
			addr_vec[int(T::Level::Rank)] = command->rank;
			 // No BankGroup
			if(int(T::Level::Bank) == int(T::Level::Rank)+1) {
				addr_vec[int(T::Level::Bank)] = command->bank;
			}
			else {
				addr_vec[int(T::Level::Bank)-1] = 0;//command->bankGroup;
				addr_vec[int(T::Level::Bank)] = command->bank;
			}
			if(int(T::Level::Row) == int(T::Level::Bank)+1) {
				addr_vec[int(T::Level::Row)] = command->row;
			}
			else {
				addr_vec[int(T::Level::Row)-1] = command->subArray;
				addr_vec[int(T::Level::Row)] = command->row;
			}
			addr_vec[int(T::Level::Column)] = command->column;			
		}
	};
}

#endif

