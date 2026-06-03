#include "ooObservationHighlight.h"

#include <wx/pen.h>
#ifdef __WXOSX__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <cmath>

ooObservationHighlight::ooObservationHighlight()
    : m_lat(0.0),
      m_lon(0.0),
      m_colour(*wxBLUE)
{
}

void ooObservationHighlight::Show(double lat, double lon, const wxColour& colour)
{
    m_lat = lat;
    m_lon = lon;
    m_colour = colour;
    m_until = wxDateTime::UNow() + wxTimeSpan::Seconds(4);
}

void ooObservationHighlight::Clear()
{
    m_until = wxDateTime();
}

bool ooObservationHighlight::IsActive() const
{
    return m_until.IsValid() && wxDateTime::UNow().IsEarlierThan(m_until);
}

bool ooObservationHighlight::Render(wxDC& dc, PlugIn_ViewPort* vp)
{
    if (!vp) return false;
    if (!IsActive()) return false;

    wxPoint point;
    GetCanvasPixLL(vp, &point, m_lat, m_lon);

    // A gentle field marker: visible enough to guide the eye,
    // temporary enough to avoid becoming another chart object.
    wxPen pen(m_colour, 3);
    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawCircle(point, 28);

    return true;
}

bool ooObservationHighlight::RenderGL(PlugIn_ViewPort* vp)
{
    if (!vp) return false;
    if (!IsActive()) return false;

    wxPoint point;
    GetCanvasPixLL(vp, &point, m_lat, m_lon);

    const double radius = 56.0;
    const int segments = 96;

    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, vp->pix_width, vp->pix_height, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glLineWidth(9.0f);
    glColor4ub(
        m_colour.Red(),
        m_colour.Green(),
        m_colour.Blue(),
        220);

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        const double angle = (2.0 * M_PI * i) / segments;
        glVertex2d(
            point.x + std::cos(angle) * radius,
            point.y + std::sin(angle) * radius);
    }
    glEnd();

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();

    return true;
}