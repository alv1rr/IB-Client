#include "globals.h"
#include "PosixClient.h"
#include "Method/Method.h"

#include "twsapi/EPosixClientSocket.h"
#include "twsapi/EPosixClientSocketPlatform.h"
#include "twsapi/Contract.h"
#include "twsapi/Order.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

#include <cstring>
#include <cmath>

#include <pthread.h>
#include <cerrno>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>



DOHLCV d[6000];

char Smb[3500][6]; ///array to keep contents of file all.txt

OrdBskLine OrdBsk[500]; ///array to keep Order Basket Symbols
OrdBskLine OrdBskPlaced[500]; ///array to keep Order Basket Symbols for Placed orders
OrdBskLine OrdBskCanceled[500]; ///array to keep Order Basket Symbols for Canceled orders
OrdBskLine MktDataExtra[500]; ///array to keep tickers over permission limit of simultaneously opened MkData
int MktDataOpenedNum(0);
int MktDataExtraNum(0);
int OrdBskLinesNum(0);

double Methods[100][9];



tcDialog* dlg;

PosixClient client;

pthread_t thread[2];


pthread_mutex_t repoMutexes[NUM_REPOTHREADS];
pthread_cond_t repoConditions[NUM_REPOTHREADS];
pthread_attr_t repoAttr;


pthread_mutex_t continue_mutex; /** mutex used for processMessages as quit flag */
pthread_attr_t attr;
pthread_mutex_t client_mutex; /** mutex used for processMessages to avoid segmentation fault */

char smbRef[6]{"AAPL"};

char AddDate[10]{"20190403"}; /// date up to which historical data is needed
char PrevDate[10]{"20190402"}; /// non-holiday day which precedes AddDate

int DateNotTradingFrom(20180201);

int SleepTime_WSJ(5);

int BarsNum(0);
int Len(280);

bool BinFile=false;

int LinesNum(0);
ResLine *pResLine;
ResLine *pResLine2;
ResLine *pResLine3;

int TradesNum(1);
double GrossTradeSize(0);
double GrossProfit(0);
double GrossLoss(1);
double NetProfit(0);
double PF(0);
double DD(0);
double MinDD(0);
double AvTrade(0);
double AvTradeSize(1);
double AvTradeP(0);

int OrdId(1917);

bool OrdBskExists(false);
bool OrdBskPlacedExists(false);
bool CancelAllOrdPermit(true);
bool CheckMktDataRunning(false);
bool ReqMktDataExtraRunning(false);

/// Parameters of the method
double EntryStDevKoeff(2.5);
double TargStDevKoeff(2.8);
double Target(0.06);
double TradeSize(1000);
double MinPrice(0.5);
double MaxPrice(120);
int DaysNum(30); ///Number of Days to calculate MA, StDev, and request data for
double k; /// k=EntryStDevKoeff*StDevNorm(d, i-1, DaysNum);  EntryPrice = floor(d[i-1].Low*(1-k)*100)/100;

bool PlaceAllOrd(true);
double PriceDeltaTrig(2.00); /// $2.00
double PriceDeltaTrig2(0.40);

int MorningTimeLine(16*60+40);  /// 16 h. 40 min
int MktDataOpenedLim(94);



const char c0D(0x0D);
const char c0A(0x0A);
const char c5C(0x5C);  /// char '\'
const char c2F(0x2F);  /// char '/'
const char c20(0x20);  /// space
const char c00(0x00);
const char c25(0x25);  ///Percent sign '%'
const char c09(0x09);  ///Tabulation





/// initialize mutexes that protect Repository vectors

void init_repoMutexes() {
    for (int i = 0; i < NUM_REPOTHREADS; i++) {
        pthread_mutex_init(&repoMutexes[i], NULL);
        pthread_cond_init(&repoConditions[i], NULL);
    }
}

void destroy_repoMutexes() {
    for (int i = 0; i < NUM_REPOTHREADS; i++) {
        pthread_mutex_destroy(&repoMutexes[i]);
        pthread_cond_destroy(&repoConditions[i]);
    }
}



/// Returns 1 (true) if the mutex is unlocked, which is the
/// thread's signal to terminate.

int needQuit(pthread_mutex_t *mtx) {
    switch (pthread_mutex_trylock(mtx)) {
        case 0: /// if we got the lock, unlock and return 1 (true)
            pthread_mutex_unlock(mtx);
            return 1;
        case EBUSY: /// return 0 (false) if the mutex was locked
            return 0;
    }
    return 1;
}



/// Thread function containing a loop that's infinite except that it checks for
/// termination with needQuit()

void* processMessages_gl(void* t) {
    pthread_mutex_t *mx = (pthread_mutex_t *) t;
    while (!needQuit(mx)) {
        pthread_mutex_lock(&client_mutex); /// avoid segmentation fault
        client.processMessages();
        pthread_mutex_unlock(&client_mutex);
    }

    return NULL;
}





void processMessages_gl() {
    ///pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    std::ofstream flog;
    std::ostringstream LogString;

    int rc;


    pthread_mutex_init(&continue_mutex, NULL);
    pthread_mutex_lock(&continue_mutex);

    pthread_mutex_init(&client_mutex, NULL);

    init_repoMutexes();

    ///Initialize and set thread detached attribute
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    LogString << "processMessages_gl: creating thread !" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "processMessages_gl: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();


    rc = pthread_create(&thread[0], &attr, ::processMessages_gl, &continue_mutex);
    if (rc)
    {
        LogString.str("");
        LogString << "ERROR: return code from pthread_create() is " << rc << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "processMessages_gl: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        exit(-1);
    }
}





void processMessages2_gl() {
    client.processMessages();
}





void endProcessMessages_gl()
/// unlock mxq to tell the processMessages thread to terminate, then join the thread
{
    std::ofstream flog;
    std::ostringstream LogString;

    pthread_mutex_unlock(&continue_mutex);
    pthread_join(thread[0], NULL);

    destroy_repoMutexes();

    ///    pthread_attr_destroy(&attr);
    ///    pthread_mutex_destroy(&mxq);
    ///    pthread_mutex_destroy(&mxq2);

    LogString << "endProcessMessages_gl executed" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "endProcessMessages_gl: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();
}





double StDevNorm(DOHLCV *SR, int i_last, int period)
///Standard Deviation Normalized
{
std::ofstream flog;
std::ostringstream LogString;

int i;
double MA(0);
double Result(0);



if (period<=0)
{
    LogString << "StDevNorm: Enter period>0" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "StDevNorm: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return 0;
}


if (i_last-period+1>=0)
{
	for (i=i_last-period+1; i<=i_last; i++) MA+=SR[i].Close;
	MA/=period;
	for (i=i_last-period+1; i<=i_last; i++) Result+=(SR[i].Close-MA)*(SR[i].Close-MA);
	Result/=period;
	Result=sqrt(Result)/MA;
}

else
{
    LogString << "StDevNorm: So many bars do not exist" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "StDevNorm: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return 0;
}


return Result;
}





int AvVol(DOHLCV *SR, int i_last, int period)
///Average Volume
{
std::ofstream flog;
std::ostringstream LogString;

int i;
int Result(0);



if (period<=0)
{
    LogString << "AvVol: Enter period>0" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "AvVol: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return 0;
}


if (i_last-period+1>=0)
{
	for (i=i_last-period+1; i<=i_last; i++) Result+=SR[i].Volume;
	Result/=period;
}

else
{
    LogString << "AvVol: So many bars do not exist" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "AvVol: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return 0;
}


return Result;
}





void* HistData(void* t)
{
std::ifstream fi;
std::ofstream flog;

std::ostringstream LogString;

char line[200];
char Symbol[10];
int i;

int LastTickerId(0);
int NextTickerId(0);

wxString ConnTime;

Contract contract;
/// set contract fields
///contract.symbol		= "MSFT"; //symbol will be set before request
contract.secType	= "STK";
contract.expiry		= "";
contract.strike		= 0;
contract.right		= "";
contract.exchange	= "SMART";
contract.currency	= "USD";
contract.primaryExchange = "NASDAQ";



/// Writing file BinPresent.txt or BinAbsent.txt into array of pointers Smb[] started

if (BinFile)
{
    fi.open("BinPresent.txt");
    if (!fi.is_open())
    {
        LogString << "HistData: ERROR: File BinPresent.txt not found; run CheckBin first" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "HistData: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        return NULL;
    }
}
else
{
    fi.open("BinAbsent.txt");
    if (!fi.is_open())
    {
        LogString << "HistData: ERROR: File BinAbsent.txt  not found; run CheckBin first" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "HistData: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        return NULL;
    }

}

i=0;
while ( fi.getline(line, 200) )
{
	sscanf(line,"%s",Symbol);
	strcpy(Smb[i], Symbol);
	++i;
}

LastTickerId=--i;
fi.close();

/// Writing of file BinPresent.txt or BinAbsent.txt into array of pointers Smb[] finished



if (LastTickerId==-1)
{
    LogString << "No tickers for request; ticker request will not follow" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "HistData: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return NULL;
}




if (!client.isConnected())
{
    LogString << "Client not connected; ticker request will not follow" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "HistData: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return NULL;
}


LogString << "History data request will follow.\nAll market data files in .bin format will be saved in folder: fbin" << std::endl;
std::cout << LogString.str();
flog.open("log.txt", std::ios::out | std::ios::app);
if (!flog.is_open()) std::cout << "HistData: can not write into log.txt\n";
flog << LogString.str();
flog.close();



if (client.isConnected() && LastTickerId>=0)
{
    for (NextTickerId=0; NextTickerId<=LastTickerId; NextTickerId++)
    {
        contract.symbol=Smb[NextTickerId];

        TagValueListSPtr chartOptions;
        client.m_pClient->reqHistoricalData(NextTickerId, contract,  "", "52 W", "1 day", "TRADES", 1, 1, chartOptions); ///(TickerID id, const Contract &contract,  String endDateTime, String durationStr, long barSizeSetting, String whatToShow, int useRTH, int formatDate, TagValueListSPtr chartOptions)

        sleep(11);

        if (!client.isConnected())
        {
            LogString.str("");
            LogString << "Connection with TWS broken; data request terminated" << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "HistData: can not write into log.txt\n";
            flog << LogString.str();
            flog.close();

            break;
        }
    }


    if (NextTickerId>LastTickerId)
    {
        LogString.str("");
        LogString << "Hist.data request finished; last ticker data requested.\nAll market data files in .bin format have been saved in folder: fbin" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "HistData: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
    }
}


   return NULL;
}





