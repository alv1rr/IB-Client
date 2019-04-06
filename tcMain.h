/***************************************************************
 * Name:      tcMain.h
 * Purpose:   Defines Application Frame
 * Author:    alv ()
 * Created:   2017-01-21
 * Copyright: alv ()
 * License:
 **************************************************************/

#ifndef TCMAIN_H
#define TCMAIN_H

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "tcApp.h"

#include <wx/button.h>


class tcDialog: public wxDialog
{
    public:
        tcDialog(wxDialog *dlg, const wxString& title);
        ~tcDialog();


    protected:
        enum
        {
            idBtn_Connect = 1000,
            idBtn_Disconnect,
            idBtn_CheckBin,
            idBtn_RemBadSyms,
            idBtn_HistData,
            idBtn_HistDatNew,
            idBtn_BINtoASCII,
            idBtn_ASCIItoBIN,
            idBtn_UpdLastDay,
            idBtn_MethodButton,
            idBtn_Optimize,
            idBtn_WriteBsk,
            idBtn_PlaceBsk
        };

        wxButton* Btn_Connect;
        wxButton* Btn_Disconnect;
        wxButton* Btn_CheckBin;
        wxButton* Btn_RemBadSyms;
        wxButton* Btn_HistData;
        wxButton* Btn_HistDatNew;
        wxButton* Btn_BINtoASCII;
        wxButton* Btn_ASCIItoBIN;
        wxButton* Btn_UpdLastDay;
        wxButton* Btn_MethodButton;
        wxButton* Btn_Optimize;
        wxButton* Btn_WriteBsk;
        wxButton* Btn_PlaceBsk;

    private:
        void OnClose(wxCloseEvent& event);
        void OnBtn_Connect(wxCommandEvent& event);
        void OnBtn_Disconnect(wxCommandEvent& event);
        void OnBtn_CheckBin(wxCommandEvent& event);
        void OnBtn_RemBadSyms(wxCommandEvent& event);
        void OnBtn_HistData(wxCommandEvent& event);
        void OnBtn_HistDatNew(wxCommandEvent& event);
        void OnBtn_BINtoASCII(wxCommandEvent& event);
        void OnBtn_ASCIItoBIN(wxCommandEvent& event);
        void OnBtn_UpdLastDay(wxCommandEvent& event);
        void OnBtn_MethodButton(wxCommandEvent& event);
        void OnBtn_Optimize(wxCommandEvent& event);
        void OnBtn_WriteBsk(wxCommandEvent& event);
        void OnBtn_PlaceBsk(wxCommandEvent& event);

        DECLARE_EVENT_TABLE()
};

#endif // TCMAIN_H
