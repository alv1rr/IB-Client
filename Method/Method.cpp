#include "Method.h"
#include "../globals.h"

#include <iostream>
#include <fstream>
#include <sstream>


///Example method from Dip Buyer group:
///buying stocks, price of which dropped too deep for a short period of time.
///Target fixed 5-7%

void Method(char *symbol)
{

std::ofstream fres, fPosOpen, flog;

std::ostringstream LogString;

double shares(0); ///number of shares

int MarketPosition(0);
int EntryDate(0);
int ExitDate(0);
double EntryPrice(0);
double TargPrice(0);
double ExitPrice(0);

double CommEntry(0), CommExit(0);

double TradeProfit(0);

int BarStart(0);

/// Also the following global variables defined in file globals.cpp are used in this file:
/// BarsNum, Len,
/// global array d[];
/// Parameters of the method:
///double EntryStDevKoeff;
///double Target;
///double TradeSize;
///double MinPrice;
///double MaxPrice;
///int DaysNum; ///Number of Days to calculate MA, StDev, and request data for
///double k; /// k=EntryStDevKoeff*StDevNorm(d, i-1, DaysNum);  EntryPrice = floor(d[i-1].Low*(1-k)*100)/100;



if (BarsNum > DaysNum)
{

fres.open("Result.txt", std::ios::out | std::ios::app);
if(!fres.is_open())
{
    LogString << "Method: can not write into file Result.txt\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Method: can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();
    return;
}
fres.setf(std::ios::fixed);
fres.precision(2);



/// Beginning of the main loop

BarStart = DaysNum;

for(int i=BarStart; i<BarsNum; i++)
{



/// EXITS started
TargPrice = ceil((EntryPrice+EntryPrice*StDevNorm(d, i-1, DaysNum)*TargStDevKoeff)*20)/20;


                                               ///AtOpen Exit from Long

if ( MarketPosition && d[i].Open > TargPrice )
{
MarketPosition=0;
ExitPrice=d[i].Open;
ExitDate=d[i].Date;
CommEntry=0.005*shares; if (CommEntry>0.005*shares*EntryPrice) CommEntry=0.005*shares*EntryPrice; if (CommEntry<1) CommEntry=1;
CommExit=0.005*shares; if (CommExit>0.005*shares*ExitPrice) CommExit=0.005*shares*ExitPrice; if (CommExit<1) CommExit=1;

TradeProfit=shares*(ExitPrice-EntryPrice)-CommEntry-CommExit;

fres << symbol << "," << EntryDate << "," << ExitDate << "," << shares << "," << EntryPrice << "," << ExitPrice << "," << TradeProfit << std::endl;
}



                                               ///Target Exit from Long

if ( MarketPosition && d[i].High > TargPrice )
{
MarketPosition=0;
ExitPrice=TargPrice;
ExitDate=d[i].Date;
CommEntry=0.005*shares; if (CommEntry>0.005*shares*EntryPrice) CommEntry=0.005*shares*EntryPrice; if (CommEntry<1) CommEntry=1;
CommExit=0.005*shares; if (CommExit>0.005*shares*ExitPrice) CommExit=0.005*shares*ExitPrice; if (CommExit<1) CommExit=1;

TradeProfit=shares*(ExitPrice-EntryPrice)-CommEntry-CommExit;

fres << symbol << "," << EntryDate << "," << ExitDate << "," << shares << "," << EntryPrice << "," << ExitPrice << "," << TradeProfit << std::endl;
}





                                               ///TimeOut Exit from Long

if (MarketPosition)
{
MarketPosition=0;
ExitPrice=d[i].Close;
ExitDate=d[i].Date;
CommEntry=0.005*shares; if (CommEntry>0.005*shares*EntryPrice) CommEntry=0.005*shares*EntryPrice; if (CommEntry<1) CommEntry=1;
CommExit=0.005*shares; if (CommExit>0.005*shares*ExitPrice) CommExit=0.005*shares*ExitPrice; if (CommExit<1) CommExit=1;

TradeProfit=shares*(ExitPrice-EntryPrice)-CommEntry-CommExit;

fres << symbol << "," << EntryDate << "," << ExitDate << "," << shares << "," << EntryPrice << "," << ExitPrice << "," << TradeProfit << std::endl;
}


///EXITS finished




/// BUY block starts

if
	(
    ///ExitDate != d[i].Date && ///не берём один символ дважды подряд (когда сегодня для данного символа и выход и вход)

	///d[i-4].Close < d[i-4].Open &&

	d[i-3].Close < d[i-3].Open &&
	d[i-2].Close < d[i-2].Open &&
	d[i-1].Close < d[i-1].Open &&

	///d[i-2].Open < d[i-3].Close &&

	d[i-1].Open < d[i-2].Close

	///&& AvVol(d, i-1, DaysNum) > 30000
	)
{
k=EntryStDevKoeff*StDevNorm(d, i-1, DaysNum);
EntryPrice = floor(d[i-1].Low*(1-k)*20)/20; /// floor(d[i-1].Low*(1-k)*100)/100;
if (EntryPrice>MinPrice && EntryPrice<MaxPrice && d[i].Low<EntryPrice && d[i].Open>=EntryPrice)
{
	///if (EntryPrice<=12) shares=floor(TradeSize/EntryPrice/100+0.2)*100;
	if (EntryPrice<=120) shares=floor(TradeSize/EntryPrice/10+0.2)*10;  /// else
	else shares=floor(TradeSize/EntryPrice+0.2);

	MarketPosition = (int)shares;
	EntryDate = d[i].Date;

	if(i==BarsNum-1)
    {
        fPosOpen.open("PosOpen.txt", std::ios::out | std::ios::app);
        if(!fPosOpen.is_open())
        {
            LogString.str("");
            LogString << "Metod: can not write into file PosOpen.txt\n";
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "Method: can not write into file log.txt\n";
            flog << LogString.str();
            flog.close();
        }
        fres.setf(std::ios::fixed);
        fPosOpen.precision(2);

        fPosOpen << symbol << "  " << MarketPosition << "  Entry Price " << EntryPrice << "  Target Price " << ceil((EntryPrice+EntryPrice*StDevNorm(d, i, DaysNum)*TargStDevKoeff)*20)/20 << std::endl;

        fPosOpen.close();
    }
}
}
/// BUY block ends


}
/// Main loop ends


fres.close();
}


}