void HistData()
{
    pthread_create(&thread[1], NULL, HistData, NULL);

}





bool FileExists(const char *fname)
{
    std::ifstream fp;


    fp.open(fname, std::ios::in | std::ios::binary);

    if (!fp.is_open()) return false;

    fp.close();
    return true;
}





bool Round(double arg)
///If third and forth digits of the argument after point - both zeros or both nines,
///this function returns true.
{
std::ostringstream s;
char s1[50];
int i(0),j(0);
bool FirstZeroFlag(false);
bool FirstNineFlag(false);

s << std::fixed << std::setprecision(8) << arg;
strcpy( s1, s.str().c_str() );

i=0; j=0;
while (s1[i] && i<50)
{
	if ((j==4) && (s1[i]=='0') &&(FirstZeroFlag==true)) return true;
	if ((j==4) && (s1[i]=='9') &&(FirstNineFlag==true)) return true;

	if ((j==3) && (s1[i]=='0')) FirstZeroFlag=true;
	if ((j==3) && (s1[i]=='9')) FirstNineFlag=true;

	if (j) j++;
	if (s1[i]=='.') j=1;
	i++;
}
if (!j) return true; ///if argument has no point

return false;
}





int BINtoMEM (char *symbol, DOHLCV *arr, int BarsRequested, int &BarsGot)
{

std::ifstream fp;
std::string filename;

int FileSize;


filename = "fbin/"; filename += symbol; filename += ".bin";

fp.open(filename.c_str(), std::ios::in | std::ios::binary);
if (!fp.is_open()) return 0;



fp.seekg(-BarsRequested*sizeof(DOHLCV), std::ios::end);
if (fp.fail())
{
	///probably BarsRequested is too big, reading all the file
	fp.clear();
	fp.seekg(0, std::ios::end);
	if (fp.fail()) return 0;
	FileSize = fp.tellg();
	BarsGot = FileSize/sizeof(DOHLCV); if (!BarsGot) {fp.close(); return 0;}
	fp.clear();
	fp.seekg(0, std::ios::beg);
	fp.read( (char*)arr, BarsGot*sizeof(DOHLCV) );
	fp.close();
	return 1;
}


///reading BarsRequested exactly
fp.read( (char*)arr, BarsRequested*sizeof(DOHLCV) );
BarsGot = BarsRequested; if (!BarsGot) {fp.close(); return 0;}

fp.close();
return 1;
}





int ASCIItoMEM(char *symbol)
{
std::ifstream fs;
char line[200];
char *token=NULL;
char seps[]=",\t\n"; ///must be NULL-teminated!

std::string fsname;


    fsname = "fascii/"; fsname += symbol; fsname += ".txt";
    fs.open(fsname.c_str());
    if (!fs.is_open()) return 0;
    else
    {
        BarsNum=0;
        while ( fs.getline(line, 200) )
        {
            token=strtok(line,seps); ///Date
            d[BarsNum].Date=atoi(token);

            token=strtok(NULL,seps); ///Open
            d[BarsNum].Open=atof(token);

            token=strtok(NULL,seps); ///High
            d[BarsNum].High=atof(token);

            token=strtok(NULL,seps); ///Low
            d[BarsNum].Low=atof(token);

            token=strtok(NULL,seps); ///Close
            d[BarsNum].Close=atof(token);

            token=strtok(NULL,seps); ///Volume
            if (token==NULL) d[BarsNum].Volume=0;
            else d[BarsNum].Volume=atoi(token);

            BarsNum++;
        }
        fs.close();
        return 1;
    }

}





int MEMtoBIN(char *symbol, int opt/*=0*/)
///opt 0: write new .bin from d[]
///opt 1: compare dates in .bin and d[], then add part of d[] to .bin

///return 0: everything OK, symbol renewing
///return 1: nothing can be done; symbol not renewing
///return 2: opt 1 must be changed to opt 0
///return 3: not enough history in d[]

