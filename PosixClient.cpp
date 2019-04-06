
#include "PosixClient.h"

#include "twsapi/EPosixClientSocket.h"
#include "twsapi/EPosixClientSocketPlatform.h"
#include "twsapi/Contract.h"
#include "twsapi/Order.h"
#include "twsapi/IBString.h"

#include "globals.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <ctime>
#include <sys/time.h>


#if defined __INTEL_COMPILER
# pragma warning (disable:869)
#elif defined __GNUC__
# pragma GCC diagnostic ignored "-Wswitch"
# pragma GCC diagnostic ignored "-Wunused-parameter"
#endif  /* __INTEL_COMPILER */


const int PING_DEADLINE(2); /// seconds
const int SLEEP_BETWEEN_PINGS(30); /// seconds





PosixClient::PosixClient()
	: m_pClient(new EPosixClientSocket(this))
	, m_state(ST_CONNECT)
	, m_sleepDeadline(0)
{
}

PosixClient::~PosixClient()
{
}





bool PosixClient::connect(const char *host, unsigned int port, int clientId)
{
    std::ofstream flog;
    std::ostringstream LogString;


    std::string Host( *host ? host : "127.0.0.1" );

	LogString << "PosixClient::connect: connecting to " << Host << ":" << port << " clientId:" << clientId << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::connect: can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();

    bool bRes = m_pClient->eConnect( host, port, clientId);

	if (bRes) {
        LogString.str("");
		LogString << "PosixClient::connect: connected to " << Host << ":" << port << " clientId:" << clientId << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PosixClient::connect: can not write into file log.txt\n";
        flog << LogString.str();
        flog.close();
	}
	else {
        LogString.str("");
		LogString << "PosixClient::connect: cannot connect to " << Host << ":" << port << " clientId:" << clientId << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PosixClient::connect: can not write into file log.txt\n";
        flog << LogString.str();
        flog.close();
	}

	return bRes;
}





void PosixClient::disconnect() const
{
    std::ofstream flog;
    std::ostringstream LogString;


	m_pClient->eDisconnect();

	LogString << "PosixClient::disconnect Disconnected\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::disconnect(): can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();
}





bool PosixClient::isConnected() const
{
	return m_pClient->isConnected();
}





void PosixClient::processMessages()
{
	fd_set readSet, writeSet;

	struct timeval tval;
	tval.tv_usec = 0;
	tval.tv_sec = 0;

	time_t now( time(NULL) );

	std::ofstream flog;
	std::ostringstream LogString;



	if( m_sleepDeadline > 0)
    {
		/// initialize timeout with m_sleepDeadline - now

		LogString.str("");
        LogString << "PosixClient::processMessages: m_sleepDeadline\n";
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PosixClient::processMessages(): can not write into file log.txt\n";
        flog << LogString.str();
        flog.close();

		tval.tv_sec = m_sleepDeadline - now;
	}

	if( m_pClient->fd() >= 0 ) {

		FD_ZERO( &readSet);
		writeSet = readSet;

		FD_SET( m_pClient->fd(), &readSet);

		if( !m_pClient->isOutBufferEmpty())
			FD_SET( m_pClient->fd(), &writeSet);

		int ret = select( m_pClient->fd() + 1, &readSet, &writeSet, NULL, &tval);

		if( ret == 0) { /// timeout
			return;
		}

		if( ret < 0)
        {
            /// error
            LogString.str("");
            LogString << "PosixClient::processMessages: disconnect\n";
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "PosixClient::processMessages(): can not write into file log.txt\n";
            flog << LogString.str();
            flog.close();

			disconnect();
			return;
        }

		if( FD_ISSET( m_pClient->fd(), &writeSet))
        {
			/// socket is ready for writing
            /// will call EPosixClientSocket::onSend()

            LogString.str("");
            LogString << "PosixClient::processMessages: onSend\n";
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "PosixClient::processMessages(): can not write into file log.txt\n";
            flog << LogString.str();
            flog.close();

			m_pClient->onSend();
		}

		if( m_pClient->fd() < 0)
			return;

		if( FD_ISSET( m_pClient->fd(), &readSet))
        {
			/// socket is ready for reading
            /// will call EPosixClientSocket::onReceive()

			m_pClient->onReceive();
		}
	}
}




///
/// METHODS ////////////////////////////////////////////////////////////////////
///



