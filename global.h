
/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

// Define global parameter (Architecture and Debug definition)
#ifdef DEBUG_ENABLED
#define DEBUG(str) std::cerr<< str <<std::endl;
#else
#define DEBUG(str) 
#endif

#define PRINT(str) std::cerr<< str <<std::endl;
#define ERROR(str) std::cerr<<"[ERROR ("<<__FILE__<<":"<<__LINE__<<")]: "<<str<<std::endl;

namespace DRAMController
{
	// Memory Organization Table
	enum memOrg
	{
		Rank, BankGroup, Bank, SubArray, Row, Column
	};
}