{

std::fstream fp;
std::ofstream fp2;
std::ofstream flog;

std::ostringstream LogString;
std::string filename;

DOHLCV LastRecord;
DOHLCV *p = d; ///pointer is set into the beginning of Array d

int i;

double AdmitKoeff(0.01);  /// 1%



filename = "fbin/"; filename += symbol; filename += ".bin";


if (opt)
{
    fp.open(filename.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if (!fp.is_open())
    {
        LogString << "MEMtoBIN: file " << filename << " not found. File will be created." << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "MEMtoBIN: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        return 2;
    }

	fp.seekg( -(int)sizeof(DOHLCV), std::ios::end);
    if (fp.fail())
    {
        LogString << "MEMtoBIN: File " << filename << " corrupted. File will be replaced." << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "MEMtoBIN: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        fp.close();
        return 2;
    }


	///reading LastRecord from .bin-file
	fp.read( (char*)&LastRecord, sizeof(DOHLCV) );

	if (d[BarsNum-1].Date == LastRecord.Date)
    {
        LogString << "MEMtoBIN: Last date in the update: " << d[BarsNum-1].Date << " is the same as last date in the file " << filename << ": " << LastRecord.Date << " . Symbol " << symbol << " not updated." << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "MEMtoBIN: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        fp.close();
        return 1;
    }

	if (d[BarsNum-1].Date < LastRecord.Date)
    {
        LogString << "MEMtoBIN: Last date in the update: " << d[BarsNum-1].Date << " is lower than last date in the file " << filename << ": " << LastRecord.Date << " . Symbol " << symbol << " not updated." << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "MEMtoBIN: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        fp.close();
        return 1;
    }

	if (d[0].Date > LastRecord.Date)
    {
        LogString << "MEMtoBIN: Not enough history in the update of symbol " << symbol << ". Attempt to get more history, or symbol can not be updated." << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "MEMtoBIN: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        fp.close();
        return 3;
    }

	///adding to .bin file
	for (i=0;i<BarsNum;i++)
	{
		if (d[i].Date == LastRecord.Date)
		{
			if ( d[i].Close<LastRecord.Close-AdmitKoeff*LastRecord.Close || d[i].Close>LastRecord.Close+AdmitKoeff*LastRecord.Close )
            {
                LogString << "MEMtoBIN: Full data reloading for " << symbol << ": d[i].Close " << d[i].Close << "  LastRecord.Close " << LastRecord.Close << "  File will be replaced" << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "MEMtoBIN: can not write into log.txt\n";
                flog << LogString.str();
                flog.close();

                fp.close();
                return 2;
            }

			p += i+1;
			fp.seekp(0, std::ios::end);
			fp.write( (char*)p, sizeof(DOHLCV)*(BarsNum-(i+1)) );
			fp.close();
			return 0;
		}
	}
}


if (!opt)
{
    fp2.open(filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	if (!fp2.is_open())
    {
        LogString << "MEMtoBIN: File " << filename << " can not be created. Symbol " << symbol << " not updated" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "MEMtoBIN: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        return 1;
    }

	fp2.write( (char*)d, sizeof(DOHLCV)*BarsNum );
	fp2.close();
}

return 0;
}





void MEMtoASCII(char *symbol)
{

std::ofstream fo, flog;

std::ostringstream s2, LogString;
std::string filename, dohlcv;

int i(0);


filename = "fascii/"; filename += symbol; filename += ".txt";


fo.open(filename.c_str());
if (!fo.is_open())
{
    LogString << "MEMtoASCII: File " << symbol << ".txt can not be created in directory fascii" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "MEMtoASCII: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return;
}


for( i=0; i < BarsNum; i++ )
{
	s2 << d[i].Date << ",";
	dohlcv=s2.str();

	s2.str("");
	s2.setf(std::ios::fixed);

	if (Round(d[i].Open)) s2 << std::setprecision(2) << d[i].Open << ","; else s2 << std::setprecision(6) << d[i].Open << ","; dohlcv+=s2.str(); s2.str("");
	if (Round(d[i].High)) s2 << std::setprecision(2) << d[i].High << ","; else s2 << std::setprecision(6) << d[i].High << ","; dohlcv+=s2.str(); s2.str("");
	if (Round(d[i].Low)) s2 << std::setprecision(2) << d[i].Low << ","; else s2 << std::setprecision(6) << d[i].Low << ","; dohlcv+=s2.str(); s2.str("");
	if (Round(d[i].Close)) s2 << std::setprecision(2) << d[i].Close << ","; else s2 << std::setprecision(6) << d[i].Close << ","; dohlcv+=s2.str(); s2.str("");

	s2 << d[i].Volume; dohlcv+=s2.str(); s2.str("");


	fo << dohlcv << std::endl;
}


fo.close();
}





void BINtoASCII()
{
std::ifstream fi;
std::ofstream flog;

std::ostringstream LogString;
char symbol[10];
char line0[200];

int RetCode(0);


LogString << "Writing ASCII files from the base of .bin files ..." << std::endl;
std::cout << LogString.str();
flog.open("log.txt", std::ios::out | std::ios::app);
if (!flog.is_open()) std::cout << "BINtoASCII: can not write into log.txt\n";
flog << LogString.str();
flog.close();


fi.open("all.txt");
if (!fi.is_open())
{
    LogString << "BINtoASCII: ERROR: file all.txt not found" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "BINtoASCII: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return;
}

while ( fi.getline(line0, 200) )
{
	sscanf(line0,"%s",symbol);
	RetCode = BINtoMEM(symbol,d,6000,BarsNum);
	if (RetCode) MEMtoASCII(symbol);
}

fi.close();


LogString.str("");
LogString << "Writing ASCII files from the base of .bin files completed.\nAll market data files converted from .bin to .txt format and saved in folder: fascii" << std::endl;
std::cout << LogString.str();
flog.open("log.txt", std::ios::out | std::ios::app);
if (!flog.is_open()) std::cout << "BINtoASCII: can not write into log.txt\n";
flog << LogString.str();
flog.close();
}





void ASCIItoBIN()
{
std::ifstream fi;
std::ofstream flog;

std::ostringstream LogString;
char symbol[10];
char line0[200];

int RetCode(0);


LogString << "Writing .bin files from the base of ASCII files ..." << std::endl;
std::cout << LogString.str();
flog.open("log.txt", std::ios::out | std::ios::app);
if (!flog.is_open()) std::cout << "ASCIItoBIN: can not write into log.txt\n";
flog << LogString.str();
flog.close();


fi.open("all.txt");
if (!fi.is_open())
{
    LogString << "file all.txt not found" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "ASCIItoBIN: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return;
}

while ( fi.getline(line0,200) )
{
	sscanf(line0,"%s",symbol);
	RetCode = ASCIItoMEM(symbol);
	if (RetCode) MEMtoBIN(symbol, 0);
}

fi.close();



LogString.str("");
LogString << "Writing .bin files from the base of ASCII files completed.\nAll market data files converted from .txt to .bin format and saved in folder: fbin" << std::endl;
std::cout << LogString.str();
flog.open("log.txt", std::ios::out | std::ios::app);
if (!flog.is_open()) std::cout << "ASCIItoBIN: can not write into log.txt\n";
flog << LogString.str();
flog.close();

}





int RecvAll(int socket_desc, void *data, int data_size)
{
    char *data_ptr = (char*) data;
    int bytes_recv;

    while (data_size > 0)
    {
        bytes_recv = recv(socket_desc, data_ptr, data_size, 0);
        if (bytes_recv <= 0)
            return bytes_recv;

        data_ptr += bytes_recv;
        data_size -= bytes_recv;
    }

    return 1;
}





void* UpdLastDay(void* t)
/// This function took last day quotes from the site of Wall Street Journal.
/// Unfortunately starting from March 2019 Wall Street Journal stopped to provide .csv files that were used by this function.
/// Some other source of last day quotes should be found.
{
    std::ofstream flog, ferr;
    std::fstream fbin;
    std::ostringstream LogString;

    int socket_desc;
    struct sockaddr_in server;
    char message[5000];
    std::ostringstream message_sstr;
    char server_reply[400000];
    char *serv_rep[400000];

    char *token(NULL);
    char seps1[]{"\n"}; ///must be NULL-teminated!
    char seps2[]{","}; ///must be NULL-teminated!
    char seps3[]{0x22, 0}; ///must be NULL-teminated!

    char *hostname = (char*)"www.wsj.com";
    char ip[100];
    struct hostent *he;
    struct in_addr **addr_list;

    int ServRepLinesNum=0;

    const char *ServerFile[] =
    {
        "Nasdaq.csv",
        "SCAP.csv",
        "Preferreds.csv",
        "USETFs.csv",
        "AMEX.csv",
        "NYSE.csv"
    };

    int RetRecv=0;

    char token1[200]; token1[0]='\0';
    char token2[200]; token2[0]='\0';
    char token3[200]; token3[0]='\0';
    char token4[200]; token4[0]='\0';

    char tokenMonday[]("_Monday,"); tokenMonday[0]=0x22;
    char tokenTuesday[]("_Tuesday,"); tokenTuesday[0]=0x22;
    char tokenWednesday[]("_Wednesday,"); tokenWednesday[0]=0x22;
    char tokenThursday[]("_Thursday,"); tokenThursday[0]=0x22;
    char tokenFriday[]("_Friday,"); tokenFriday[0]=0x22;

    char AddDateWSJ[]("19171107");
    std::ostringstream AddDateWSJ_sstr;

    char Month_str[200], Day_str[200], Year_str[200];

    char CompanyName[200];
    char Symbol[200];
    double Open(0), High(0), Low(0), Close(0), NetChange(0), PrevClose(0);
    int Volume(0);
    char Volume_str[200];  Volume_str[0]=0;

    std::string filename1;
    DOHLCV CurRecord, LastRecord;

    double AdmitKoeff=0.01; /// 1%




    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "UpdLastDay: can not write into log.txt\n";



    ///Resolving server name

    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        LogString << "ERROR: Could non resolve server name" << std::endl;

        std::cout << LogString.str();
        flog << LogString.str();

        flog.close();
        return NULL;
    }

    addr_list = (struct in_addr**)he->h_addr_list; /// h_addr_list is Null-terminated array of in_addr structures

    for(int i = 0; addr_list[i] != NULL; i++)
    {
        strcpy(ip , inet_ntoa(*addr_list[i]) );

        LogString.str("");
        LogString << hostname << " resolved to : " << ip << std::endl;

        std::cout << LogString.str();
        flog << LogString.str();
    }



    for (int ServFileIndex=0; ServFileIndex<6; ServFileIndex++) /// I have 6 ServerFile-s withe indexes from 0 to 5
    {

    AddDateWSJ_sstr.str(""); AddDateWSJ_sstr << "1900000" << ServFileIndex;
    strcpy(AddDateWSJ, AddDateWSJ_sstr.str().c_str());

    ///Creating socket

    socket_desc = socket(AF_INET , SOCK_STREAM , 0);

    if (socket_desc == -1)
    {
        LogString.str("");
        LogString << "ERROR: Could not create socket" << std::endl;

        std::cout << LogString.str();
        flog << LogString.str();

        flog.close();
        return NULL;
    }

        LogString.str("");
        LogString << "Socket created successfully" << std::endl;

        std::cout << LogString.str();
        flog << LogString.str();



    ///Connect to server

    server.sin_addr.s_addr = addr_list[0]->s_addr;
    server.sin_family = AF_INET;
    server.sin_port = htons( 80 );

    if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        LogString.str("");
        LogString << "ERROR: Could not connect to sin_addr throw sin_port" << std::endl;

        std::cout << LogString.str();
        flog << LogString.str();

        flog.close();
        return NULL;
    }

    LogString.str("");
    LogString << "Connection to sin_addr trhrow sin_port established successfully" << std::endl;

    std::cout << LogString.str();
    flog << LogString.str();



    ///Send some data

    message_sstr << "GET /public/resources/documents/" << ServerFile[ServFileIndex] << " HTTP/1.1\r\nHost: www.wsj.com\r\nConnection: close\r\n\r\n";
    strcpy(message, message_sstr.str().c_str());

    ///examples: "GET / HTTP/1.1\r\n\r\n"  "GET /sensor?value=10.3\r\n\r\n"


    LogString.str("");
    LogString << "MESSAGE:" << message << std::endl;

    std::cout << LogString.str();
    flog << LogString.str();


    if( send(socket_desc , message , strlen(message) , 0) < 0)
    {
        LogString.str("");
        LogString << "ERROR: Sending request to server failed" << std::endl;

        std::cout << LogString.str();
        flog << LogString.str();

        flog.close();
        return NULL;
    }

    LogString.str("");
    LogString << "Request sent to server successfully\nWAITING for server response ..." << std::endl;

    std::cout << LogString.str();
    flog << LogString.str();




    ///Receive a reply from the server

    ///if( recv(socket_desc, server_reply , 400000 , 0) < 0)

    RetRecv = RecvAll(socket_desc, server_reply , 400000);

    ///if (RetRecv<0)
    ///{
    ///    LogString.str(""); LogString << "ERROR: Receiving from server failed" << std::endl; std::cout << LogString; flog << LogString; flog.close();  return NULL;
    ///}

    LogString.str("");
    LogString << "The following server response received successfully:" << std::endl;

    std::cout << LogString.str();
    flog << LogString.str();

    std::cout << server_reply;
    flog << server_reply;

    LogString.str("");
    LogString << std::endl << "size of server_reply: " <<  strlen(server_reply) << "  RetRecv: " << RetRecv << std::endl;

    std::cout << LogString.str();
    flog << LogString.str();


    for (int i=0;;i++)
    {
        if(i==0) { (serv_rep[i]=strtok(server_reply, seps1)); if (serv_rep[i]==NULL) {ServRepLinesNum=i; break;} }
        else { (serv_rep[i]=strtok(NULL, seps1)); if (serv_rep[i]==NULL) {ServRepLinesNum=i; break;} }
    }

    LogString.str("");
    LogString << std::endl << "Lines number in server reply: " << std::endl << ServRepLinesNum << std::endl;

    std::cout << LogString.str();
    flog << LogString.str();


    for (int i=0; i<ServRepLinesNum; i++)
    {
        std::cout << serv_rep[i] << std::endl;
        flog << serv_rep[i] << std::endl;
    }



    for (int ServRepLineIndex=0; ServRepLineIndex<ServRepLinesNum; ServRepLineIndex++)
    {
        token1[0]='\0';
        sscanf(serv_rep[ServRepLineIndex], "%s %s %s %s", token1, token2, token3, token4);

        if ( !strcmp(token1, tokenMonday) || !strcmp(token1, tokenTuesday) || !strcmp(token1, tokenWednesday) || !strcmp(token1, tokenThursday) || !strcmp(token1, tokenFriday) )
        {
            if (!strcmp(token2, "January")) strcpy(Month_str, "01");  ///Month
            if (!strcmp(token2, "February")) strcpy(Month_str, "02");
            if (!strcmp(token2, "March")) strcpy(Month_str, "03");
            if (!strcmp(token2, "April")) strcpy(Month_str, "04");
            if (!strcmp(token2, "May")) strcpy(Month_str, "05");
            if (!strcmp(token2, "June")) strcpy(Month_str, "06");
            if (!strcmp(token2, "July")) strcpy(Month_str, "07");
            if (!strcmp(token2, "August")) strcpy(Month_str, "08");
            if (!strcmp(token2, "September")) strcpy(Month_str, "09");
            if (!strcmp(token2, "October")) strcpy(Month_str, "10");
            if (!strcmp(token2, "November")) strcpy(Month_str, "11");
            if (!strcmp(token2, "December")) strcpy(Month_str, "12");

            {unsigned int i=0; unsigned int j=0; for (i=0; i<strlen(token3); i++) if (token3[i]!=',') {Day_str[j]=token3[i]; j++;} Day_str[j]='\0';} ///Day

            { unsigned int i=0; unsigned int j=0; for (i=0; i<strlen(token4); i++) if (token4[i]!=0x22) {Year_str[j]=token4[i]; j++;} Year_str[j]='\0';} ///Year

            AddDateWSJ_sstr.str(""); AddDateWSJ_sstr << Year_str << Month_str << Day_str; /// Date
            strcpy(AddDateWSJ, AddDateWSJ_sstr.str().c_str());
        }



        if
        (
         strlen(serv_rep[ServRepLineIndex]) > 30 &&
         token1[0]!='\0' &&
         strcmp(token1, "HTTP/1.1") &&
         strcmp(token1, "Content-Type:") &&
         strcmp(token1, "Content-Length:") &&
         strcmp(token1, "Connection:") &&
         strcmp(token1, "Server:") &&
         strcmp(token1, "Last-Modified:") &&
         strcmp(token1, "Accept-Ranges:") &&
         strcmp(token1, "P3P:") &&
         strcmp(token1, "Expires:") &&
         strcmp(token1, "Cache-Control:") &&
         strcmp(token1, "Pragma:") &&
         strcmp(token1, "Date:") &&
         strcmp(token1, "X-Cache:") &&
         strcmp(token1, "Via:") &&
         strcmp(token1, "X-Amz-Cf-Id:") &&
         strcmp(token1, "Nasdaq") &&
         strcmp(token1, "Preferred") &&
         strcmp(token1, "U.S.") &&
         strcmp(token1, "NYSE") &&
         strcmp(token1, "Name,Symbol,Open,High,Low,Close,Net") &&
         strcmp(token1, tokenMonday) &&
         strcmp(token1, tokenTuesday) &&
         strcmp(token1, tokenWednesday) &&
         strcmp(token1, tokenThursday) &&
         strcmp(token1, tokenFriday)
        )

        {


        std::cout << serv_rep[ServRepLineIndex] << std::endl;
        flog << serv_rep[ServRepLineIndex] << std::endl;



        CompanyName[0]='\0';
        Symbol[0]='\0';
        Open=0;
        High=0;
        Low=0;
        Close=0;
        NetChange=0;
        PrevClose=0;
        Volume=0;


        token=strtok(serv_rep[ServRepLineIndex],seps2); ///CompanyName
        if (token)
        {
            strcpy(CompanyName, token);
            std::cout << CompanyName << std::endl; flog << CompanyName << std::endl;
        }

        token=strtok(NULL,seps2); ///Symbol
        if (token)
        {
            strcpy(Symbol, token);
            std::cout << Symbol << std::endl; flog << Symbol << std::endl;
        }
            if (token==NULL || Symbol[0]==0) continue;

            token=strtok(NULL,seps2); ///Open
            if (token)
            {
                Open=atof(token);
                std::cout << Open << std::endl; flog << Open << std::endl;
            }

            token=strtok(NULL,seps2); ///High
            if (token)
            {
                High=atof(token);
                std::cout << High << std::endl; flog << High << std::endl;
            }

            token=strtok(NULL,seps2); ///Low
            if (token)
            {
                Low=atof(token);
                std::cout << Low << std::endl; flog << Low << std::endl;
            }

            token=strtok(NULL,seps2); ///Close
            if (token)
            {
                Close=atof(token);
                std::cout << Close << std::endl; flog << Close << std::endl;
            }

            token=strtok(NULL,seps2); ///NetChange
            if (token)
            {
                NetChange=atof(token);
                std::cout << NetChange << std::endl; flog << NetChange << std::endl;

                PrevClose = Close - NetChange;
                std::cout << PrevClose << std::endl; flog << PrevClose << std::endl;
            }

            token=strtok(NULL,seps2); ///%Change

            token=strtok(NULL,seps3); ///Volume
            if (token)
            {
                unsigned int i=0; unsigned int j=0;
                while(i<strlen(token))
                {
                    if (token[i]!=',') {Volume_str[j]=token[i]; j++;}
                    i++;
                }
                Volume_str[j]='\0';
                Volume=atoi(Volume_str);
                LogString.str("");
                LogString << Volume_str << "    " << strlen(Volume_str) << "    " << Volume << std::endl;
                std::cout << LogString.str(); flog << LogString.str();
            }


            if (Open==0 || High==0 || Low==0 || Close==0 || Volume==0)
            {
                LogString.str("");
                LogString << "Open==0 || High==0 || Low==0 || Close==0 || Volume==0 for Symbol " << Symbol << "; update not performed" << std::endl;
                std::cout << LogString.str(); flog << LogString.str();

                ferr.open("err.txt", std::ios::out | std::ios::app);
                if (!ferr.is_open())
                {
                    std::cout << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl;
                    flog << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl;
                }
                ferr << LogString.str();
                ferr.close();

                continue;
            }


            filename1 = "fbin/"; filename1 += Symbol; filename1 += ".bin";

            fbin.open(filename1.c_str(), std::ios::in | std::ios::out | std::ios::binary);
            if (!fbin.is_open())
            {
                    std::cout << "UpdLastDay: Error: file " << filename1 << " can not be opened for reading and writing" << std::endl;
                    flog << "UpdLastDay: Error: file " << filename1 << " can not be opened for reading and writing" << std::endl;
                    continue;
            }

            fbin.seekg( -sizeof(DOHLCV), std::ios::end );

            fbin.read( (char*)&LastRecord, sizeof(DOHLCV) );

            if ( atoi(AddDate)!=atoi(AddDateWSJ) ) {LogString.str(""); LogString << "Symbol " << Symbol << ": AddDate " << AddDate << " is not equal AddDateWSJ " << AddDateWSJ << "; update not performed" << std::endl; std::cout << LogString.str(); flog << LogString.str(); ferr.open("err.txt", std::ios::out | std::ios::app); if (!ferr.is_open()) {std::cout << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl; flog << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl;} ferr << LogString.str(); ferr.close(); fbin.close(); continue;}
            else if ( LastRecord.Date!=atoi(PrevDate) ) {LogString.str(""); LogString << "Symbol " << Symbol << ": LastRecord.Date " << LastRecord.Date << " is not equal PrevDate " << PrevDate << "; update not performed" << std::endl; std::cout << LogString.str(); flog << LogString.str(); ferr.open("err.txt", std::ios::out | std::ios::app); if (!ferr.is_open()) {std::cout << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl; flog << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl;} ferr << LogString.str(); ferr.close(); fbin.close(); continue;}
            else if ( fabs(LastRecord.Close-PrevClose)>AdmitKoeff*LastRecord.Close ) {LogString.str(""); LogString << "Symbol " << Symbol << ": LastRecord.Close " << LastRecord.Close << " differs from PrevClose " << PrevClose << "; POSSIBLE SPLIT; update not performed" << std::endl; std::cout << LogString.str(); flog << LogString.str(); ferr.open("err.txt", std::ios::out | std::ios::app); if (!ferr.is_open()) {std::cout << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl; flog << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl;} ferr << LogString.str(); ferr.close(); fbin.close(); continue;}
                else
                {
                    CurRecord.Date = atoi(AddDate);
                    CurRecord.Open = Open;
                    CurRecord.High = High;
                    CurRecord.Low = Low;
                    CurRecord.Close = Close;
                    CurRecord.Volume = Volume;

                    LogString.str(""); LogString << std::fixed << std::setprecision(2) << filename1 << " Date=" << CurRecord.Date << " Open=" << CurRecord.Open << " High=" << CurRecord.High << " Low=" << CurRecord.Low << " Close=" << CurRecord.Close << " Volume=" << CurRecord.Volume << std::endl; std::cout << LogString.str(); flog << LogString.str();

                    fbin.seekp(0, std::ios::end);
                    fbin.write( (char*)&CurRecord, sizeof(DOHLCV) );
                }

            fbin.close();


        }

    }


    ///Closing socket
    close(socket_desc);


    LogString.str(""); LogString << "\nServer File " << ServFileIndex << " " << ServerFile[ServFileIndex] << " read and processed" << std::endl; std::cout << LogString.str(); flog << LogString.str();
    LogString.str(""); LogString << "\nSLEEPING " << SleepTime_WSJ << " seconds ..." << std::endl << std::endl; std::cout << LogString.str(); flog << LogString.str();

    sleep(SleepTime_WSJ);  ///Делаем паузу

	}
    ///конец блока for (int ServFileIndex=0; ServFileIndex<6; ServFileIndex++)


    LogString.str(""); LogString << "\nAll Server Files read and processed" << std::endl; std::cout << LogString.str(); flog << LogString.str();


    flog.close();
    return NULL;
}