void PosixClient::reqCurrentTime()
{
	std::ofstream flog;
	std::ostringstream LogString;


	LogString << "--> Requesting Current Time2\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::reqCurrentTime(): can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();

	/// set ping deadline to "now + n seconds"
	m_sleepDeadline = time( NULL) + PING_DEADLINE;

	m_state = ST_PING_ACK;

	m_pClient->reqCurrentTime();
}



int MSFT_OrdNum=0;

void PosixClient::placeOrder_MSFT()
{
	std::ofstream flog;
	std::ostringstream LogString;

	Contract contract;
	Order order;

	contract.symbol = "MSFT";
	contract.secType = "STK";
	contract.exchange = "SMART";
	contract.currency = "USD";
	order.action = "BUY";
	order.totalQuantity = 1000;
	order.orderType = "LMT";
	order.lmtPrice = 26.7;

	m_state = ST_PLACEORDER_ACK;

	MSFT_OrdNum = OrdId++;

	LogString << "PosixClient: Placing Order " << OrdId << ": " << order.action << " " << order.totalQuantity << " " << contract.symbol << " at " << order.lmtPrice << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::placeOrder_MSFT(): can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();

	m_pClient->placeOrder( OrdId, contract, order);
}



void PosixClient::reqMktData_MSFT()
{
	std::ofstream flog;
	std::ostringstream LogString;

    Contract contract;
	Order order;

	contract.symbol = "MSFT";
	contract.secType = "STK";
	contract.exchange = "SMART";
	contract.currency = "USD";

    IBString i="233";


    TagValueListSPtr mktDataOptions;


	LogString << "PosixClient: Requesting MSFT mktData " << 100 << ": " << order.action << " " << order.totalQuantity << " " << contract.symbol << " at " << order.lmtPrice << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::reqMktData_MSFT(): can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();


    m_pClient->reqMktData( 100, contract, i, false, mktDataOptions);
}



void PosixClient::reqMktData(IBString symbol, IBString secType,
     IBString exchange, IBString currency, int tickerId, IBString genericTicks, IBString localSymbol, bool snapshot)
{
	std::ofstream flog;
	std::ostringstream LogString;

    Contract contract;

	contract.symbol = symbol;
	contract.secType = secType;
	contract.exchange = exchange;//"SMART";//exchange;
	contract.currency = currency;
    contract.localSymbol = localSymbol;

	LogString << "PosixClient: Requesting mktData. symbol: " << contract.symbol << " secType: " << contract.secType << " exchange: " << contract.exchange << " currency: " << contract.currency << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::reqMktData: can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();


	m_state = ST_REQMKTDATA_ACK;


    TagValueListSPtr mktDataOptions;


    m_pClient->reqMktData( tickerId, contract, genericTicks, snapshot, mktDataOptions);
}



void PosixClient::reqMktDepth(TickerId tickerId, Contract contract, int numRows)
{

    TagValueListSPtr mktDepthOptions;



    m_pClient->reqMktDepth(tickerId, contract, numRows, mktDepthOptions);
}



void PosixClient::reqHistData()
{
    Contract contract;

    contract.symbol		= "INTC";
    contract.secType	= "STK";
    contract.expiry		= "";
    contract.strike		= 0;
    contract.right		= "";
    contract.exchange	= "SMART";
    contract.currency	= "USD";
    contract.primaryExchange = "NASDAQ";


    TagValueListSPtr chartOptions;

    m_pClient->reqHistoricalData(1, contract,  "", "3 D", "1 day", "TRADES", 1, 1, chartOptions);

}





void PosixClient::cancelOrder()
{
	std::ofstream flog;
	std::ostringstream LogString;


	LogString << "Cancelling Order " << MSFT_OrdNum << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::cancelOrder(): can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();

	m_state = ST_CANCELORDER_ACK;

	m_pClient->cancelOrder(MSFT_OrdNum);
}





void PosixClient::cancelMktData(TickerId tickerId)
{
	std::ofstream flog;
	std::ostringstream LogString;

    LogString << "[PosixClient::cancelMktData] for tickerId: " << tickerId << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::cancelMktData: can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();

    m_pClient->cancelMktData(tickerId);
}





///
/// EVENTS ///////////////////////////////////////////////////////////////////
///



void PosixClient::orderStatus( OrderId orderId, const IBString &status, int filled,
	   int remaining, double avgFillPrice, int permId, int parentId,
	   double lastFillPrice, int clientId, const IBString& whyHeld)
{
std::ofstream flog;
std::ostringstream LogString;

time_t CurTime=0;
struct tm * timeinfo;



if (status == "Cancelled")
{
    CurTime=time(NULL);
    timeinfo = localtime(&CurTime);

    LogString << "Order: id=" << orderId << ", status=" << status << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::orderStatus: can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();

    if (CancelAllOrdPermit)
    {
        CancelAllOrdPermit=false;
        CancelAllOrders();

        if (timeinfo->tm_hour<23)
        {
            PriceDeltaTrig = PriceDeltaTrig2;
            OrdBskExists = false;
            PlaceBsk(2);
        }
    }
}

}





