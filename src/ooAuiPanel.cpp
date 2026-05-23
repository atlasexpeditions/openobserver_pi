#include "ooAuiPanel.h"

#include <wx/sizer.h>
#include <wx/colour.h>
#include <wx/settings.h>

ooAuiPanel::ooAuiPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_titleText(nullptr),
      m_projectText(nullptr),
      m_buttonStartStop(nullptr),
      m_timerText(nullptr),
      m_timer(this),
      m_running(false)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_titleText = new wxStaticText(this, wxID_ANY, _("Open Observer"));
    m_projectText = new wxStaticText(this, wxID_ANY, _("Project: —"));
    m_buttonStartStop = new wxButton(this, wxID_ANY, _("Start Observation"));
    m_timerText = new wxStaticText(this, wxID_ANY, _("00:00:00"));

    wxFont titleFont = m_titleText->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_titleText->SetFont(titleFont);

    wxFont timerFont = m_timerText->GetFont();
    timerFont.SetPointSize(timerFont.GetPointSize() + 6);
    timerFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_timerText->SetFont(timerFont);

    sizer->Add(m_titleText, 0, wxLEFT | wxRIGHT | wxTOP | wxALIGN_CENTER_HORIZONTAL, 8);
    sizer->Add(m_projectText, 0, wxLEFT | wxRIGHT | wxTOP | wxALIGN_CENTER_HORIZONTAL, 8);
    sizer->Add(m_buttonStartStop, 0, wxALL | wxEXPAND, 8);
    sizer->Add(m_timerText, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 8);

    SetSizer(sizer);
    Layout();

    m_buttonStartStop->Bind(wxEVT_BUTTON, &ooAuiPanel::OnStartStop, this);
    m_timer.Bind(wxEVT_TIMER, &ooAuiPanel::OnTimer, this);
}

void ooAuiPanel::OnStartStop(wxCommandEvent& event)
{
    if (!m_running) {
        m_running = true;
        m_startTime = wxDateTime::Now();
        m_buttonStartStop->SetLabel(_("Stop Observation"));
        m_timerText->SetForegroundColour(*wxRED);
        m_timer.Start(1000);
    } else {
        m_running = false;
        m_buttonStartStop->SetLabel(_("Start Observation"));
        m_timerText->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
        m_timer.Stop();
    }

    UpdateTimerDisplay();
    event.Skip();
}

void ooAuiPanel::OnTimer(wxTimerEvent& event)
{
    UpdateTimerDisplay();
    event.Skip();
}

void ooAuiPanel::UpdateTimerDisplay()
{
    if (!m_running) {
        m_timerText->SetLabel(_("00:00:00"));
        Layout();
        return;
    }

    wxTimeSpan elapsed = wxDateTime::Now() - m_startTime;
    m_timerText->SetLabel(elapsed.Format("%H:%M:%S"));
    Layout();
}