void UpdLastDay()
{
    pthread_create(&thread[1], NULL, UpdLastDay, NULL);

}





void UpdLastDay_file()
/// This function took last day quotes from the file LastDay.txt which was formed manually from several .csv data files taken from the site of Wall Street Journal.
/// Unfortunately starting from March 2019 Wall Street Journal stopped to provide these .csv files.
/// Some other source of last day quotes should be found.
{
std::ofstream flog, ferr;
std::ifstream fLastDay;
std::fstream fbin;

std::ostringstream LogString;

char line[200];
char *token(NULL);
char seps[]("\t\n");
char token2[200];

char CompanyName[200];
char Symbol[200];
double Open(0), High(0), Low(0), Close(0), NetChange(0), PrevClose(0);
int Volume(0);
char Volume_str[200];

unsigned int i(0), j(0);

std::string filename1;
DOHLCV CurRecord, LastRecord;

double AdmitKoeff(0.01); /// 1%



    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "UpdLastDay_file: can not write into log.txt" << std::endl;


    fLastDay.open("LastDay.txt");
    if (!fLastDay.is_open()) {LogString << "UpdLastDay: файл LastDay.txt не найден" << std::endl; std::cout << LogString.str(); flog << LogString.str(); return;}

    while ( fLastDay.getline(line,200) )
    {
        sscanf(line, "%*s %s", token2);
        if
        ( strcmp(token2, "Symbol") &&
          strcmp(line, "0-9 | A | B | C | D | E | F | G | H | I | J | K | L | M | N | O | P | Q | R | S | T | U | V | W | X | Y | Z\n") &&
          strcmp(line, "Chg 	%Chg 	Vol 	52 Week\n") &&
          strcmp(line, "High 	52 Week\n") &&
          strcmp(line, "Low 	Div 	Yield 	PE 	YTD\n") &&
          strcmp(line, "Low 	Div 	Yield 	YTD\n") &&
          strcmp(line, "%Chg\n") &&
          strcmp(line, "NONE\n")
        )
        {
            CompanyName[0]='\0';
            Symbol[0]='\0';
            Open=0;
            High=0;
            Low=0;
            Close=0;
            NetChange=0;
            PrevClose=0;
            Volume=0;

            std::cout << std::endl << line << std::endl; flog << std::endl << line << std::endl;

            token=strtok(line,seps); ///CompanyName
            if (token)
            {
                strcpy(CompanyName, token);
                std::cout << CompanyName << std::endl; flog << CompanyName << std::endl;
            }

            token=strtok(NULL,seps); ///Symbol
            if (token)
            {
                i=0; j=0;
                while(i<strlen(token)-1)
                {
                    if (token[i]!=0x20) {Symbol[j]=token[i]; j++;}
                    i++;
                }
                Symbol[j]='\0';
                std::cout << Symbol << std::endl; flog << Symbol << std::endl;
            }
            if (token==NULL || Symbol[0]==0) continue;

            token=strtok(NULL,seps); ///Open
            if (token)
            {
                Open=atof(token);
                std::cout << Open << std::endl; flog << Open << std::endl;
            }

            token=strtok(NULL,seps); ///High
            if (token)
            {
                High=atof(token);
                std::cout << High << std::endl; flog << High << std::endl;
            }

            token=strtok(NULL,seps); ///Low
            if (token)
            {
                Low=atof(token);
                std::cout << Low << std::endl; flog << Low << std::endl;
            }

            token=strtok(NULL,seps); ///Close
            if (token)
            {
                Close=atof(token);
                std::cout << Close << std::endl; flog << Close << std::endl;
            }

            token=strtok(NULL,seps); ///NetChange
            if (token)
            {
                NetChange=atof(token);
                std::cout << NetChange << std::endl; flog << NetChange << std::endl;

                PrevClose = Close - NetChange;
                std::cout << PrevClose << std::endl; flog << PrevClose << std::endl;
            }

            token=strtok(NULL,seps); ///%Change

            token=strtok(NULL,seps); ///Volume
            if (token)
            {
                i=0; j=0;
                while(i<strlen(token)-1)
                {
                    if (token[i]!=0x2C && token[i]!=0x20) {Volume_str[j]=token[i]; j++;}
                    i++;
                }
                Volume_str[j]='\0';

                Volume=atoi(Volume_str);

                LogString.str(""); LogString << token << "  " << strlen(token) << "  " << Volume_str << "  " << strlen(Volume_str) << "  " << Volume << std::endl; std::cout << LogString.str(); flog << LogString.str();
            }


            if (Open==0 || High==0 || Low==0 || Close==0 || Volume==0)
            {
                LogString.str("");
                LogString << "Open==0 || High==0 || Low==0 || Close==0 || Volume==0 for Symbol " << Symbol << "; update not performed" << std::endl;
                std::cout << LogString.str(); flog << LogString.str();

                ferr.open("err.txt", std::ios::out | std::ios::app);
                if (!ferr.is_open())
                {
                    std::cout << "UpdLastDay_file: Error: file err.txt can not be opened for writing" << std::endl;
                    flog << "UpdLastDay_file: Error: file err.txt can not be opened for writing" << std::endl;
                }
                ferr << LogString.str();
                ferr.close();

                continue;
            }


            filename1 = "fbin/"; filename1 += Symbol; filename1 += ".bin";

            fbin.open(filename1.c_str(), std::ios::in | std::ios::out | std::ios::binary);
            if (!fbin.is_open())
            {
                    std::cout << "UpdLastDay_file: Error: file " << filename1 << " can not be opened for reading and writing" << std::endl;
                    flog << "UpdLastDay_file: Error: file " << filename1 << " can not be opened for reading and writing" << std::endl;
                    continue;
            }

            fbin.seekg( -sizeof(DOHLCV), std::ios::end );

            fbin.read( (char*)&LastRecord, sizeof(DOHLCV) );

            if ( LastRecord.Date!=atoi(PrevDate) ) {LogString.str(""); LogString << "Symbol " << Symbol << ": LastRecord.Date " << LastRecord.Date << " is not equal PrevDate " << PrevDate << "; update not performed" << std::endl; std::cout << LogString.str(); flog << LogString.str(); ferr.open("err.txt", std::ios::out | std::ios::app); if (!ferr.is_open()) {std::cout << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl; flog << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl;} ferr << LogString.str(); ferr.close(); fbin.close(); continue;}
            else if ( fabs(LastRecord.Close-PrevClose)>AdmitKoeff*LastRecord.Close ) {LogString.str(""); LogString << "Symbol " << Symbol << ": LastRecord.Close " << LastRecord.Close << " differs from PrevClose " << PrevClose << "; POSSIBLE SPLIT; update not performed" << std::endl; std::cout << LogString.str(); flog << LogString.str(); ferr.open("err.txt", std::ios::out | std::ios::app); if (!ferr.is_open()) {std::cout << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl; flog << "UpdLastDay: Error: file err.txt can not be opened for writing" << std::endl;} ferr << LogString.str(); ferr.close(); fbin.close(); continue;}
                else
                {
                    CurRecord.Date = atoi(AddDate);
                    CurRecord.Open = Open;
                    CurRecord.High = High;
                    CurRecord.Low = Low;
                    CurRecord.Close = Close;
                    CurRecord.Volume = Volume;

                    LogString.str(""); LogString << std::fixed << std::setprecision(2) << filename1 << " Date=" << CurRecord.Date << " Open=" << CurRecord.Open << " High=" << CurRecord.High << " Low=" << CurRecord.Low << " Close=" << CurRecord.Close << " Volume=" << CurRecord.Volume << std::endl; std::cout << LogString.str(); flog << LogString.str();

                    fbin.seekp(0, std::ios::end);
                    fbin.write( (char*)&CurRecord, sizeof(DOHLCV) );
                }

            fbin.close();

        }
    }


    fLastDay.close();

    flog.close();

}





