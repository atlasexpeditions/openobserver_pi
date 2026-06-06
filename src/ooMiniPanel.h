/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Open Observer Plugin Mini Panel
 * Author:   Alex Mansfield
 *
 * Project:  OpenCPN
 *
 ***************************************************************************
 *   Copyright (C) 2013 by David S. Register                               *
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

#pragma once

#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////////
/// Custom events
///////////////////////////////////////////////////////////////////////////////
wxDECLARE_EVENT(OBSERVATION_STARTED, wxCommandEvent);
wxDECLARE_EVENT(OBSERVATION_STOPPED, wxCommandEvent);

///////////////////////////////////////////////////////////////////////////////
/// Class ooMiniPanel
///////////////////////////////////////////////////////////////////////////////
class ooMiniPanel : public wxPanel {
public:
  ooMiniPanel();
  ooMiniPanel(wxWindow* parent, wxWindowID id = wxID_ANY,
              const wxString& msg = wxEmptyString,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL,
              const wxString& name = wxS("ooMiniPanel"));

  ~ooMiniPanel();

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
              const wxString& msg = wxEmptyString,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL,
              const wxString& name = wxS("ooMiniPanel"));

  void SetToggleWindowButtonLabel(const wxString& label);

  void StartOrStopObservation();
  void SetProjectInfo(const wxString& projectName,
                      const wxColor& projectColor);

  void OnShow(wxShowEvent& event);
  void RefreshObservationDisplay();

protected:
  void UpdateObservationStatus();
  void UpdateObservationDuration();

  void ooControlStartStopObservationClick(wxCommandEvent& event);
  void OnToggleWindowClick(wxCommandEvent& event);
  
  void OnObservationDurationTimer(wxTimerEvent& event);

  wxButton* m_StartStopObservation;
  wxStaticText* m_ObservationsDurationLabel;
  wxStaticText* m_ObservationDuration;
  wxButton* m_buttonToggleWindow;
  wxPanel* m_ProjectLabelPanel;
  wxStaticText* m_ProjectLabel;

  wxTimer m_ObservationDurationTimer;
};
