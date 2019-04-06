#ifndef GLOBALS_H
#define GLOBALS_H

#include "tcMain.h"
#include "PosixClient.h"




struct DOHLCV
{
	int Date;
	double Open;
	double High;
	double Low;
	double Close;
	int Volume;

	DOHLCV()
        : Date(0), Open(0), High(0), Low(0), Close(0), Volume(0)
	{}

};



struct ISDOHLCV
{
    int Id;
    char Smb[6];
	int Date;
	double Open;
	double High;
	double Low;
	double Close;
	int Volume;

	ISDOHLCV()
        : Id(0), Smb{}, Date(0), Open(0), High(0), Low(0), Close(0), Volume(0)
	{}
};



struct QRec
{
	char	date[10];
	char	time[10];
	char	ticker[10];
	double	price;
	double	bid;
	double	ask;
	int		vol;
	double	open;
	double	high;
	double	low;

	QRec()
        : date{}, time{}, ticker{}, price(0), bid(0), ask(0), vol(0), open(0), high(0), low(0)
    {}
};



struct all2Line
{
	char Symbol[10];
	int HiLoPeriod;
	double KoefUp;
	double KoefDown;
    double EntryStDevKoeff;
    double Target;
	bool Cond;
	int CondCount;
	int CondCountChosen;
	int DateExited;

	all2Line()
        : Symbol{}, HiLoPeriod(0), KoefUp(0), KoefDown(0), EntryStDevKoeff(0), Target(0), Cond(false), CondCount(0), CondCountChosen(0), DateExited(0)
    {}
};



struct ResLine
{
	char Symbol[10];
	int EntryDate;
	int ExitDate;
	int Shares;
	double EntryPrice;
	double ExitPrice;
	double PnL;

	ResLine()
        : Symbol{}, EntryDate(0), ExitDate(0), Shares(0), EntryPrice(0), ExitPrice(0), PnL(0)
    {}
};



struct OrdBskLine
{
    int OrdId;
	char symbol[10];
	int shares;
	double EntryPrice;
	double price;
	double PriceDelta;
	time_t time;
	char LocalTime[9];

	OrdBskLine()
        : OrdId(0), symbol{}, shares(0), EntryPrice(0), price(0), PriceDelta(0), time(2000000000), LocalTime{"00:00:00"}
	{}
};




extern DOHLCV d[6000];

extern char Smb[3500][6]; ///array to keep contents of file all.txt

extern OrdBskLine OrdBsk[500]; ///array to keep Order Basket Symbols
extern OrdBskLine OrdBskPlaced[500]; ///array to keep Order Basket Symbols for Placed orders
extern OrdBskLine OrdBskCanceled[500]; ///array to keep Order Basket Symbols for Canceled orders
extern OrdBskLine MktDataExtra[500]; ///array to keep tickers over limit of simultaneously opened MkData
extern int MktDataOpenedNum;
extern int MktDataExtraNum;
extern int OrdBskLinesNum;

extern double Methods[100][9];





extern tcDialog* dlg;

extern PosixClient client;

#define NUM_REPOTHREADS 20
extern pthread_mutex_t repoMutexes[NUM_REPOTHREADS];
extern pthread_cond_t repoConditions[NUM_REPOTHREADS];
extern pthread_attr_t repoAttr;


extern pthread_mutex_t continue_mutex; /** mutex used for processMessages as quit flag */
extern pthread_attr_t attr;
extern pthread_mutex_t client_mutex; /** mutex used for processMessages to avoid segmentation fault */

extern char smbRef[6];

extern char AddDate[10];
extern char PrevDate[10];

extern int DateNotTradingFrom;

extern int SleepTime_WSJ;

extern int BarsNum;
extern int Len;

extern bool BinFile;

extern int LinesNum;
extern ResLine *pResLine;
extern ResLine *pResLine2;
extern ResLine *pResLine3;

extern int TradesNum;
extern double GrossTradeSize;
extern double GrossProfit;
extern double GrossLoss;
extern double NetProfit;
extern double PF;
extern double DD;
extern double MinDD;
extern double AvTrade;
extern double AvTradeSize;
extern double AvTradeP;

extern int OrdId;

extern bool OrdBskExists;
extern bool OrdBskPlacedExists;
extern bool CancelAllOrdPermit;
extern bool CheckMktDataRunning;
extern bool ReqMktDataExtraRunning;

/// Parameters of the method
extern double EntryStDevKoeff;
extern double TargStDevKoeff;
extern double Target;
extern double TradeSize;
extern double MinPrice;
extern double MaxPrice;
extern int DaysNum; ///Number of Days to calculate MA, StDev, and request data for
extern double k; /// k=EntryStDevKoeff*StDevNorm(d, i-1, DaysNum);  EntryPrice = floor(d[i-1].Low*(1-k)*100)/100;

extern bool PlaceAllOrd;
extern double PriceDeltaTrig;
extern double PriceDeltaTrig2;

extern int MorningTimeLine;
extern int MktDataOpenedLim;




extern const char c0D;
extern const char c0A;
extern const char c5C;  /// char '\'
extern const char c2F;  /// char '/'
extern const char c20;  /// space
extern const char c00;
extern const char c25;  /// Percent sign '%'
extern const char c09;  /// Tabulation





void init_repoMutexes();
void destroy_repoMutexes();
int needQuit(pthread_mutex_t *mtx);
void* processMessages_gl(void* t);
void processMessages_gl();
void processMessages2_gl();
void endProcessMessages_gl();

double StDevNorm(DOHLCV *SR, int i_last, int period);
int AvVol(DOHLCV *SR, int i_last, int period);

void* HistData(void* t);
void HistData();

int RecvAll(int socket_desc, void *data, int data_size);

void* UpdLastDay(void* t);
void UpdLastDay();
void UpdLastDay_file();

bool FileExists(const char *fname);
bool Round(double arg);

int BINtoMEM (char *symbol, DOHLCV *arr, int BarsRequested, int &BarsGot);
int ASCIItoMEM(char *symbol);
int MEMtoBIN(char *symbol, int opt/*=0*/);
void MEMtoASCII(char *symbol);
void BINtoASCII();
void ASCIItoBIN();

void FillAbscentDates(char smb[6], char smbRef[6]);
void FillLastAbscentDates(char smb[6], char smbRef[6]);

void Method(char *symbol);

void* MethodButton(void* t);
void MethodButton();

int ReadMethRes(int Option);
void SortMethRes();
void ProcessMethRes(int Option);

void* Optimize(void* t);
void Optimize();

void WriteBsk();
void PlaceBsk(int option);

void* CheckMktData(void* t);
void* ReqMktDataExtra(void*t);

void CancelAllOrders();




#endif  // GLOBALS_H
