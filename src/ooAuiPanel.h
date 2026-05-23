#pragma once

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/datetime.h>

class ooAuiPanel : public wxPanel
{
public:
    explicit ooAuiPanel(wxWindow* parent);

private:
    void OnStartStop(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void UpdateTimerDisplay();

    wxStaticText* m_titleText;
    wxStaticText* m_projectText;
    wxButton* m_buttonStartStop;
    wxStaticText* m_timerText;
    wxTimer m_timer;
    wxDateTime m_startTime;
    bool m_running;
};