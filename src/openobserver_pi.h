/******************************************************************************
 * $Id: openobserver_pi.h,v 1.0 2015/01/28 01:54:37 jongough Exp $
 *
 * Project:  OpenCPN
 * Purpose:  OpenCPN General Drawing Plugin
 * Author:   Jon Gough
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register   *
 *   $EMAIL$   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 */

#pragma once

#ifdef __OCPN__ANDROID__
#include "qopengl.h"                  // this gives us the qt runtime gles2.h
#endif

#include "wxWTranslateCatalog.h"

#include "ocpn_plugin.h"

#include <wx/string.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>
#include <wx/splitter.h>
#include <wx/fileconf.h>
#include <wx/dynarray.h>
#include <set>
#include <unordered_map>

#include "ooObservations.h"
#include "ooNmeaRecorder.h"
#include "ooDataLogger.h"
#include "ooObservationHighlight.h"
#include "globals.h"

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

class openobserver_pi : public opencpn_plugin_118
{
public:

    openobserver_pi(void *ppimgr);
    ~openobserver_pi();

    //    The required PlugIn Methods
    int Init(void);
    bool DeInit(void);

    int GetPlugInVersionMajor();
    int GetPlugInVersionMinor();
    int GetPlugInVersionPatch();
    int GetPlugInVersionPost();

    int GetAPIVersionMajor();
    int GetAPIVersionMinor();
    wxBitmap *GetPlugInBitmap();
    wxString GetCommonName();
    wxString GetShortDescription();
    wxString GetLongDescription();

    int GetToolbarToolCount(void);
    void OnToolbarToolCallback(int id);
    void OnToolbarToolDownCallback(int id);
    void OnToolbarToolUpCallback(int id);
    void OnContextMenuItemCallback(int id);
    void ShowPreferencesDialog(wxWindow *parent) override;

    void LateInit(void);
    bool KeyboardEventHook( wxKeyEvent &event );
    bool MouseEventHook( wxMouseEvent &event );
    void SetCursorLatLon(double lat, double lon);
    void SetPositionFix(PlugIn_Position_Fix &pfix);
    void SetNMEASentence(wxString& sentence);
    void SetCurrentViewPort(PlugIn_ViewPort& vp);
    bool RenderOverlayMultiCanvas(wxDC& dc, PlugIn_ViewPort* vp, int canvas_ix, int priority) override;
    bool RenderGLOverlayMultiCanvas(wxGLContext* pcontext, PlugIn_ViewPort* vp, int canvas_ix, int priority) override;
    bool RenderOverlay(wxDC& dc, PlugIn_ViewPort* vp) override;
    
    void SetProject(const wxString& projectName, const wxColor& projectColor, int observationsIndex);
    void HighlightObservationOnChart(double lat, double lon, const wxColour& colour);
    void JumpToObservationOnChart(double lat, double lon);
    void RefreshObservationDisplay();
    void MarkObservationsDirty();
    void CreateMarkForCompletedObservationIfRequested();
    void FocusCurrentObservationRow();
    void StartNmeaRecordingIfNeeded();
    void StopNmeaRecordingIfNeeded();
    ooDataLogger& GetDataLogger();
    const ooDataLogger& GetDataLogger() const;
    void ShowMiniPanel();

    void ToggleToolbarIcon();
    void ToggleWindow();

    ooObservations *m_ooObservations;

    static void WriteNmeaXML(const std::unordered_map<wxString, std::set<int>>& scannedNmeaFields);
    static std::vector<NMEAField> ReadNmeaXML();
    static void RefreshListings();
  private:
    void    SaveConfig();
    void    LoadConfig();
    void    ShowObservationForSelectedMark();
    void    CreateObservationFromSelectedMark();

    wxWindow            *m_parent_window;
    wxFileConfig        *m_pConfig;

    tpicons *m_ptpicons;
    ooControlDialogImpl *m_ooControlDialogImpl;
    ooMiniDialogImpl *m_ooMiniDialogImpl;

    ooDataLogger m_dataLogger;

    bool    m_bReadyForRequests;
    int     m_iCallerId;
    bool    m_bShowMainDialog;
    bool    m_recordNmeaStreamDuringEachObservation;
    ooNmeaRecorder m_nmeaRecorder;
    int     m_openobserver_button_id;
    int     m_observationsIndex, m_observationsChoiceCount;
    wxRect  m_dialogPosition, m_miniDialogPosition;
    wxString m_currentProjectName;
    wxColor m_currentProjectColor;
    int     m_addObservationItem;
    int     m_showObservationItem;
    int     m_createObservationFromMarkItem;

    double  m_cursor_lat;
    double  m_cursor_lon;
    ooObservationHighlight m_observationHighlight;
    PlugIn_ViewPort m_lastViewPort;
    bool m_hasLastViewPort;
    double  m_click_lat;
    double  m_click_lon;
};