void FillAbscentDates(char smb[6], char smbRef[6])
{
std::ifstream fsmb, fsmbRef;
std::ofstream fsmbNew,  frem, flog;

DOHLCV Record, PrevRecord, RefRecord;

std::ostringstream LogString;

std::string smbFilename, smbRefFilename, smbNewFilename;

bool ReadRecordPermit(true), FirstEquel(false), RecordFileEnd(false);

int AbscentDatesCount(0), AbscentDatesCountMax(40);



    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "FillAbscentDates: can not write into log.txt" << std::endl;



	smbFilename = "fbin/"; smbFilename += smb; smbFilename += ".bin";
	fsmb.open(smbFilename.c_str(), std::ios::in | std::ios::binary);
	if (!fsmb.is_open())
    {
        LogString << "FillAbscentDates: .bin file " << smbFilename << " not found" << std::endl;
        std::cout << LogString.str(); flog << LogString.str();
        return;
    }

    smbRefFilename = "fbin/"; smbRefFilename += smbRef; smbRefFilename += ".bin";
	fsmbRef.open(smbRefFilename.c_str(), std::ios::in | std::ios::binary);
	if (!fsmbRef.is_open())
    {
        LogString << "FillAbscentDates: .bin file " << smbRefFilename << " not found" << std::endl;
        std::cout << LogString.str(); flog << LogString.str();
        return;
    }

    smbNewFilename = "fbin/"; smbNewFilename += smb; smbNewFilename += "_new.bin";
    fsmbNew.open(smbNewFilename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!fsmbNew.is_open())
    {
        LogString << "FillAbscentDates: can not open file " << smbNewFilename << " for writing" << std::endl;
        std::cout << LogString.str(); flog << LogString.str();
        return;
    }



    while ( fsmbRef.read((char*)&RefRecord, sizeof(DOHLCV)) )
    {
        if (ReadRecordPermit)
        {
            PrevRecord=Record;

            ///case when Record is not written, because end of file fsmb reached:
            if (RecordFileEnd) RecordFileEnd=false;
            if ( !fsmb.read((char*)&Record, sizeof(DOHLCV)) && RefRecord.Date>Record.Date ) {Record.Date=RefRecord.Date; RecordFileEnd=true;}
        }

        while (RefRecord.Date>Record.Date && !FirstEquel) if ( !fsmb.read((char*)&Record, sizeof(DOHLCV)) ) break;

        if (RefRecord.Date==Record.Date)
            {
                fsmbNew.write((char*)&Record, sizeof(DOHLCV));
                if (RecordFileEnd)
                    {
                        LogString.str(""); LogString << "FillAbscentDates: smb: " << smb << "; writing date line at file end: " << RefRecord.Date << "; Record.Date: " << Record.Date << std::endl;
                        std::cout << LogString.str(); flog << LogString.str();

                        AbscentDatesCount++;
                    }

                FirstEquel=true; ReadRecordPermit=true;
            }

        if (RefRecord.Date<Record.Date)
        {
            ReadRecordPermit=false;
            if (FirstEquel)
            {
                PrevRecord.Date=RefRecord.Date;
                PrevRecord.Open = PrevRecord.Close;
                PrevRecord.High = PrevRecord.Close;
                PrevRecord.Low = PrevRecord.Close;
                PrevRecord.Volume=0;
                fsmbNew.write((char*)&PrevRecord, sizeof(DOHLCV));
                LogString.str(""); LogString << "FillAbscentDates: smb: " << smb << "; writing absent date line: " << RefRecord.Date << "; Record.Date: " << Record.Date << std::endl;
                std::cout << LogString.str(); flog << LogString.str();

                AbscentDatesCount++;
            }

        }

    }


        fsmb.close();
        fsmbRef.close();
        fsmbNew.close();

        remove(smbFilename.c_str());
        rename(smbNewFilename.c_str(), smbFilename.c_str());

        LogString.str(""); LogString << "AbscentDatesCount " << AbscentDatesCount << std::endl;
        std::cout << LogString.str(); flog << LogString.str();

        if (AbscentDatesCount>=AbscentDatesCountMax)
        {
            frem.open("SymsToRem.txt", std::ios::out | std::ios::app);
            if (!frem.is_open())
            {
                LogString.str(); LogString << "FillAbscentDates: can not write into SymsToRem.txt" << std::endl;
                std::cout << LogString.str(); flog << LogString.str();
            }
            frem << smb << std::endl;
            frem.close();
        }

        flog.close();
}





