	/*********************************************************************************
	*  Copyright (c) 2018-2019, 
	*  Reza Mirosanlou
	*  University of Waterloo               
	*  All rights reserved.
	*********************************************************************************/

	#include "../../CommandScheduler.h"
	//#include "MemoryController.h"
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
			int oppslot;
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
			//int check;

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
				oppslot = 0;
				//int check = 0;
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
				//////////cout<<"tCCD   "<<tCCD<<endl;
				tRRD = getTiming("tRRD");
				//////////cout<<"tRRD   "<<tRRD<<endl;
				tRCD = getTiming("tRCD");
				//////////cout<<"tRCD   "<<tRCD<<endl;
				tFAW = getTiming("tFAW");
				//////////cout<<"tFAW   "<<tFAW<<endl;
				tRTW = getTiming("tRTW");
				//////////cout<<"tRTW   "<<tRTW<<endl;
				tWL  = getTiming("tWL");
				//////////cout<<"tWL   "<<tWL<<endl;
				tWTR = getTiming("tWTR"); 
				//////////cout<<"tWTR   "<<tWTR<<endl;
				tBUS = getTiming("tBus");
				//////////cout<<"tBUS   "<<tBUS<<endl;
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
									//////////cout << checkCommand->busPacketType<<endl;
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
									if(consideredScheduled[RR] == 1)
									{
										if(servicebuffer[RR] == 0)
										{									
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

					if (!jump_4)
					{
						
						if (roundType == true)
						{
							for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
							{			
								checkCommand = it->second;
								if (checkCommand->busPacketType == WR) 
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
									first = true;
									blockACT = false;
									countACT = 0;
									countFAW = 0;
									countCAS = 0;
									jump_1 = true;							
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
											BypassReset = false;																				
											jump_3 = true;							
										}
									}	

									for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
									{			
										checkCommand = it->second;
										if (checkCommand->busPacketType == ACT_R)
										{											
											BypassReset = false;
											jump_3 = true;
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
							for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
							{		
								checkCommand = it->second;
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
									first = true;
									countACT = 0;
									countCAS = 0;	
									countFAW = 0;
									jump_1 = true;			
								}
							}	
							if(!jump_1)
							{
								for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
								{			
									checkCommand = it->second;
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
										lastACT = 0;
										blockACT = false;
										first = true;
										countACT = 0;
										countCAS = 0;
										countFAW = 0;
										jump_2 = true;
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
										if (checkCommand->busPacketType == WR) 
										{																				
											BypassReset = false;																				
											jump_3 = true;		
										}
									}	

									for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
									{			
										checkCommand = it->second;
										if (checkCommand->busPacketType == ACT_W)
										{											
											BypassReset = false;
											jump_3 = true;	
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
									}
									else 
									{
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
					// ////////cout<<"the round is   "<<roundType<<endl; 
					// ////////cout<<"counter is   "<<counter<<endl;
					// ////////cout<<"Last ACT     "<<lastACT<<endl;
					// ////////cout<<"Last CAS   "<<lastCAS<<endl;
					// ////////cout<<"SW ACT   "<<swACT<<endl;
					// ////////cout<<"SW CAS   "<<swCAS<<endl;
					// ////////cout<<"Line:   483 "<<endl;
					// ////////cout<<"*********************"<<endl;
				}
				//////////cout<<"*******BAAYAD INJA BASHE  2222*******"<<endl;


				// ////////cout<<"**********Chaap kon***********"<<endl;

				// for(unsigned int i = 0; i < Order.size(); i++)
				// {	
					
				// 	int RR = Order.at(i);
				// 	////////cout<<"ORder is=        "<<RR<<endl;
				// }					
				// for(std::map<unsigned int, BusPacket*>::iterator it=tempqueue.begin(); it!=tempqueue.end(); it++) // for all requestors from "num". getRequestorIndex() gives the number of requestors	
				// {			
				// 	////////cout<<" the command ghabl az too raftan requestor ID "<<it->first<<endl;
				// 	checkCommand = it->second;
				// 	////////cout<<" the command ghabl az too raftan  "<<checkCommand->busPacketType<<endl;
				// }
				// checkCommand = NULL;


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
									if (consideredScheduled[RR] == 0)
									{
										consideredScheduled[RR] = 1;
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
											swCAS = swCAS + tCCD;		
											swACT = swCAS - tRCD;
											if(swACT < 0)
											{
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
							if (roundType == false)
							{
								if(servicebuffer[RR] == 0)
								{							
									if (consideredScheduled[RR] == 0)
									{									
										consideredScheduled[RR] = 1;
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
											first = false;
											swCAS = lastCAS + tCCD;
											swACT = 0;
										}
										else 
										{
											countCAS++;
											lastCAS = lastCAS + tCCD;											
											swCAS = swCAS + tCCD;	
											swACT = swCAS - tRCD;	
											if(swACT < 0)
											{
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
																	swCAS = swCAS + tCCD;
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
							if(!blockACT)
							{
								if (roundType == false)
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
													if (checkCommand_1->busPacketType == WR)
													{
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
																	swCAS = swCAS + tCCD;
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
						else if (checkCommand->busPacketType == PRE)
						{				
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
												if(servicebuffer[RR1] == 0)
												{
													if (consideredScheduled[RR1] == 0)
													{
														consideredScheduled[RR1] = 1;															
														if (first)
														{
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
															countCAS++;
															lastCAS = lastCAS + tCCD;														
															swCAS = swCAS + tCCD;	
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
										// ////////cout<<"Line:   1282 "<<endl;
										// ////////cout<<"*********************"<<endl;
									for(unsigned int i = 0; i < Order.size(); i++)
									{
										int RR1 = Order.at(i);

										if (tempqueue.find(RR1) == tempqueue.end())
										{
											continue;
										}
										else
										{
											// ////////cout<<"Line:   1295 "<<endl;
											// ////////cout<<"*********************"<<endl;
											checkCommand_1 = tempqueue[RR1];
											// ////////cout<<"Line:   1299 "<<endl;
											// ////////cout<<"*********************"<<endl;	
											if (checkCommand_1->busPacketType == ACT_R)
											{
												////////cout<<"the round is   "<<roundType<<endl; 
												////////cout<<"counter is   "<<counter<<endl;
												////////cout<<"Last ACT     "<<lastACT<<endl;
												////////cout<<"Last CAS   "<<lastCAS<<endl;
												////////cout<<"SW ACT   "<<swACT<<endl;
												////////cout<<"SW CAS   "<<swCAS<<endl;
												////////cout<<"Line:   580 "<<endl;
												////////cout<<"*********************"<<endl;
												if(!blockACT)
												{
													if(servicebuffer[RR1] == 0)
													{
														if(consideredScheduled[RR1] == 0)
														{
															//update ACT
															consideredScheduled[RR1] = 1;
															
															if(first)
															{
																// ////////cout<<"Line:   1316 "<<endl;
																// ////////cout<<"*********************"<<endl;
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
																// ////////cout<<"Line:   1332 "<<endl;
																// ////////cout<<"*********************"<<endl;
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
																// ////////cout<<"Line:   1365 "<<endl;
																// ////////cout<<"*********************"<<endl;
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
										// 	////////cout<<"Line:   1402 "<<endl;
										// ////////cout<<"*********************"<<endl;
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
													////////cout<<"the round is   "<<roundType<<endl; 
													////////cout<<"counter is   "<<counter<<endl;
													////////cout<<"Last ACT     "<<lastACT<<endl;
													////////cout<<"Last CAS   "<<lastCAS<<endl;
													////////cout<<"SW ACT   "<<swACT<<endl;
													////////cout<<"SW CAS   "<<swCAS<<endl;
													////////cout<<"Line:   941 "<<endl;
													////////cout<<"*********************"<<endl;
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
																swCAS = swCAS + tCCD;
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
													if(!blockACT)
													{	
														////////cout<<"the round is   "<<roundType<<endl; 
														////////cout<<"counter is   "<<counter<<endl;
														////////cout<<"Last ACT     "<<lastACT<<endl;
														////////cout<<"Last CAS   "<<lastCAS<<endl;
														////////cout<<"SW ACT   "<<swACT<<endl;
														////////cout<<"SW CAS   "<<swCAS<<endl;
														////////cout<<"Line:   983 "<<endl;
														////////cout<<"*********************"<<endl;
														
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
							}
							// ////////cout<<"Line:   1580 "<<endl;
							// 			////////cout<<"*********************"<<endl;							
						}
					}												
				}
				if(BypassReset == true)
				{
					swCAS = counter + 1;
				}
	
				if(!tempqueue.empty())
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
								if(roundType == true)
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
											queuePending[scheduledCommand->requestorID] = false;	
												// cout<<"last CAS  "<<lastCAS<<endl;
												// 				cout<<"SW CAS  "<<swCAS<<endl;
											cout<<counter<<"    Read "<<endl;
											counter++;
											if(jump_7)
											{
												lastCAS = counter;
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
							else if (checkCommand->busPacketType == WR)
							{
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
											queuePending[scheduledCommand->requestorID] = false;	
											
												// cout<<"last CAS  "<<lastCAS<<endl;
												// 				cout<<"SW CAS  "<<swCAS<<endl;
											cout<<counter<<"    Write  "<<endl;
											counter++;
											if(jump_7)
											{
												lastCAS = counter;
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
															queuePending[scheduledCommand->requestorID] = false;	
													// cout<<"last CAS  "<<lastCAS<<endl;
													// 			cout<<"SW CAS  "<<swCAS<<endl;
															cout<<counter<<"    Read "<<endl;
															counter++;
															if(jump_7)
															{
																lastCAS = counter;
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
										if(!blockACT)
										{
											if(isIssuable(checkCommand))
											{	
												scheduledCommand = checkCommand;
												sendCommand(scheduledCommand,0);
												tempqueue.erase(RR);
												queuePending[scheduledCommand->requestorID] = false;
												// cout<<"last CAS  "<<lastCAS<<endl;
												// 				cout<<"SW CAS  "<<swCAS<<endl;
												cout<<counter<<"    ACT_R "<<endl;
												if(jump_6)
												{
													lastACT = counter;
													lastCAS = lastACT + tRCD;
													swCAS = lastCAS + tCCD;
												}
												if(jump_7)
												{
													lastCAS = counter;
													swCAS = lastCAS + tCCD;
												}
												jump_6 = false;
												jump_7 = false;	
												BypassReset = false;
												counter++;
												return scheduledCommand;
											}
										}																																	
									}								
								}
							checkCommand = NULL;	
							}	
							else if (checkCommand->busPacketType == ACT_W)
							{
								if(!blockACT)
								{
									if (roundType == false)
									{
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
																queuePending[scheduledCommand->requestorID] = false;
																// cout<<"last CAS  "<<lastCAS<<endl;
																// cout<<"SW CAS  "<<swCAS<<endl;
																cout<<counter<<"    Write "<<endl;
																counter++;
																if(jump_7)
																{
																	lastCAS = counter;
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
											if (isIssuable(checkCommand))
											{	
												scheduledCommand = checkCommand;
												sendCommand(scheduledCommand,0);
												tempqueue.erase(RR);
												queuePending[scheduledCommand->requestorID] = false;
												if(jump_6)
												{
													lastACT = counter;
													lastCAS = lastACT + tRCD;
													swCAS = lastCAS + tCCD;
												}
												if(jump_7)
												{
													lastCAS = counter;
												swCAS = lastCAS + tCCD;
												}
												jump_6 = false;
												jump_7 = false;	
												BypassReset = false;
												// cout<<"last CAS  "<<lastCAS<<endl;
												// 	cout<<"SW CAS  "<<swCAS<<endl;
												cout<<counter<<"    ACT_W "<<endl;
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
								if (roundType == true)
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
													// 	cout<<"last CAS  "<<lastCAS<<endl;
													// cout<<"SW CAS  "<<swCAS<<endl;
														cout<<counter<<"    Read "<<endl;
														counter++;
														if(jump_7)
														{
															lastCAS = counter;
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
												if(!blockACT)
												{
													if(servicebuffer[RR1] == 0)
													{

														if(isIssuable(checkCommand_1))
														{
															scheduledCommand = checkCommand_1;
															sendCommand(scheduledCommand,0);
															tempqueue.erase(RR1);
															queuePending[scheduledCommand->requestorID] = false;
													// 		cout<<"last CAS  "<<lastCAS<<endl;
													// cout<<"SW CAS  "<<swCAS<<endl;
															cout<<counter<<"    ACT_R "<<endl;
															counter++;
															if(jump_6)
															{
																lastACT = counter;
																lastCAS = lastACT + tRCD;
																swCAS = lastCAS + tCCD;
															}
															if(jump_7)
															{
																lastCAS = counter;
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
									}
						
									if(isIssuable(checkCommand))
									{
										scheduledCommand = checkCommand;
										sendCommand(scheduledCommand,0);
										tempqueue.erase(RR);
										
										queuePending[scheduledCommand->requestorID] = false;										
									// cout<<"last CAS  "<<lastCAS<<endl;
									// 				cout<<"SW CAS  "<<swCAS<<endl;
										cout<<counter<<"    PRE "<<endl;
										counter++;	
										jump_6 = false;
										jump_7 = false;	
										BypassReset = false;
										return scheduledCommand;
									}								
								}
								else if (roundType == false)
								{
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
															scheduledCommand = checkCommand_1;
															sendCommand(scheduledCommand,0);
															tempqueue.erase(RR1);
															servicebuffer[RR1] = 1;
															queuePending[scheduledCommand->requestorID] = false;
													// 		cout<<"last CAS  "<<lastCAS<<endl;
													// cout<<"SW CAS  "<<swCAS<<endl;		
															cout<<counter<<"    Write "<<endl;
															counter++;
															if(jump_7)
															{
																lastCAS = counter;
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
												if (checkCommand_1->busPacketType == ACT_W)
												{
													if(!blockACT)
													{
														if(servicebuffer[RR1] == 0)
														{
															if(isIssuable(checkCommand_1))
															{
																scheduledCommand = checkCommand_1;
																sendCommand(scheduledCommand,0);
																tempqueue.erase(RR1);
																queuePending[scheduledCommand->requestorID] = false;	
													// 			cout<<"last CAS  "<<lastCAS<<endl;
													// cout<<"SW CAS  "<<swCAS<<endl;
																cout<<counter<<"    ACT_W "<<endl;
																counter++;
																if(jump_6)
																{
																	lastACT = counter;
																	lastCAS = lastACT + tRCD;
																	swCAS = lastCAS + tCCD;
																}
																if(jump_7)
																{
																	lastCAS = counter;
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
										}
										if(isIssuable(checkCommand))
										{
											scheduledCommand = checkCommand;
											sendCommand(scheduledCommand,0);
											tempqueue.erase(RR);
											queuePending[scheduledCommand->requestorID] = false;
											// cout<<"last CAS  "<<lastCAS<<endl;
											// 		cout<<"SW CAS  "<<swCAS<<endl;	
											cout<<counter<<"    PRE Opp "<<endl;
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
					
					// if(counter < (swCAS - tRCD))
					// {
					// 	if((jump_6 == false) && (jump_7 == false))
					// 	{	
							
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
												oppslot++;										
												if(isIssuable(checkCommand))
												{ 
													Order.erase(Order.begin() + i);
													Order.push_back(RR);
													scheduledCommand = checkCommand;
													sendCommand(scheduledCommand,0);
													tempqueue.erase(RR);
													queuePending[scheduledCommand->requestorID] = false;
													// cout<<"last CAS  "<<lastCAS<<endl;
													// cout<<"SW CAS  "<<swCAS<<endl;	
													cout<<counter<<"    Read Opp "<<endl;
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
												oppslot++;
												if(isIssuable(checkCommand))
												{
													Order.erase(Order.begin() + i);
													Order.push_back(RR);
													scheduledCommand = checkCommand;
													sendCommand(scheduledCommand,0);
													tempqueue.erase(RR);
													queuePending[scheduledCommand->requestorID] = false;	
													// cout<<"last CAS  "<<lastCAS<<endl;
													// cout<<"SW CAS  "<<swCAS<<endl;	
													cout<<counter<<"    Write Opp "<<endl;
													countWOPP++;
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
						//}	
					//}
					
					jump_6 = false;
					jump_7 = false;						
				}	
				cout<<"last CAS  "<<lastCAS<<endl;
				cout<<"SW CAS  "<<swCAS<<endl;	
				cout<<counter<<"    None "<<endl;
				counter++;
				BypassReset = false;
			return NULL;		
			}		
		};	
	}