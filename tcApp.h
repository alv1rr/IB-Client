/***************************************************************
 * Name:      tcApp.h
 * Purpose:   Defines Application Class
 * Author:    alv ()
 * Created:   2017-01-21
 * Copyright: alv ()
 * License:
 **************************************************************/

#ifndef TCAPP_H
#define TCAPP_H

#include <wx/app.h>

class tcApp : public wxApp
{
    public:
        virtual bool OnInit();
};

#endif // TCAPP_H
