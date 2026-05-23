#pragma once

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/timer.h>

class ooAuiPanel : public wxPanel
{
public:
    explicit ooAuiPanel(wxWindow* parent);

    void SetProjectInfo(const wxString& projectName,
                        const wxColor& projectColor);
    void RefreshObservationDisplay();

private:
    void OnStartStop(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnShow(wxShowEvent& event);
    void OnContextMenu(wxContextMenuEvent& event);
    void OnUndockPanel(wxCommandEvent& event);
    void UpdateObservationStatus();
    void UpdateObservationDuration();

    wxStaticText* m_titleText;
    wxStaticText* m_projectText;
    wxButton* m_buttonStartStop;
    wxStaticText* m_timerText;
        wxTimer m_timer;
};