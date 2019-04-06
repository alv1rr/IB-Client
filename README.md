# IB-Client
Interactive Brokers POSIX C++ client which can collect market data also trade and optimize some sample method.
The project has been built in Code Blocks IDE for linux.
It has not been tested in Windows. To run it in Windows some minor changes may be necessary.


USAGE

- Run Trader Workstation or IB Gateway.

- Press "Connect" then "CheckBin" then "HistDataNew".
Market data collecting from IB will start. Binary files with market data will appear in folder "fbin".
Collect necessary market data.

- Press "BINtoASCII".
Market data files will be converted to ASCII format and saved in folder "fascii".

- To run a back test of some sample method press "RunMethod".