void FillLastAbscentDates(char smb[6], char smbRef[6])
{
std::ifstream fsmbRef;
std::ofstream flog;
std::fstream fsmb;

DOHLCV LastRecord, RefRecord;

std::ostringstream LogString;

std::string SmbFilename, smbRefFilename;

bool WritePermit(false);



	///Reading LastRecord from smb .bin file
	SmbFilename = "fbin/"; SmbFilename += smb; SmbFilename += ".bin";

	fsmb.open(SmbFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if (!fsmb.is_open())
        {
            LogString << "FillAbscentDates: .bin file " << SmbFilename << " not found" << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "FillAbscentDates: can not write into log.txt\n";
            flog << LogString.str();
            flog.close();
            return;
        }

	fsmb.seekg(-sizeof(DOHLCV), std::ios::end);

	fsmb.read((char*)&LastRecord, sizeof(DOHLCV)); LastRecord.Volume=0;


    smbRefFilename = "fbin/"; smbRefFilename +=  smbRef; smbRefFilename += ".bin";
    fsmbRef.open(smbRefFilename.c_str());
    if (!fsmbRef)
    {
        LogString << "FillAbscentDates: .bin file " << SmbFilename << " not found" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "FillAbscentDates: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
        return;
    }


    while ( fsmbRef.read((char*)&RefRecord, sizeof(DOHLCV)) )
    {
        if ( WritePermit && RefRecord.Date<=atoi(AddDate) )
        {
            LastRecord.Date=RefRecord.Date;
            fsmb.seekp(0, std::ios::end);
            fsmb.write( (char*)&LastRecord, sizeof(DOHLCV) );
        }



        if (RefRecord.Date==LastRecord.Date) WritePermit=true;
    }


    fsmb.close();
    fsmbRef.close();

}





void* MethodButton(void* t)
{
    std::ifstream fi;
    std::ofstream flog;

    char line0[200];

    std::ostringstream LogString;

    int k(0);

    int all2Num(0);
    all2Line *all2Array;


    LogString << "BackTest started" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "MethodButton: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();


    remove("Result.txt");
    remove("PosOpen.txt");



    fi.open("all.txt");
    if (!fi.is_open())
    {
        LogString << "MethodButton: file all.txt not found" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "MethodButton: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
        return NULL;
    }


    ///Determining the number of lines in the file all.txt: all2Num
    k=0;
    while ( fi.getline(line0, 200) ) k++;
    all2Num=k;

    ///Creating all2Array with size all2Num in the memory
    all2Array = new all2Line [all2Num];

    fi.clear();
    fi.seekg(0, std::ios::beg);


    ///Writing all.txt into all2Array
    k=0;
    while ( fi.getline(line0, 200) )
    {
        sscanf(line0,"%s", all2Array[k].Symbol);
        k++;
    }


    fi.close();


    for (k=0;k<all2Num;k++)
    {
        if ( BINtoMEM(all2Array[k].Symbol,d,6000,BarsNum) )
        {
            ///Running Method-function
            Method(all2Array[k].Symbol);
        }
    }


    ///Removing all2Array from the memory
    delete [] all2Array;




	if ( ReadMethRes(0) )
	{
        SortMethRes();
        ProcessMethRes(0);
	}

    remove("Result.txt");

    LogString.str("");
    LogString << std::endl << "BackTest completed.\nFiles: Result.xls and PosOpen.txt (if some positions are open) created." << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "MethodButton: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return NULL;
}





void MethodButton()
{
    pthread_create(&thread[1], NULL, MethodButton, NULL);

}





int ReadMethRes(int Option)
{
    std::ifstream fi;
    std::ofstream flog;
    int j;
    char line[1000];
    char *token;
    char seps[]{',', '\t', '/', c0D, c00};

    std::ostringstream LogString;



    if (Option==1) fi.open("Result1.txt");
    else fi.open("Result.txt");

    if (!fi.is_open())
    {
        LogString << "ReadMethRes: File Result.txt not found" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "ReadMethRes: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
        return 0;
    }


    j=0;
    while ( fi.getline(line, 1000) )
    {
        if (strlen(line)>10) j++;
    }



    LinesNum = j; ///LinesNum (number of lines in the file with scan result)


    pResLine = new ResLine[LinesNum];



    fi.clear();
    fi.seekg(0, std::ios::beg);



    j=0;
    while ( fi.getline(line, 1000) )
    {
        if (strlen(line)>10)
        {
            token=strtok(line,seps); sprintf(pResLine[j].Symbol, "%s", token); ///Symbol

            token=strtok( NULL,seps ); pResLine[j].EntryDate=atoi(token);

            token=strtok( NULL,seps ); pResLine[j].ExitDate=atoi(token);

            token=strtok( NULL,seps ); pResLine[j].Shares=atoi(token); ///Shares

            token=strtok( NULL,seps ); pResLine[j].EntryPrice=atof(token); ///EntryPrice

            token=strtok( NULL,seps ); pResLine[j].ExitPrice=atof(token); ///ExitPrice

            token=strtok( NULL,seps ); pResLine[j].PnL=atof(token); ///PnL

            j++;
        }
    }

    fi.close();
    return 1;
}




void SortMethRes()
{
int i,j,k,l;

pResLine2 = new ResLine[LinesNum];


for (i=0; i<LinesNum; i++)
for (j=0; j<LinesNum; j++) {
	if (pResLine[i].EntryDate>=pResLine2[j].EntryDate) {
		for (k=LinesNum-1; k>j; k--) {
			strcpy( pResLine2[k].Symbol, pResLine2[k-1].Symbol );
			pResLine2[k].EntryDate=pResLine2[k-1].EntryDate;
			pResLine2[k].ExitDate=pResLine2[k-1].ExitDate;
			pResLine2[k].Shares=pResLine2[k-1].Shares;
			pResLine2[k].EntryPrice=pResLine2[k-1].EntryPrice;
			pResLine2[k].ExitPrice=pResLine2[k-1].ExitPrice;
			pResLine2[k].PnL=pResLine2[k-1].PnL; }
		strcpy( pResLine2[j].Symbol, pResLine[i].Symbol );
		pResLine2[j].EntryDate=pResLine[i].EntryDate;
		pResLine2[j].ExitDate=pResLine[i].ExitDate;
		pResLine2[j].Shares=pResLine[i].Shares;
		pResLine2[j].EntryPrice=pResLine[i].EntryPrice;
		pResLine2[j].ExitPrice=pResLine[i].ExitPrice;
		pResLine2[j].PnL=pResLine[i].PnL;
		break; } }

delete [] pResLine;

pResLine3 = new ResLine[LinesNum];

l=0;
while (l<LinesNum)
{
pResLine3[l]=pResLine2[LinesNum-1-l];
l++;
}

delete [] pResLine2;

}




void ProcessMethRes(int Option)
///Option==0: original version
///else: nothing printed, no files created
{

    std::ofstream fo, flog;
    std::ostringstream LogString;
    int i;



    if (!Option)
    {
        fo.open("Result.xls");
        if (!fo.is_open())
        {
            LogString << "ProcessMethRes: can not write into file Result.xls" << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "ProcessMethRes: can not write into log.txt\n";
            flog << LogString.str();
            flog.close();
            return;

        }
    }


    TradesNum=0; GrossTradeSize=0; GrossProfit=0; GrossLoss=0; NetProfit=0; DD=0; MinDD=0;


    for (i=0; i<LinesNum; i++)
    {
        if (pResLine3[i].ExitDate)
        {
            TradesNum++;
            GrossTradeSize+=pResLine3[i].EntryPrice*pResLine3[i].Shares;
            if (pResLine3[i].PnL>0) GrossProfit+=pResLine3[i].PnL; else GrossLoss+=pResLine3[i].PnL;
            NetProfit+=pResLine3[i].PnL;
            DD= DD+pResLine3[i].PnL>0 ? 0 : DD+pResLine3[i].PnL; if (DD<MinDD) MinDD=DD;
        }

        if (!Option)
        {
            fo << pResLine3[i].Symbol << '\t' << pResLine3[i].EntryDate << '\t' << pResLine3[i].ExitDate << '\t' << pResLine3[i].Shares << '\t' << pResLine3[i].EntryPrice << '\t' << pResLine3[i].ExitPrice << '\t' << pResLine3[i].PnL << '\t' << DD << '\t' << NetProfit << std::endl;

            LogString << pResLine3[i].Symbol << '\t' << pResLine3[i].EntryDate << '\t' << pResLine3[i].ExitDate << '\t' << pResLine3[i].Shares << '\t' << pResLine3[i].EntryPrice << '\t' << pResLine3[i].ExitPrice << '\t' << pResLine3[i].PnL << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "ProcessMethRes: can not write into log.txt\n";
            flog << LogString.str();
            flog.close();
        }
    }


AvTrade=NetProfit/TradesNum;
AvTradeSize=GrossTradeSize/TradesNum;
AvTradeP=100*AvTrade/AvTradeSize;
PF= GrossLoss ? -GrossProfit/GrossLoss : 100;


if (!Option)
{
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "ProcessMethRes: can not write into log.txt\n";

    LogString.str("");
    LogString << std::endl << std::endl;
    LogString << "NetProfit" << '\t' << NetProfit << std::endl;
    LogString << "Trades" << '\t' << TradesNum << std::endl;
    LogString << "AvTrade" << c25 << '\t' << AvTradeP << std::endl;
    LogString << "AvTrade$" << '\t' << AvTrade << std::endl;
    LogString << "MinDD" << '\t' << MinDD << std::endl;
    LogString << "PF" << '\t' << PF << std::endl;

    std::cout << LogString.str();
    flog << LogString.str();
    fo << LogString.str();

    fo.close();
    flog.close();
}

delete [] pResLine3;

}





void* Optimize(void* t)

{
    std::ifstream fall2;
    std::ofstream fmeth, flog;

    std::ostringstream LogString;

    char line0[200];

    int all2LinesCount(0);

    int z(0);
    int l,m; ///l and m are used for initialization and printing of array Methods[][]

    int TradesNumMin(5); ///variants with trades number lower than this value are not included into the list for selection (for creation of array Methods)

    int all2Num(0);
    all2Line *all2Array;



    LogString << "\nOptimize(): Optimization started ..." << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Optimize: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();




    ///Writing all.txt into all2Array

    fall2.open("all.txt");
    if (!fall2.is_open())
    {
        LogString.str("");
        LogString << "Optimize: file all.txt not found" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "Optimize: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
        return NULL;
    }


    ///Calculating number of lines in the file all.txt: all2Num
    all2LinesCount=0;
    while ( fall2.getline(line0, 200) ) all2LinesCount++;
    all2Num=all2LinesCount;

    ///Creating all2Array with size all2Num in the memory
    all2Array = new all2Line [all2Num];

    fall2.clear();
    fall2.seekg(0, std::ios::beg);


    ///Writing all.txt into all2Array
    all2LinesCount=0;
    while ( fall2.getline(line0, 200) )
    {
        sscanf(line0,"%s", all2Array[all2LinesCount].Symbol);
        all2LinesCount++;
    }


    fall2.close();

    ///Writing all.txt into all2Array completed




	/// Initialization of array Methods[][]:
	for (l=0; l<100; l++)
		for (m=0; m<=8; m++) Methods[l][m]=-100000.00;


    /// Main optimization loop starts

	for (EntryStDevKoeff=0; EntryStDevKoeff<=10; EntryStDevKoeff+=0.1)
	///for (TargStDevKoeff=0; TargStDevKoeff<=10; TargStDevKoeff+=0.1)
    {

		remove("Result.txt");
		remove("PosOpen.txt");



        for (z=0;z<all2Num;z++)
        {
            if ( BINtoMEM(all2Array[z].Symbol,d,6000,BarsNum) ) Method(all2Array[z].Symbol);
        }


		if ( ReadMethRes(0) )
		{
			SortMethRes();
			ProcessMethRes(2);
		}



		/// fillng the array Methods[][]
		if ( TradesNum>=TradesNumMin ) /// && TradesNum*255/BarsNum>=2 )
			for (int j=0; j<100; j++)
			{
				if (PF>=Methods[j][2]) ///(TradesNum >= Methods[j][3]) ///(NetProfit>=Methods[j][0])
				{
					for (int k=99; k>j; k--)
					{
						Methods[k][0]=Methods[k-1][0];
						Methods[k][1]=Methods[k-1][1];
						Methods[k][2]=Methods[k-1][2];
						Methods[k][3]=Methods[k-1][3];
						Methods[k][4]=Methods[k-1][4];
						Methods[k][5]=Methods[k-1][5];
						Methods[k][6]=Methods[k-1][6];
						Methods[k][7]=Methods[k-1][7];
						Methods[k][8]=Methods[k-1][8];
					}
					Methods[j][0]=NetProfit;
					Methods[j][1]=MinDD;
					Methods[j][2]=PF;
					Methods[j][3]=(double)TradesNum;
					Methods[j][4]=EntryStDevKoeff;
					Methods[j][5]=TargStDevKoeff;
					Methods[j][6]=all2Array[z].KoefUp;
					Methods[j][7]=all2Array[z].KoefDown;
					Methods[j][8]=(double)all2Array[z].CondCountChosen;
					break;
				}
			}


    }
    /// Main optimization loop completed

    remove("Result.txt");
    remove("PosOpen.txt");




	///Printing array Methods[][]

	fmeth.open("Methods.csv", std::ios::out | std::ios::app);
    if (!fmeth.is_open())
    {
        LogString.str("");
        LogString << "Optimize(): can not write into file Methods.csv" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "Optimize(): can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
        return NULL;

    }

	fmeth << "NetProfit,MinDD,PF,TradesNum,EntryStDevKoeff,TargStdevKoeff\n\n";

	for (m=0;m<100;m++)
        fmeth << std::fixed << std::setprecision(2) << Methods[m][0] << "," << Methods[m][1] << "," << Methods[m][2] << "," << std::setprecision(0) << Methods[m][3] << "," << std::setprecision(1) << Methods[m][4] << "," << std::setprecision(3) << Methods[m][5] << std::endl;

	fmeth.close();




    LogString.str("");
    LogString << "Optimize(): Optimization completed.\nFile with optimization result: Methods.csv created." << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Optimize(): can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

    return NULL;
}





void Optimize()
{
    pthread_create(&thread[1], NULL, Optimize, NULL);
}





void WriteBsk()
{

    std::ifstream fi;
    std::ofstream fbsk, flog;
    std::ostringstream LogString;
    char line0[200];

    char symbolL[10];

    int RetCode_s(0);

    double shares(0); ///number of shares
    double EntryPrice(0);
    double PriceDelta(0);

    int OrdersCount(0);



    LogString << "Creating basket file ..." << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "WriteBsk(): can not write into log.txt\n";
    flog << LogString.str();
    flog.close();


    fi.open("all.txt");
    if (!fi.is_open())
    {
        LogString.str("");
        LogString << "File all.txt not found" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "WriteBsk(): can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
        return;
    }


    fbsk.open("EntryBasket.txt");
    if (!fbsk.is_open())
    {
        LogString.str("");
        LogString << "File EntryBasket.txt can not be created" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "WriteBsk(): can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
        fi.close();
        return;
    }


    while ( fi.getline(line0, 200) )
    {
        sscanf(line0, "%s", symbolL);


        RetCode_s = BINtoMEM(symbolL,d,DaysNum+1,BarsNum); ///Filling data arrays from .bin files

        if (!RetCode_s)
            {
                LogString.str("");
                LogString << "Symbol " << symbolL << ": data not loaded" << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "WriteBsk(): can not write into log.txt\n";
                flog << LogString.str();
                flog.close();
            }
        else
        {
            if (BarsNum<DaysNum)
            {
                LogString.str("");
                LogString << "Symbol " << symbolL << ": BarsNum<DaysNum, calculation not done" << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "WriteBsk(): can not write into log.txt\n";
                flog << LogString.str();
                flog.close();
            }
            else
            {

                if
                (
                    d[BarsNum-3].Close < d[BarsNum-3].Open &&
                    d[BarsNum-2].Close < d[BarsNum-2].Open &&
                    d[BarsNum-1].Close < d[BarsNum-1].Open &&

                    d[BarsNum-1].Open < d[BarsNum-2].Close
                )

                {
                    k=EntryStDevKoeff*StDevNorm(d, BarsNum-1, DaysNum);
                    EntryPrice = floor(d[BarsNum-1].Low*(1-k)*20)/20;
                    if (EntryPrice>MinPrice && EntryPrice<MaxPrice)
                    {
                        if (EntryPrice<=120) shares=floor(TradeSize/EntryPrice/10+0.2)*10;
                        else shares=floor(TradeSize/EntryPrice+0.2);


                        PriceDelta = d[BarsNum-1].Low-EntryPrice;


                        LogString.str("");
                        LogString << symbolL << "  " <<  std::fixed << std::setprecision(0) << shares << "  YestLow " << std::setprecision(2) << d[BarsNum-1].Low << "  EntryPrice " << EntryPrice << "  PriceDelta " << std::setprecision(3) << PriceDelta << std::endl;
                        fbsk << LogString.str();
                        std::cout << LogString.str();
                        flog.open("log.txt", std::ios::out | std::ios::app);
                        if (!flog.is_open()) std::cout << "WriteBsk(): can not write into log.txt\n";
                        flog << LogString.str();
                        flog.close();

                        OrdersCount++;
                    }
                }

            }

        }

    }



    fbsk.close();
    fi.close();


    LogString.str("");
    LogString << "Basket file: EntryBasket.txt created; " << OrdersCount << " orders in the basket" << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "WriteBsk(): can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

}





void PlaceBsk(int option)
/// option 2: case of canceling orders and placing back
{
    std::ifstream fbsk;
    std::ofstream ford, flog;
    std::ostringstream LogString;
    char line[200];
    char symbol[6];
    int shares(0);
    double EntryPrice(0);
    int OrdBskLinesCount(0); ///line counter in the basket of orders

    Contract contract;
    Order order;
    TagValueListSPtr mktDataOptions;
    bool snapshot(false);




    ///contract.symbol = symbol; ///this parameter will be set later
    contract.secType = "STK";
    contract.currency = "USD";
    contract.exchange = "SMART";
    contract.primaryExchange = "NASDAQ";

    order.action = "BUY";
    ///order.totalQuantity = shares; ///this parameter will be set later
    order.orderType = "LMT";
    ///order.lmtPrice = EntryPrice; ///this parameter will be set later
    order.outsideRth = true;
    order.transmit = false;


    if (!client.isConnected())
    {
        LogString << "Client not connected" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PlaceBsk(): can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
        return;
    }



    if (!OrdBskExists)

    {
        fbsk.open("EntryBasket.txt");
        if(!fbsk.is_open())
        {
            LogString << "File EntryBasket.txt not found" << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "PlaceBsk(): can not write into log.txt\n";
            flog << LogString.str();
            flog.close();
            return;
        }


        OrdBskLinesCount=0;
        while ( fbsk.getline(line, 200) )
        {
            sscanf(line, "%s %i %*s %*s %*s %lf", symbol, &shares, &EntryPrice);

            OrdBsk[OrdBskLinesCount].OrdId = ++OrdId;
            strcpy(OrdBsk[OrdBskLinesCount].symbol, symbol);
            OrdBsk[OrdBskLinesCount].shares = shares;
            OrdBsk[OrdBskLinesCount].EntryPrice = EntryPrice;


            contract.symbol = symbol;
            order.totalQuantity = shares;
            order.lmtPrice = EntryPrice;


            ///OrdId icrement already done a few lines higher
            client.m_pClient->placeOrder(OrdBsk[OrdBskLinesCount].OrdId, contract, order);


            OrdBskLinesCount++;
            sleep(1);
        }
        OrdBskLinesNum=OrdBskLinesCount;
        fbsk.close();


        ford.open("OrdBsk.txt");
        if (!ford.is_open())
        {
            LogString << "File OrdBsk.txt can not be created" << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "PlaceBsk(): can not write into log.txt\n";
            flog << LogString.str();
            flog.close();
        }

        for (int i=0; i<OrdBskLinesNum; i++)
            ford << OrdBsk[i].OrdId << "  " << OrdBsk[i].symbol << "  " << OrdBsk[i].shares << "  " << std::fixed << std::setprecision(2) << OrdBsk[i].EntryPrice << std::endl;

        ford.close();

    }



    if (!CheckMktDataRunning)
    {
        pthread_create(&thread[1], NULL, CheckMktData, NULL);
        CheckMktDataRunning=true;
        LogString.str("");
        LogString << "Thread CheckMktData started" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PlaceBsk(): can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
    }



    for (int i=0; i<OrdBskLinesNum; i++)
    {
        contract.symbol = OrdBsk[i].symbol;

        if (option==2)
        {
            if (OrdBsk[i].OrdId && OrdBskCanceled[i].OrdId)
            {
                if (PlaceAllOrd || MktDataOpenedNum<MktDataOpenedLim)
                {
                    client.m_pClient->reqMktData(i, contract, "", snapshot, mktDataOptions);
                    LogString.str("");
                    LogString << "MktData for contract " << OrdBsk[i].symbol << " requested   OrdBskPlaced[i].OrdId " << OrdBskPlaced[i].OrdId << "   OrdBsk[i].time " << OrdBsk[i].time << "   MktDataOpenedNum " << MktDataOpenedNum << std::endl;
                    std::cout << LogString.str();
                    flog.open("log.txt", std::ios::out | std::ios::app);
                    if (!flog.is_open()) std::cout << "PlaceBsk(): can not write into log.txt\n";
                    flog << LogString.str();
                    flog.close();
                }
                else
                {
                    MktDataExtra[MktDataExtraNum].OrdId = i;
                    strcpy(MktDataExtra[MktDataExtraNum].symbol, OrdBsk[i].symbol);
                    MktDataExtraNum++;
                }

                if (!PlaceAllOrd) MktDataOpenedNum++;
            }
        }
        else
        {
            if (OrdBsk[i].OrdId && OrdBskPlaced[i].OrdId==0)
            {
                if (PlaceAllOrd || MktDataOpenedNum<MktDataOpenedLim)
                {
                    client.m_pClient->reqMktData(i, contract, "", snapshot, mktDataOptions);
                    LogString.str("");
                    LogString << "MktData for contract " << OrdBsk[i].symbol << " requested   OrdBskPlaced[i].OrdId " << OrdBskPlaced[i].OrdId << "   OrdBsk[i].time " << OrdBsk[i].time << "   MktDataOpenedNum " << MktDataOpenedNum << std::endl;
                    std::cout << LogString.str();
                    flog.open("log.txt", std::ios::out | std::ios::app);
                    if (!flog.is_open()) std::cout << "PlaceBsk(): can not write into log.txt\n";
                    flog << LogString.str();
                    flog.close();
                }
                else
                {
                    MktDataExtra[MktDataExtraNum].OrdId = i;
                    strcpy(MktDataExtra[MktDataExtraNum].symbol, OrdBsk[i].symbol);
                    MktDataExtraNum++;
                }

                MktDataOpenedNum++;
            }
        }

        sleep(1);
    }


    if (MktDataExtraNum && !ReqMktDataExtraRunning)
    {
        pthread_create(&thread[1], NULL, ReqMktDataExtra, NULL);
        ReqMktDataExtraRunning=true;
        LogString.str("");
        LogString << "Thread ReqMktDataExtra started" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PlaceBsk(): can not write into log.txt\n";
        flog << LogString.str();
        flog.close();
    }

}





void* CheckMktData(void* t)
{
    std::ofstream fSlowTickers, fOrdBsk, fOrdBskPlaced, fMktDataExtra, flog;
    std::ostringstream LogString;
    time_t CurTime(0);
    struct tm *timeinfo;
    int NoChangeTime(0);



    if (OrdBskLinesNum)
    {
        sleep(15);
        for(;;)
        {
            CurTime=time(NULL);
            timeinfo = localtime(&CurTime);


            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog) std::cout << "CheckMktData(): can not write into log.txt\n";


            fSlowTickers.open("SlowTickers.txt");
            if (!fSlowTickers)
            {
                LogString << "can not write into file SlowTickers.txt" << std::endl;
                std::cout << LogString.str();
                flog << LogString.str();
            }


            for(int i=0; i<OrdBskLinesNum; i++)
            {
                NoChangeTime=CurTime-OrdBsk[i].time;

                if ( OrdBskPlaced[i].OrdId==0 && NoChangeTime>150 )
                {
                    LogString.str("");
                    LogString << OrdBsk[i].OrdId << "  " << OrdBsk[i].symbol << "  " << OrdBsk[i].shares << "  " << std::fixed << std::setprecision(2) << OrdBsk[i].EntryPrice << "  " << OrdBsk[i].price << "  " << std::setprecision(3) << OrdBsk[i].PriceDelta << "  " << NoChangeTime << "  " << OrdBsk[i].LocalTime << std::endl;
                    fSlowTickers << LogString.str();
                    flog << LogString.str();
                    std::cout << LogString.str();
                }
                else fSlowTickers << "0\n";
            }

            std::cout << "CancelAllOrdPermit: " << CancelAllOrdPermit << "  PriceDeltaTrig " << std::fixed << std::setprecision(2) << PriceDeltaTrig << "  hours " << timeinfo->tm_hour << "  MktDataOpenedNum " << MktDataOpenedNum << "  MktDataExtraNum " << MktDataExtraNum << std::endl << std::endl;
            flog << "CancelAllOrdPermit: " << CancelAllOrdPermit << "  PriceDeltaTrig " << std::fixed << std::setprecision(2) << PriceDeltaTrig << "  hours " << timeinfo->tm_hour << std::endl << std::endl;

            fSlowTickers.close();
            flog.close();



            fOrdBsk.open("OrdBsk.txt");
            if (!fOrdBsk.is_open())
            {
                LogString.str("");
                LogString << "can not write into file OrdBsk.txt" << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "CheckMktData(): can not write into log.txt\n";
                flog << LogString.str();
                flog.close();
            }

            for(int i=0; i<OrdBskLinesNum; i++)
                fOrdBsk << OrdBsk[i].OrdId << "  " << OrdBsk[i].symbol << "  " << OrdBsk[i].shares << "  " << std::fixed << std::setprecision(2) << OrdBsk[i].EntryPrice << "  " << OrdBsk[i].price << "  " << std::setprecision(3) << OrdBsk[i].PriceDelta << "  " << OrdBsk[i].time << "  " << OrdBsk[i].LocalTime << std::endl;
            fOrdBsk.close();


            fOrdBskPlaced.open("OrdBskPlaced.txt");
            if (!fOrdBskPlaced.is_open())
            {
                LogString.str("");
                LogString << "can not write into file OrdBskPlaced.txt" << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "CheckMktData(): can not write into log.txt\n";
                flog << LogString.str();
                flog.close();
            }

            for(int i=0; i<OrdBskLinesNum; i++)
                fOrdBskPlaced << OrdBskPlaced[i].OrdId << "  " << OrdBskPlaced[i].symbol << "  " << OrdBskPlaced[i].shares << "  " << std::fixed << std::setprecision(2) << OrdBskPlaced[i].EntryPrice << std::endl;
            fOrdBskPlaced.close();


            if (MktDataExtraNum)
            {
                fMktDataExtra.open("MktDataExtra.txt");
                if (!fMktDataExtra.is_open())
                {
                    LogString.str("");
                    LogString << "can not write into file MktDataExtra.txt" << std::endl;
                    std::cout << LogString.str();
                    flog.open("log.txt", std::ios::out | std::ios::app);
                    if (!flog.is_open()) std::cout << "CheckMktData(): can not write into log.txt\n";
                    flog << LogString.str();
                    flog.close();
                }

                for(int i=0; i<MktDataExtraNum; i++)
                    fMktDataExtra << MktDataExtra[i].OrdId << "  " << MktDataExtra[i].symbol << std::endl;
                fMktDataExtra.close();
            }


            sleep(15);
        }

    }

    return NULL;
}





