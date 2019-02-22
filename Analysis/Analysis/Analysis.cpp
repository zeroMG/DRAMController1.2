#include <iostream>

#include <math.h>
#include <cmath>

using namespace std;

int tACT(int m,
    int tFAW,
    int tRRD){
int x = m;
int out;
out = (m-1)*tRRD + ceil((m-1)/4)*(tFAW - 4*tRRD);
return out;
};


int main(){
    int tBUS;
    int tWtoR;
    int tRTW;
    int tRL;
    int tWL;
    int tFAW;
    int tRRD;
    int tCCD;
    int tRCD;
    int tRP;
    int tRTP;
    int tRC;
    int tRAS;
    int tWTR;
    int tWR;
    int opt;
    int n = 4; //number of requestors
    cout<<"Choose the device that you want the constraints for   "<<endl;
    cout<<"1- DDR3 800D   "<<endl;
    cout<<"2- DDR3 1066E   "<<endl;
    cout<<"3- DDR3 1333G   "<<endl;
    cout<<"4- DDR3 1600H   "<<endl;
    cout<<"5- DDR3 1866K   "<<endl;
    cout<<"6- DDR3 2133L   "<<endl;
    cin>>opt;

    if (opt == 1){
        
        tBUS = 4;
        tRTW = 6;
        tRL = 5;
        tWL = 5;
        tFAW = 16;
        tRRD = 4;
        tCCD = 4;
        tRCD = 5;
        tRP = 5;
        tRTP = 4;
        tRC = 20;
        tRAS = 15;
        tWR = 6;
        tWTR = 4;
    }
    else if(opt == 2){
        tBUS = 4;
        tRTW = 6;
        tRL = 6;
        tWL = 6;
        tFAW = 20;
        tRRD = 4;
        tCCD = 4;
        tRCD = 6;
        tRP = 6;
        tRTP = 4;
        tRC = 26;
        tRAS = 20;
        tWTR = 4;
        tWR = 8;
    }
    else if (opt ==3){
        tBUS = 4;
        tRTW = 7;
        tRL = 8;
        tWL = 8;
        tFAW = 20;
        tRRD = 4;
        tCCD = 4;
        tRCD = 8;
        tRP = 8;
        tRTP = 5;
        tRC = 32;
        tRAS = 24;
        tWTR = 5;
        tWR = 10;
    }
    else if(opt == 4){
        tBUS = 4;
        tRTW = 7;
        tRL = 9;
        tWL = 8;
        tFAW = 24;
        tRRD = 5;
        tCCD = 4;
        tRCD = 9;
        tRP = 9;
        tRTP = 6;
        tRC = 37;
        tRAS = 28;
        tWTR = 6;
        tWR = 12;
    }
    else if (opt ==5){
        tBUS = 4;
        tRTW = 8;
        tRL = 11;
        tWL = 11;
        tFAW = 26;
        tRRD = 5;
        tCCD = 4;
        tRCD = 11;
        tRP = 11;
        tRTP = 7;
        tRC = 43;
        tRAS = 32;
        tWTR = 7;
        tWR = 14;
    }
    else if(opt == 6){
        tBUS = 4;
        tRTW = 8;
        tRL = 12;
        tWL = 10;
        tFAW = 27;
        tRRD = 5;
        tCCD = 4;
        tRCD = 12;
        tRP = 12;
        tRTP = 8;
        tRC = 48;
        tRAS = 36;
        tWTR = 8;
        tWR = 16;
    }
    else {
        cout<<"invalid option"<<endl;
    }
    int tDelayR = tRL + tBUS;
    tWtoR = tWL + tWTR + tBUS;
    int CRAW;
    int CRAR;
    int CWAW;
    int CWAR;
    int ORAW;
    int ORAR;

    int findn;
    int i;

    for (i = 0; i< 16; i++){
        if (tRAS + tRP <= tACT((i-1),tFAW,tRRD)){
            findn = i;
            break;
        }
    }

    //Close Read arrive during Write round/no-round
    CRAW = tWtoR + tRL + tBUS + tRCD + 1 + max(tACT((n-1),tFAW,tRRD), (n-2)*tCCD) + tCCD - 1 + tCCD - 1 + (n-1) + tRP;
    //Close Read arrive during Read round/no-round
   
    if(n == (findn - 1)){
        CRAR = tWtoR + tRCD + 1 + tRL + tBUS + tRC + tACT((n-1),tFAW,tRRD) - (tCCD + 1) + (tCCD - 1), tWtoR + tRCD + 1 + tRL + tBUS + (n-1)*tCCD + tACT((n-1),tFAW,tRRD) + (tCCD - 1) + (tCCD - 1)+ (n-1) + tRP;
    }
    else if(n >= findn){
        int first = tWtoR + tRL + tBUS + tCCD + max((n-2)*tCCD + tRTW + tACT((n-1),tFAW,tRRD) + 1 - tDelayR, tACT((n-1),tFAW,tRRD) + tACT((n),tFAW,tRRD) + 1 - tDelayR) + tCCD -1 + (n-1) + tRP ;
        int second = max(tWtoR + tRCD + 1 + tRL + tBUS + tRC + tACT((n-1), tFAW,tRRD) - (tCCD + 1) + (tCCD - 1), tWtoR + tRCD + 1 + tRL + tBUS + (n-1)*tCCD + tACT((n-1),tFAW,tRRD) + (tCCD - 1) + (tCCD - 1)+ (n-1) + tRP);
        CRAR = max(first,second);
    }
    else{
        CRAR = CRAW;
    }
    
    //Close Write arrive during Write round/no-round
    CWAW = tRCD + 1 + max(tACT((n),tFAW,tRRD),(n-1)*tCCD) + (tCCD - 1) + (tCCD - 1) + tWL + tBUS + (n-1) + tRP;
    //Close Write arrive during Read round/no-round
    CWAR = tRTW + tWL + tBUS + tRCD + 1 + max(tACT((n-1),tFAW,tRRD), (n-2)*tCCD) + tCCD -1 + tCCD - 1 + (n-1) + tRP;
    //Open Read arrive during Write round/no-round
    ORAW = tWtoR + tRL + tBUS + tRCD + 1 + max((n-2)*tCCD , tACT((n-1),tFAW,tRRD)) + tCCD - 1;

    //Open Read arrive during Read round/no-round
    if(n>=findn){
        ORAR = tWtoR + tRL + tBUS + tRCD + 1 + tACT((n-1),tFAW,tRRD) + tACT((n),tFAW,tRRD) - (tRCD + 1 + tDelayR) + tCCD - 1;
    }
    else{
        ORAR = ORAW;
    }
   


    cout<<"******************Analysis Result for Round******************"<<endl;
    cout<<"The Analysis is for the "<<opt<<endl;
    cout<<"The number of Requestor is   "<<n<<endl;
    cout<<"                                                             "<<endl;
    cout<<"Worst Case for the close Read arrive during Write round is   "<<CRAW<<endl;
    cout<<"-------------------------------------------------------------------"<<endl;
    cout<<"Worst Case for the close Read arrive during Read round is   "<<CRAR<<endl;
    cout<<"-------------------------------------------------------------------"<<endl;
    cout<<"Worst Case for the close Write arrive during Write round is   "<<CWAW<<endl;
    cout<<"-------------------------------------------------------------------"<<endl;
    cout<<"Worst Case for the close Write arrive during Read round is   "<<CWAR<<endl;
    cout<<"-------------------------------------------------------------------"<<endl;
    cout<<"Worst Case for the Open Read arrive during Write round is   "<<ORAW<<endl;
    cout<<"-------------------------------------------------------------------"<<endl;
    cout<<"Worst Case for the Open Read arrive during Read round is   "<<ORAR<<endl;
    cout<<"-------------------------------------------------------------------"<<endl;


}