void PosixClient::nextValidId( OrderId orderId)
{

std::ofstream flog;
std::ostringstream LogString;

	OrdId = orderId;

	m_state = ST_PLACEORDER;


    LogString << "Event nextValidId: OrdId=" << OrdId << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::nextValidId: can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();
}





void PosixClient::currentTime( long time)
{
	if ( m_state == ST_PING_ACK) {
		time_t t = ( time_t)time;
		struct tm * timeinfo = localtime ( &t);
		printf( "The current date/time is: %s", asctime( timeinfo));

		time_t now = ::time(NULL);
		m_sleepDeadline = now + SLEEP_BETWEEN_PINGS;

		m_state = ST_IDLE;
	}
}





void PosixClient::error(const int id, const int errorCode, const IBString errorString)
{
    std::ofstream flog, fList005;
    std::ostringstream LogString;

	LogString << "Error id=" << id << ", errorCode=" << errorCode << ", msg=" << errorString << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::error: can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();




    ///"The price does not conform to the minimum price variation for this contract."
    if (errorCode==110)
        for (int i=0; i<OrdBskLinesNum; i++)
            if(OrdBsk[i].OrdId==id)
            {
                fList005.open("List005.txt", std::ios::out | std::ios::app);
                if (!fList005.is_open())
                {
                    LogString.str("");
                    LogString << "PosixClient::error: can not write into file List005.txt\n";
                    std::cout << LogString.str();
                    flog.open("log.txt", std::ios::out | std::ios::app);
                    if (!flog.is_open()) std::cout << "PosixClient::error: can not write into file log.txt\n";
                    flog << LogString.str();
                    flog.close();
                }

                fList005 << OrdBsk[i].symbol << std::endl;
                fList005.close();
            }

}





void PosixClient::tickPrice( TickerId tickerId, TickType field, double price, int canAutoExecute)
{
std::ofstream fEntryRejected, flog;
std::ostringstream LogString, LocalTime("00:00:00");

Contract contract;
Order order;
TagValueListSPtr mktDataOptions;

///contract.symbol = OrdBsk[tickerId].symbol; ///this parameter will be set later
contract.secType = "STK";
contract.currency = "USD";
contract.exchange = "SMART";
contract.primaryExchange = "NASDAQ";

order.action = "BUY";
///order.totalQuantity = OrdBsk[tickerId].shares; ///this parameter will be set later
order.orderType = "LMT";
///order.lmtPrice = OrdBsk[tickerId].EntryPrice; ///this parameter will be set later
order.outsideRth = true;
order.transmit = true;

time_t CurTime=0;
struct tm * timeinfo;
int CurTime_min(0);


CurTime=time(NULL);
timeinfo = localtime(&CurTime);
CurTime_min = timeinfo->tm_hour*60+timeinfo->tm_min;
LocalTime << timeinfo->tm_hour << ":" << timeinfo->tm_min << ":" << timeinfo->tm_sec;


    if ( field==LAST && price>0.01 && OrdBskPlaced[tickerId].OrdId==0 )
    {
        if ( price<OrdBsk[tickerId].EntryPrice ) /// && CurTime_min<MorningTimeLine )
        {
            m_pClient->cancelMktData(tickerId);
            MktDataOpenedNum--;

            fEntryRejected.open("EntryRejected.txt", std::ios::out | std::ios::app);
            if (!fEntryRejected.is_open())
            {
                LogString << "PosixClient::tickPrice: can not write into file EntryRejected.txt\n";
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "PosixClient::tickPrice: can not write into file log.txt\n";
                flog << LogString.str();
                flog.close();
            }
            fEntryRejected.setf(std::ios::fixed);
            fEntryRejected.precision(2);
            fEntryRejected << "tickerId " << tickerId << "  OrdId " << OrdBsk[tickerId].OrdId << "  symbol " << OrdBsk[tickerId].symbol << "  EntryPrice " << OrdBsk[tickerId].EntryPrice << "  price " << price << std::endl;
            fEntryRejected.close();

            LogString.str("");
            LogString << "ENTRY REJECTED, price<EntyPrice: tickerId " << tickerId << "  OrdId " << OrdBsk[tickerId].OrdId << "  symbol " << OrdBsk[tickerId].symbol << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "PosixClient::tickPrice: can not write into file log.txt\n";
            flog << LogString.str();
            flog.close();


            OrdBsk[tickerId].OrdId=0;
            OrdBsk[tickerId].symbol[0]='0'; OrdBsk[tickerId].symbol[1]='\0';
            OrdBsk[tickerId].shares=0;
            OrdBsk[tickerId].EntryPrice=0;
            OrdBsk[tickerId].price=0;
            OrdBsk[tickerId].PriceDelta=0;
            OrdBsk[tickerId].time=2000000000;
        }

        else
        {

            OrdBsk[tickerId].time=CurTime;
            strcpy(OrdBsk[tickerId].LocalTime, LocalTime.str().c_str());
            OrdBsk[tickerId].price=price;
            OrdBsk[tickerId].PriceDelta = price-OrdBsk[tickerId].EntryPrice;



            if( PlaceAllOrd || (!PlaceAllOrd && OrdBsk[tickerId].PriceDelta<PriceDeltaTrig) )
            {
                OrdBskPlaced[tickerId].OrdId = OrdBsk[tickerId].OrdId;
                strcpy(OrdBskPlaced[tickerId].symbol, OrdBsk[tickerId].symbol);
                OrdBskPlaced[tickerId].shares = OrdBsk[tickerId].shares;
                OrdBskPlaced[tickerId].EntryPrice = OrdBsk[tickerId].EntryPrice;


                contract.symbol = OrdBsk[tickerId].symbol;
                order.totalQuantity = OrdBsk[tickerId].shares;
                order.lmtPrice = OrdBsk[tickerId].EntryPrice;

                m_pClient->cancelMktData(tickerId);
                MktDataOpenedNum--;
                OrdBsk[tickerId].time=2000000000;

                ///this is modification of already existing order, so OrdId increment not performed
                client.m_pClient->placeOrder(OrdBsk[tickerId].OrdId, contract, order);
            }
        }
    }


}





