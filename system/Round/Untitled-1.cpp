	/*********************************************************************************
	*  Copyright (c) 2018-2019, 
	*  Reza Mirosanlou
	*  University of Waterloo               
	*  All rights reserved.
	*********************************************************************************/

	#include "../../CommandScheduler.h"
	#include <cstdlib>
	namespace DRAMController
	{
		class CommandScheduler_Round: public CommandScheduler
		{
		private:
			//vector<pair<BusPacket*, unsigned int>> cmdFIFO;	// FIFO contains ready commands and queue index
			vector<unsigned int> Order;
			int  servicebuffer[16];
			int  consideredScheduled[16];
			// Pending Command indicator based on requestorID
			std::map<unsigned int, bool> queuePending;
			std::map<unsigned int, BusPacket*> tempqueue;
			bool roundType; // true = Read and false = write
			bool skipCAS;
			bool cycleDone;
			bool currRound;
			int lastACT;
			int lastCAS;
			int countWOPP;
			int countROPP;
			unsigned int expectCAS;
			unsigned int countFAW;
			unsigned int countACT;
			unsigned int countCAS;
			int counter;
			int swCAS;
			int swACT;
			bool blockACT;
			bool jump_1;
			bool jump_2;
			bool jump_3;
			bool jump_4;
			bool jump_5;
			bool jump_6;
			bool jump_7;
			bool first;
			bool BypassReset;
			int tCCD;
			int tRTR;
			int tRCD;
			int tFAW;
			int tRRD;
			int tRTW;
			int tWL;
			int tWTR;
			int tBUS;
			int tWtoR;

		public:
			CommandScheduler_Round(vector<CommandQueue*>& commandQueues, const map<unsigned int, bool>& requestorTable):
				CommandScheduler(commandQueues, requestorTable)
			{
				counter = 0;
				roundType = true; // true = Read and false = write
				skipCAS = false;
				cycleDone = false;
				currRound = 0;
				lastACT = 0;
				countFAW = 0;
				blockACT = false;
				swCAS = tCCD;
				
				
				for (unsigned int i = 0 ; i < 8 ; i++)
				{
					servicebuffer[i] = 0;
				}
				for (unsigned int i = 0; i < 8; i++)
				{
					consideredScheduled[i] = 0;
				}
				for (unsigned int i = 0; i < 8; i++)
				{
					Order.push_back(i);
				}
			}

			~CommandScheduler_Round()
			{
				tempqueue.clear();
				Order.clear();
				queuePending.clear();
				// for (unsigned int i = 0; i < tempqueue.size(); i++)
				// {
				// 	consideredScheduled[i] = 0;
				// }
				// queuePending.clear();
				// tempqueue.clear();
				
				// for (unsigned int i = 0 ; i < tempqueue.size() ; i++)
				// {
				// 	servicebuffer[i] = 0;
				// }
			}

			BusPacket* commandSchedule()
			{
					
				scheduledCommand = NULL;
				tCCD = getTiming("tCCD");
				//cout<<"tCCD   "<<tCCD<<endl;
				tRRD = getTiming("tRRD");
				//cout<<"tRRD   "<<tRRD<<endl;
				tRCD = getTiming("tRCD");
				//cout<<"tRCD   "<<tRCD<<endl;
				tFAW = getTiming("tFAW");
				//cout<<"tFAW   "<<tFAW<<endl;
				tRTW = getTiming("tRTW");
				//cout<<"tRTW   "<<tRTW<<endl;
				tWL  = getTiming("tWL");
				//cout<<"tWL   "<<tWL<<endl;
				tWTR = getTiming("tWTR"); 
				//cout<<"tWTR   "<<tWTR<<endl;
				tBUS = getTiming("tBus");
				//cout<<"tBUS   "<<tBUS<<endl;
				tWtoR = tWL + tBUS + tWTR;
				checkCommand = NULL;
				checkCommand_1 = NULL;
				count_ACT = false;
				count_CAS = false;
				
				//Constructing the intra-ready tempqueue here!

				for(unsigned int index = 0; index < commandQueue.size(); index++)
				{				
					// PerRequestor is enabled and there is some requestors in this memory level
					if(commandQueue[index]->isPerRequestor()) 
					{
						if(commandQueue[index]->getRequestorIndex() > 0) // there is more than 0 requestor in the design
						{
							for(unsigned int num = 0; num < commandQueue[index]->getRequestorIndex(); num++) // for all requestors from "num". getRequestorIndex() gives the number of requestors
							{
								if(commandQueue[index]->getRequestorSize(num) > 0 ) //return the buffer size of the requestor
								{
									
									checkCommand = commandQueue[index]->getRequestorCommand(num);
									//cout << checkCommand->busPacketType<<endl;
									if(queuePending.find(checkCommand->requestorID) == queuePending.end()) 
									{											
										queuePending[checkCommand->requestorID] = false;										
									}
									if(queuePending[checkCommand->requestorID] == false && isReady(checkCommand, index)) 
									{																
										tempqueue[checkCommand->requestorID]= checkCommand;														
										queuePending[checkCommand->requestorID] = true;
									}
									checkCommand = NULL;
								}
							}
						}	
					}		
				}
			

				


				if(counter == swACT)
				{
					// cout<<"Line:   169 "<<endl;
					// cout<<"*********************"<<endl;
					//To check whether we are scheduled ACT but did not serviced. In this Case we just push SW CAS 1 point further.
					for(unsigned int i = 0; i < Order.size(); i++)
					{	
						if(!jump_5)	
						{		
							int RR = Order.at(i);
							if(tempqueue.find(RR) == tempqueue.end())
							{							
								continue;
							}
							else
							{
								checkCommand = tempqueue[RR];
								if((checkCommand->busPacketType == ACT_R) || (checkCommand->busPacketType == ACT_W))
								{	
									// cout<<"Line:   184 "<<endl;
									// cout<<"*********************"<<endl;				
									if(consideredScheduled[RR] == 1)
									{
										if(servicebuffer[RR] == 0)
										{
											cout<<"the command is   "<<checkCommand->busPacketType<<endl;
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   188 "<<endl;
											cout<<"*********************"<<endl;
											swACT = swACT + 1;	
											swCAS = swCAS + 1;								
											checkCommand = NULL;
											count_ACT = true;	
											jump_5 = true;	
											jump_6 = true;									
										}
									}
								}	
							}	
							checkCommand = NULL;																						
						}
					}
					// cout<<"Line:   210 "<<endl;	
					// cout<<"*********************"<<endl;
					jump_5 =false;

					if(!count_ACT)
					{
						blockACT = true;
					}
					else
					{
						blockACT = false;
					}					
				}
				
				if (counter == swCAS)
				{
					// cout<<"the round is   "<<roundType<<endl; 
					// cout<<"counter is   "<<counter<<endl;
					// cout<<"Last ACT     "<<lastACT<<endl;
					// cout<<"Last CAS   "<<lastCAS<<endl;
					// cout<<"SW ACT   "<<swACT<<endl;
					// cout<<"SW CAS   "<<swCAS<<endl;
					// cout<<"Line:   231 "<<endl;
					// cout<<"*********************"<<endl;

					
					//To check whether we are scheduled something but did not serviced. In this Case we just push SW CAS 1 point further.
					for(unsigned int i = 0; i < Order.size(); i++)
					{	
						if(!jump_5)	
						{		
							int RR = Order.at(i);
							if(tempqueue.find(RR) == tempqueue.end())
							{
								continue;
							}
							else
							{
								checkCommand = tempqueue[RR];
								if((checkCommand->busPacketType == RD) || (checkCommand->busPacketType == WR))
								{					
									if(consideredScheduled[RR] == 1)
									{
										if(servicebuffer[RR] == 0)
										{
											cout<<"the command is   "<<checkCommand->busPacketType<<endl;
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   246 "<<endl;
											cout<<"*********************"<<endl;
											swCAS = swCAS + 1;
											jump_4 = true;
											jump_5 = true;
											jump_7 = true;
											checkCommand = NULL;											
										}
									}
								}	
							}	
							checkCommand = NULL;																						
						}
					}	
					jump_5 =false;	

					// cout<<"Line:   324 "<<endl;	
					// cout<<"*********************"<<endl;

					if (!jump_4)
					{
						
						if (roundType == true)
						{
							cout<<"the round is   "<<roundType<<endl; 
							cout<<"counter is   "<<counter<<endl;
							cout<<"Last ACT     "<<lastACT<<endl;
							cout<<"Last CAS   "<<lastCAS<<endl;
							cout<<"SW ACT   "<<swACT<<endl;
							cout<<"SW CAS   "<<swCAS<<endl;
							cout<<"Line:   278 "<<endl;
							cout<<"*********************"<<endl;
							for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
							{			
								checkCommand = it->second;
								if (checkCommand->busPacketType == WR) 
								// RD to WR (Open)
								{	
									
									for(uint64_t i = 0 ; i < 8; i++)
									{
										servicebuffer[i] = 0;
									}
									for (unsigned int i = 0; i < 8; i++)
									{
										consideredScheduled[i] = 0;
									}
									roundType = false; // WR									
									lastACT = 0;
									
									//counter = 0; 
									first = true;
									blockACT = false;
									countACT = 0;
									countFAW = 0;
									countCAS = 0;
									jump_1 = true;	
									cout<<"the round is   "<<roundType<<endl; 
									cout<<"counter is   "<<counter<<endl;
									cout<<"Last ACT     "<<lastACT<<endl;
									cout<<"Last CAS   "<<lastCAS<<endl;
									cout<<"SW ACT   "<<swACT<<endl;
									cout<<"SW CAS   "<<swCAS<<endl;
									cout<<"Line:   291 "<<endl;
									cout<<"*********************"<<endl;
									cout<<"I did switch to WR open"<<endl;						
								}
							}	
							if(!jump_1)
							{
								
								for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
								{			
									checkCommand = it->second;
									if (checkCommand->busPacketType == ACT_W)
									{	
										
										for(uint64_t i = 0 ; i < 8; i++)
										{
											servicebuffer[i] = 0;
										}	
										for (unsigned int i = 0; i < 8; i++)
										{
											consideredScheduled[i] = 0;
										}
										roundType = false;										
										lastACT = 0;
										blockACT = false;										
										//counter = 0;
										countACT = 0;
										countFAW = 0;
										first = true;
										countCAS = 0;
										jump_2 = true;
										cout<<"the round is   "<<roundType<<endl; 
										cout<<"counter is   "<<counter<<endl;
										cout<<"Last ACT     "<<lastACT<<endl;
										cout<<"Last CAS   "<<lastCAS<<endl;
										cout<<"SW ACT   "<<swACT<<endl;
										cout<<"SW CAS   "<<swCAS<<endl;
										cout<<"Line:   365 "<<endl;
										cout<<"*********************"<<endl;
									}
								}
							}
							if (!jump_1)
							{
								if(!jump_2)
								{
									
									for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
									{			
										checkCommand = it->second;
										if (checkCommand->busPacketType == RD) 
										{	
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   371 "<<endl;
											cout<<"*********************"<<endl;																			
											BypassReset = false;																				
											jump_3 = true;							
										}
									}	

									for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
									{			
										checkCommand = it->second;
										if (checkCommand->busPacketType == ACT_R)
										{			
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   389 "<<endl;
											cout<<"*********************"<<endl;								
											BypassReset = false;
											jump_3 = true;
										}
									}
									
									if (jump_3)
									{		
										// cout<<"Line:   487 "<<endl;	
										// cout<<"*********************"<<endl;								
										for(uint64_t i = 0 ; i < 8; i++)
										{
											servicebuffer[i] = 0;
										}	
										for (unsigned int i = 0; i < 8; i++)
										{
											consideredScheduled[i] = 0;
										}
										blockACT = false;
										counter = 0;
										lastACT = 0;
										lastCAS = -tCCD;
										swCAS = 0;
										swACT = 0;	
										countACT = 0;									
										countCAS = 0;	
										countFAW = 0;
										jump_3 = false;
																				
									}
									else 
									{
										cout<<"the round is   "<<roundType<<endl; 
										cout<<"counter is   "<<counter<<endl;
										cout<<"Last ACT     "<<lastACT<<endl;
										cout<<"Last CAS   "<<lastCAS<<endl;
										cout<<"SW ACT   "<<swACT<<endl;
										cout<<"SW CAS   "<<swCAS<<endl;
										cout<<"Line:   434 "<<endl;
										cout<<"*********************"<<endl;
										BypassReset = true;
										for(uint64_t i = 0 ; i < 8; i++)
										{
											servicebuffer[i] = 0;
											countCAS = 0;	
											countACT = 0;
											countFAW = 0;
										}
										for (unsigned int i = 0; i < 8; i++)
										{
											consideredScheduled[i] = 0;
										}
										blockACT = false;	
									}						
								}
							}																													
						}
						else if (roundType == false )
						{
							cout<<"the round is   "<<roundType<<endl; 
							cout<<"counter is   "<<counter<<endl;
							cout<<"Last ACT     "<<lastACT<<endl;
							cout<<"Last CAS   "<<lastCAS<<endl;
							cout<<"SW ACT   "<<swACT<<endl;
							cout<<"SW CAS   "<<swCAS<<endl;
							cout<<"Line:   385 "<<endl;
							// cout<<"**********KOSE GAAV 1111***********"<<endl;
							for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
							{	
								//cout<<"**********KOSE GAAV 222***********"<<endl;		
								checkCommand = it->second;
								//cout<<" the command here is kose gaaav  "<<checkCommand->busPacketType<<endl;
								if (checkCommand->busPacketType == RD) 
								{	
									
									for(uint64_t i = 0 ; i < 8; i++)
									{
										servicebuffer[i] = 0;
									}
									for (unsigned int i = 0; i < 8; i++)
									{
										consideredScheduled[i] = 0;
									}
									roundType = true;	
									lastACT = 0;
									blockACT = false;
									//counter = 0; 
									first = true;
									countACT = 0;
									countCAS = 0;	
									countFAW = 0;
									jump_1 = true;		
									cout<<"the round is   "<<roundType<<endl; 
									cout<<"counter is   "<<counter<<endl;
									cout<<"Last ACT     "<<lastACT<<endl;
									cout<<"Last CAS   "<<lastCAS<<endl;
									cout<<"SW ACT   "<<swACT<<endl;
									cout<<"SW CAS   "<<swCAS<<endl;
									cout<<"Line:   398 "<<endl;
									cout<<"*********************"<<endl;
														
								}
							}	
							//cout<<"**********KOSE GAAV 2222***********"<<endl;
							if(!jump_1)
							{
								//cout<<"Line:   582 "<<endl;				
								//cout<<"*********************"<<endl;
								
								for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
								{			
									//cout<<"**********KOSE GAAV 333***********"<<endl;
									checkCommand = it->second;
									//cout<<" the command here is  "<<checkCommand->busPacketType<<endl;
									if (checkCommand->busPacketType == ACT_R)
									{	
													
										for(uint64_t i = 0 ; i < 8; i++)
										{
											servicebuffer[i] = 0;
										}	
										for (unsigned int i = 0; i < 8; i++)
										{
											consideredScheduled[i] = 0;
										}
										roundType = true;										
										//counter = 0;
										lastACT = 0;
										blockACT = false;
										first = true;
										countACT = 0;
										countCAS = 0;
										countFAW = 0;
										jump_2 = true;
										cout<<"the round is   "<<roundType<<endl; 
										cout<<"counter is   "<<counter<<endl;
										cout<<"Last ACT     "<<lastACT<<endl;
										cout<<"Last CAS   "<<lastCAS<<endl;
										cout<<"SW ACT   "<<swACT<<endl;
										cout<<"SW CAS   "<<swCAS<<endl;
										cout<<"Line:   440 "<<endl;				
										cout<<"*********************"<<endl;	
										
									}
								}
							}
							if (!jump_1)
							{
								cout<<"Line:   628 "<<endl;				
									cout<<"*********************"<<endl;
								if(!jump_2)
								{

									cout<<"Line:   631 "<<endl;				
									cout<<"*********************"<<endl;
									for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
									{			
										checkCommand = it->second;
										if (checkCommand->busPacketType == WR) 
										{																				
											BypassReset = false;																				
											jump_3 = true;
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   560 "<<endl;				
											cout<<"*********************"<<endl;	
																		
										}
									}	

									for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
									{			
										checkCommand = it->second;
										if (checkCommand->busPacketType == ACT_W)
										{											
											BypassReset = false;
											jump_3 = true;
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   577 "<<endl;				
											cout<<"*********************"<<endl;	
											
										}
									}
									
									if (jump_3)
									{										
										for(uint64_t i = 0 ; i < 8; i++)
										{
											servicebuffer[i] = 0;
										}	
										for (unsigned int i = 0; i < 8; i++)
										{
											consideredScheduled[i] = 0;
										}
										blockACT = false;
										lastACT = 0;
										lastCAS = -tCCD;
										swCAS = 0;
										swACT = 0;
										counter = 0;
										countCAS = 0;	
										countACT = 0;
										countFAW = 0;
										jump_3 = false;	
										cout<<"the round is   "<<roundType<<endl; 
										cout<<"counter is   "<<counter<<endl;
										cout<<"Last ACT     "<<lastACT<<endl;
										cout<<"Last CAS   "<<lastCAS<<endl;
										cout<<"SW ACT   "<<swACT<<endl;
										cout<<"SW CAS   "<<swCAS<<endl;
										cout<<"Line:   616 "<<endl;				
										cout<<"*********************"<<endl;	
																				
									}
									else 
									{
										cout<<"the round is   "<<roundType<<endl; 
										cout<<"counter is   "<<counter<<endl;
										cout<<"Last ACT     "<<lastACT<<endl;
										cout<<"Last CAS   "<<lastCAS<<endl;
										cout<<"SW ACT   "<<swACT<<endl;
										cout<<"SW CAS   "<<swCAS<<endl;
										cout<<"Line:   637 "<<endl;				
										cout<<"*********************"<<endl;
										BypassReset = true;
										for(uint64_t i = 0 ; i < 8; i++)
										{
											servicebuffer[i] = 0;
											countCAS = 0;	
											countFAW = 0;
											countACT = 0;
												
										}
										for (unsigned int i = 0; i < 8; i++)
										{
											consideredScheduled[i] = 0;
										}
										blockACT = false;	
									}
									//cout<<"*******BAAYAD INJA BASHE  1111*******"<<endl;
								}
							}
						}											
					}
					jump_1 = false;
					jump_2 = false;
					jump_4 = false;																
				}
				else 
				{
					// cout<<"the round is   "<<roundType<<endl; 
					// cout<<"counter is   "<<counter<<endl;
					// cout<<"Last ACT     "<<lastACT<<endl;
					// cout<<"Last CAS   "<<lastCAS<<endl;
					// cout<<"SW ACT   "<<swACT<<endl;
					// cout<<"SW CAS   "<<swCAS<<endl;
					// cout<<"Line:   483 "<<endl;
					// cout<<"*********************"<<endl;
				}
				//cout<<"*******BAAYAD INJA BASHE  2222*******"<<endl;










				// cout<<"**********Chaap kon***********"<<endl;

				// for(unsigned int i = 0; i < Order.size(); i++)
				// {	
					
				// 	int RR = Order.at(i);
				// 	cout<<"ORder is=        "<<RR<<endl;
				// }					
				// for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
				// {			
				// 	cout<<" the command ghabl az too raftan requestor ID "<<it->first<<endl;
				// 	checkCommand = it->second;
				// 	cout<<" the command ghabl az too raftan  "<<checkCommand->busPacketType<<endl;
				// }
				// checkCommand = NULL;


				for(unsigned int i = 0; i < Order.size(); i++)
				{	
					//cout<<"*******BAAYAD INJA BASHE  333*******"<<endl;			


					int RR = Order.at(i);
					//cout<<"ORder is   "<<RR<<endl;
					
					if(tempqueue.find(RR) == tempqueue.end())
					{
						//cout<<"*******BAAYAD INJA BASHE  4444*******"<<endl;
						continue;
					}
					else
					{
						//cout<<"*******BAAYAD INJA BASHE  5555*******"<<endl;
						checkCommand = tempqueue[RR];
						if (checkCommand->busPacketType == RD)
						{
							cout<<"the round is   "<<roundType<<endl; 
							cout<<"counter is   "<<counter<<endl;
							cout<<"Last ACT     "<<lastACT<<endl;
							cout<<"Last CAS   "<<lastCAS<<endl;
							cout<<"SW ACT   "<<swACT<<endl;
							cout<<"SW CAS   "<<swCAS<<endl;
							cout<<"Line:   687 "<<endl;
						
							cout<<"*********************"<<endl;
							if(roundType == true)
							{
								
								if(servicebuffer[RR] == 0)
								{	
															
									if (consideredScheduled[RR] == 0)
									{
									
										consideredScheduled[RR] = 1;
										if (first)
										{
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   732 "<<endl;
											cout<<"*********************"<<endl;
											countCAS++;
											if(lastCAS < 0)
											{
												lastCAS = -lastCAS;											
											}
											
											lastCAS = tWtoR - (counter - lastCAS);
											if(lastCAS < 0){
												lastCAS = 0;
											}										
											counter = 0;
											first = false;
											swCAS = lastCAS + tCCD;
											swACT = 0;
											
										}
										else 
										{
											countCAS++;
											lastCAS = lastCAS + tCCD;
											swCAS = lastCAS + tCCD;		
											swACT = swCAS - tRCD;
											if(swACT < 0){
												swACT = 0;
											}													
										}	
									}
								}	
							}												
						checkCommand = NULL;
						}
						else if (checkCommand->busPacketType == WR)
						{
							//cout<<"*******BAAYAD INJA BASHE  6666*******"<<endl;
							if (roundType == false)
							{
								//cout<<"*******BAAYAD INJA BASHE  777*******"<<endl;
								if(servicebuffer[RR] == 0)
								{		
									//cout<<"*******BAAYAD INJA BASHE  8888*******"<<endl;					
									if (consideredScheduled[RR] == 0)
									{			
										//cout<<"*******BAAYAD INJA BASHE  99999*******"<<endl;							
										consideredScheduled[RR] = 1;
										if (first)
										{
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   870 "<<endl;
											cout<<"*********************"<<endl;
											countCAS++;
											if(lastCAS < 0)
											{
												lastCAS = -lastCAS;
											}
											lastCAS = tRTW - (counter - lastCAS);
											if(lastCAS < 0){
												lastCAS = 0;
											}
											counter = 0;
											first = false;
											swCAS = lastCAS + tCCD;
											swACT = 0;
										}
										else 
										{
											
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   892 "<<endl;
											cout<<"*********************"<<endl;
											countCAS++;
											lastCAS = lastCAS + tCCD;											
											swCAS = lastCAS + tCCD;	
											swACT = swCAS - tRCD;	
											if(swACT < 0){
												swACT = 0;
											}									
										}									
									}
								}	
							}
						checkCommand = NULL;
						}
						else if (checkCommand->busPacketType == ACT_R)
						{
							cout<<"the round is   "<<roundType<<endl; 
							cout<<"counter is   "<<counter<<endl;
							cout<<"Last ACT     "<<lastACT<<endl;
							cout<<"Last CAS   "<<lastCAS<<endl;
							cout<<"SW ACT   "<<swACT<<endl;
							cout<<"SW CAS   "<<swCAS<<endl;
							cout<<"Line:   589 "<<endl;
							cout<<"*********************"<<endl;
							if (!blockACT)
							{
								
								if (roundType == true)
								{
									if(servicebuffer[RR] == 0)
									{
										if(consideredScheduled[RR] == 0)
										{
											for(unsigned int i = 0; i < Order.size(); i++)
											{											
												int RR1 = Order.at(i);
												if(tempqueue.find(RR1) == tempqueue.end())
												{
													continue;
												}
												else
												{
													checkCommand_1 = tempqueue[RR1];	
													if (checkCommand_1->busPacketType == RD)
													{
														cout<<"the round is   "<<roundType<<endl; 
														cout<<"counter is   "<<counter<<endl;
														cout<<"Last ACT     "<<lastACT<<endl;
														cout<<"Last CAS   "<<lastCAS<<endl;
														cout<<"SW ACT   "<<swACT<<endl;
														cout<<"SW CAS   "<<swCAS<<endl;
														cout<<"Line:   608 "<<endl;
														cout<<"*********************"<<endl;
														if(servicebuffer[RR1] == 0)
														{
															if (consideredScheduled[RR1] == 0)
															{
																
																consideredScheduled[RR1] = 1;
																if (first)
																{
																	countCAS++;
																	if(lastCAS < 0)
																	{
																		lastCAS = -lastCAS;
																	}
																	lastCAS = tWtoR - (counter - lastCAS);
																	if(lastCAS < 0){
																		lastCAS = 0;
																	}
																	counter = 0;
																	first = false;
																	swCAS = lastCAS + tCCD;	
																	swACT = 0;																
																}
																else 
																{
																	countCAS++;
																	lastCAS = lastCAS + tCCD;
																	swCAS = lastCAS + tCCD;
																	swACT = swCAS - tRCD;	
																	if(swACT < 0){
																		swACT = 0;
																	}	
																}	
															}																									
														}
													}	
												}																																					
											}	
										
											//update ACT
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   641 "<<endl;
											cout<<"*********************"<<endl;
											consideredScheduled[RR] = 1;
											//cout<<"the first here is    "<<first<<endl;
											if(first)
											{
												
												lastACT = 0;
												lastCAS = tRCD;
												counter = 0;
												first = false;
												countACT++;
												swCAS = lastCAS + tCCD;
												swACT = swCAS - tRCD;
												if(swACT < 0){
												swACT = 0;
												}
												
											}
											else if ((!first) && ((countACT+1) == 5))
											{
												countFAW++;
												countACT = 0;
												
												int temp = lastACT + (tFAW - 3*tRRD); 
												if(counter > temp)
												{
													lastACT = counter;
												}
												else
												{
													lastACT = temp; 
												}
												int x = lastACT + tRCD;
												int y = lastCAS + tCCD;	
												if(x >= y)
												{
													lastCAS = x;
												}
												else
												{
													lastCAS = y;
												}
												swCAS = lastCAS + tCCD;										
												
												swACT = swCAS - tRCD;
												if(swACT < 0){
												swACT = 0;
												}
											}
											else
											{
												countACT++;
												int temp = lastACT + tRRD;
												if(counter >= temp)
												{
													lastACT = counter;
												}
												else
												{
													lastACT = temp; 
												}
												
												int x = lastACT + tRCD;
												int y = lastCAS + tCCD;	
												if(x >= y)
												{
													lastCAS = x;
												}
												else
												{
													lastCAS = y;
												}
												swCAS = lastCAS + tCCD;
												swACT = swCAS - tRCD;
												if(swACT < 0){
												swACT = 0;
												}
											}
										}												
									}
								}
							}	
							checkCommand = NULL;
						}	
						else if (checkCommand->busPacketType == ACT_W)
						{
							// cout<<"the round is   "<<roundType<<endl; 
							// cout<<"counter is   "<<counter<<endl;
							// cout<<"Last ACT     "<<lastACT<<endl;
							// cout<<"Last CAS   "<<lastCAS<<endl;
							// cout<<"SW ACT   "<<swACT<<endl;
							// cout<<"SW CAS   "<<swCAS<<endl;
							// cout<<"Line:   1013 "<<endl;
							// cout<<"*********************"<<endl;
							if(!blockACT)
							{
								// cout<<"the round is   "<<roundType<<endl; 
								// cout<<"counter is   "<<counter<<endl;
								// cout<<"Last ACT     "<<lastACT<<endl;
								// cout<<"Last CAS   "<<lastCAS<<endl;
								// cout<<"SW ACT   "<<swACT<<endl;
								// cout<<"SW CAS   "<<swCAS<<endl;
								// cout<<"Line:   1023 "<<endl;
								// cout<<"*********************"<<endl;
								if (roundType == false)
								{
									// cout<<"the round is   "<<roundType<<endl; 
									// cout<<"counter is   "<<counter<<endl;
									// cout<<"Last ACT     "<<lastACT<<endl;
									// cout<<"Last CAS   "<<lastCAS<<endl;
									// cout<<"SW ACT   "<<swACT<<endl;
									// cout<<"SW CAS   "<<swCAS<<endl;
									// cout<<"Line:   1033 "<<endl;
									// cout<<"*********************"<<endl;
									if(servicebuffer[RR] == 0)
									{
										// cout<<"the round is   "<<roundType<<endl; 
										// cout<<"counter is   "<<counter<<endl;
										// cout<<"Last ACT     "<<lastACT<<endl;
										// cout<<"Last CAS   "<<lastCAS<<endl;
										// cout<<"SW ACT   "<<swACT<<endl;
										// cout<<"SW CAS   "<<swCAS<<endl;
										// cout<<"Line:   1043 "<<endl;
										// cout<<"*********************"<<endl;
										if(consideredScheduled[RR] == 0)
										{
											for(unsigned int i = 0; i < Order.size(); i++)
											{											
												int RR1 = Order.at(i);
												if(tempqueue.find(RR1) == tempqueue.end())
												{
													continue;
												}
												else
												{
													checkCommand_1 = tempqueue[RR1];	
													if (checkCommand_1->busPacketType == WR)
													{
														cout<<"the round is   "<<roundType<<endl; 
														cout<<"counter is   "<<counter<<endl;
														cout<<"Last ACT     "<<lastACT<<endl;
														cout<<"Last CAS   "<<lastCAS<<endl;
														cout<<"SW ACT   "<<swACT<<endl;
														cout<<"SW CAS   "<<swCAS<<endl;
														cout<<"Line:   700 "<<endl;
														cout<<"*********************"<<endl;
														if(servicebuffer[RR1] == 0)
														{
															if (consideredScheduled[RR1] == 0)
															{
																consideredScheduled[RR1] = 1;
																if (first)
																{
																	countCAS++;
																	if(lastCAS < 0)
																	{
																		lastCAS = -lastCAS;
																	}
																	lastCAS = tRTW - (counter - lastCAS);
																	if(lastCAS < 0){
																		lastCAS = 0;
																	}
																	counter = 0;
																	swCAS = lastCAS + tCCD;
																	first = false;
																	swACT = 0;
																}
																else 
																{
																	countCAS++;
																	lastCAS = lastCAS + tCCD;																	
																	swCAS = lastCAS + tCCD;
																	swACT = swCAS - tRCD;	
																	if(swACT < 0){
																		swACT = 0;
																	}															
																}	
															}																									
														}
													}
												}																																					
											}	
										
										
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   730 "<<endl;
										
											cout<<"*********************"<<endl;
											consideredScheduled[RR] = 1;
											
											if(first)
											{
												
												lastACT = 0;
												lastCAS = tRCD;
												counter = 0;
												first = false;
												countACT++;
												swCAS = lastCAS + tCCD;
												swACT = swCAS - tRCD;
												if(swACT < 0){
												swACT = 0;
												}
											//	cout<<"SW CAS 1079 HERE IS   "<<swCAS<<endl;
											}
											
											else if ((!first) && ((countACT+1) == 5))
											{
												countFAW++;
												countACT = 0;
												
												int temp = lastACT + (tFAW - 3*tRRD); 
												if(counter > temp)
												{
													lastACT = counter;
												}
												else
												{
													lastACT = temp; 
												}
												int x = lastACT + tRCD;
												int y = lastCAS + tCCD;	
												if(x >= y)
												{
													lastCAS = x;
												}
												else
												{
													lastCAS = y;
												}
												swCAS = lastCAS + tCCD;
												swACT = swCAS - tRCD;	
												if(swACT < 0){
												swACT = 0;
												}									
												
											}
											else
											{
												countACT++;
												int temp = lastACT + tRRD;
												if(counter >= temp)
												{
													lastACT = counter;
												}
												else
												{
													lastACT = temp; 
												}

												int x = lastACT + tRCD;
												int y = lastCAS + tCCD;	
												if(x >= y)
												{
													lastCAS = x;
												}
												else
												{
													lastCAS = y;
												}
												swCAS = lastCAS + tCCD;
												swACT = swCAS - tRCD;
												if(swACT < 0){
												swACT = 0;
												}
											}			
											//cout<<"SW CAS 1134 HERE IS   "<<swCAS<<endl;
										}											
									}
								}
							}	
							checkCommand = NULL;
						}
						else if (checkCommand->busPacketType == PRE)
						{	
							cout<<"the round is   "<<roundType<<endl; 
							cout<<"counter is   "<<counter<<endl;
							cout<<"Last ACT     "<<lastACT<<endl;
							cout<<"Last CAS   "<<lastCAS<<endl;
							cout<<"SW ACT   "<<swACT<<endl;
							cout<<"SW CAS   "<<swCAS<<endl;
							cout<<"Line:   771 "<<endl;
							cout<<"*********************"<<endl;
												
							if (roundType == true)
							{
								
								if(servicebuffer[RR] == 0)
								{							
									for(unsigned int i = 0; i < Order.size(); i++)
									{
							
										unsigned int RR1 = Order.at(i);
										if (tempqueue.find(RR1) == tempqueue.end())
										{
											continue;
										}
										else
										{
											checkCommand_1 = tempqueue[RR1];
											if (checkCommand_1->busPacketType == RD)
											{
												cout<<"the round is   "<<roundType<<endl; 
												cout<<"counter is   "<<counter<<endl;
												cout<<"Last ACT     "<<lastACT<<endl;
												cout<<"Last CAS   "<<lastCAS<<endl;
												cout<<"SW ACT   "<<swACT<<endl;
												cout<<"SW CAS   "<<swCAS<<endl;
												cout<<"Line:   806 "<<endl;
												cout<<"*********************"<<endl;
												if(servicebuffer[RR1] == 0)
												{
													// cout<<"Line:   1240 "<<endl;
													// cout<<"*********************"<<endl;
													if (consideredScheduled[RR1] == 0)
													{
														cout<<"the round is   "<<roundType<<endl; 
														cout<<"counter is   "<<counter<<endl;
														cout<<"Last ACT     "<<lastACT<<endl;
														cout<<"Last CAS   "<<lastCAS<<endl;
														cout<<"SW ACT   "<<swACT<<endl;
														cout<<"SW CAS   "<<swCAS<<endl;
														cout<<"Line:   1249 "<<endl;
														cout<<"*********************"<<endl;
														consideredScheduled[RR1] = 1;															
														if (first)
														{
														// 		cout<<"Line:   1253 "<<endl;
														// cout<<"*********************"<<endl;
															countCAS++;
															lastCAS = tWtoR - (counter - lastCAS);
															if(lastCAS < 0){
																lastCAS = 0;
															}
															counter = 0;
															first = false;
															swCAS = lastCAS + tCCD;
															swACT = 0;
														}
														else 
														{
														// 		cout<<"Line:   1267 "<<endl;
														// cout<<"*********************"<<endl;
															countCAS++;
															lastCAS = lastCAS + tCCD;														
															swCAS = lastCAS + tCCD;	
															swACT = swCAS - tRCD;	
															if(swACT < 0){
																swACT = 0;
															}														
														}
													}																									
												}
											}
										}																																														
									}
										// cout<<"Line:   1282 "<<endl;
										// cout<<"*********************"<<endl;
									for(unsigned int i = 0; i < Order.size(); i++)
									{
										int RR1 = Order.at(i);

										if (tempqueue.find(RR1) == tempqueue.end())
										{
											continue;
										}
										else
										{
											// cout<<"Line:   1295 "<<endl;
											// cout<<"*********************"<<endl;
											checkCommand_1 = tempqueue[RR1];
											// cout<<"Line:   1299 "<<endl;
											// cout<<"*********************"<<endl;	
											if (checkCommand_1->busPacketType == ACT_R)
											{
												cout<<"the round is   "<<roundType<<endl; 
												cout<<"counter is   "<<counter<<endl;
												cout<<"Last ACT     "<<lastACT<<endl;
												cout<<"Last CAS   "<<lastCAS<<endl;
												cout<<"SW ACT   "<<swACT<<endl;
												cout<<"SW CAS   "<<swCAS<<endl;
												cout<<"Line:   580 "<<endl;
												cout<<"*********************"<<endl;
												if(servicebuffer[RR1] == 0)
												{
													if(consideredScheduled[RR1] == 0)
													{
														//update ACT
														consideredScheduled[RR1] = 1;
														
														if(first)
														{
															// cout<<"Line:   1316 "<<endl;
															// cout<<"*********************"<<endl;
															lastACT = 0;
															lastCAS = tRCD;
															counter = 0;
															first = false;
															countACT++;
															swCAS = lastCAS + tCCD;
															swACT = swCAS - tRCD;
															if(swACT < 0){
																swACT = 0;
															}
														
														}
														else if ((!first) && ((countACT+1) == 5))
														{
															// cout<<"Line:   1332 "<<endl;
															// cout<<"*********************"<<endl;
															countFAW++;
															countACT = 0;
															
															int temp = lastACT + (tFAW - 3*tRRD); 
															if(counter > temp)
															{
																lastACT = counter;
															}
															else
															{
																lastACT = temp; 
															}
															int x = lastACT + tRCD;
															int y = lastCAS + tCCD;	
															if(x >= y)
															{
																lastCAS = x;
															}
															else
															{
																lastCAS = y;
															}
															swCAS = lastCAS + tCCD;										
															
															swACT = swCAS - tRCD;
															if(swACT < 0){
																swACT = 0;
															}
														}
														else
														{
															// cout<<"Line:   1365 "<<endl;
															// cout<<"*********************"<<endl;
															countACT++;
															int temp = lastACT + tRRD;
															if(counter >= temp)
															{
																lastACT = counter;
															}
															else
															{
																lastACT = temp; 
															}
														

															int x = lastACT + tRCD;
															int y = lastCAS + tCCD;	
															if(x >= y)
															{
																lastCAS = x;
															}
															else
															{
																lastCAS = y;
															}
															swCAS = lastCAS + tCCD;
															swACT = swCAS - tRCD;
															if(swACT < 0){
																swACT = 0;
															}
														}	
													}																																			
												}
											}
										// 	cout<<"Line:   1402 "<<endl;
										// cout<<"*********************"<<endl;
										}																																			
									}																															
								}
							}
							else if (roundType == false)
							{
								if(servicebuffer[RR] == 0)
								{
									if(consideredScheduled[RR] == 0)
									{
										for(unsigned int i = 0; i < Order.size(); i++)
										{
		
											int RR1 = Order.at(i);
											if (tempqueue.find(RR1) == tempqueue.end())	
											{
												continue;
											}
											else
											{
												checkCommand_1 = tempqueue[RR1];	
												if (checkCommand_1->busPacketType == WR)
												{
													cout<<"the round is   "<<roundType<<endl; 
													cout<<"counter is   "<<counter<<endl;
													cout<<"Last ACT     "<<lastACT<<endl;
													cout<<"Last CAS   "<<lastCAS<<endl;
													cout<<"SW ACT   "<<swACT<<endl;
													cout<<"SW CAS   "<<swCAS<<endl;
													cout<<"Line:   941 "<<endl;
													cout<<"*********************"<<endl;
													if(servicebuffer[RR1] == 0)
													{
														if (consideredScheduled[RR1] == 0)
														{
															consideredScheduled[RR1] = 1;
															if (first)
															{
																countCAS++;
																lastCAS = tRTW - (counter - lastCAS);
																if(lastCAS < 0){
																	lastCAS = 0;
																}
																counter = 0;
																first = false;
																swCAS = lastCAS + tCCD;	
																swACT = 0;
															}
															else 
															{
																countCAS++;
																lastCAS = lastCAS + tCCD;																
																swCAS = lastCAS + tCCD;
																swACT = swCAS - tRCD;	
																if(swACT < 0){
																	swACT = 0;
																}															
															}	
														}																									
													}
												}	
											}																										
										}
										
										for(unsigned int i = 0; i < Order.size(); i++)
										{
											int RR1 = Order.at(i);

											if(tempqueue.find(RR1) == tempqueue.end())
											{
												continue;
											}
											else
											{
												checkCommand_1 = tempqueue[RR1];	
												if (checkCommand_1->busPacketType == ACT_W)
												{
													cout<<"the round is   "<<roundType<<endl; 
													cout<<"counter is   "<<counter<<endl;
													cout<<"Last ACT     "<<lastACT<<endl;
													cout<<"Last CAS   "<<lastCAS<<endl;
													cout<<"SW ACT   "<<swACT<<endl;
													cout<<"SW CAS   "<<swCAS<<endl;
													cout<<"Line:   983 "<<endl;
													cout<<"*********************"<<endl;
													if(servicebuffer[RR1] == 0)
													{
														if(consideredScheduled[RR1] == 0)
														{
															//update ACT
															consideredScheduled[RR1] = 1;
															
															if(first)
															{
																
																lastACT = 0;
																lastCAS = tRCD;
																counter = 0;
																first = false;
																countACT++;
																swCAS = lastCAS + tCCD;
																swACT = swCAS - tRCD;
																if(swACT < 0){
																	swACT = 0;
																}
															}
															else if ((!first) && ((countACT+1) == 5))
															{
																countFAW++;
																countACT = 0;
																
																int temp = lastACT + (tFAW - 3*tRRD); 
																if(counter > temp)
																{
																	lastACT = counter;
																}
																else
																{
																	lastACT = temp; 
																}
																int x = lastACT + tRCD;
																int y = lastCAS + tCCD;	
																if(x >= y)
																{
																	lastCAS = x;
																}
																else
																{
																	lastCAS = y;
																}
																swCAS = lastCAS + tCCD;										
																
																swACT = swCAS - tRCD;
																if(swACT < 0){
																	swACT = 0;
																}
															}
															else
															{
																countACT++;
																int temp = lastACT + tRRD;
																if(counter >= temp)
																{
																	lastACT = counter;
																}
																else
																{
																	lastACT = temp; 
																}



																int x = lastACT + tRCD;
																int y = lastCAS + tCCD;	
																if(x >= y)
																{
																	lastCAS = x;
																}
																else
																{
																	lastCAS = y;
																}
																swCAS = lastCAS + tCCD;
																swACT = swCAS - tRCD;
																if(swACT < 0){
																	swACT = 0;
																}
															}	
														}																																			
													}
												}	
											}																																				
										}
									}		
								}
							}
							// cout<<"Line:   1580 "<<endl;
							// 			cout<<"*********************"<<endl;							
						}
					}												
				}
				//cout<<"SW CAS 1473 HERE IS   "<<swCAS<<endl;
				if(BypassReset == true)
				{
					// cout<<"Line:   1588 "<<endl;
					// cout<<"*********************"<<endl;
					swCAS = counter + 1;
				}
	
				if(!tempqueue.empty())
				{
					cout<<"the round is   "<<roundType<<endl; 
					cout<<"counter is   "<<counter<<endl;
					cout<<"Last ACT     "<<lastACT<<endl;
					cout<<"Last CAS   "<<lastCAS<<endl;
					cout<<"SW ACT   "<<swACT<<endl;
					cout<<"SW CAS   "<<swCAS<<endl;
					cout<<"Line:   1077 "<<endl;
					cout<<"*********************"<<endl;
					// cout<<"not empty"<<endl;
					for(unsigned int i = 0; i < Order.size(); i++)
					{
						
						int RR = Order.at(i);

						if(tempqueue.find(RR) == tempqueue.end())
						{
							continue;
						}
						else
						{				
							checkCommand = tempqueue[RR];
							if (checkCommand->busPacketType == RD)
							{
								if(roundType == true)
								{
									if(servicebuffer[RR] == 0)
									{
										cout<<"the round is   "<<roundType<<endl; 
										cout<<"counter is   "<<counter<<endl;
										cout<<"Last ACT     "<<lastACT<<endl;
										cout<<"Last CAS   "<<lastCAS<<endl;
										cout<<"SW ACT   "<<swACT<<endl;
										cout<<"SW CAS   "<<swCAS<<endl;
										cout<<"Line:   Read Before ISSUE "<<endl;
										cout<<"*********************"<<endl;
										if(isIssuable(checkCommand))
										{	
											
											Order.erase(Order.begin() + i);
											Order.push_back(RR);
											servicebuffer[RR] = 1;
											cout<<"the ID is   "<<RR<<endl;
											scheduledCommand = checkCommand;
											cout<<"Scheduled Command is "<<scheduledCommand->busPacketType<<endl;
											sendCommand(scheduledCommand,0);
											cout<<" What is the requestor ID Before "<<scheduledCommand->requestorID<<endl;
											tempqueue.erase(RR);
											
											cout<<" What is the requestor ID  "<<scheduledCommand->requestorID<<endl;
											queuePending[scheduledCommand->requestorID] = false;		
																		
											
											cout<<"4444   "<<endl;
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   Read "<<endl;
											cout<<"*********************"<<endl;
											counter++;
											
											if(jump_7)
											{
												lastCAS = counter + tCCD;
												swCAS = lastCAS + tCCD;
											}
											jump_6 = false;
											jump_7 = false;	
											BypassReset = false;
											return scheduledCommand;
										}	
										// cout<<"Line:   1667 "<<endl;
										// cout<<"*********************"<<endl;
									}	
									// cout<<"Line:   1671 "<<endl;
									// 	cout<<"*********************"<<endl;
								}												
							checkCommand = NULL;
							}
							else if (checkCommand->busPacketType == WR)
							{
								// cout<<"Line:   1677 "<<endl;
								// 		cout<<"*********************"<<endl;
								if (roundType == false)
								{
									if(servicebuffer[RR] == 0)
									{								
										if(isIssuable(checkCommand))
										{			
						
											Order.erase(Order.begin() + i);
											Order.push_back(RR);
											servicebuffer[RR] = 1;
											scheduledCommand = checkCommand;
											sendCommand(scheduledCommand,0);
											tempqueue.erase(RR);
											//consideredScheduled[RR1] = 0;
											queuePending[scheduledCommand->requestorID] = false;		
																		
											
											cout<<"4444   "<<endl;
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   Read "<<endl;
											cout<<"*********************"<<endl;
											counter++;
											
											if(jump_7)
											{
												lastCAS = counter + tCCD;
												swCAS = lastCAS + tCCD;
											}
											jump_6 = false;
											jump_7 = false;	
											BypassReset = false;
											return scheduledCommand;
										}
									}	
								}
							checkCommand = NULL;
							}
							else if (checkCommand->busPacketType == ACT_R)
							{
								if (roundType == true)
								{
									// cout<<"Line:   1723 "<<endl;
									// 	cout<<"*********************"<<endl;
									if(servicebuffer[RR] == 0)
									{
										for(unsigned int i = 0; i < Order.size(); i++)
										{											
											int RR1 = Order.at(i);
											if(tempqueue.find(RR1) == tempqueue.end())
											{
												continue;
											}
											else
											{
												checkCommand_1 = tempqueue[RR1];	
												if (checkCommand_1->busPacketType == RD)
												{
													//cout<<"33333   "<<endl;
													if(servicebuffer[RR1] == 0)
													{
														if(isIssuable(checkCommand_1))
														{
															
															Order.erase(Order.begin() + i);
															Order.push_back(RR1);
															servicebuffer[RR1] = 1;
															scheduledCommand = checkCommand_1;
															sendCommand(scheduledCommand,0);
															tempqueue.erase(RR1);
															//consideredScheduled[RR1] = 0;
															queuePending[scheduledCommand->requestorID] = false;		
																						
															
															cout<<"4444   "<<endl;
															cout<<"the round is   "<<roundType<<endl; 
															cout<<"counter is   "<<counter<<endl;
															cout<<"Last ACT     "<<lastACT<<endl;
															cout<<"Last CAS   "<<lastCAS<<endl;
															cout<<"SW ACT   "<<swACT<<endl;
															cout<<"SW CAS   "<<swCAS<<endl;
															cout<<"Line:   Read "<<endl;
															cout<<"*********************"<<endl;
															counter++;
															
															if(jump_7)
															{
																lastCAS = counter + tCCD;
																swCAS = lastCAS + tCCD;
															}
															jump_6 = false;
															jump_7 = false;	
															BypassReset = false;
															return scheduledCommand;	
														}																									
													}
												}	
											}																																			
										}	
										cout<<"the round is   "<<roundType<<endl; 
										cout<<"counter is   "<<counter<<endl;
										cout<<"Last ACT     "<<lastACT<<endl;
										cout<<"Last CAS   "<<lastCAS<<endl;
										cout<<"SW ACT   "<<swACT<<endl;
										cout<<"SW CAS   "<<swCAS<<endl;
										cout<<"Line:   1683 "<<endl;
										cout<<"*********************"<<endl;																				
										if(isIssuable(checkCommand))
										{
											scheduledCommand = checkCommand;
											sendCommand(scheduledCommand,0);
											tempqueue.erase(RR);
											//consideredScheduled[RR] = 0;
											queuePending[scheduledCommand->requestorID] = false;										
											
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   ACT_R "<<endl;
											cout<<"*********************"<<endl;
											if(jump_6)
											{
												lastACT = counter;
												lastCAS = lastACT + tRCD;
												swCAS = lastCAS + tCCD;
											}
											if(jump_7)
											{
												lastCAS = counter + tCCD;
												swCAS = lastCAS + tCCD;
											}
											jump_6 = false;
											jump_7 = false;	
											BypassReset = false;
											counter++;
											return scheduledCommand;								
											//tempqueue.erase(RR);	
										}
																																											
									}								
								}
							checkCommand = NULL;	
							}	
							else if (checkCommand->busPacketType == ACT_W)
							{
								// cout<<"the round is   "<<roundType<<endl; 
								// cout<<"counter is   "<<counter<<endl;
								// cout<<"Last ACT     "<<lastACT<<endl;
								// cout<<"Last CAS   "<<lastCAS<<endl;
								// cout<<"SW ACT   "<<swACT<<endl;
								// cout<<"SW CAS   "<<swCAS<<endl;
								// cout<<"Line:   673 "<<endl;
								// cout<<"*********************"<<endl;
								//cout<<"SW CAS 1704 HERE IS   "<<swCAS<<endl;
								if(!blockACT)
								{
									// cout<<"the round is   "<<roundType<<endl; 
									// cout<<"counter is   "<<counter<<endl;
									// cout<<"Last ACT     "<<lastACT<<endl;
									// cout<<"Last CAS   "<<lastCAS<<endl;
									// cout<<"SW ACT   "<<swACT<<endl;
									// cout<<"SW CAS   "<<swCAS<<endl;
									// cout<<"Line:   1807 "<<endl;
									// cout<<"*********************"<<endl;
									if (roundType == false)
									{
										// cout<<"the round is   "<<roundType<<endl; 
										// cout<<"counter is   "<<counter<<endl;
										// cout<<"Last ACT     "<<lastACT<<endl;
										// cout<<"Last CAS   "<<lastCAS<<endl;
										// cout<<"SW ACT   "<<swACT<<endl;
										// cout<<"SW CAS   "<<swCAS<<endl;
										// cout<<"Line:   1817 "<<endl;
										// cout<<"*********************"<<endl;
										if(servicebuffer[RR] == 0)
										{
											for(unsigned int i = 0; i < Order.size(); i++)
											{
												int RR1 = Order.at(i);
												if(tempqueue.find(RR1) == tempqueue.end())
												{
													continue;
												}	
												else
												{
													checkCommand_1 = tempqueue[RR1];	
													if (checkCommand_1->busPacketType == WR)
													{
													
														if(servicebuffer[RR1] == 0)
														{
															if(isIssuable(checkCommand_1))
															{
																
																Order.erase(Order.begin() + i);
																Order.push_back(RR1);
																servicebuffer[RR1] = 1;
																scheduledCommand = checkCommand_1;
																sendCommand(scheduledCommand,0);
																tempqueue.erase(RR1);
																//consideredScheduled[RR1] = 0;
																queuePending[scheduledCommand->requestorID] = false;	
													
																
																cout<<"the round is   "<<roundType<<endl; 
																cout<<"counter is   "<<counter<<endl;
																cout<<"Last ACT     "<<lastACT<<endl;
																cout<<"Last CAS   "<<lastCAS<<endl;
																cout<<"SW ACT   "<<swACT<<endl;
																cout<<"SW CAS   "<<swCAS<<endl;
																cout<<"Line:   Write "<<endl;
																cout<<"*********************"<<endl;
																counter++;
																
																if(jump_7)
																{
																	lastCAS = counter + tCCD;
																	swCAS = lastCAS + tCCD;
																}
																jump_6 = false;
																jump_7 = false;
																BypassReset = false;	
																return scheduledCommand;	
															}																								
														}
													}
												}																																			
											}
											// cout<<"the round is   "<<roundType<<endl; 
											// cout<<"counter is   "<<counter<<endl;
											// cout<<"Last ACT     "<<lastACT<<endl;
											// cout<<"Last CAS   "<<lastCAS<<endl;
											// cout<<"SW ACT   "<<swACT<<endl;
											// cout<<"SW CAS   "<<swCAS<<endl;
											// cout<<"Line:   1790 "<<endl;
											// cout<<"*********************"<<endl;
											//cout<<"SW CAS 1770 HERE IS   "<<swCAS<<endl;
											if (isIssuable(checkCommand))
											{
												
												scheduledCommand = checkCommand;
												sendCommand(scheduledCommand,0);
												tempqueue.erase(RR);
												//consideredScheduled[RR] = 0;
												queuePending[scheduledCommand->requestorID] = false;										
												
											//cout<<"SW CAS 1780 HERE IS   "<<swCAS<<endl;
												if(jump_6)
												{
													//cout<<"SW CAS 1783 HERE IS   "<<swCAS<<endl;
													lastACT = counter;
													lastCAS = lastACT + tRCD;
													swCAS = lastCAS + tCCD;
												}
												if(jump_7)
												{
													//cout<<"SW CAS 1790 HERE IS   "<<swCAS<<endl;
													lastCAS = counter + tCCD;
													swCAS = lastCAS + tCCD;
												}
												//cout<<"SW CAS 1794 HERE IS   "<<swCAS<<endl;
												jump_6 = false;
												jump_7 = false;	
												BypassReset = false;
												cout<<"the round is   "<<roundType<<endl; 
												cout<<"counter is   "<<counter<<endl;
												cout<<"Last ACT     "<<lastACT<<endl;
												cout<<"Last CAS   "<<lastCAS<<endl;
												cout<<"SW ACT   "<<swACT<<endl;
												cout<<"SW CAS   "<<swCAS<<endl;
												cout<<"Line:   ACT_W "<<endl;
												cout<<"*********************"<<endl;
												counter++;
												return scheduledCommand;	
											}
											
										}
										
									}
								
								}	
								
								checkCommand = NULL;
							}
							else if (checkCommand->busPacketType == PRE)
							{
							cout<<"the round is   "<<roundType<<endl; 
							cout<<"counter is   "<<counter<<endl;
							cout<<"Last ACT     "<<lastACT<<endl;
							cout<<"Last CAS   "<<lastCAS<<endl;
							cout<<"SW ACT   "<<swACT<<endl;
							cout<<"SW CAS   "<<swCAS<<endl;
							cout<<"Line:   1897 "<<endl;
							cout<<"*********************"<<endl;
								if (roundType == true)
								{
									//cout<<"koskesh bayad biyay inja 1"<<endl;
									for(unsigned int i = 0; i < Order.size(); i++)
									{
										//cout<<"koskesh bayad biyay inja  2"<<endl;
										int RR1 = Order.at(i);
										
										if(tempqueue.find(RR1) == tempqueue.end())
										{
											//cout<<"koskesh bayad biyay inja 333"<<endl;
											continue;
										}
										else
										{
											//cout<<"koskesh bayad biyay inja444"<<endl;
											checkCommand_1 = tempqueue[RR1];	
											if (checkCommand_1->busPacketType == RD)
											{
											
												if(servicebuffer[RR1] == 0)
												{
													if(isIssuable(checkCommand_1))
													{
														
																				
														Order.erase(Order.begin() + i);
													
														Order.push_back(RR1);
														
														scheduledCommand = checkCommand_1;
																			
														sendCommand(scheduledCommand,0); 
														tempqueue.erase(RR1);
														
														servicebuffer[RR1] = 1;
														
														queuePending[scheduledCommand->requestorID] = false;
														
														cout<<"the round is   "<<roundType<<endl; 
														cout<<"counter is   "<<counter<<endl;
														cout<<"Last ACT     "<<lastACT<<endl;
														cout<<"Last CAS   "<<lastCAS<<endl;
														cout<<"SW ACT   "<<swACT<<endl;
														cout<<"SW CAS   "<<swCAS<<endl;
														cout<<"Line:   Read "<<endl;
														cout<<"*********************"<<endl;
														counter++;
														//cout<<"counter after is  "<<counter<<endl;
														
														if(jump_7)
														{
															lastCAS = counter + tCCD;
															swCAS = lastCAS + tCCD;
														}
														jump_6 = false;
														jump_7 = false;	
														BypassReset = false;
														return scheduledCommand;	
													}																								
												}
												
											}
											
										}
										
																																		
									}
									
									for(unsigned int i = 0; i < Order.size(); i++)
									{
										int RR1 = Order.at(i);

										if(tempqueue.find(RR1) == tempqueue.end())
										{
											continue;
										}
										else
										{
											checkCommand_1 = tempqueue[RR1];	
											if (checkCommand_1->busPacketType == ACT_R)
											{
												cout<<"the round is   "<<roundType<<endl; 
												cout<<"counter is   "<<counter<<endl;
												cout<<"Last ACT     "<<lastACT<<endl;
												cout<<"Last CAS   "<<lastCAS<<endl;
												cout<<"SW ACT   "<<swACT<<endl;
												cout<<"SW CAS   "<<swCAS<<endl;
												cout<<"Line:   1460 "<<endl;
												cout<<"*********************"<<endl;
												if(servicebuffer[RR1] == 0)
												{
													
													if(isIssuable(checkCommand_1))
													{
														
														scheduledCommand = checkCommand;
														sendCommand(scheduledCommand,0);
														tempqueue.erase(RR1);
														
														queuePending[scheduledCommand->requestorID] = false;	
																						
														
														cout<<"the round is   "<<roundType<<endl; 
														cout<<"counter is   "<<counter<<endl;
														cout<<"Last ACT     "<<lastACT<<endl;
														cout<<"Last CAS   "<<lastCAS<<endl;
														cout<<"SW ACT   "<<swACT<<endl;
														cout<<"SW CAS   "<<swCAS<<endl;
														cout<<"Line:   ACT_R "<<endl;
														cout<<"*********************"<<endl;
														counter++;
														if(jump_6)
														{
															lastACT = counter;
															lastCAS = lastACT + tRCD;
															swCAS = lastCAS + tCCD;
															
														}
														if(jump_7)
														{
															lastCAS = counter + tCCD;
															swCAS = lastCAS + tCCD;
														}
														jump_6 = false;
														jump_7 = false;	
														BypassReset = false;
														return scheduledCommand;		
													}																								
												}
											}	
										}																																	
									}
						
									if(isIssuable(checkCommand))
									{
										
										//tempqueue.erase(RR);
										scheduledCommand = checkCommand;
										sendCommand(scheduledCommand,0);
										tempqueue.erase(RR);
										
										queuePending[scheduledCommand->requestorID] = false;										
									
										cout<<"the round is   "<<roundType<<endl; 
										cout<<"counter is   "<<counter<<endl;
										cout<<"Last ACT     "<<lastACT<<endl;
										cout<<"Last CAS   "<<lastCAS<<endl;
										cout<<"SW ACT   "<<swACT<<endl;
										cout<<"SW CAS   "<<swCAS<<endl;
										cout<<"Line:  PRE "<<endl;
										cout<<"*********************"<<endl;
										counter++;	
										jump_6 = false;
										jump_7 = false;	
										BypassReset = false;
										return scheduledCommand;
									}								
								}
								else if (roundType == false)
								{
									cout<<"the round is   "<<roundType<<endl; 
									cout<<"counter is   "<<counter<<endl;
									cout<<"Last ACT     "<<lastACT<<endl;
									cout<<"Last CAS   "<<lastCAS<<endl;
									cout<<"SW ACT   "<<swACT<<endl;
									cout<<"SW CAS   "<<swCAS<<endl;
									cout<<"Line:   2066 "<<endl;
									cout<<"*********************"<<endl;
									if(servicebuffer[RR] == 0)
									{
										cout<<"the round is   "<<roundType<<endl; 
										cout<<"counter is   "<<counter<<endl;
										cout<<"Last ACT     "<<lastACT<<endl;
										cout<<"Last CAS   "<<lastCAS<<endl;
										cout<<"SW ACT   "<<swACT<<endl;
										cout<<"SW CAS   "<<swCAS<<endl;
										cout<<"Line:   2071 "<<endl;
										cout<<"*********************"<<endl;
										for(unsigned int i = 0; i < Order.size(); i++)
										{
											int RR1 = Order.at(i);

											if(tempqueue.find(RR1) == tempqueue.end())
											{
												cout<<"the round is   "<<roundType<<endl; 
												cout<<"counter is   "<<counter<<endl;
												cout<<"Last ACT     "<<lastACT<<endl;
												cout<<"Last CAS   "<<lastCAS<<endl;
												cout<<"SW ACT   "<<swACT<<endl;
												cout<<"SW CAS   "<<swCAS<<endl;
												cout<<"Line:   2087 "<<endl;
												cout<<"*********************"<<endl;
												continue;
											}
											else
											{
												checkCommand_1 = tempqueue[RR1];	
												if (checkCommand_1->busPacketType == WR)
												{
													if(servicebuffer[RR1] == 0)
													{
														if(isIssuable(checkCommand_1))
														{
															
															Order.erase(Order.begin() + i);
															Order.push_back(RR1);												
															scheduledCommand = checkCommand_1;
															sendCommand(scheduledCommand,0);
															tempqueue.erase(RR1);
															servicebuffer[RR1] = 1;
															queuePending[scheduledCommand->requestorID] = false;											
															
															cout<<"the round is   "<<roundType<<endl; 
															cout<<"counter is   "<<counter<<endl;
															cout<<"Last ACT     "<<lastACT<<endl;
															cout<<"Last CAS   "<<lastCAS<<endl;
															cout<<"SW ACT   "<<swACT<<endl;
															cout<<"SW CAS   "<<swCAS<<endl;
															cout<<"Line:   WR "<<endl;
															cout<<"*********************"<<endl;
															counter++;
															
															if(jump_7)
															{
																lastCAS = counter + tCCD;
																swCAS = lastCAS + tCCD;
															}
															jump_6 = false;
															jump_7 = false;	
															BypassReset = false;
															return scheduledCommand;											
														}																								
													}
												}	
											}																										
										}
										for(unsigned int i = 0; i < Order.size(); i++)
										{
											cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   20144 "<<endl;
											cout<<"*********************"<<endl;
											int RR1 = Order.at(i);
											if(tempqueue.find(RR1) == tempqueue.end())
											{
												continue;
											}
											else
											{
												cout<<"the round is   "<<roundType<<endl; 
												cout<<"counter is   "<<counter<<endl;
												cout<<"Last ACT     "<<lastACT<<endl;
												cout<<"Last CAS   "<<lastCAS<<endl;
												cout<<"SW ACT   "<<swACT<<endl;
												cout<<"SW CAS   "<<swCAS<<endl;
												cout<<"Line:   2159 "<<endl;
												cout<<"*********************"<<endl;
												checkCommand_1 = tempqueue[RR1];	
												if (checkCommand_1->busPacketType == ACT_W)
												{
													cout<<"the round is   "<<roundType<<endl; 
													cout<<"counter is   "<<counter<<endl;
													cout<<"Last ACT     "<<lastACT<<endl;
													cout<<"Last CAS   "<<lastCAS<<endl;
													cout<<"SW ACT   "<<swACT<<endl;
													cout<<"SW CAS   "<<swCAS<<endl;
													cout<<"Line:   2121 "<<endl;
													cout<<"*********************"<<endl;
													if(servicebuffer[RR1] == 0)
													{
														if(isIssuable(checkCommand_1))
														{
															
															cout<<"*********************"<<endl;
															scheduledCommand = checkCommand;
															sendCommand(scheduledCommand,0);
															tempqueue.erase(RR1);
													
															queuePending[scheduledCommand->requestorID] = false;	
																								
															
															cout<<"the round is   "<<roundType<<endl; 
															cout<<"counter is   "<<counter<<endl;
															cout<<"Last ACT     "<<lastACT<<endl;
															cout<<"Last CAS   "<<lastCAS<<endl;
															cout<<"SW ACT   "<<swACT<<endl;
															cout<<"SW CAS   "<<swCAS<<endl;
															cout<<"Line:   ACT_W "<<endl;
															counter++;
															if(jump_6)
															{
																lastACT = counter;
																lastCAS = lastACT + tRCD;
																swCAS = lastCAS + tCCD;
															}
															if(jump_7)
															{
																lastCAS = counter + tCCD;
																swCAS = lastCAS + tCCD;
															}
															jump_6 = false;
															jump_7 = false;	
															BypassReset = false;
															return scheduledCommand;		
														}																								
													}
												}	/* code */
											}																									
										}
										if(isIssuable(checkCommand))
										{
											
											//tempqueue.erase(RR);
											scheduledCommand = checkCommand;
											sendCommand(scheduledCommand,0);
											tempqueue.erase(RR);
											//consideredScheduled[RR] = 0;
											queuePending[scheduledCommand->requestorID] = false;										
											
											//cout<<"the round is   "<<roundType<<endl; 
											cout<<"counter is   "<<counter<<endl;
											cout<<"Last ACT     "<<lastACT<<endl;
											cout<<"Last CAS   "<<lastCAS<<endl;
											cout<<"SW ACT   "<<swACT<<endl;
											cout<<"SW CAS   "<<swCAS<<endl;
											cout<<"Line:   PRE "<<endl;
											cout<<"*********************"<<endl;
											counter++;
											jump_6 = false;
											jump_7 = false;	
											BypassReset = false;
											return scheduledCommand;
										}
									}
								}
							checkCommand = NULL;																						
							}
						}												
					}
					
					//if there is nothing ready in the Q, we can simply issue the opportunistically
					
					if(counter < (swCAS - tRCD))
					{
						if((jump_6 == false) && (jump_7 == false))
						{
							for(unsigned int i = 0; i < Order.size(); i++)
							{				
								int RR = Order.at(i);
								if(tempqueue.find(RR) == tempqueue.end())
								{
									
									continue;
								}
								else
								{
									
									checkCommand = tempqueue[RR];
								
									if (checkCommand->busPacketType == RD)
									{				
													
										if (roundType == true)
										{
											
											if(servicebuffer[RR] == 1)
											{											
												if(isIssuable(checkCommand))
												{
													//tempqueue.erase(RR);
													Order.erase(Order.begin() + i);
													Order.push_back(RR);
													//lastCAS = lastCAS + tCCD;	
													scheduledCommand = checkCommand;
													sendCommand(scheduledCommand,0);
													tempqueue.erase(RR);
													//consideredScheduled[RR] = 0;
													queuePending[scheduledCommand->requestorID] = false;										
													
													cout<<"the round is   "<<roundType<<endl; 
													cout<<"counter is   "<<counter<<endl;
													cout<<"Last ACT     "<<lastACT<<endl;
													cout<<"Last CAS   "<<lastCAS<<endl;
													cout<<"SW ACT   "<<swACT<<endl;
													cout<<"SW CAS   "<<swCAS<<endl;
													cout<<"Line:   Read Opp "<<endl;
													cout<<"*********************"<<endl;
													countROPP++;	
													counter++;		
													BypassReset = false;									
													return scheduledCommand;
												}															
											}
										}
									checkCommand = NULL;
									}
									else if (checkCommand->busPacketType == WR)
									{
										
										if (roundType == false)
										{
											
											if(servicebuffer[RR] == 1)
											{
												
												if(isIssuable(checkCommand))
												{
													
													Order.erase(Order.begin() + i);
													Order.push_back(RR);
													//lastCAS = lastCAS + tCCD;	
													scheduledCommand = checkCommand;
													sendCommand(scheduledCommand,0);
													tempqueue.erase(RR);
													//consideredScheduled[RR] = 0;
													queuePending[scheduledCommand->requestorID] = false;										
													
													cout<<"the round is   "<<roundType<<endl; 
													cout<<"counter is   "<<counter<<endl;
													cout<<"Last ACT     "<<lastACT<<endl;
													cout<<"Last CAS   "<<lastCAS<<endl;
													cout<<"SW ACT   "<<swACT<<endl;
													cout<<"SW CAS   "<<swCAS<<endl;
													cout<<"Line:   Write Opp "<<endl;
													cout<<"*********************"<<endl;
													countROPP++;
													BypassReset = false;
													counter++;
													return scheduledCommand;
												}
																											
											}
										
										}
									
									checkCommand = NULL;
									}	
								}																	
							}
						}
					}
					
					jump_6 = false;
					jump_7 = false;						
				}	
						
				
				cout<<"the round is   "<<roundType<<endl; 
				cout<<"counter is   "<<counter<<endl;
				cout<<"Last ACT     "<<lastACT<<endl;
				cout<<"Last CAS   "<<lastCAS<<endl;
				cout<<"SW ACT   "<<swACT<<endl;
				cout<<"SW CAS   "<<swCAS<<endl;
				cout<<"Line:    Waste Cycle "<<endl;
				cout<<"*********************"<<endl;
				cout<<"Write opp = "<<countWOPP<<endl;
				cout<<"Read opp = "<<countROPP<<endl;
				counter++;
				BypassReset = false;
			return NULL;		
			}		
		};	
	}