#pragma once

#include "ocpn_plugin.h"

#include <wx/colour.h>
#include <wx/dc.h>
#include <wx/datetime.h>

class ooObservationHighlight
{
public:
    ooObservationHighlight();

    void Show(double lat, double lon, const wxColour& colour);
    void Clear();

    bool IsActive() const;
    bool Render(wxDC& dc, PlugIn_ViewPort* vp);
    bool RenderGL(PlugIn_ViewPort* vp);

private:
    double m_lat;
    double m_lon;
    wxColour m_colour;
    wxDateTime m_until;
};