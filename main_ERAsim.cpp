
#include <iostream>
#include <fstream>
#include <sstream>
#include <getopt.h>

#include <time.h>

#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <cstring>
#include <functional>

#include "ERAsimStub.h"

#include "Requestor.h"
#include "Request.h"
#include "MemoryController.h"
#include "Ramulator.h"

using namespace std;
using namespace DRAMController;

// ./DRAMController -C 1 

void usage() {
	cout << "DRAMController Usage: " << endl;
	cout << "DRAMController -n 8 -s system.ini -d device.ini -t memoryTraces -c cycles" <<endl;
	cout << "\t-n, --Requestor=# \t\tspecify number of requestors to run the simulation for [default=8] "<<endl;
	cout << "\t-t, --tracefile=FILENAME \tspecify multiple tracefile to run  "<<endl;
	cout << "\t-s, --systemini=FILENAME \tspecify multiple ini file that describes the memory system parameters  "<<endl;
	cout << "\t-c, --cycles=# \t\tspecify number of cycles to run the simulation for [default=30] "<<endl;
	cout << "\t-C, --Channel=# \t\tspecify number of channels "<<endl;
	cout << "\t-R, --Ranks=# \t\tspecify number of ranks per Channel "<<endl;
	cout << "\t-G, --DevGene=string \t\tspecify DRAM device generation "<<endl;
	cout << "\t-D, --DevVersion=string \t\tspecify DRAM device "<<endl;
	cout << "\t-S, --DevSize=string \t\tspecify DRAM device organization "<<endl;			
}