void PosixClient::tickSize( TickerId tickerId, TickType field, int size)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::tickSize\n";
    flog.close();
    std::cout << "event: PosixClient::tickSize\n";
    #endif

}





void PosixClient::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
                                         double optPrice, double pvDividend,
                                         double gamma, double vega, double theta, double undPrice)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::tickOptionComputation\n";
    flog.close();
    std::cout << "event: PosixClient::tickOptionComputation\n";
    #endif
}





void PosixClient::tickGeneric(TickerId tickerId, TickType tickType, double value)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::tickGeneric\n";
    flog.close();
    std::cout << "event: PosixClient::tickGeneric\n";
    #endif
}





void PosixClient::tickString(TickerId tickerId, TickType field, const IBString& value)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::tickString\n";
    flog.close();
    std::cout << "event: PosixClient::tickString\n";
    #endif

}





void PosixClient::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
                          double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact,
                          double dividendsToExpiry)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::tickEFP\n";
    flog.close();
    std::cout << "event: PosixClient::tickEFP\n";
    #endif
}





void PosixClient::openOrder( OrderId orderId, const Contract&, const Order&, const OrderState& ostate)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::openOrder\n";
    flog.close();
    std::cout << "event: PosixClient::openOrder\n";
    #endif
}





void PosixClient::openOrderEnd()
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::openOrderEnd\n";
    flog.close();
    std::cout << "event: PosixClient::openOrderEnd\n";
    #endif
}





void PosixClient::winError( const IBString &str, int lastError)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::winError\n";
    flog.close();
    std::cout << "event: PosixClient::winError\n";
    #endif
}





void PosixClient::connectionClosed()
{
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::connectionClosed\n";
    flog.close();
    std::cout << "event: PosixClient::connectionClosed\n";
}





void PosixClient::updateAccountValue(const IBString& key, const IBString& val,
                                     const IBString& currency, const IBString& accountName)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::updateAccountValue\n";
    flog.close();
    std::cout << "event: PosixClient::updateAccountValue\n";
    #endif
}





void PosixClient::updatePortfolio(const Contract& contract, int position,
		                          double marketPrice, double marketValue, double averageCost,
		                          double unrealizedPNL, double realizedPNL, const IBString& accountName)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::updatePortfolio\n";
    flog.close();
    std::cout << "event: PosixClient::updatePortfolio\n";
    #endif
}





