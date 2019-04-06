/***************************************************************
 * Name:      tcApp.cpp
 * Purpose:   Code for Application Class
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

#include "tcApp.h"
#include "tcMain.h"

#include "globals.h"


IMPLEMENT_APP(tcApp);

bool tcApp::OnInit()
{

    dlg = new tcDialog(0L, _("wxWidgets Application Template")); ///pointer to this dlg-object: tcDialog* dlg is declared in globals.cpp and is present in globals.h

    dlg->Show();
    return true;
}