int main(int argc, char **argv)
{	
	int argument = 0;
	string systemIniFilename = "system.ini";
	unsigned requestors = 8;
	string traceFileName = "mem.trc";
	unsigned cycles = 0;

	unsigned int channels = 1;
	unsigned int ranks = 1;
	string deviceGene = "DDR3";
	string deviceSpeed = "1333G";
	string deviceSize = "2Gb_x8";
	while(1) {
		static struct option options[] = {
			{"numREQs", required_argument, 0, 'n'},
			{"sysFile", required_argument, 0, 's'},
			{"devFile", required_argument, 0, 'd'},
			{"trcFile", required_argument, 0, 't'},
			{"numCycl", required_argument, 0, 'c'},
			{"numChan", required_argument, 0, 'C'},
			{"numRank", required_argument, 0, 'R'},
			{"DeviceGeen", required_argument, 0, 'G'},
			{"DeviceSpeed", required_argument, 0, 'D'},
			{"DeviceSize", required_argument, 0, 'S'},
			{"help", no_argument, 0, 'h'}
		};
		int option_index = 0;
		argument = getopt_long(argc, argv, "n:s:d:t:c:C:R:G:D:S:h", options, &option_index);
		if (argument == -1) {
			break;
		}
		switch (argument) {
			case 'h':
			case '?':
				usage();
				exit(0);
			case 'n':
				requestors = atoi(optarg);
				break;
			case 's':
				systemIniFilename = string(optarg);
				break;
			case 't':
				traceFileName = string(optarg);
				break;
			case 'c':
				cycles = atoi(optarg);
				break;
			case 'C':
				channels = atoi(optarg);
				break;
			case 'R':
				ranks = atoi(optarg);
				break;
			case 'G':
				deviceGene = string(optarg);
				break;
			case 'D':
				deviceSpeed = string(optarg);
				break;
			case 'O':
				deviceSize = string(optarg);
				break;
		}
	}

	// Channel, MemoryController, Requestor
	map<int, MemorySystem*> channelsMap;
	map<int, MemoryController*> controllersMap;
	map<int, Requestor*> requestorsMap;
	// Callback function pass complete request to requestor
	// auto callBack = [&requestorsMap](DRAMController::Request& r)
	vector<ERAsim_Transaction_t*> pendingRequest;
	auto callBack = [&pendingRequest](DRAMController::Request& r) {
		if(!pendingRequest.empty()) {
			for(int index=0; index<pendingRequest.size(); index++) {
				if(pendingRequest[index]->ID == r.requestorID && pendingRequest[index]->Address == r.address 
					&& pendingRequest[index]->AccessTime == r.arriveTime) {
					pendingRequest[index]->FinishTime = r.returnTime;
					ERAsim.put(pendingRequest[index]);
					pendingRequest.erase(pendingRequest.begin() + index);
				}
			}
		}
		// requestorsMap[r.requestorID]->returnData(&r);
	};
	// DEBUG("Memory Systems "<<channels);
	for(int c = 0; c < channels; c++) {
		// DEBUG("Memory Controller "<<systemIniFilename);
		controllersMap[c] = new MemoryController(systemIniFilename, callBack);
		MemorySystem* memSys = NULL;
		const string GeneSpeed = deviceGene + '_' + deviceSpeed;
		const string GeneSize = deviceGene + '_' + deviceSize;
		if (deviceGene == "DDR3") {
			DDR3* ddr3 = new DDR3(GeneSize, GeneSpeed);
			memSys = new Ramulator<DDR3>(ddr3, ranks);
		}
		else if (deviceGene == "DDR4") {
			// DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
			// memSys = new Ramulator<DDR4>(&configs, ddr4, dataBusSize);  	 			
	    } 
		else {
			std::cout<<"Wrong DRAM standard"<<std::endl;
		}
		channelsMap[c] = memSys;
		memSys->connectMemoryController(controllersMap[c]); 
		controllersMap[c]->connectMemorySystem(memSys);
	}

	ifstream memTrace;
	memTrace.open(traceFileName.c_str());
	if(!memTrace.is_open()) {
		cout << " == Error - Could not open trace file"<<endl;
		exit(0);
	}
	bool inOrder = true;
	bool isHRT = true;
	int requestSize = 64;
	string line;
	for(int id = 0; id < requestors; id++) {
		getline(memTrace, line);
		switch(id)
		{
			case 0:
			case 1:
			case 2:
			case 3:
				isHRT = true;
				requestSize = 64;
				break;
			case 4:
			case 5:
			case 6:
			case 7:
				isHRT = true;
				requestSize = 2048;
				break;
			default:
				isHRT = false;
				requestSize = 64;
				break;
		}
		requestorsMap[id] = new Requestor(id, inOrder, line);
		requestorsMap[id]->RequestSize = requestSize;
		// Channel Assignment
		int ch = (int)(id%channels);
		requestorsMap[id]->connectMemoryController(controllersMap[ch]);
		controllersMap[ch]->setRequestor(id, isHRT);
		requestorsMap[id]->memoryClock = channelsMap[ch]->get_constraints("tCK");
	}
	memTrace.close();
	DEBUG("\n************Start Simulation************");
	for(int i=0; i<channels; i++) 
	{
		controllersMap[i]->displayConfiguration();
	}


	/* ERASim Interface Engine */
	int QueueSize = 5000; // {get it from argv[]}: Number of elements that can be buffered in the Queue.
	int ItemSize = 100; // {get it from argv[]}: the maximum size(in bytes) of the Item in the Queue
	ERAsim_Stub_t ERAsim = ERAsim_init_Stub("/tmp/SharedQueue_DDRAM", QueueSize, ItemSize); //Pass the PIPE Name from the cmdline
   
	ERAsim_Transaction_t* TA_ptr;
	printf("Ready...\n");

	int64_t myClock = 0;
	clock_t begin = clock();
	while(1) {
		myClock++;      
		ERAsim.wait(myClock);
		if(ERAsim_Finished==1) 
			break;
		while(ERAsim.count()>0) {              
			TA_ptr = ERAsim.get(); // You can store the transaction pointer anywhere you want      
			if(TA_ptr) {
			    //printf("Got Request [%d]\n", TA_ptr->ID);
			    TA_ptr->Data = (int)'A'; // Probebly you dont use this
			    if(controllersMap[TA_ptr->ID%channels]->addRequest(TA_ptr->Requestor_Id, TA_ptr->Address, TA_ptr->type, TA_ptr->Size)) {
					pendingRequest.push_back(TA_ptr);
				}
		    }
		}
		// Step Memory System
		for(int c=0; c < channels; c++) {
			// Step DRAMController
			controllersMap[c]->step();
			// Step DRAM Device
			channelsMap[c]->update();
		}    
	}
	printf("Simulation Finished\n");

	// /* Simple Requestor Simulation Engine	*/
	// int currentClockCyle = 0;
	// clock_t begin = clock();
	// bool simDone = false;

	// while(!simDone){
	// 	if(cycles != 0 && currentClockCyle == cycles) {simDone = true;}
	// 	// Step Requestor
	// 	for(int id = 0; id < requestors; id++) {
	// 		requestorsMap[id]->update();
	// 		// Determine if simulation is complete
	// 		if(requestorsMap[id]->sim_end()) {
	// 			simDone = true;
	// 		}
	// 	}
	// 	// Step Memory System
	// 	for(int c=0; c < channels; c++) {
	// 		// Step DRAMController
	// 		controllersMap[c]->step();
	// 		// Step DRAM Device
	// 		channelsMap[c]->update();
	// 	}
	// 	currentClockCyle++;
	// }
	clock_t end = clock();
	// --- Memory Deallocation
	//
	for(int i=0; i<channels; i++) {
		controllersMap[i]->printResult();
		delete controllersMap[i];
		controllersMap.erase(i);
		delete channelsMap[i];
		channelsMap.erase(i);
	}
	controllersMap.clear();
	channelsMap.clear();
	for(int i=0; i<requestors; i++) {
		requestorsMap[i]->printResult();
		delete requestorsMap[i];
		requestorsMap.erase(i);
	}
	requestorsMap.clear();
	std::cout<<"Simulation End @ "<<myClock<<" time="<<end-begin<<std::endl;
	return 0;
}