void PosixClient::updateAccountTime(const IBString& timeStamp)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::updateAccountTime\n";
    flog.close();
    std::cout << "event: PosixClient::updateAccountTime\n";
    #endif
}





void PosixClient::accountDownloadEnd(const IBString& accountName)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::accountDownloadEnd\n";
    flog.close();
    std::cout << "event: PosixClient::accountDownloadEnd\n";
    #endif
}





void PosixClient::contractDetails( int reqId, const ContractDetails& contractDetails)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::contractDetails\n";
    flog.close();
    std::cout << "event: PosixClient::contractDetails\n";
    #endif
}





void PosixClient::bondContractDetails( int reqId, const ContractDetails& contractDetails)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::bondContractDetails\n";
    flog.close();
    std::cout << "event: PosixClient::bondContractDetails\n";
    #endif
}





void PosixClient::contractDetailsEnd( int reqId)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::contractDetailsEnd\n";
    flog.close();
    std::cout << "event: PosixClient::contractDetailsEnd\n";
    #endif
}





void PosixClient::execDetails( int reqId, const Contract& contract, const Execution& execution)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::execDetails\n";
    flog.close();
    std::cout << "event: PosixClient::execDetails\n";
    #endif
}





void PosixClient::execDetailsEnd( int reqId)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::execDetailsEnd\n";
    flog.close();
    std::cout << "event: PosixClient::execDetailsEnd\n";
    #endif
}





void PosixClient::updateMktDepth(TickerId id, int position, int operation, int side,
									  double price, int size)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::updateMktDepth\n";
    flog.close();
    std::cout << "event: PosixClient::updateMktDepth\n";
    #endif

}





void PosixClient::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
                                   int side, double price, int size)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::updateMktDepthL2\n";
    flog.close();
    std::cout << "event: PosixClient::updateMktDepthL2\n";
    #endif

}





void PosixClient::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::updateNewsBulletin\n";
    flog.close();
    std::cout << "event: PosixClient::updateNewsBulletin\n";
    #endif
}





void PosixClient::managedAccounts( const IBString& accountsList)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::managedAccounts\n";
    flog.close();
    std::cout << "event: PosixClient::managedAccounts\n";
    #endif
}





void PosixClient::receiveFA(faDataType pFaDataType, const IBString& cxml)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::receiveFA\n";
    flog.close();
    std::cout << "event: PosixClient::receiveFA\n";
    #endif
}





