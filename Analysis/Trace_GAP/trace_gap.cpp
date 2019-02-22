#include <iostream>
#include<fstream>
#include <math.h>
#include <cmath>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <cstring>

using namespace std;

int main()
{
	int i = 1;
	int prev;
	ifstream L;
	ofstream fout("rspeed_IO_WB_GAP.txt");
	string add;
	char* temp = new char [100];
	string key ("0");
	L.open("rspeed.txt");
	while(getline(L,add))
	{	
		if(i == 1){
			i=0;
			fout<<add<<endl;
			strcpy(temp,add.c_str());
			char* str = strtok(temp,"	");
			char* str2 = strtok(NULL,"	");
			char* str3 = strtok(NULL,"	");
			prev = atoi(str3);
			//cout<<str3<<endl;
		}
		else
		{
			strcpy(temp,add.c_str());
			char* str = strtok(temp,"	");
			char* str2 = strtok(NULL,"	");
			char* str3 = strtok(NULL,"	");

			fout<<str<<" "<<str2<<" "<<atoi(str3)-prev<<endl;
			prev=atoi(str3);
		}
		
		
		
		//size_t found = add.rfind("key");
		//cout<<str3<<endl;
	}		
}
	
