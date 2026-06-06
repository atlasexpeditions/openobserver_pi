#include "ooAuiPanel.h"

#include <wx/sizer.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/menu.h>

#include "ooObservations.h"
#include "openobserver_pi.h"

extern openobserver_pi* g_openobserver_pi;

static bool IsDarkColour(const wxColour& colour)
{
    const int brightness =
        (colour.Red() * 299 + colour.Green() * 587 + colour.Blue() * 114) / 1000;

    return brightness < 128;
}

static wxColour ContrastTextColour(const wxColour& background)
{
    return IsDarkColour(background) ? *wxWHITE : *wxBLACK;
}

enum
{
    ID_OO_AUI_UNDOCK_PANEL = wxID_HIGHEST + 3100
};

ooAuiPanel::ooAuiPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_titleText(nullptr),
      m_projectText(nullptr),
      m_buttonStartStop(nullptr),
      m_timerText(nullptr),
      m_utcText(nullptr),
      m_timer()
{
    wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_titleText = new wxStaticText(this, wxID_ANY, _("Open Observer"));
    m_projectText = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_buttonStartStop = new wxButton(this, wxID_ANY, _("Start Observation"));
    m_timerText = new wxStaticText(this, wxID_ANY, _("00:00:00"));
    m_utcText = new wxStaticText(this, wxID_ANY, _("UTC: --:--:--"));

    wxFont titleFont = m_titleText->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_titleText->SetFont(titleFont);

    wxFont timerFont = m_timerText->GetFont();
    timerFont.SetPointSize(timerFont.GetPointSize() + 6);
    timerFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_timerText->SetFont(timerFont);

    wxFont utcFont = m_utcText->GetFont();
    utcFont.SetPointSize(utcFont.GetPointSize() - 1);
    m_utcText->SetFont(utcFont);

    sizer->Add(m_titleText, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 6);
    sizer->Add(m_projectText, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);
    sizer->Add(m_timerText, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);
    sizer->Add(m_buttonStartStop, 0, wxEXPAND | wxBOTTOM, 8);
    sizer->Add(m_utcText, 0, wxALIGN_CENTER_HORIZONTAL, 0);

    m_buttonStartStop->SetMinSize(wxSize(-1, 38));

    outerSizer->Add(sizer, 1, wxEXPAND | wxALL, 12);

    SetSizer(outerSizer);
    SetMinSize(wxSize(280, 180));
    ApplyThemeColours();
    Layout();

    UpdateObservationStatus();
    UpdateUtcDisplay();
    m_timer.Start(1000);

    m_buttonStartStop->Bind(wxEVT_BUTTON, &ooAuiPanel::OnStartStop, this);
    m_timer.Bind(wxEVT_TIMER,
                 &ooAuiPanel::OnTimer,
                 this,
                 m_timer.GetId());
    Bind(wxEVT_SHOW, &ooAuiPanel::OnShow, this);
    Bind(wxEVT_CONTEXT_MENU, &ooAuiPanel::OnContextMenu, this);
    Bind(wxEVT_MENU, &ooAuiPanel::OnUndockPanel, this, ID_OO_AUI_UNDOCK_PANEL);
}

void ooAuiPanel::ApplyThemeColours()
{
    const wxColour backgroundColour =
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    const wxColour textColour =
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);

    SetBackgroundColour(backgroundColour);

    if (m_titleText) m_titleText->SetForegroundColour(textColour);
    if (m_timerText) m_timerText->SetForegroundColour(textColour);
    if (m_utcText) m_utcText->SetForegroundColour(textColour);
}

void ooAuiPanel::SetProjectInfo(const wxString& projectName,
                                const wxColor& projectColor)
{
    m_projectText->SetLabel(projectName);
    m_projectText->SetBackgroundColour(projectColor);
    m_projectText->SetForegroundColour(ContrastTextColour(projectColor));

    wxFont projectFont = m_projectText->GetFont();
    projectFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_projectText->SetFont(projectFont);

    Layout();
}

void ooAuiPanel::OnContextMenu(wxContextMenuEvent& event)
{
    wxMenu menu;
    menu.Append(ID_OO_AUI_UNDOCK_PANEL, _("Undock panel"));
    PopupMenu(&menu);
}

void ooAuiPanel::OnUndockPanel(wxCommandEvent& event)
{
    if (g_openobserver_pi) {
        g_openobserver_pi->UndockAuiPanel();
    }
}

void ooAuiPanel::OnStartStop(wxCommandEvent& event)
{
    if (!g_openobserver_pi || !g_openobserver_pi->m_ooObservations) {
        event.Skip();
        return;
    }

    const bool wasObserving = g_openobserver_pi->m_ooObservations->IsObserving();

    if (wasObserving) {
        g_openobserver_pi->m_ooObservations->StopObservation();
        g_openobserver_pi->StopNmeaRecordingIfNeeded();

        g_openobserver_pi->CreateMarkForCompletedObservationIfRequested();

    } else {
        g_openobserver_pi->m_ooObservations->StartObservation();
        g_openobserver_pi->StartNmeaRecordingIfNeeded();

    }

    UpdateObservationStatus();
    UpdateObservationDuration();
    g_openobserver_pi->RefreshObservationDisplay();

    event.Skip();
}

void ooAuiPanel::OnTimer(wxTimerEvent& event)
{
    UpdateObservationStatus();
    UpdateObservationDuration();
    UpdateUtcDisplay();
    event.Skip();
}

void ooAuiPanel::OnShow(wxShowEvent& event)
{
    ApplyThemeColours();
    RefreshObservationDisplay();
    event.Skip();
}

void ooAuiPanel::RefreshObservationDisplay()
{
    UpdateObservationDuration();
    UpdateObservationStatus();
    UpdateUtcDisplay();
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
    } else {
        m_buttonStartStop->SetLabel(_("Start Observation"));
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

void ooAuiPanel::UpdateUtcDisplay()
{
    if (!g_openobserver_pi || !g_openobserver_pi->m_ooObservations) {
        m_utcText->SetLabel(_("UTC: --:--:--"));
        return;
    }

    const wxString utcTime =
        g_openobserver_pi->m_ooObservations->GetUtcTimeFromNMEA(
            ooObservations::UTC_TIME_TIME);

    const wxString utcSource =
        g_openobserver_pi->m_ooObservations->GetUtcTimeSourceLabel();

    m_utcText->SetLabel(wxString::Format(_("UTC: %s %s"),
                                         utcTime,
                                         utcSource));

    Layout();
}
