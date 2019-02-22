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

	ifstream L;
	ofstream fout("cache_background_arrive0.txt");
	string add;
	char* temp = new char [100];
	string key ("0");
	L.open("cache.txt");
	while(getline(L,add))
	{	
		strcpy(temp,add.c_str());
		char* str = strtok(temp," ");
		char* str2 = strtok(NULL," ");
		fout<<str<<" "<<str2<<" 0"<<endl;
		
		//size_t found = add.rfind("key");
		//cout<<str3<<endl;
	}		
}
	
