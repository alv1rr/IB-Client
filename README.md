# IB-Client
Interactive Brokers POSIX C++ client collecting market data also trading and optimizing some sample method.
The project build in Code Blocks IDE for linux.
It has not been tested in Windows. To run it in Windows some minor changes may be necessary.


USAGE

- Run Trader Workstation or IB Gateway.

- Press "Connect"; then press "CheckBin"; then press "HistDataNew".
Market data collecting from IB will start. Binary files with market data will appear in folder "fbin".
Collect necessary market data.

- Press "BINtoASCII".
Market data files will be converted to ASCII format and saved in folder "fascii".

- To run some sample method press "RunMethod".