void PosixClient::historicalData(TickerId reqId, const IBString& date, double open, double high,
                                 double low, double close, int volume, int barCount, double WAP, int hasGaps)
{

static int LastDate(0);

static std::ifstream fpr;
static std::ofstream fpw;

std::ofstream flog, ferr, frem;

std::ostringstream LogString;

static DOHLCV LastRecord;
DOHLCV CurRecord;
static ISDOHLCV CurRecord2;

static int prevId(-1);///real reqIds are starting from 0

static std::string filename;

Contract contract;

double AdmitKoeff(0.01); /// 1%
double CorrKoeff(1);

bool SharedDateExists(false);

int FileSize(0);
int BarsGot(0);

int j(0);




if (reqId!=prevId) /// first function call for a new symbol
{


    /// If .bin file exists and has dates before date, this dates will be included into new .bin

    if (BinFile)
    {
        filename = "fbin/"; filename += Smb[reqId]; filename += ".bin";
        fpr.open(filename.c_str(), std::ios::in | std::ios::binary);
        if (!fpr.is_open())
        {
            LogString.str("");
            LogString << "ERROR: attempt to open file fbin/" << Smb[reqId] << ".bin which does not exist" << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
            flog << LogString.str();
            flog.close();
            return;
        }

        LogString.str("");
        LogString << "supposed that old .bin file exists for symbol " << Smb[reqId] << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
        flog << LogString.str();
        flog.close();

        fpr.seekg(0, std::ios::end);
        FileSize = fpr.tellg();
        BarsGot = FileSize/sizeof(DOHLCV);
        fpr.clear();
        fpr.seekg(0, std::ios::beg);
        fpr.read( (char*)d, BarsGot*sizeof(DOHLCV) );

        fpr.close();
    }
    else
    {
        LogString.str("");
        LogString << "supposed that .bin file does not exist for symbol " << Smb[reqId] << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
        flog << LogString.str();
        flog.close();

    }


	prevId=reqId;
	if(fpw.is_open()) fpw.close();
    filename = "fbin/"; filename += Smb[reqId]; filename += ".bin";
	fpw.open(filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!fpw.is_open())
    {
        LogString.str("");
        LogString << "PosixClient::historicalData: ERROR: " <<  Smb[reqId] << " Impossible to write into .bin file" << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
        flog << LogString.str();
        flog.close();
        return;
    }



	if (!atoi(date.data())) ///the line, which signals about the end of data for current symbol, has been received
	{
        if(fpw.is_open()) fpw.close();

		if (LastDate<atoi(AddDate))
            {
                LogString.str("");
                LogString << Smb[reqId] << " Some last days absent; LastDate " << LastDate << " AddDate " << atoi(AddDate) << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                flog << LogString.str();
                flog.close();


                if (LastDate<DateNotTradingFrom)
                    {
                        LogString.str("");
                        LogString << "Symbol " << Smb[reqId] << " not trading from LastDate " << LastDate << "< " << DateNotTradingFrom << std::endl;
                        std::cout << LogString.str();
                        flog.open("log.txt", std::ios::out | std::ios::app);
                        if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                        flog << LogString.str();
                        flog.close();

                        ferr.open("err.txt", std::ios::out | std::ios::app);
                        if (!ferr.is_open())
                        {
                            std::cout << "PosixClient::historicalData: can not write into file err.txt\n";
                            flog.open("log.txt", std::ios::out | std::ios::app);
                            if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                            flog << "PosixClient::historicalData: can not write into file err.txt\n";
                            flog.close();
                        }
                        ferr << LogString.str();
                        ferr.close();

                        frem.open("SymsToRem.txt", std::ios::out | std::ios::app);
                        if (!frem.is_open())
                        {
                            std::cout << "PosixClient::historicalData: can not write into file SymsToRem.txt\n";
                            flog.open("log.txt", std::ios::out | std::ios::app);
                            if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                            flog << "PosixClient::historicalData: can not write into file SymsToRem.txt\n";
                            flog.close();
                        }
                        frem << Smb[reqId] << std::endl;
                        frem.close();
                    }
            }

        LogString.str("");
        LogString << CurRecord2.Id << "  " << CurRecord2.Smb << "  " << CurRecord2.Date << "  " << CurRecord2.Open << "  " << CurRecord2.High << "  " << CurRecord2.Low << "  " << CurRecord2.Close << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
        flog << LogString.str();
        flog.close();


        FillAbscentDates(CurRecord2.Smb, smbRef);

		return;
	}


	if( atoi(date.data())<=atoi(AddDate) )
	{




        if (BinFile)
        /// Including lines from old .bin file into new .bin
        {
            LogString.str("");
            LogString << "BinFile==true for symbol " << Smb[reqId] << "; search of SharedDate started; BarsGot==" << BarsGot << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
            flog << LogString.str();
            flog.close();

            for(j=0; j<BarsGot; j++) if( d[j].Date==atoi(date.data()) ) {SharedDateExists=true; break;}

            LogString.str("");
            LogString << "SharedDateExists for symbol " << Smb[reqId] << " == " << SharedDateExists << "; Shared date: " << atoi(date.data()) << std::endl;
            std::cout << LogString.str();
            flog.open("log.txt", std::ios::out | std::ios::app);
            if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
            flog << LogString.str();
            flog.close();


            if (SharedDateExists)
            {

                if ( fabs(close-d[j].Close)>AdmitKoeff*close )
                {
                    CorrKoeff = d[j].Close / close;

                    LogString.str("");
                    LogString << "SPLIT for symbol " << Smb[reqId] << "; CorrKoeff=" << CorrKoeff << std::endl;
                    std::cout << LogString.str();

                    flog.open("log.txt", std::ios::out | std::ios::app);
                    if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                    flog << LogString.str();
                    flog.close();

                    ferr.open("err.txt", std::ios::out | std::ios::app);
                    if (!ferr.is_open()) std::cout << "PosixClient::historicalData: can not write into file err.txt\n";
                    ferr << LogString.str();
                    ferr.close();


                    for(int i=0; i<BarsGot; i++)
                    {
                        if( d[i].Date==atoi(date.data()) ) break;

                        CurRecord.Date=d[i].Date;
                        CurRecord.Open=d[i].Open / CorrKoeff;
                        CurRecord.High=d[i].High / CorrKoeff;
                        CurRecord.Low=d[i].Low / CorrKoeff;
                        CurRecord.Close=d[i].Close / CorrKoeff;
                        CurRecord.Volume = int(d[i].Volume * CorrKoeff);

                        if (!fpw.is_open())
                            {
                                LogString.str("");
                                LogString << "PosixClient::historicalData: ERROR " << Smb[reqId] << " fpw is not open" << std::endl;
                                std::cout << LogString.str();
                                flog.open("log.txt", std::ios::out | std::ios::app);
                                if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                                flog << LogString.str();
                                flog.close();
                                return;
                            }

                        fpw.write( (char*)&CurRecord, sizeof(DOHLCV) );
                    }
                }

                else
                {

                    for(int i=0; i<BarsGot; i++)
                    {
                        if( d[i].Date==atoi(date.data()) ) break;

                        CurRecord.Date=d[i].Date;
                        CurRecord.Open=d[i].Open;
                        CurRecord.High=d[i].High;
                        CurRecord.Low=d[i].Low;
                        CurRecord.Close=d[i].Close;
                        CurRecord.Volume=d[i].Volume;

                        if (!fpw.is_open())
                            {
                                LogString.str("");
                                LogString << "PosixClient::historicalData: ERROR " << Smb[reqId] << " fpw is not open" << std::endl;
                                std::cout << LogString.str();
                                flog.open("log.txt", std::ios::out | std::ios::app);
                                if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                                flog << LogString.str();
                                flog.close();
                                return;
                            }

                        fpw.write( (char*)&CurRecord, sizeof(DOHLCV) );
                    }
                }

            }
        }





		CurRecord.Date=atoi(date.data());
		CurRecord.Open=open;
		CurRecord.High=high;
		CurRecord.Low=low;
		CurRecord.Close=close;
		CurRecord.Volume=volume*100;


        if (!fpw.is_open())
            {
                LogString.str("");
                LogString << "PosixClient::historicalData: ERROR " << Smb[reqId] << " fpw is not open" << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                flog << LogString.str();
                flog.close();
                return;
            }

        fpw.write( (char*)&CurRecord, sizeof(DOHLCV) );
	}


	LogString.str("");
    LogString << reqId << "  " << Smb[reqId] << "  " << atoi(date.data()) << "  " << open << "  " << high << "  " << low << "  " << close << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
    flog << LogString.str();
    flog.close();
}


else  /// continuing receiving data for the same symbol
{
	if (!atoi(date.data())) ///the line, which signals about the end of data for current symbol, has been received
	{
		if(fpw.is_open()) fpw.close();

		if (LastDate<atoi(AddDate))
            {
                LogString.str("");
                LogString << Smb[reqId] << " Some last days absent; LastDate " << LastDate << "  AddDate " << atoi(AddDate) << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                flog << LogString.str();
                flog.close();

                if (LastDate<DateNotTradingFrom)
                    {
                        LogString.str("");
                        LogString << "Symbol " << Smb[reqId] << " not trading from LastDate " << LastDate << "< " << DateNotTradingFrom << std::endl;
                        std::cout << LogString.str();
                        flog.open("log.txt", std::ios::out | std::ios::app);
                        if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                        flog << LogString.str();
                        flog.close();

                        ferr.open("err.txt", std::ios::out | std::ios::app);
                        if (!ferr.is_open())
                        {
                            std::cout << "PosixClient::historicalData: can not write into file err.txt\n";
                            flog.open("log.txt", std::ios::out | std::ios::app);
                            if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                            flog << "PosixClient::historicalData: can not write into file err.txt\n";
                            flog.close();
                        }
                        ferr << LogString.str();
                        ferr.close();

                        frem.open("SymsToRem.txt", std::ios::out | std::ios::app);
                        if (!frem.is_open())
                        {
                            std::cout << "PosixClient::historicalData: can not write into file SymsToRem.txt\n";
                            flog.open("log.txt", std::ios::out | std::ios::app);
                            if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                            flog << "PosixClient::historicalData: can not write into file SymsToRem.txt\n";
                            flog.close();
                        }
                        frem << Smb[reqId] << std::endl;
                        frem.close();
                    }
            }

        LogString.str("");
        LogString << CurRecord2.Id << "  " << CurRecord2.Smb << "  " << CurRecord2.Date << "  " << CurRecord2.Open << "  " << CurRecord2.High << "  " << CurRecord2.Low << "  " << CurRecord2.Close << std::endl;
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
        flog << LogString.str();
        flog.close();

        FillAbscentDates(CurRecord2.Smb, smbRef);

		return;
	}



	if( atoi(date.data()) && atoi(date.data())<=atoi(AddDate) )
	{
		CurRecord.Date=atoi(date.data());
		CurRecord.Open=open;
		CurRecord.High=high;
		CurRecord.Low=low;
		CurRecord.Close=close;
		CurRecord.Volume=volume*100;

        if (!fpw.is_open())
            {
                LogString.str("");
                LogString << "PosixClient::historicalData: ERROR " << Smb[reqId] << " fpw is not open" << std::endl;
                std::cout << LogString.str();
                flog.open("log.txt", std::ios::out | std::ios::app);
                if (!flog.is_open()) std::cout << "PosixClient::historicalData: can not write into file log.txt\n";
                flog << LogString.str();
                flog.close();
                return;
            }

        fpw.write( (char*)&CurRecord, sizeof(DOHLCV) );
	}


    CurRecord2.Id=reqId;
    strcpy(CurRecord2.Smb, Smb[reqId]);
    CurRecord2.Date=atoi(date.data());
    CurRecord2.Open=open;
    CurRecord2.High=high;
    CurRecord2.Low=low;
    CurRecord2.Close=close;
}


LastDate = atoi(date.data());



}





