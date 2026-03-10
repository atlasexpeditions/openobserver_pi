/**************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Open Observer Plugin Mini Panel
 * Author:   Alex Mansfield
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 **************************************************************************/

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "ooMiniPanel.h"

#include "ooObservations.h"
#include "openobserver_pi.h"

extern openobserver_pi* g_openobserver_pi;

wxDEFINE_EVENT(OBSERVATION_STARTED, wxCommandEvent);
wxDEFINE_EVENT(OBSERVATION_STOPPED, wxCommandEvent);

ooMiniPanel::ooMiniPanel() : wxPanel() {}

ooMiniPanel::ooMiniPanel(wxWindow* parent, wxWindowID id, const wxString& msg,
                         const wxPoint& pos, const wxSize& size, long style,
                         const wxString& name)
    : wxPanel() {
  Create(parent, id, msg, pos, size, style, name);
}

bool ooMiniPanel::Create(wxWindow* parent, wxWindowID id, const wxString& msg,
                         const wxPoint& pos, const wxSize& size, long style,
                         const wxString& name) {
  if (!wxPanel::Create(parent, id, pos, size, style, name)) return false;

  wxBoxSizer* bSizerTopButtons = new wxBoxSizer(wxHORIZONTAL);

  m_StartStopObservation =
      new wxButton(this, wxID_ANY, _("Start Observation"), wxDefaultPosition,
                   wxDefaultSize, 0);

  bSizerTopButtons->Add(m_StartStopObservation, 0,
                        wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_ObservationsDurationLabel =
      new wxStaticText(this, wxID_ANY, _("Observation Duration"),
                       wxDefaultPosition, wxDefaultSize, 0);
  m_ObservationsDurationLabel->Wrap(-1);
  bSizerTopButtons->Add(m_ObservationsDurationLabel, 0,
                        wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_ObservationDuration =
      new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                     wxDefaultSize, wxTE_READONLY);
  bSizerTopButtons->Add(m_ObservationDuration, 0,
                        wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_buttonToggleWindow = new wxButton(this, wxID_ANY, _("Toggle Window"),
                                        wxDefaultPosition, wxDefaultSize, 0);
  bSizerTopButtons->Add(m_buttonToggleWindow, 0,
                        wxALIGN_CENTER_VERTICAL | wxALL, 5);

  this->SetSizerAndFit(bSizerTopButtons);
  bSizerTopButtons->SetSizeHints(this);

  m_StartStopObservation->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(ooMiniPanel::ooControlStartStopObservationClick),
      NULL, this);

  m_buttonToggleWindow->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(ooMiniPanel::OnToggleWindowClick), NULL,
      this);

  m_ObservationDurationTimer.Bind(wxEVT_TIMER,
                                  &ooMiniPanel::OnObservationDurationTimer,
                                  this, m_ObservationDurationTimer.GetId());

    return true;
}

ooMiniPanel::~ooMiniPanel() {
  m_StartStopObservation->Disconnect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(ooMiniPanel::ooControlStartStopObservationClick),
      NULL, this);

  m_buttonToggleWindow->Disconnect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(ooMiniPanel::OnToggleWindowClick), NULL,
      this);

  m_ObservationDurationTimer.Stop();
}

void ooMiniPanel::SetToggleWindowButtonLabel(const wxString& label)
{
  m_buttonToggleWindow->SetLabel(label);
}

void ooMiniPanel::StartOrStopObservation()
{
  if (!g_openobserver_pi->m_ooObservations) return;

  if (g_openobserver_pi->m_ooObservations->IsObserving()) {
    // stop observation
    g_openobserver_pi->m_ooObservations->StopObservation();

    // issue event
    wxCommandEvent event(OBSERVATION_STOPPED, GetId());
    event.SetEventObject(this);
    ProcessWindowEvent(event);
  } else {
    // start observation
    g_openobserver_pi->m_ooObservations->StartObservation();

    // issue event
    wxCommandEvent event(OBSERVATION_STARTED, GetId());
    event.SetEventObject(this);
    ProcessWindowEvent(event);
  }

  UpdateObservationStatus();
}

void ooMiniPanel::ooControlStartStopObservationClick(wxCommandEvent& event) 
{
    StartOrStopObservation();
}

void ooMiniPanel::OnShow(wxShowEvent& event)
{
  UpdateObservationDuration();
  UpdateObservationStatus();
}

void ooMiniPanel::UpdateObservationStatus()
{
  if (!g_openobserver_pi->m_ooObservations) return;

  if (g_openobserver_pi->m_ooObservations->IsObserving())
  {
    m_StartStopObservation->SetLabel("Stop Observation");
    
    m_ObservationDuration->SetBackgroundColour(*wxRED);

    // start timer to update observation duration
    m_ObservationDurationTimer.Start(100);  // 100 ms = 0.1 s
  } else {
    //VPE color timer 
    m_ObservationDuration->SetBackgroundColour(*wxWHITE);
    m_ObservationDuration->Refresh();
    m_StartStopObservation->SetLabel("Start Observation");
 
    m_ObservationDurationTimer.Stop();
    
  }
}

void ooMiniPanel::UpdateObservationDuration()
{
  if (!g_openobserver_pi->m_ooObservations) return;

  const long duration_ms =
      g_openobserver_pi->m_ooObservations->GetObservationDuration();

  const unsigned int hours = duration_ms / 3600000;
  const unsigned int minutes = (duration_ms % 3600000) / 60000;
  const unsigned int seconds = (duration_ms % 60000) / 1000;

  char durationString[16];
  sprintf(durationString, "%02u:%02u:%02u", hours, minutes, seconds);

  m_ObservationDuration->SetValue(durationString);
}

void ooMiniPanel::OnObservationDurationTimer(wxTimerEvent& event) {
  UpdateObservationDuration();
}

void ooMiniPanel::OnToggleWindowClick(wxCommandEvent& event) {
  g_openobserver_pi->ToggleWindow();
}