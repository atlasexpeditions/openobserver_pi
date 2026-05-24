/******************************************************************************
 * updated: 4-5-2012
 * Project:  OpenCPN
 * Purpose:  Open Observer Plugin
 * Author:   Jon Gough
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register   *
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
 ***************************************************************************
 */


#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers
#include <wx/stdpaths.h>
#include <wx/timer.h>
#include <wx/event.h>
#include <wx/sysopt.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/listbook.h>
#include <wx/panel.h>
#include <wx/ffile.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

#include <wx/aui/aui.h>
#include <wx/gdicmn.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <algorithm>

#include "openobserver_pi.h"
#include "version.h"
#include "wxWTranslateCatalog.h"

#include "tpicons.h"
#include "ooObservations.h"
#include "ooControlDialogImpl.h"
#include "ooMiniDialogImpl.h"
#include "ooAuiPanel.h"
#include "ooAuiObservationsPanel.h"

#include "wx/jsonwriter.h"


#ifndef DECL_EXP
#ifdef __WXMSW__
#define DECL_EXP     __declspec(dllexport)
#else
#define DECL_EXP
#endif
#endif

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif

openobserver_pi         *g_openobserver_pi;
wxString                *g_PrivateDataDir;

wxString                *g_pData;
wxString                *g_SData_Locn;
wxString                *g_pLayerDir;
wxString                *g_pListingDir;

wxString                *g_tplocale;
void                    *g_ppimgr;

int                     g_iLocaleDepth;

wxFont                  *g_pFontTitle;
wxFont                  *g_pFontData;
wxFont                  *g_pFontLabel;
wxFont                  *g_pFontSmall;

// Needed for ocpndc.cpp to compile. Normally would be in glChartCanvas.cpp
float g_GLMinSymbolLineWidth;

void appendOSDirSlash(wxString* pString)
{
    wxChar sep = wxFileName::GetPathSeparator();

    if (pString->Last() != sep)
        pString->Append(sep);
}

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return new openobserver_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}


//---------------------------------------------------------------------------------------------------------
//
//    openobserver PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------



//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

