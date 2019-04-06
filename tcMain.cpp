/***************************************************************
 * Name:      tcMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    alv ()
 * Created:   2017-01-21
 * Copyright: alv ()
 * License:
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include "tcMain.h"

#include "globals.h"

#include <fstream>
#include <sstream>


//helper functions
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__WXMAC__)
        wxbuild << _T("-Mac");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}





BEGIN_EVENT_TABLE(tcDialog, wxDialog)
    EVT_CLOSE(tcDialog::OnClose)
    EVT_BUTTON(idBtn_Connect, tcDialog::OnBtn_Connect)
    EVT_BUTTON(idBtn_Disconnect, tcDialog::OnBtn_Disconnect)
    EVT_BUTTON(idBtn_CheckBin, tcDialog::OnBtn_CheckBin)
    EVT_BUTTON(idBtn_RemBadSyms, tcDialog::OnBtn_RemBadSyms)
    EVT_BUTTON(idBtn_HistData, tcDialog::OnBtn_HistData)
    EVT_BUTTON(idBtn_HistDatNew, tcDialog::OnBtn_HistDatNew)
    EVT_BUTTON(idBtn_BINtoASCII, tcDialog::OnBtn_BINtoASCII)
    EVT_BUTTON(idBtn_ASCIItoBIN, tcDialog::OnBtn_ASCIItoBIN)
    EVT_BUTTON(idBtn_UpdLastDay, tcDialog::OnBtn_UpdLastDay)
    EVT_BUTTON(idBtn_MethodButton, tcDialog::OnBtn_MethodButton)
    EVT_BUTTON(idBtn_Optimize, tcDialog::OnBtn_Optimize)
    EVT_BUTTON(idBtn_WriteBsk, tcDialog::OnBtn_WriteBsk)
    EVT_BUTTON(idBtn_PlaceBsk, tcDialog::OnBtn_PlaceBsk)
END_EVENT_TABLE()





tcDialog::tcDialog(wxDialog *dlg, const wxString &title)
    : wxDialog(dlg, -1, title)
{
    std::ofstream flog;
    std::ifstream fOrdBsk, fOrdBskPlaced;

    std::ostringstream LogString;

    time_t rawtime;
    struct tm * timeinfo;

    int OrdBskLinesCount(0);
    char line[200];




    this->SetSizeHints(wxDefaultSize, wxDefaultSize);
    wxBoxSizer* bSizer1;
    bSizer1 = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer* bSizer2;
    bSizer2 = new wxBoxSizer(wxVERTICAL);

    Btn_Connect = new wxButton(this, idBtn_Connect, wxT("Connect"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_Connect, 0, wxALL, 5);

    Btn_Disconnect = new wxButton(this, idBtn_Disconnect, wxT("Disconnect"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_Disconnect, 0, wxALL, 5);

    Btn_CheckBin = new wxButton(this, idBtn_CheckBin, wxT("CheckBin"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_CheckBin, 0, wxALL, 5);

    Btn_HistDatNew = new wxButton(this, idBtn_HistDatNew, wxT("HistDatNew"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_HistDatNew, 0, wxALL, 5);

    Btn_HistData = new wxButton(this, idBtn_HistData, wxT("HistData"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_HistData, 0, wxALL, 5);

    Btn_UpdLastDay = new wxButton(this, idBtn_UpdLastDay, wxT("UpdLastDay"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_UpdLastDay, 0, wxALL, 5);

    Btn_BINtoASCII = new wxButton(this, idBtn_BINtoASCII, wxT("BINtoASCII"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_BINtoASCII, 0, wxALL, 5);

    Btn_ASCIItoBIN = new wxButton(this, idBtn_ASCIItoBIN, wxT("ASCIItoBIN"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_ASCIItoBIN, 0, wxALL, 5);

    Btn_RemBadSyms = new wxButton(this, idBtn_RemBadSyms, wxT("RemBadSyms"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_RemBadSyms, 0, wxALL, 5);

    Btn_MethodButton = new wxButton(this, idBtn_MethodButton, wxT("RunMethod"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_MethodButton, 0, wxALL, 5);

    Btn_Optimize = new wxButton(this, idBtn_Optimize, wxT("Optimize"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_Optimize, 0, wxALL, 5);

    Btn_WriteBsk = new wxButton(this, idBtn_WriteBsk, wxT("WriteBsk"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_WriteBsk, 0, wxALL, 5);

    Btn_PlaceBsk = new wxButton(this, idBtn_PlaceBsk, wxT("PlaceBsk"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(Btn_PlaceBsk, 0, wxALL, 5);

    bSizer1->Add(bSizer2, 1, wxEXPAND, 5);
    this->SetSizer(bSizer1);
    this->Layout();
    bSizer1->Fit(this);



    time( &rawtime );
    timeinfo = localtime( &rawtime );

    LogString << "Program tc started " << asctime(timeinfo) << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "tcDialog: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();


    fOrdBsk.open("OrdBsk.txt");
    if ( fOrdBsk.is_open() )
    {
        OrdBskExists=true;

        OrdBskLinesCount=0;
        while ( fOrdBsk.getline(line,200) )
        {
            sscanf(line, "%i %s %i %lf", &OrdBsk[OrdBskLinesCount].OrdId, OrdBsk[OrdBskLinesCount].symbol, &OrdBsk[OrdBskLinesCount].shares, &OrdBsk[OrdBskLinesCount].EntryPrice);
            OrdBskLinesCount++;
        }
        OrdBskLinesNum=OrdBskLinesCount;
        fOrdBsk.close();
    }


    fOrdBskPlaced.open("OrdBskPlaced.txt");
    if ( fOrdBskPlaced.is_open() )
    {
        OrdBskPlacedExists=true;

        OrdBskLinesCount=0;
        while ( fOrdBskPlaced.getline(line,200) )
        {
            sscanf(line, "%i %s %i %lf", &OrdBskPlaced[OrdBskLinesCount].OrdId, OrdBskPlaced[OrdBskLinesCount].symbol, &OrdBskPlaced[OrdBskLinesCount].shares, &OrdBskPlaced[OrdBskLinesCount].EntryPrice);
            OrdBskLinesCount++;
        }
        fOrdBskPlaced.close();
    }


    LogString.str("");
    LogString << "PlaceAllOrd " << PlaceAllOrd << std::endl << "OrdBskExists " << OrdBskExists << std::endl << "OrdBskPlacedExists " << OrdBskPlacedExists << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "tcDialog: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

}





tcDialog::~tcDialog()
{
}





void tcDialog::OnClose(wxCloseEvent &event)
{
    std::ofstream flog;
    std::ostringstream LogString;

    time_t rawtime;
    struct tm * timeinfo;



    if (client.isConnected())
    {
        endProcessMessages_gl();
        client.disconnect();
    }


    time( &rawtime );
    timeinfo = localtime( &rawtime );

    LogString << "\nProgram tc terminated " << asctime(timeinfo) << std::endl;
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "tcDialog: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();



    Destroy();
}





void tcDialog::OnBtn_Connect(wxCommandEvent &event)
{
    if (!client.isConnected())
    {
        client.connect("", 7496, 41);
        if (client.isConnected()) {
             processMessages_gl();
        }
    }
}





void tcDialog::OnBtn_Disconnect(wxCommandEvent &event)
{
    if (client.isConnected())
    {
        endProcessMessages_gl();
        client.disconnect();
    }

}





void tcDialog::OnBtn_CheckBin(wxCommandEvent &event)
/// This function scans all.txt file and fbin folder;
/// files: BinAbsent.txt and BinPresent.txt are created,
/// reflecting tickers having or not having .bin-files with marker data
{
    std::ifstream fall, fbin;
    std::ofstream fBinPresent, fBinAbsent, flog;

    std::ostringstream LogString;

    std::string FileName;

    char line[200], symbol[20];

    DOHLCV LastRecord;



    LogString << "CheckBin started\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();


fall.open("all.txt");
if(!fall.is_open())
{
    LogString.str("");
    LogString << "CheckBin: файл all.txt not found\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();
    return;
}

fBinPresent.open("BinPresent.txt");
if(!fBinPresent.is_open())
{
    LogString.str("");
    LogString << "CheckBin: impossible to create file BinPresent.txt for writing\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();
    return;
}

fBinAbsent.open("BinAbsent.txt");
if(!fBinAbsent.is_open())
{
    LogString.str("");
    LogString << "CheckBin: impossible to create file BinAbsent.txt for writing\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();
    return;
}


    while ( fall.getline(line,200) )
    {
        sscanf(line,"%s", symbol);
        FileName = "fbin/"; FileName += symbol; FileName += ".bin";

        fbin.open( FileName.c_str(), std::ios::in | std::ios::binary );
        if ( !fbin.is_open() ) fBinAbsent << symbol << std::endl;
        else
        {
            fbin.seekg(-sizeof(DOHLCV), std::ios::end);

            fbin.read( (char*)&LastRecord,sizeof(DOHLCV) );

            if ( LastRecord.Date!=atoi(AddDate) ) fBinPresent << symbol << std::endl;

            fbin.close();
        }

    }


    fall.close();
    fBinPresent.close();
    fBinAbsent.close();

    LogString.str("");
    LogString << "CheckBin completed\nFiles BinAbsent.txt and BinPresent.txt created\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();

}





void tcDialog::OnBtn_RemBadSyms(wxCommandEvent &event)
/// This function scans all.txt file
/// and removes from it tickers that are printed in file SymsToRem.txt
{
std::ifstream frem, fall;
std::ofstream fall_tmp, flog;

std::ostringstream LogString;

char line0[200];

int k(0);

int fremLinesNum(0);
all2Line *fremArray;

char Symbol[20];

bool RemSymbol(false);



LogString << "Removing bad symbols...\n";
std::cout << LogString.str();
flog.open("log.txt", std::ios::out | std::ios::app);
if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
flog << LogString.str();
flog.close();


frem.open("SymsToRem.txt");
if(!frem.is_open())
{
    LogString.str("");
    LogString << "RemBadSyms: file SymsToRem.txt not found\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();
    return;
}


///determining number of lines in file SymsToRem.txt: fremLinesNum
k=0;
while ( frem.getline(line0,200) ) k++;
fremLinesNum=k;

///creating in memory fremArray of size fremLinesNum
fremArray = new all2Line [fremLinesNum];

frem.clear();
frem.seekg(0, std::ios::beg);


///writing SymsToRem.txt into fremArray
k=0;
while ( frem.getline(line0,200) )
    {
        sscanf(line0,"%s", fremArray[k].Symbol);
        k++;
    }

frem.close();



fall.open("all.txt");
if(!fall.is_open())
{
    LogString.str("");
    LogString << "RemBadSyms: file all.txt not found\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();
    return;
}

fall_tmp.open("all_tmp.txt");
if(!fall_tmp.is_open())
{
    LogString.str("");
    LogString << "RemBadSyms: impossible to write into file all_tmp.txt\n";
    std::cout << LogString.str();
    flog.open("log.txt", std::ios::out | std::ios::app);
    if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
    flog << LogString.str();
    flog.close();
    return;
}



while ( fall.getline(line0,200) )
{
    sscanf(line0,"%s", Symbol);
	RemSymbol=false;
	for(k=0; k<fremLinesNum; k++) if ( !strcmp(Symbol, fremArray[k].Symbol) ) {RemSymbol=true; break;}
	if (!RemSymbol) fall_tmp << Symbol << std::endl;
}


fall.close();
fall_tmp.close();


remove("all.txt");
rename("all_tmp.txt", "all.txt");


///Removing fremArray from memory
delete [] fremArray;


LogString.str("");
LogString << "Bad symbols removed\n";
std::cout << LogString.str();
flog.open("log.txt", std::ios::out | std::ios::app);
if (!flog.is_open()) std::cout << "RemBadSyms: can not write into log.txt\n";
flog << LogString.str();
flog.close();
}





void tcDialog::OnBtn_HistData(wxCommandEvent &event)
{
    BinFile=true;
    HistData();
}





void tcDialog::OnBtn_HistDatNew(wxCommandEvent &event)
{
    BinFile=false;
    HistData();
}





void tcDialog::OnBtn_BINtoASCII(wxCommandEvent &event)
{
    BINtoASCII();

}





void tcDialog::OnBtn_ASCIItoBIN(wxCommandEvent &event)
{
    ASCIItoBIN();

}





void tcDialog::OnBtn_UpdLastDay(wxCommandEvent &event)
{
    std::ofstream flog;
    std::ostringstream LogString;


    if (FileExists("LastDay.txt"))
    {
        LogString << "File LastDay.txt found;\nbase update will be taken from this file\n";
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "OnBtn_UpdLastDay: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        sleep(SleepTime_WSJ);
        UpdLastDay_file();
    }
    else
    {
        LogString << "File LastDay.txt not found;\nbase update will be taken from site wsj.com\n";
        std::cout << LogString.str();
        flog.open("log.txt", std::ios::out | std::ios::app);
        if (!flog.is_open()) std::cout << "OnBtn_UpdLastDay: can not write into log.txt\n";
        flog << LogString.str();
        flog.close();

        sleep(SleepTime_WSJ);
        UpdLastDay();
    }
}





void tcDialog::OnBtn_MethodButton(wxCommandEvent &event)
{
    MethodButton();

}





void tcDialog::OnBtn_Optimize(wxCommandEvent &event)
{
    Optimize();

}





void tcDialog::OnBtn_WriteBsk(wxCommandEvent &event)
{
    WriteBsk();

}





void tcDialog::OnBtn_PlaceBsk(wxCommandEvent &event)
{
    PlaceBsk(0);

}
