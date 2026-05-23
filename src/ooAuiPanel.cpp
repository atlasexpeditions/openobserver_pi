#include "ooAuiPanel.h"

#include <wx/sizer.h>
#include <wx/colour.h>
#include <wx/settings.h>

#include "ooObservations.h"
#include "openobserver_pi.h"

extern openobserver_pi* g_openobserver_pi;

ooAuiPanel::ooAuiPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_titleText(nullptr),
      m_projectText(nullptr),
      m_buttonStartStop(nullptr),
      m_timerText(nullptr),
      m_timer()
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
    m_timer.Bind(wxEVT_TIMER,
                 &ooAuiPanel::OnTimer,
                 this,
                 m_timer.GetId());
    Bind(wxEVT_SHOW, &ooAuiPanel::OnShow, this);
}

void ooAuiPanel::SetProjectInfo(const wxString& projectName,
                                const wxColor& projectColor)
{
    m_projectText->SetLabel(wxString::Format(_("Project: %s"), projectName));
    m_projectText->SetBackgroundColour(projectColor);
    m_projectText->SetForegroundColour(*wxWHITE);

    wxFont projectFont = m_projectText->GetFont();
    projectFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_projectText->SetFont(projectFont);

    Layout();
}

void ooAuiPanel::OnStartStop(wxCommandEvent& event)
{
    if (!g_openobserver_pi || !g_openobserver_pi->m_ooObservations) {
        event.Skip();
        return;
    }

    if (g_openobserver_pi->m_ooObservations->IsObserving()) {
        g_openobserver_pi->m_ooObservations->StopObservation();
    } else {
        g_openobserver_pi->m_ooObservations->StartObservation();
    }

    UpdateObservationStatus();

    UpdateObservationDuration();

    if (g_openobserver_pi) {

        g_openobserver_pi->RefreshObservationDisplay();

    }

    event.Skip();
}

void ooAuiPanel::OnTimer(wxTimerEvent& event)
{
    UpdateObservationDuration();
    event.Skip();
}

void ooAuiPanel::OnShow(wxShowEvent& event)
{
    RefreshObservationDisplay();
    event.Skip();
}

void ooAuiPanel::RefreshObservationDisplay()
{
    UpdateObservationDuration();
    UpdateObservationStatus();
}

void ooAuiPanel::UpdateObservationStatus()
{
    if (!g_openobserver_pi || !g_openobserver_pi->m_ooObservations) {
        return;
    }

    if (g_openobserver_pi->m_ooObservations->IsObserving()) {
        m_buttonStartStop->SetLabel(_("Stop Observation"));
        m_timerText->SetForegroundColour(*wxRED);
        UpdateObservationDuration();
        if (!m_timer.IsRunning()) {
            m_timer.Start(100);
        }
    } else {
        m_buttonStartStop->SetLabel(_("Start Observation"));
        m_timer.Stop();
        m_timerText->SetLabel(_("00:00:00"));
        m_timerText->SetForegroundColour(
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    }

    Layout();
}

void ooAuiPanel::UpdateObservationDuration()
{
    if (!g_openobserver_pi || !g_openobserver_pi->m_ooObservations) {
        return;
    }

    if (!g_openobserver_pi->m_ooObservations->IsObserving()) {
        m_timerText->SetLabel(_("00:00:00"));
        return;
    }

    const long duration_ms =
        g_openobserver_pi->m_ooObservations->GetObservationDuration();

    const unsigned int hours = duration_ms / 3600000;
    const unsigned int minutes = (duration_ms % 3600000) / 60000;
    const unsigned int seconds = (duration_ms % 60000) / 1000;

    m_timerText->SetLabel(
        wxString::Format("%02u:%02u:%02u", hours, minutes, seconds));

    Layout();
}