openobserver_pi::openobserver_pi(void *ppimgr) 
    : opencpn_plugin_118(ppimgr),
      m_ooObservations(nullptr),
      m_ooControlDialogImpl(nullptr),
      m_ooMiniDialogImpl(nullptr),
      m_ooAuiPanel(nullptr),
      m_ooAuiObservationsPanel(nullptr),
      m_useAuiPanel(false),
      m_showAuiObservationsPanel(false),
      m_currentProjectName(wxEmptyString),
      m_currentProjectColor(*wxBLACK),
      m_showMiniPanelItem(-1),
      m_showObservationsPanelItem(-1)
{
    // Create the PlugIn icons
    g_ppimgr = ppimgr;
    g_openobserver_pi = this;

    wxString *l_pDir = new wxString(*GetpPrivateApplicationDataLocation());
    appendOSDirSlash(l_pDir);

    l_pDir->Append(_T("plugins"));
    appendOSDirSlash(l_pDir);
    if (!wxDir::Exists(*l_pDir))
        wxMkdir(*l_pDir);

    g_PrivateDataDir = new wxString(*l_pDir);
    g_PrivateDataDir->Append(_T("openobserver_pi"));
    appendOSDirSlash(g_PrivateDataDir);
    if (!wxDir::Exists(*g_PrivateDataDir))
        wxMkdir(*g_PrivateDataDir);

    g_pData = new wxString(*g_PrivateDataDir);
    g_pData->append(wxS("data"));
    appendOSDirSlash(g_pData);
    if (!wxDir::Exists(*g_pData))
        wxMkdir(*g_pData);

    g_pLayerDir = new wxString(*g_PrivateDataDir);
    g_pLayerDir->Append(wxT("Layers"));
    appendOSDirSlash(g_pLayerDir);
    if (!wxDir::Exists(*g_pLayerDir))
        wxMkdir(*g_pLayerDir);

    g_pListingDir = new wxString(*g_PrivateDataDir);
    g_pListingDir->Append(wxT("Listings"));
    appendOSDirSlash(g_pListingDir);
    if (!wxDir::Exists(*g_pListingDir))
        wxMkdir(*g_pListingDir);

    // Copy packaged listings.
    // Do not overwrite user-customized listings.
    //
    // Packaged listings are installed with the plugin resources.
    // On macOS imported plugins usually place resources under:
    // ~/Library/Application Support/OpenCPN/Contents/SharedSupport/plugins/openobserver_pi/data/Listings
    const wxUniChar sep = wxFileName::GetPathSeparator();

    wxArrayString possiblePackagedListingsPaths;
   
    // 1. Standard OpenCPN plugin data directory, when available.
    // Prefer the package layout installed by CMake:
    //   openobserver_pi/data/Listings
    // Keep openobserver_pi/Listings as a compatibility fallback.
    const wxString pluginDataDir = GetPluginDataDir("openobserver_pi");

    if (!pluginDataDir.IsEmpty()) {
        possiblePackagedListingsPaths.Add(
            wxString::Format(wxT("%s%c%s%c%s"),
                pluginDataDir,
                sep, wxT("data"),
                sep, wxT("Listings")));

        possiblePackagedListingsPaths.Add(
            wxString::Format(wxT("%s%c%s"),
                pluginDataDir,
                sep, wxT("Listings")));
    }


#ifdef __WXOSX__
    // 2. macOS imported plugin location in the user's Application Support folder.
    const wxString macUserListingsPath =
        wxString::Format(
            wxT("%s%cLibrary%cApplication Support%cOpenCPN%cContents%cSharedSupport%cplugins%copenobserver_pi%cdata%cListings"),
            wxGetHomeDir(),
            sep, sep, sep, sep, sep, sep, sep, sep, sep);

    possiblePackagedListingsPaths.Add(macUserListingsPath);
#endif

    // 3. Fallback for layouts where plugin resources are located relative to the executable.
    const wxString exePath = GetOCPN_ExePath();
    possiblePackagedListingsPaths.Add(
        wxString::Format(wxT("%s%c%s%c%s%c%s%c%s"),
            wxFileName(exePath).GetPath(),
            sep, wxT("plugins"),
            sep, wxT("openobserver_pi"),
            sep, wxT("data"),
            sep, wxT("Listings")));

    for (const auto& packagedListingsPath : possiblePackagedListingsPaths) {
        if (!wxDir::Exists(packagedListingsPath)) {
            continue;
        }

        wxString sourceRoot = packagedListingsPath;
        appendOSDirSlash(&sourceRoot);

        wxArrayString allPackagedListings;
        wxDir::GetAllFiles(sourceRoot, &allPackagedListings, wxT("*.xml"));

        for (const auto& f : allPackagedListings) {
            wxFileName relativeFile(f);

            if (!relativeFile.MakeRelativeTo(sourceRoot)) {
                continue;
            }

            const wxString relativePath = relativeFile.GetFullPath();
            const wxString targetPath = wxString::Format(wxT("%s%s"), *g_pListingDir, relativePath);

            wxFileName targetFile(targetPath);

            if (!wxDir::Exists(targetFile.GetPath())) {
                wxFileName::Mkdir(targetFile.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
            }

            if (!wxFile::Exists(targetFile.GetFullPath())) {
                wxCopyFile(f, targetFile.GetFullPath());
            }
        }
    }
    // Copy packaged user icons.
    // Do not overwrite user-customized icons.
    //
    // Source:
    //   plugin data/UserIcons
    //
    // Destination:
    //   OpenCPN user directory/UserIcons
    wxString userIconsDir = *GetpPrivateApplicationDataLocation();
    appendOSDirSlash(&userIconsDir);
    userIconsDir.Append(wxT("UserIcons"));
    appendOSDirSlash(&userIconsDir);

    if (!wxDir::Exists(userIconsDir)) {
        wxMkdir(userIconsDir);
    }

    wxArrayString possiblePackagedUserIconsPaths;

    // 1. Standard OpenCPN plugin data directory, when available.
    // Prefer the package layout installed by CMake:
    //   openobserver_pi/data/UserIcons
    // Keep openobserver_pi/UserIcons as a compatibility fallback.
    if (!pluginDataDir.IsEmpty()) {
        possiblePackagedUserIconsPaths.Add(
            wxString::Format(wxT("%s%c%s%c%s"),
                pluginDataDir,
                sep, wxT("data"),
                sep, wxT("UserIcons")));

        possiblePackagedUserIconsPaths.Add(
            wxString::Format(wxT("%s%c%s"),
                pluginDataDir,
                sep, wxT("UserIcons")));
    }


#ifdef __WXOSX__
    // 2. macOS imported plugin location in the user's Application Support folder.
    const wxString macUserIconsPath =
        wxString::Format(
            wxT("%s%cLibrary%cApplication Support%cOpenCPN%cContents%cSharedSupport%cplugins%copenobserver_pi%cdata%cUserIcons"),
            wxGetHomeDir(),
            sep, sep, sep, sep, sep, sep, sep, sep, sep);

    possiblePackagedUserIconsPaths.Add(macUserIconsPath);
    
#endif

    // 3. Fallback for layouts where plugin resources are located relative to the executable.
    possiblePackagedUserIconsPaths.Add(
        wxString::Format(wxT("%s%c%s%c%s%c%s%c%s"),
            wxFileName(exePath).GetPath(),
            sep, wxT("plugins"),
            sep, wxT("openobserver_pi"),
            sep, wxT("data"),
            sep, wxT("UserIcons")));

    for (const auto& packagedUserIconsPath : possiblePackagedUserIconsPaths) {
        if (!wxDir::Exists(packagedUserIconsPath)) {
            continue;
        }

        wxString sourceRoot = packagedUserIconsPath;
        appendOSDirSlash(&sourceRoot);

        wxArrayString allPackagedUserIcons;
        wxDir::GetAllFiles(sourceRoot, &allPackagedUserIcons);

        for (const auto& f : allPackagedUserIcons) {
            if (wxFileName(f).GetFullName() == wxT(".DS_Store")) {
                continue;
            }

            wxFileName relativeFile(f);

            if (!relativeFile.MakeRelativeTo(sourceRoot)) {
                continue;
            }

            const wxString relativePath = relativeFile.GetFullPath();
            const wxString targetPath = wxString::Format(wxT("%s%s"), userIconsDir, relativePath);

            wxFileName targetFile(targetPath);

            if (!wxDir::Exists(targetFile.GetPath())) {
                wxFileName::Mkdir(targetFile.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
            }

            if (!wxFile::Exists(targetFile.GetFullPath())) {
                wxCopyFile(f, targetFile.GetFullPath());
            }
        }
    }

    ooObservations::SetNMEAFields(ReadNmeaXML());

    m_ptpicons = new tpicons();

    delete l_pDir;
}

openobserver_pi::~openobserver_pi()
{
    delete g_SData_Locn;
    g_SData_Locn = NULL;

    delete g_PrivateDataDir;
    g_PrivateDataDir = NULL;

    delete g_pData;
    g_pData = NULL;

    delete g_pLayerDir;
    g_pLayerDir = NULL;

    delete g_pListingDir;
    g_pListingDir = NULL;
}

int openobserver_pi::Init(void)
{
    g_tplocale = nullptr;
    m_bReadyForRequests = false;
    m_bShowMainDialog = true;
    m_ooControlDialogImpl = nullptr;
    m_ooMiniDialogImpl = nullptr;
    m_cursor_lat = 0.0;
    m_cursor_lon = 0.0;
    m_click_lat = 0.0;
    m_click_lon = 0.0;
    m_observationsIndex = -1;
    m_observationsChoiceCount = -1;

    // Adds local language support for the plugin to OCPN
    AddLocaleCatalog(PLUGIN_CATALOG_NAME);

    // Get a pointer to the opencpn display canvas, to use as a parent for windows created
    m_parent_window = wxGetTopLevelParent(GetOCPNCanvasWindow());

    if (!m_parent_window) {
        m_parent_window = GetOCPNCanvasWindow();
    }

    m_pConfig = GetOCPNConfigObject();
    LoadConfig();

#ifdef PLUGIN_USE_SVG
    m_openobserver_button_id  = InsertPlugInToolSVG(_("Open Observer Plugin"), m_ptpicons->m_s_openobserver_grey_pi, m_ptpicons->m_s_openobserver_pi, m_ptpicons->m_s_openobserver_toggled_pi, wxITEM_CHECK,
                                                  _("Open Observer Plugin"), wxS(""), NULL, openobserver_POSITION, 0, this);
#else
    m_openobserver_button_id  = InsertPlugInTool(_("Open Observer Plugin"), &m_ptpicons->m_bm_openobserver_grey_pi, &m_ptpicons->m_bm_openobserver_pi, wxITEM_CHECK,
                                             _("Open Observer Plugin"), wxS(""), NULL, openobserver_POSITION, 0, this);
#endif

    //    In order to avoid an ASSERT on msw debug builds,
    //    we need to create a dummy menu to act as a surrogate parent of the created MenuItems
    //    The Items will be re-parented when added to the real context meenu
    wxMenu dummy_menu;
    wxMenuItem* pmi =
        new wxMenuItem(&dummy_menu, -1, _("Open Observer: add observation here"));
    m_addObservationItem = AddCanvasContextMenuItem(pmi, this);
    SetCanvasContextMenuItemViz(m_addObservationItem, true);

    wxMenuItem* pmiShowMiniPanel =
        new wxMenuItem(&dummy_menu, -1, _("Open Observer: show mini panel"));
    m_showMiniPanelItem = AddCanvasContextMenuItem(pmiShowMiniPanel, this);
    SetCanvasContextMenuItemViz(m_showMiniPanelItem, true);

    wxMenuItem* pmiShowObservationsPanel =
        new wxMenuItem(&dummy_menu, -1, _("Open Observer: show observations table"));
    m_showObservationsPanelItem =
        AddCanvasContextMenuItem(pmiShowObservationsPanel, this);
    SetCanvasContextMenuItemViz(m_showObservationsPanelItem, true);

    // Get item into font list in options/user interface
    AddPersistentFontKey( wxT("tp_Label") );
    AddPersistentFontKey( wxT("tp_Data") );
    g_pFontTitle = GetOCPNScaledFont_PlugIn( wxS("tp_Title") );
    g_pFontLabel = GetOCPNScaledFont_PlugIn( wxS("tp_Label") );
    g_pFontData = GetOCPNScaledFont_PlugIn( wxS("tp_Data") );
    g_pFontSmall = GetOCPNScaledFont_PlugIn( wxS("tp_Small") );
    wxColour l_fontcolour = GetFontColour_PlugIn( wxS("tp_Label") );
    l_fontcolour = GetFontColour_PlugIn( wxS("tp_Data") );

    m_ooObservations = new ooObservations();

    m_ooMiniDialogImpl = new ooMiniDialogImpl(m_parent_window);
    m_ooMiniDialogImpl->Fit();
    m_ooMiniDialogImpl->Layout();
    m_ooMiniDialogImpl->Hide();
    m_ooMiniDialogImpl->Move(m_miniDialogPosition.x, m_miniDialogPosition.y);
    m_ooMiniDialogImpl->SetSize(m_miniDialogPosition.width,
                                m_miniDialogPosition.height);
    if (m_useAuiPanel) {
        m_ooAuiPanel = new ooAuiPanel(m_parent_window);

        wxAuiManager* aui = GetFrameAuiManager();
        if (aui) {
            if (!m_currentProjectName.IsEmpty()) {
                m_ooAuiPanel->SetProjectInfo(m_currentProjectName,
                                             m_currentProjectColor);
            }

            aui->AddPane(m_ooAuiPanel,
                         wxAuiPaneInfo()
                             .Name("OpenObserverAuiPanel")
                             .Caption("Open Observer")
                             .PaneBorder(true)
                             .CaptionVisible(true)
                             .Movable(true)
                             .Resizable(true)
                             .Floatable(true)
                             .Dockable(true)
                             .TopDockable(true)
                             .BottomDockable(true)
                             .LeftDockable(true)
                             .RightDockable(true)
                             .Float()
                             .FloatingPosition(100, 100)
                             .FloatingSize(m_ooAuiPanel->GetMinSize())
                             .CloseButton(true)
                             .Show(true));
            aui->Update();
         }
    }

    if (m_showAuiObservationsPanel) {
        ShowObservationsPanel();
    }

    m_ooControlDialogImpl = new ooControlDialogImpl(m_parent_window);   
    m_ooControlDialogImpl->SetObservationsChoiceCount(m_observationsChoiceCount);
    m_ooControlDialogImpl->CreateObservationsTable(m_ooObservations);

    // restore backup observations and start backing up on a timer
    if (!m_ooControlDialogImpl->RestoreBackupObservations(m_observationsIndex)) {
        m_ooControlDialogImpl->NewProject();
        m_ooControlDialogImpl->UseProject();
    }

    m_ooControlDialogImpl->Fit();
    m_ooControlDialogImpl->Layout();
    m_ooControlDialogImpl->Hide();
    m_ooControlDialogImpl->Move(m_dialogPosition.x, m_dialogPosition.y);
    m_ooControlDialogImpl->SetSize(m_dialogPosition.width,
                                   m_dialogPosition.height);


    return (
        WANTS_CURSOR_LATLON       |
        WANTS_TOOLBAR_CALLBACK    |
        INSTALLS_TOOLBAR_TOOL     |
//        WANTS_CONFIG              |
        INSTALLS_TOOLBOX_PAGE     |
        INSTALLS_CONTEXTMENU_ITEMS  |
        WANTS_NMEA_EVENTS         |
        WANTS_NMEA_SENTENCES        |
        USES_AUI_MANAGER            |
        WANTS_PREFERENCES         |
        WANTS_ONPAINT_VIEWPORT      |
        WANTS_PLUGIN_MESSAGING    |
        WANTS_LATE_INIT           |
        WANTS_MOUSE_EVENTS        |
        WANTS_KEYBOARD_EVENTS
    );
}

void openobserver_pi::LateInit(void)
{
    SendPluginMessage(wxS("OPENOBSERVER_PI_READY_FOR_REQUESTS"), wxS("TRUE"));
    m_bReadyForRequests = true;
    return;
}

bool openobserver_pi::DeInit(void)
{


    if (m_ooControlDialogImpl)
    {
        m_dialogPosition = m_ooControlDialogImpl->GetRect();
        
        m_ooControlDialogImpl->Close();
        delete m_ooControlDialogImpl;
        m_ooControlDialogImpl = nullptr;
    }
    if (m_ooMiniDialogImpl)
    {
        m_miniDialogPosition = m_ooMiniDialogImpl->GetRect();

        m_ooMiniDialogImpl->Close();
        delete m_ooMiniDialogImpl;
        m_ooMiniDialogImpl = nullptr;
    }
    if (m_ooAuiObservationsPanel)
    {
        wxAuiManager* aui = GetFrameAuiManager();
        if (aui) {
            aui->DetachPane(m_ooAuiObservationsPanel);
            aui->Update();
        }

        m_ooAuiObservationsPanel->Destroy();
        m_ooAuiObservationsPanel = nullptr;
    }    
    if (m_ooAuiPanel)
    {
        wxAuiManager* aui = GetFrameAuiManager();
        if (aui) {
            aui->DetachPane(m_ooAuiPanel);
            aui->Update();
        }

        m_ooAuiPanel->Destroy();
        m_ooAuiPanel = nullptr;
    }
    if (m_pConfig) SaveConfig();

    return true;
}

int openobserver_pi::GetAPIVersionMajor()
{
      return OCPN_API_VERSION_MAJOR;
}

int openobserver_pi::GetAPIVersionMinor()
{
      return OCPN_API_VERSION_MINOR;
}

int openobserver_pi::GetPlugInVersionMajor()
{
      return PLUGIN_VERSION_MAJOR;
}

int openobserver_pi::GetPlugInVersionMinor()
{
      return PLUGIN_VERSION_MINOR;
}

int openobserver_pi::GetPlugInVersionPatch()
{
    return PLUGIN_VERSION_PATCH;
}

int openobserver_pi::GetPlugInVersionPost()
{
    return PLUGIN_VERSION_TWEAK;
}

wxString openobserver_pi::GetCommonName()
{
    return _T(PLUGIN_COMMON_NAME);
}

wxString openobserver_pi::GetShortDescription()
{
    return _(PLUGIN_SHORT_DESCRIPTION);
}

wxString openobserver_pi::GetLongDescription()
{
    return _(PLUGIN_LONG_DESCRIPTION);
}

int openobserver_pi::GetToolbarToolCount(void)
{
      return 1;
}

void openobserver_pi::OnToolbarToolCallback(int id)
{
    m_iCallerId = id;
    ToggleToolbarIcon();
}

void openobserver_pi::OnToolbarToolDownCallback(int id)
{
    return;
}

void openobserver_pi::OnToolbarToolUpCallback(int id)
{
    m_ptpicons->SetScaleFactor();
    return;
}

void openobserver_pi::OnContextMenuItemCallback(int id)
{
    if (id == m_showMiniPanelItem) {
        ShowMiniPanel();
        return;
    }

    if (id == m_showObservationsPanelItem) {
        ShowObservationsPanel();
        return;
    }

    if (id != m_addObservationItem) return;
    if (m_ooObservations == nullptr) return;

    if (m_ooObservations->IsObserving()) {
        wxMessageBox("Could not add observation ! An observation is already running.", "Error",
                   wxOK, wxGetActiveWindow());
        return;
    }

    m_ooObservations->AddObservation(m_cursor_lat, m_cursor_lon);
    m_ooObservations->AddMarks(0);

    RefreshObservationDisplay();
}

bool openobserver_pi::KeyboardEventHook( wxKeyEvent &event )
{
    bool bret = false;

    if( event.GetKeyCode() < 128 ) // ASCII
    {
        int key_char = event.GetKeyCode();

        if ( event.ControlDown() )
            key_char -= 64;

        switch( key_char ) {
            case WXK_CONTROL_W: // Ctrl W
                if ( event.ShiftDown() ) { // Shift-Ctrl-W
                    if(event.GetEventType() == wxEVT_KEY_DOWN) {
                        OnToolbarToolDownCallback( m_openobserver_button_id);
                    }
                    bret = true;
                } else bret = false;
                break;
        }
    }
    if(bret) RequestRefresh(m_parent_window);
    return bret;
}

bool openobserver_pi::MouseEventHook( wxMouseEvent &event )
{
    if (event.LeftDown()) {
        m_click_lat = m_cursor_lat;
        m_click_lon = m_cursor_lon;
    }

    return false;
}

void openobserver_pi::SetCursorLatLon(double lat, double lon)
{
    m_cursor_lat = lat;
    m_cursor_lon = lon;
}

void openobserver_pi::SetPositionFix(PlugIn_Position_Fix &pfix)
{
    if (m_ooObservations)
        m_ooObservations->SetPositionFix(pfix.FixTime, pfix.Lat, pfix.Lon);
    if (m_ooControlDialogImpl)
        m_ooControlDialogImpl->SetPositionFix(pfix.FixTime, pfix.Lat, pfix.Lon);
}

void openobserver_pi::SetNMEASentence(wxString& sentence)
{
    if (m_ooObservations)
        m_ooObservations->SetNmeaSentFix(sentence);
    if (m_ooControlDialogImpl)
        m_ooControlDialogImpl->SetNmeaSentence(sentence);
}

void openobserver_pi::SetCurrentViewPort(PlugIn_ViewPort& vp)
{
    if (m_ooControlDialogImpl)
        m_ooControlDialogImpl->SetViewScale(vp.view_scale_ppm);
}

wxBitmap *openobserver_pi::GetPlugInBitmap()
{
    return &m_ptpicons->m_bm_openobserver_pi;
}

void openobserver_pi::RefreshObservationDisplay()
{
    if (m_ooControlDialogImpl) {
        m_ooControlDialogImpl->RefreshObservationsGrid();
    }

    if (m_ooAuiPanel) {
        m_ooAuiPanel->RefreshObservationDisplay();
    }

    if (m_ooAuiObservationsPanel) {
        m_ooAuiObservationsPanel->RefreshObservations();
    }
}

void openobserver_pi::UndockAuiPanel()
{
    if (!m_ooAuiPanel) return;

    wxAuiManager* aui = GetFrameAuiManager();
    if (!aui) return;

    wxAuiPaneInfo& pane = aui->GetPane(m_ooAuiPanel);

    if (pane.IsOk()) {
        pane.Float()
            .FloatingPosition(100, 100)
            .FloatingSize(280, 180)
            .Show(true);
    } else {
        aui->AddPane(m_ooAuiPanel,
                     wxAuiPaneInfo()
                         .Name("OpenObserverAuiPanel")
                         .Caption("Open Observer")
                         .PaneBorder(true)
                         .CaptionVisible(true)
                         .Movable(true)
                         .Resizable(true)
                         .Floatable(true)
                         .Dockable(true)
                         .TopDockable(true)
                         .BottomDockable(true)
                         .LeftDockable(true)
                         .RightDockable(true)
                         .Float()
                         .FloatingPosition(100, 100)
                         .FloatingSize(280, 180)
                         .CloseButton(true)
                         .Show(true));
    }

    m_ooAuiPanel->RefreshObservationDisplay();
    m_ooAuiPanel->Show();
    aui->Update();
    m_ooAuiPanel->Raise();
}

void openobserver_pi::UndockAuiObservationsPanel()
{
    if (!m_ooAuiObservationsPanel) return;

    wxAuiManager* aui = GetFrameAuiManager();
    if (!aui) return;

    wxAuiPaneInfo& pane = aui->GetPane(m_ooAuiObservationsPanel);

    if (pane.IsOk()) {
        pane.Float()
            .FloatingPosition(140, 140)
            .FloatingSize(900, 420)
            .Show(true);
    } else {
        aui->AddPane(m_ooAuiObservationsPanel,
                     wxAuiPaneInfo()
                         .Name("OpenObserverAuiObservationsPanel")
                         .Caption("Open Observer Observations")
                         .PaneBorder(true)
                         .CaptionVisible(true)
                         .Movable(true)
                         .Resizable(true)
                         .Floatable(true)
                         .Dockable(true)
                         .TopDockable(true)
                         .BottomDockable(true)
                         .LeftDockable(true)
                         .RightDockable(true)
                         .Float()
                         .FloatingPosition(140, 140)
                         .FloatingSize(900, 420)
                         .CloseButton(true)
                         .Show(true));
    }

    m_showAuiObservationsPanel = true;
    m_ooAuiObservationsPanel->RefreshObservations();
    m_ooAuiObservationsPanel->Show();
    aui->Update();
    m_ooAuiObservationsPanel->Raise();
    SaveConfig();
}

void openobserver_pi::ShowMiniPanel()
{
    if (m_useAuiPanel) {
        wxAuiManager* aui = GetFrameAuiManager();
        if (!aui) return;

        if (!m_ooAuiPanel) {
            m_ooAuiPanel = new ooAuiPanel(m_parent_window);

            if (!m_currentProjectName.IsEmpty()) {
                m_ooAuiPanel->SetProjectInfo(m_currentProjectName,
                                             m_currentProjectColor);
            }

            aui->AddPane(m_ooAuiPanel,
                         wxAuiPaneInfo()
                             .Name("OpenObserverAuiPanel")
                             .Caption("Open Observer")
                             .PaneBorder(true)
                             .CaptionVisible(true)
                             .Movable(true)
                             .Resizable(true)
                             .Floatable(true)
                             .Dockable(true)
                             .TopDockable(true)
                             .BottomDockable(true)
                             .LeftDockable(true)
                             .RightDockable(true)
                             .Float()
                             .FloatingPosition(100, 100)
                             .FloatingSize(m_ooAuiPanel->GetMinSize())
                             .CloseButton(true)
                             .Show(true));
        } else {
            wxAuiPaneInfo& pane = aui->GetPane(m_ooAuiPanel);

            if (pane.IsOk()) {
                pane.Show(true);
            } else {
                aui->AddPane(m_ooAuiPanel,
                             wxAuiPaneInfo()
                                 .Name("OpenObserverAuiPanel")
                                 .Caption("Open Observer")
                                 .PaneBorder(true)
                                 .CaptionVisible(true)
                                 .Movable(true)
                                 .Resizable(true)
                                 .Floatable(true)
                                 .Dockable(true)
                                 .TopDockable(true)
                                 .BottomDockable(true)
                                 .LeftDockable(true)
                                 .RightDockable(true)
                                 .Float()
                                 .FloatingPosition(100, 100)
                                 .FloatingSize(m_ooAuiPanel->GetMinSize())
                                 .CloseButton(true)
                                 .Show(true));
            }
        }

        m_ooAuiPanel->Show();
        m_ooAuiPanel->RefreshObservationDisplay();
        aui->Update();
        m_ooAuiPanel->Raise();
        return;
    }

    if (!m_ooControlDialogImpl || !m_ooMiniDialogImpl) return;

    m_ooControlDialogImpl->Hide();
    m_ooMiniDialogImpl->Show();
    m_ooMiniDialogImpl->Raise();
    m_bShowMainDialog = false;
    SetToolbarItemState(m_openobserver_button_id, true);
}

void openobserver_pi::ShowObservationsPanel()
{
    wxAuiManager* aui = GetFrameAuiManager();
    if (!aui) return;

    if (!m_ooAuiObservationsPanel) {
        m_ooAuiObservationsPanel = new ooAuiObservationsPanel(m_parent_window);
        m_ooAuiObservationsPanel->SetObservations(m_ooObservations);

        aui->AddPane(m_ooAuiObservationsPanel,
                     wxAuiPaneInfo()
                         .Name("OpenObserverAuiObservationsPanel")
                         .Caption("Open Observer Observations")
                         .PaneBorder(true)
                         .CaptionVisible(true)
                         .Movable(true)
                         .Resizable(true)
                         .Floatable(true)
                         .Dockable(true)
                         .TopDockable(true)
                         .BottomDockable(true)
                         .LeftDockable(true)
                         .RightDockable(true)
                         .Float()
                         .FloatingPosition(140, 140)
                         .FloatingSize(900, 420)
                         .CloseButton(true)
                         .Show(true));
    } else {
        wxAuiPaneInfo& pane = aui->GetPane(m_ooAuiObservationsPanel);

        if (pane.IsOk()) {
            pane.Show(true);
        } else {
            aui->AddPane(m_ooAuiObservationsPanel,
                         wxAuiPaneInfo()
                             .Name("OpenObserverAuiObservationsPanel")
                             .Caption("Open Observer Observations")
                             .PaneBorder(true)
                             .CaptionVisible(true)
                             .Movable(true)
                             .Resizable(true)
                             .Floatable(true)
                             .Dockable(true)
                             .TopDockable(true)
                             .BottomDockable(true)
                             .LeftDockable(true)
                             .RightDockable(true)
                             .Float()
                             .FloatingPosition(140, 140)
                             .FloatingSize(900, 420)
                             .CloseButton(true)
                             .Show(true));
        }

        m_ooAuiObservationsPanel->SetObservations(m_ooObservations);
    }

    m_ooAuiObservationsPanel->RefreshObservations();
    m_ooAuiObservationsPanel->Show();
    aui->Update();
    m_ooAuiObservationsPanel->Raise();
}

void openobserver_pi::SetProject(const wxString& projectName, const wxColor& projectColor, int observationsIndex)
{
    m_observationsIndex = observationsIndex;
    m_currentProjectName = projectName;
    m_currentProjectColor = projectColor;
    
    wxString title = "Open Observer - " + projectName;
    m_ooControlDialogImpl->SetTitle(title);
    m_ooMiniDialogImpl->SetTitle(title);
    m_ooMiniDialogImpl->SetProjectInfo(projectName, projectColor);

    if (m_ooAuiPanel) {
        m_ooAuiPanel->SetProjectInfo(projectName, projectColor);
    }
    if (m_ooAuiObservationsPanel) {
        m_ooAuiObservationsPanel->RefreshObservations();
    }    
}



void openobserver_pi::ShowPreferencesDialog(wxWindow *parent)
{
    wxDialog dialog(parent, wxID_ANY, _("Open Observer Preferences"),
                    wxDefaultPosition, wxDefaultSize,
                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    wxCheckBox* useAuiPanelCheckBox =
        new wxCheckBox(&dialog, wxID_ANY, _("Use dockable mini panel"));
    useAuiPanelCheckBox->SetValue(m_useAuiPanel);

    wxCheckBox* showAuiObservationsPanelCheckBox =
        new wxCheckBox(&dialog, wxID_ANY, _("Use dockable observations table"));
    showAuiObservationsPanelCheckBox->SetValue(m_showAuiObservationsPanel);

    wxStaticText* infoText = new wxStaticText(
        &dialog,
        wxID_ANY,
        _("This display mode can be changed immediately."));

    topSizer->Add(useAuiPanelCheckBox, 0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 10);
    topSizer->Add(showAuiObservationsPanelCheckBox, 0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 10);
    topSizer->Add(infoText, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM | wxEXPAND, 10);

    wxStdDialogButtonSizer* buttonSizer =
        dialog.CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    topSizer->Add(buttonSizer, 0, wxALL | wxALIGN_RIGHT, 10);

    dialog.SetSizerAndFit(topSizer);

    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    bool newUseAuiPanel = useAuiPanelCheckBox->GetValue();
    bool newShowAuiObservationsPanel =
        showAuiObservationsPanelCheckBox->GetValue();

    const bool useAuiPanelChanged = (newUseAuiPanel != m_useAuiPanel);
    const bool showAuiObservationsPanelChanged =
        (newShowAuiObservationsPanel != m_showAuiObservationsPanel);

    if (!useAuiPanelChanged && !showAuiObservationsPanelChanged) {
        SaveConfig();
        return;
    }

    m_useAuiPanel = newUseAuiPanel;
    m_showAuiObservationsPanel = newShowAuiObservationsPanel;

    if (m_useAuiPanel) {
        if (!m_ooAuiPanel) {
            m_ooAuiPanel = new ooAuiPanel(m_parent_window);

            wxAuiManager* aui = GetFrameAuiManager();
            if (aui) {
                if (!m_currentProjectName.IsEmpty()) {
                    m_ooAuiPanel->SetProjectInfo(m_currentProjectName,
                                                 m_currentProjectColor);
                }

                aui->AddPane(m_ooAuiPanel,
                             wxAuiPaneInfo()
                                 .Name("OpenObserverAuiPanel")
                                 .Caption("Open Observer")
                                 .PaneBorder(true)
                                 .CaptionVisible(true)
                                 .Movable(true)
                                 .Resizable(true)
                                 .Floatable(true)
                                 .Dockable(true)
                                 .TopDockable(true)
                                 .BottomDockable(true)
                                 .LeftDockable(true)
                                 .RightDockable(true)
                                 .Float()
                                 .FloatingPosition(100, 100)
                                 .FloatingSize(m_ooAuiPanel->GetMinSize())
                                 .CloseButton(true)
                                 .Show(true));
                aui->Update();
            }
        }
    } else {
        if (m_ooAuiPanel) {
            wxAuiManager* aui = GetFrameAuiManager();
            if (aui) {
                aui->DetachPane(m_ooAuiPanel);
                aui->Update();
            }

            m_ooAuiPanel->Destroy();
            m_ooAuiPanel = nullptr;
        }
    }

    if (showAuiObservationsPanelChanged) {
        if (m_showAuiObservationsPanel) {
            ShowObservationsPanel();
        } else {
            if (m_ooAuiObservationsPanel) {
                wxAuiManager* aui = GetFrameAuiManager();
                if (aui) {
                    aui->DetachPane(m_ooAuiObservationsPanel);
                    aui->Update();
                }

                m_ooAuiObservationsPanel->Destroy();
                m_ooAuiObservationsPanel = nullptr;
            }
        }
    }

    SaveConfig();
}

void openobserver_pi::ToggleToolbarIcon()
{
    if (!m_ooControlDialogImpl || !m_ooMiniDialogImpl) return;

    if (m_ooControlDialogImpl->IsShown() || m_ooMiniDialogImpl->IsShown()) {
        SetToolbarItemState(m_openobserver_button_id, false);
        m_ooControlDialogImpl->Hide();
        m_ooMiniDialogImpl->Hide();
    } else {
        SetToolbarItemState(m_openobserver_button_id, true);

        if (m_bShowMainDialog) {
            m_ooMiniDialogImpl->Hide();
            m_ooControlDialogImpl->Show();
            m_ooControlDialogImpl->Raise();
        } else {
            m_ooControlDialogImpl->Hide();
            m_ooMiniDialogImpl->Show();
            m_ooMiniDialogImpl->Raise();
        }
    }
}

void openobserver_pi::ToggleWindow()
{
    if (!m_ooControlDialogImpl || !m_ooMiniDialogImpl) return;

    if (m_ooControlDialogImpl->IsShown()) {
        m_ooControlDialogImpl->Hide();
        m_ooMiniDialogImpl->Show();
        m_ooMiniDialogImpl->Raise();
        m_bShowMainDialog = false;
    } else {
        m_ooMiniDialogImpl->Hide();
        m_ooControlDialogImpl->Show();
        m_ooControlDialogImpl->Raise();
        m_bShowMainDialog = true;
    }
}

void openobserver_pi::SaveConfig()
{
    #ifndef __WXMSW__
    wxSetlocale(LC_NUMERIC, NULL);
    #if wxCHECK_VERSION(3,0,0)  && !defined(_WXMSW_)
    //#if wxCHECK_VERSION(3,0,0)
    wxSetlocale(LC_NUMERIC, "C");
    #else
    setlocale(LC_NUMERIC, "C");
    #endif
    #endif

    if (!m_pConfig) return;

    // section in the main OpenCPN setting file (Mac ~/Library/preferences/opencpn/opencpn.ini)
    m_pConfig->SetPath("/Settings/openobserver_pi");
    m_pConfig->DeleteEntry("ProjectFile");
    m_pConfig->Write("ObservationsIndex", m_observationsIndex);
    m_pConfig->Write("ObservationsChoiceCount", m_observationsChoiceCount);
    m_pConfig->Write("DialogX", m_dialogPosition.x);
    m_pConfig->Write("DialogY", m_dialogPosition.y);
    m_pConfig->Write("DialogWidth", m_dialogPosition.width);
    m_pConfig->Write("DialogHeight", m_dialogPosition.height);
    m_pConfig->Write("MiniDialogX", m_miniDialogPosition.x);
    m_pConfig->Write("MiniDialogY", m_miniDialogPosition.y);
    m_pConfig->Write("MiniDialogWidth", m_miniDialogPosition.width);
    m_pConfig->Write("MiniDialogHeight", m_miniDialogPosition.height);
    m_pConfig->Write("UseAuiPanel", m_useAuiPanel);
    m_pConfig->Write("ShowAuiObservationsPanel", m_showAuiObservationsPanel);
}

void openobserver_pi::LoadConfig()
{
    #ifndef __WXMSW__
    wxSetlocale(LC_NUMERIC, NULL);
    #if wxCHECK_VERSION(3,0,0)
    wxSetlocale(LC_NUMERIC, "C");
    #else
    setlocale(LC_NUMERIC, "C");
    #endif
    #endif

    if (!m_pConfig) return;

    m_pConfig->SetPath("/Settings/openobserver_pi");
    m_pConfig->Read("ObservationsIndex", &m_observationsIndex, -1);
    m_pConfig->Read("ObservationsChoiceCount", &m_observationsChoiceCount, 5);

    if (m_observationsChoiceCount < 5) {
    m_observationsChoiceCount = 5;
    }
    m_pConfig->Read("DialogX", &m_dialogPosition.x, -1);
    m_pConfig->Read("DialogY", &m_dialogPosition.y, -1);
    m_pConfig->Read("DialogWidth", &m_dialogPosition.width, -1);
    m_pConfig->Read("DialogHeight", &m_dialogPosition.height, -1);
    m_pConfig->Read("MiniDialogX", &m_miniDialogPosition.x, -1);
    m_pConfig->Read("MiniDialogY", &m_miniDialogPosition.y, -1);
    m_pConfig->Read("MiniDialogWidth", &m_miniDialogPosition.width, -1);
    m_pConfig->Read("MiniDialogHeight", &m_miniDialogPosition.height, -1);
    m_pConfig->Read("UseAuiPanel", &m_useAuiPanel, false);
    m_pConfig->Read("ShowAuiObservationsPanel", &m_showAuiObservationsPanel, false);
}

static wxString GetHumanReadableNmeaFieldDescription(const wxString& sentenceId, int fieldIndex)
{
    wxString label;

    if (sentenceId == "RMC") {
        if (fieldIndex == 1) label = "UTC time";
        else if (fieldIndex == 2) label = "Navigation status";
        else if (fieldIndex == 3) label = "GPS latitude";
        else if (fieldIndex == 4) label = "Latitude hemisphere";
        else if (fieldIndex == 5) label = "GPS longitude";
        else if (fieldIndex == 6) label = "Longitude hemisphere";
        else if (fieldIndex == 7) label = "GPS speed over ground";
        else if (fieldIndex == 8) label = "GPS course over ground";
        else if (fieldIndex == 9) label = "UTC date";
        else if (fieldIndex == 10) label = "Magnetic variation";
        else if (fieldIndex == 11) label = "Magnetic variation direction";
        else if (fieldIndex == 12) label = "Positioning system mode";
    } else if (sentenceId == "GGA") {
        if (fieldIndex == 1) label = "UTC time";
        else if (fieldIndex == 2) label = "GPS latitude";
        else if (fieldIndex == 3) label = "Latitude hemisphere";
        else if (fieldIndex == 4) label = "GPS longitude";
        else if (fieldIndex == 5) label = "Longitude hemisphere";
        else if (fieldIndex == 6) label = "GPS fix quality";
        else if (fieldIndex == 7) label = "Satellites in use";
        else if (fieldIndex == 8) label = "Horizontal dilution of precision";
        else if (fieldIndex == 9) label = "Altitude";
        else if (fieldIndex == 11) label = "Geoid separation";
        else if (fieldIndex == 13) label = "DGPS data age";
        else if (fieldIndex == 14) label = "DGPS station ID";
    } else if (sentenceId == "GLL") {
        if (fieldIndex == 1) label = "GPS latitude";
        else if (fieldIndex == 2) label = "Latitude hemisphere";
        else if (fieldIndex == 3) label = "GPS longitude";
        else if (fieldIndex == 4) label = "Longitude hemisphere";
        else if (fieldIndex == 5) label = "UTC time";
        else if (fieldIndex == 6) label = "Navigation status";
        else if (fieldIndex == 7) label = "Positioning system mode";
    } else if (sentenceId == "VTG") {
        if (fieldIndex == 1) label = "GPS course over ground true";
        else if (fieldIndex == 3) label = "GPS course over ground magnetic";
        else if (fieldIndex == 5) label = "GPS speed over ground knots";
        else if (fieldIndex == 7) label = "GPS speed over ground km/h";
        else if (fieldIndex == 9) label = "Positioning system mode";
    } else if (sentenceId == "MWV") {
        if (fieldIndex == 1) label = "Apparent wind angle";
        else if (fieldIndex == 2) label = "Wind angle reference";
        else if (fieldIndex == 3) label = "Apparent wind speed";
        else if (fieldIndex == 4) label = "Wind speed unit";
        else if (fieldIndex == 5) label = "Wind data status";
    } else if (sentenceId == "HDG") {
        if (fieldIndex == 1) label = "Magnetic heading";
        else if (fieldIndex == 2) label = "Magnetic deviation";
        else if (fieldIndex == 3) label = "Magnetic deviation direction";
        else if (fieldIndex == 4) label = "Magnetic variation";
        else if (fieldIndex == 5) label = "Magnetic variation direction";
    } else if (sentenceId == "GSA") {
        if (fieldIndex == 1) label = "GPS selection mode";
        else if (fieldIndex == 2) label = "GPS fix type";
        else if (fieldIndex >= 3 && fieldIndex <= 14) label = "Satellite PRN used for fix";
        else if (fieldIndex == 15) label = "Position dilution of precision";
        else if (fieldIndex == 16) label = "Horizontal dilution of precision";
        else if (fieldIndex == 17) label = "Vertical dilution of precision";
    } else if (sentenceId == "GSV") {
        if (fieldIndex == 1) label = "GSV total messages";
        else if (fieldIndex == 2) label = "GSV message number";
        else if (fieldIndex == 3) label = "Satellites in view";
        else label = "Satellite view detail";
    } else if (sentenceId == "GST") {
        if (fieldIndex == 1) label = "UTC time";
        else if (fieldIndex == 2) label = "RMS range residual";
        else if (fieldIndex == 3) label = "Error ellipse semi-major axis";
        else if (fieldIndex == 4) label = "Error ellipse semi-minor axis";
        else if (fieldIndex == 5) label = "Error ellipse orientation";
        else if (fieldIndex == 6) label = "Latitude error";
        else if (fieldIndex == 7) label = "Longitude error";
        else if (fieldIndex == 8) label = "Altitude error";
    }

    if (label.IsEmpty()) {
        label = wxString::Format(wxT("NMEA %s field %i"), sentenceId, fieldIndex);
    }

    return wxString::Format(wxT("%s [%s:%i]"), label, sentenceId, fieldIndex);
}

static int GetNmeaFieldSortPriority(const wxString& sentenceId, int fieldIndex)
{
    // Time
    if (sentenceId == "RMC" && fieldIndex == 9) return 10;  // UTC date
    if (sentenceId == "RMC" && fieldIndex == 1) return 20;  // UTC time
    if (sentenceId == "GGA" && fieldIndex == 1) return 21;  // UTC time
    if (sentenceId == "GLL" && fieldIndex == 5) return 22;  // UTC time
    if (sentenceId == "ZDA" && fieldIndex == 1) return 23;  // UTC time

    // Position
    if (sentenceId == "RMC" && fieldIndex == 3) return 30;  // GPS latitude
    if (sentenceId == "GGA" && fieldIndex == 2) return 31;  // GPS latitude
    if (sentenceId == "GLL" && fieldIndex == 1) return 32;  // GPS latitude

    if (sentenceId == "RMC" && fieldIndex == 5) return 40;  // GPS longitude
    if (sentenceId == "GGA" && fieldIndex == 4) return 41;  // GPS longitude
    if (sentenceId == "GLL" && fieldIndex == 3) return 42;  // GPS longitude

    // Movement
    if (sentenceId == "RMC" && fieldIndex == 7) return 50;  // SOG
    if (sentenceId == "VTG" && fieldIndex == 5) return 51;  // SOG knots
    if (sentenceId == "VTG" && fieldIndex == 7) return 52;  // SOG km/h

    if (sentenceId == "RMC" && fieldIndex == 8) return 60;  // COG
    if (sentenceId == "VTG" && fieldIndex == 1) return 61;  // COG true
    if (sentenceId == "VTG" && fieldIndex == 3) return 62;  // COG magnetic

    // Heading
    if (sentenceId == "HDG" && fieldIndex == 1) return 70;  // Magnetic heading
    if (sentenceId == "HDT" && fieldIndex == 1) return 71;  // True heading
    if (sentenceId == "HDM" && fieldIndex == 1) return 72;  // Magnetic heading

    // Wind
    if (sentenceId == "MWV" && fieldIndex == 1) return 80;  // Apparent wind angle
    if (sentenceId == "MWV" && fieldIndex == 3) return 90;  // Apparent wind speed
    if (sentenceId == "MWD" && fieldIndex == 1) return 91;  // True wind direction
    if (sentenceId == "MWD" && fieldIndex == 5) return 92;  // Wind speed knots

    // GPS quality
    if (sentenceId == "RMC" && fieldIndex == 2) return 100; // Navigation status
    if (sentenceId == "GLL" && fieldIndex == 6) return 101; // Navigation status
    if (sentenceId == "GGA" && fieldIndex == 6) return 110; // GPS fix quality
    if (sentenceId == "GGA" && fieldIndex == 7) return 120; // Satellites in use
    if (sentenceId == "GGA" && fieldIndex == 8) return 130; // HDOP
    if (sentenceId == "GSA" && fieldIndex == 15) return 131; // PDOP
    if (sentenceId == "GSA" && fieldIndex == 16) return 132; // HDOP
    if (sentenceId == "GSA" && fieldIndex == 17) return 133; // VDOP

    // Altitude / depth / temperature
    if (sentenceId == "GGA" && fieldIndex == 9) return 140;  // Altitude
    if (sentenceId == "DPT" && fieldIndex == 1) return 150;  // Water depth
    if (sentenceId == "DBT" && fieldIndex == 3) return 151;  // Depth meters
    if (sentenceId == "MTW" && fieldIndex == 1) return 160;  // Water temperature

    // Advanced or unknown scanned fields.
    return 1000;
}

void openobserver_pi::WriteNmeaXML(const std::unordered_map<wxString, std::set<int>>& scannedNmeaFields)
{
    const wxString filePath =
      wxFileName(*g_PrivateDataDir, "NMEAFields.xml").GetFullPath();
    
    wxXmlDocument xmlDoc;
    
    wxXmlNode* root = new wxXmlNode(NULL, wxXML_ELEMENT_NODE, "nmea_export");
    root->AddAttribute("creator", "Open Observer for OpenCPN");
    xmlDoc.SetRoot(root);

    // Sentence IDs
    for (const auto& sentencePair : scannedNmeaFields) {
        wxString sentenceId = sentencePair.first;
        if (sentenceId.IsEmpty()) continue;
        
        wxXmlNode* sentenceNode = new wxXmlNode(NULL, wxXML_ELEMENT_NODE, "sentence");
        sentenceNode->AddAttribute("id", sentenceId);

        // Field indexes
        for (int index : sentencePair.second) {
            wxXmlNode* fieldNode = new wxXmlNode(NULL, wxXML_ELEMENT_NODE, "field");
            fieldNode->AddAttribute("index", wxString::Format("%d", index));
            
            wxXmlNode* textNode = new wxXmlNode(
                wxXML_TEXT_NODE,
                "",
                GetHumanReadableNmeaFieldDescription(sentenceId, index)
            );

            
            fieldNode->AddChild(textNode);
            sentenceNode->AddChild(fieldNode);
        }
        
        root->AddChild(sentenceNode);
    }
    
    xmlDoc.Save(filePath);
}

// Read NMEA XML file and return list of NMEA Fields
std::vector<NMEAField> openobserver_pi::ReadNmeaXML()
{
    const wxString filePath =
      wxFileName(*g_PrivateDataDir, "NMEAFields.xml").GetFullPath();

    std::vector<NMEAField> res;

    wxXmlDocument xmlDoc;
    if (!xmlDoc.Load(filePath)) {
        wxLogError("Cannont read NMEA XML : %s", filePath);
        return res;
    }

    wxXmlNode* root = xmlDoc.GetRoot();
    if (!root || root->GetName() != "nmea_export") {
        wxLogError("Invalid NMEA XML file : %s", filePath);
        return res;
    }

    for (wxXmlNode* sentenceNode = root->GetChildren(); sentenceNode != NULL; sentenceNode = sentenceNode->GetNext()) {
        if (sentenceNode->GetName() != "sentence") continue;

        wxString sentenceId = sentenceNode->GetAttribute("id", "");
        if (sentenceId.IsEmpty()) continue;

        for (wxXmlNode* fieldNode = sentenceNode->GetChildren(); fieldNode != NULL; fieldNode = fieldNode->GetNext()) {
            if (fieldNode->GetName() != "field") continue;

            wxString indexStr = fieldNode->GetAttribute("index", "");
            long fieldIndex = 0;
            if (!indexStr.ToLong(&fieldIndex)) continue;

            wxString description;
            if (fieldNode->GetChildren()) {
                description = fieldNode->GetChildren()->GetContent();
            }
            if (description.IsEmpty()) {
                description = GetHumanReadableNmeaFieldDescription(sentenceId, (int)fieldIndex);
            }

            res.push_back({sentenceId, (int)fieldIndex, description, ""});
        }
    }

    std::sort(res.begin(), res.end(), [](const NMEAField& a, const NMEAField& b) {
        const int priorityA = GetNmeaFieldSortPriority(a.m_sentenceId, a.m_fieldIndex);
        const int priorityB = GetNmeaFieldSortPriority(b.m_sentenceId, b.m_fieldIndex);

        if (priorityA != priorityB) {
            return priorityA < priorityB;
        }

        if (a.m_sentenceId != b.m_sentenceId) {
            return a.m_sentenceId < b.m_sentenceId;
        }

        return a.m_fieldIndex < b.m_fieldIndex;
    });

    return res;
}

void openobserver_pi::RefreshListings()
{
    ooObservations::ClearListings();
    wxArrayString allListings;
    wxDir::GetAllFiles(*g_pListingDir, &allListings);
    for (auto f : allListings) {
        wxArrayString items, icons;
        if (ooObservations::ReadListingFromXML(f, items, icons)) {
            wxString filename = wxFileName(f).GetName();
            ooObservations::AddListing(filename, items);
            if (icons.GetCount() > 0) ooObservations::SetIcons(filename, icons);
        }
    }
}