void PosixClient::scannerParameters(const IBString &xml)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::scannerParameters\n";
    flog.close();
    std::cout << "event: PosixClient::scannerParameters\n";
    #endif
}





void PosixClient::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
	                          const IBString &distance, const IBString &benchmark, const IBString &projection,
	                          const IBString &legsStr)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::scannerData\n";
    flog.close();
    std::cout << "event: PosixClient::scannerData\n";
    #endif
}





void PosixClient::scannerDataEnd(int reqId)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::scannerDataEnd\n";
    flog.close();
    std::cout << "event: PosixClient::scannerDataEnd\n";
    #endif
}





void PosixClient::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
                              long volume, double wap, int count)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::realtimeBar\n";
    flog.close();
    std::cout << "event: PosixClient::realtimeBar\n";
    #endif
}





void PosixClient::fundamentalData(TickerId reqId, const IBString& data)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::fundamentalData\n";
    flog.close();
    std::cout << "event: PosixClient::fundamentalData\n";
    #endif
}





void PosixClient::deltaNeutralValidation(int reqId, const UnderComp& underComp)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog <<"event: PosixClient::deltaNeutralValidation\n";
    flog.close();
    std::cout << "event: PosixClient::deltaNeutralValidation\n";
    #endif
}





void PosixClient::tickSnapshotEnd(int reqId)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::tickSnapshotEnd\n";
    flog.close();
    std::cout << "event: PosixClient::tickSnapshotEnd\n";
    #endif
}





