#pragma once

#include <wx/panel.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>

class ooObservations;

class ooAuiObservationsPanel : public wxPanel
{
public:
    ooAuiObservationsPanel(wxWindow* parent);
    ~ooAuiObservationsPanel();

    void SetObservations(ooObservations* observations);
    void RefreshObservations();

private:
    wxStaticText* m_titleText;
    wxGrid* m_grid;
    ooObservations* m_observations;
    wxTimer m_refreshTimer;

    void RebuildGrid();
    void ApplyReadOnlyGridStyle();
    void OnContextMenu(wxContextMenuEvent& event);
    void OnUndockPanel(wxCommandEvent& event);
    void OnRefreshTimer(wxTimerEvent& event);
};