void* ReqMktDataExtra(void*t)
{
Contract contract;
TagValueListSPtr mktDataOptions;
bool snapshot(true);

///contract.symbol = symbol; ///this parameter will be set later
contract.secType = "STK";
contract.currency = "USD";
contract.exchange = "SMART";
contract.primaryExchange = "NASDAQ";




for(;;)
{

for(int i=0; i<MktDataExtraNum; i++)
{
	contract.symbol = MktDataExtra[i].symbol;


    client.m_pClient->reqMktData(MktDataExtra[i].OrdId, contract, "", snapshot, mktDataOptions);



    sleep(1);
}

}

}





void CancelAllOrders()
{
    std::ofstream flog;
    std::ostringstream LogString;

    if (OrdBskLinesNum)
    {

        for (int i=0; i<OrdBskLinesNum; i++)
        {
            if (OrdBskPlaced[i].OrdId)
            {
                client.m_pClient->cancelOrder(OrdBskPlaced[i].OrdId);

                LogString << "Request sent for cancelling order " << OrdBskPlaced[i].OrdId << " for symbol " << OrdBskPlaced[i].symbol << "; MktDataOpenedNum " << MktDataOpenedNum << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "CancelAllOrders(): can not write into log.txt\n";
                flog << LogString.str();
                flog.close();

                OrdBskCanceled[i].OrdId = OrdBskPlaced[i].OrdId;
                OrdBskPlaced[i].OrdId=0; OrdBskPlaced[i].symbol[0]='0'; OrdBskPlaced[i].symbol[1]='\0'; OrdBskPlaced[i].shares=0; OrdBskPlaced[i].EntryPrice=0;


                sleep(2);
            }
        }

    }

}