void PosixClient::marketDataType(TickerId reqId, int marketDataType)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::marketDataType\n";
    flog.close();
    std::cout << "event: PosixClient::marketDataType\n";
    #endif
}





void PosixClient::commissionReport( const CommissionReport &commissionReport)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::commissionReport\n";
    flog.close();
    std::cout << "event: PosixClient::commissionReport\n";
    #endif
}





void PosixClient::position( const IBString& account, const Contract& contract, int position, double avgCost)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::position\n";
    flog.close();
    std::cout << "event: PosixClient::position\n";
    #endif
}





void PosixClient::positionEnd()
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::positionEnd\n";
    flog.close();
    std::cout << "event: PosixClient::positionEnd\n";
    #endif
}





void PosixClient::accountSummary( int reqId, const IBString& account, const IBString& tag, const IBString& value, const IBString& curency)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::accountSummary\n";
    flog.close();
    std::cout << "event: PosixClient::accountSummary\n";
    #endif
}





void PosixClient::accountSummaryEnd( int reqId)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::accountSummaryEnd\n";
    flog.close();
    std::cout << "event: PosixClient::accountSummaryEnd\n";
    #endif
}





void PosixClient::verifyMessageAPI( const IBString& apiData)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::verifyMessageAPI\n";
    flog.close();
    std::cout << "event: PosixClient::verifyMessageAPI\n";
    #endif
}





void PosixClient::verifyCompleted( bool isSuccessful, const IBString& errorText)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::verifyCompleted\n";
    flog.close();
    std::cout << "event: PosixClient::verifyCompleted\n";
    #endif
}





void PosixClient::displayGroupList( int reqId, const IBString& groups)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::displayGroupList\n";
    flog.close();
    std::cout << "event: PosixClient::displayGroupList\n";
    #endif
}





void PosixClient::displayGroupUpdated( int reqId, const IBString& contractInfo)
{
    #ifdef DEBUG
    std::ofstream flog("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "Can not write into file log.txt\n";
    flog << "event: PosixClient::displayGroupUpdated\n";
    flog.close();
    std::cout << "event: PosixClient::displayGroupUpdated\n";
    #endif
}

