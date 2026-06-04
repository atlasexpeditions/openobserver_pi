/**************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Open Observer Plugin Control Dialog
 * Author:   Jon Gough
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

#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>
#include <wx/file.h>
#include <wx/checklst.h>
#include <wx/textfile.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/dir.h>

#include <wx/xml/xml.h>

#include <wx/base64.h>
#include <wx/mstream.h>

#include "ooControlDialogImpl.h"
#include "ooScientificPackage.h"
#include "openobserver_pi.h"

#include "ocpn_plugin.h"

#if wxCHECK_VERSION(3,0,0)
#include <wx/valnum.h>
#endif

#include <wx/fontdlg.h>
#include <wx/settings.h>

extern openobserver_pi *g_openobserver_pi;
extern wxString *g_pData;
extern wxString *g_PrivateDataDir;

static bool IsDarkColour(const wxColour& colour);
static wxColour BlendColour(const wxColour& base, const wxColour& overlay, double ratio);
static wxColour ContrastTextColour(const wxColour& background);
static wxColour AlternateRowColour(const wxColour& base);
static void ApplyStandardBoldGridLabelFont(wxGrid* grid);

static wxString GetDefaultProjectTemplatePath()
{
    if (!g_PrivateDataDir || g_PrivateDataDir->IsEmpty()) {
        return wxEmptyString;
    }

    wxFileName templateFile(*g_PrivateDataDir, wxEmptyString);
    templateFile.AppendDir("ProjectTemplates");
    templateFile.SetFullName("Default.xml");

    return templateFile.GetFullPath();
}

static ooProject CreateFallbackDefaultProject()
{
    wxArrayString labels;
    wxArrayString fieldTypes;
    wxGridSizesInfo colSizes;

    labels.Add("Start timestamp");
    fieldTypes.Add("Start Timestamp UTC");
    colSizes.m_customSizes[0] = 200;

    labels.Add("Lat");
    fieldTypes.Add("Start Latitude");
    colSizes.m_customSizes[1] = 90;

    labels.Add("Lon");
    fieldTypes.Add("Start Longitude");
    colSizes.m_customSizes[2] = 90;

    labels.Add("Notes");
    fieldTypes.Add("Text");
    colSizes.m_customSizes[3] = 240;

    labels.Add("Mark GUID");
    fieldTypes.Add("Mark GUID");
    colSizes.m_customSizes[4] = 0;

    return ooProject(
        "Default Project",
        "Minimal fallback project used when the external Default project template is not available.",
        colSizes,
        fieldTypes,
        labels,
        DEFAULT_PROJECT_COLOUR,
        DEFAULT_PROJECT_ICON);
}

static wxString JoinControlPath(const wxString& a, const wxString& b)
{
    wxFileName fn(a, b);
    return fn.GetFullPath();
}

static wxXmlNode* FindControlXmlChild(wxXmlNode* parent, const wxString& name)
{
    if (!parent) return nullptr;

    wxXmlNode* child = parent->GetChildren();
    while (child) {
        if (child->GetName() == name) {
            return child;
        }

        child = child->GetNext();
    }

    return nullptr;
}

static int FindObservationColumnByFieldTypeOrLabel(
    ooObservations* observations,
    const wxString& fieldType,
    const wxArrayString& acceptedLabels = wxArrayString())
{
    if (!observations) {
        return wxNOT_FOUND;
    }

    const int columnCount = observations->GetColsCount();
    const wxArrayString fieldTypes = observations->GetColFieldTypes();
    const wxArrayString labels = observations->GetProject().GetColLabels();

    for (int c = 0; c < columnCount; ++c) {
        if (c < (int)fieldTypes.GetCount() &&
            fieldTypes[c].IsSameAs(fieldType)) {
            return c;
        }

        if (c < (int)labels.GetCount()) {
            for (size_t i = 0; i < acceptedLabels.GetCount(); ++i) {
                if (labels[c].IsSameAs(acceptedLabels[i], false)) {
                    return c;
                }
            }
        }
    }

    return wxNOT_FOUND;
}

static wxString FindNmeaRecordingPathByName(const wxString& recordingName)
{
    if (!g_PrivateDataDir || g_PrivateDataDir->IsEmpty() || recordingName.IsEmpty()) {
        return wxEmptyString;
    }

    wxFileName recordingFile(recordingName);
    const wxString cleanRecordingName = recordingFile.GetFullName();

    if (!cleanRecordingName.StartsWith("NMEA-") ||
        !cleanRecordingName.Lower().EndsWith(".nmea") ||
        cleanRecordingName.Length() < 18) {
        return wxEmptyString;
    }

    const wxString compactDate = cleanRecordingName.Mid(5, 8);

    if (compactDate.Length() != 8) {
        return wxEmptyString;
    }

    const wxString dayFolderName =
        compactDate.Mid(0, 4) + "-" +
        compactDate.Mid(4, 2) + "-" +
        compactDate.Mid(6, 2);

    const wxString recordingPath = JoinControlPath(
        JoinControlPath(
            JoinControlPath(*g_PrivateDataDir, "NMEArecordings"),
            dayFolderName),
        cleanRecordingName);

    if (wxFileExists(recordingPath)) {
        return recordingPath;
    }

    return wxEmptyString;
}

static wxArrayString CollectNmeaRecordingPathsForRows(
    ooObservations* observations,
    const wxArrayInt& selectedRows)
{
    wxArrayString recordingPaths;

    if (!observations || !g_PrivateDataDir || g_PrivateDataDir->IsEmpty()) {
        return recordingPaths;
    }

    wxArrayString recordingLabels;
    recordingLabels.Add("NMEA Recording");
    recordingLabels.Add("NMEA REC");
    recordingLabels.Add("NMEA record");
    recordingLabels.Add("NMEA recording");

    const int recordingCol = FindObservationColumnByFieldTypeOrLabel(
        observations,
        "NMEA Recording",
        recordingLabels);

    if (recordingCol == wxNOT_FOUND) {
        return recordingPaths;
    }

    for (int i = 0; i < (int)selectedRows.GetCount(); ++i) {
        const int row = selectedRows[i];

        if (row < 0 || row >= observations->GetNumberRows()) {
            continue;
        }

        wxString recordingValue = observations->GetValue(row, recordingCol);
        recordingValue.Trim(true);
        recordingValue.Trim(false);

        if (recordingValue.IsEmpty() ||
            recordingValue.Lower().StartsWith("no data")) {
            continue;
        }

        wxFileName recordingFile(recordingValue);
        const wxString recordingName = recordingFile.GetFullName();

        if (recordingName.IsEmpty() ||
            !recordingName.Lower().EndsWith(".nmea")) {
            continue;
        }

        const wxString recordingPath = FindNmeaRecordingPathByName(recordingName);

        if (!recordingPath.IsEmpty() &&
            wxFileExists(recordingPath) &&
            recordingPaths.Index(recordingPath) == wxNOT_FOUND) {
            recordingPaths.Add(recordingPath);
        }
    }

    return recordingPaths;
}

static int DeleteExistingFiles(const wxArrayString& filePaths)
{
    int deletedCount = 0;

    for (size_t i = 0; i < filePaths.GetCount(); ++i) {
        if (wxFileExists(filePaths[i]) && wxRemoveFile(filePaths[i])) {
            deletedCount++;
        }
    }

    return deletedCount;
}

static void AddReferencedNmeaRecordingName(
    const wxString& value,
    wxArrayString& recordingNames)
{
    wxString cleanedValue = value;
    cleanedValue.Trim(true);
    cleanedValue.Trim(false);

    if (cleanedValue.IsEmpty() || cleanedValue.Lower().StartsWith("no data")) {
        return;
    }

    wxFileName recordingFile(cleanedValue);
    const wxString recordingName = recordingFile.GetFullName();

    if (!recordingName.IsEmpty() &&
        recordingName.Lower().EndsWith(".nmea") &&
        recordingNames.Index(recordingName) == wxNOT_FOUND) {
        recordingNames.Add(recordingName);
    }
}

static void CollectReferencedNmeaRecordingNamesFromTable(
    ooObservations* observations,
    wxArrayString& recordingNames)
{
    if (!observations) return;

    wxArrayString recordingLabels;
    recordingLabels.Add("NMEA Recording");
    recordingLabels.Add("NMEA REC");
    recordingLabels.Add("NMEA record");
    recordingLabels.Add("NMEA recording");

    const int recordingCol = FindObservationColumnByFieldTypeOrLabel(
        observations,
        "NMEA Recording",
        recordingLabels);

    if (recordingCol == wxNOT_FOUND) return;

    for (int row = 0; row < observations->GetNumberRows(); ++row) {
        AddReferencedNmeaRecordingName(
            observations->GetValue(row, recordingCol),
            recordingNames);
    }
}

static void CollectReferencedNmeaRecordingNamesFromXml(
    const wxString& xmlPath,
    wxArrayString& recordingNames)
{
    wxXmlDocument xmlDoc;
    if (!xmlDoc.Load(xmlPath)) return;

    wxXmlNode* root = xmlDoc.GetRoot();
    if (!root || root->GetName() != "observations") return;

    wxXmlNode* projectNode = FindControlXmlChild(root, "project");
    if (!projectNode) return;

    wxArrayInt recordingFieldIds;

    wxXmlNode* projectField = projectNode->GetChildren();
    while (projectField) {
        if (projectField->GetName() == "field" &&
            projectField->GetAttribute("field_type").IsSameAs("NMEA Recording")) {
            long id = -1;
            if (projectField->GetAttribute("id").ToLong(&id) && id >= 0) {
                recordingFieldIds.Add((int)id);
            }
        }

        projectField = projectField->GetNext();
    }

    if (recordingFieldIds.IsEmpty()) return;

    wxXmlNode* dataNode = FindControlXmlChild(root, "data");
    if (!dataNode) {
        dataNode = root;
    }

    wxXmlNode* observationNode = dataNode->GetChildren();
    while (observationNode) {
        if (observationNode->GetName() == "observation") {
            wxXmlNode* fieldNode = observationNode->GetChildren();

            while (fieldNode) {
                if (fieldNode->GetName() == "field" && fieldNode->GetChildren()) {
                    long id = -1;
                    if (fieldNode->GetAttribute("id").ToLong(&id) &&
                        recordingFieldIds.Index((int)id) != wxNOT_FOUND) {
                        AddReferencedNmeaRecordingName(
                            fieldNode->GetChildren()->GetContent(),
                            recordingNames);
                    }
                }

                fieldNode = fieldNode->GetNext();
            }
        }

        observationNode = observationNode->GetNext();
    }
}

static void CollectObservationXmlFiles(
    const wxString& folder,
    wxArrayString& xmlPaths)
{
    wxDir dir(folder);
    if (!dir.IsOpened()) return;

    wxString itemName;
    bool hasItem = dir.GetFirst(&itemName, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);

    while (hasItem) {
        const wxString itemPath = JoinControlPath(folder, itemName);

        if (wxDirExists(itemPath)) {
            CollectObservationXmlFiles(itemPath, xmlPaths);
        } else if (itemName.StartsWith("observations") &&
                   itemName.Lower().EndsWith(".xml") &&
                   xmlPaths.Index(itemPath) == wxNOT_FOUND) {
            xmlPaths.Add(itemPath);
        }

        hasItem = dir.GetNext(&itemName);
    }
}

static wxArrayString CollectReferencedNmeaRecordingNames(
    ooObservations* currentObservations)
{
    wxArrayString recordingNames;

    CollectReferencedNmeaRecordingNamesFromTable(currentObservations, recordingNames);

    if (!g_PrivateDataDir || g_PrivateDataDir->IsEmpty()) {
        return recordingNames;
    }

    wxArrayString xmlPaths;
    CollectObservationXmlFiles(*g_PrivateDataDir, xmlPaths);

    for (size_t i = 0; i < xmlPaths.GetCount(); ++i) {
        CollectReferencedNmeaRecordingNamesFromXml(xmlPaths[i], recordingNames);
    }
    return recordingNames;
}

static void CollectLocalNmeaRecordingFiles(
    const wxString& folder,
    wxArrayString& recordingPaths)
{
    wxDir dir(folder);
    if (!dir.IsOpened()) return;

    wxString fileName;
    bool hasFile = dir.GetFirst(&fileName, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);

    while (hasFile) {
        const wxString path = JoinControlPath(folder, fileName);

        if (wxDirExists(path)) {
            CollectLocalNmeaRecordingFiles(path, recordingPaths);
        } else if (fileName.Lower().EndsWith(".nmea")) {
            recordingPaths.Add(path);
        }

        hasFile = dir.GetNext(&fileName);
    }
}

static bool IsFolderEmptyExceptDsStore(const wxString& folder)
{
    wxDir dir(folder);
    if (!dir.IsOpened()) return false;

    wxString itemName;
    bool hasItem = dir.GetFirst(&itemName, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);

    while (hasItem) {
        if (itemName != ".DS_Store") {
            return false;
        }

        hasItem = dir.GetNext(&itemName);
    }

    return true;
}

static int DeleteEmptyFoldersRecursively(const wxString& folder)
{
    wxDir dir(folder);
    if (!dir.IsOpened()) return 0;

    int deletedCount = 0;

    wxString itemName;
    bool hasItem = dir.GetFirst(&itemName, wxEmptyString, wxDIR_DIRS);

    while (hasItem) {
        const wxString childFolder = JoinControlPath(folder, itemName);
        deletedCount += DeleteEmptyFoldersRecursively(childFolder);

        if (IsFolderEmptyExceptDsStore(childFolder)) {
            const wxString dsStorePath = JoinControlPath(childFolder, ".DS_Store");
            if (wxFileExists(dsStorePath)) {
                wxRemoveFile(dsStorePath);
            }

            if (wxFileName::Rmdir(childFolder)) {
                deletedCount++;
            }
        }

        hasItem = dir.GetNext(&itemName);
    }

    return deletedCount;
}

static wxArrayString CollectOrphanNmeaRecordingFiles(ooObservations* observations)
{
    wxArrayString orphanPaths;

    if (!g_PrivateDataDir || g_PrivateDataDir->IsEmpty()) {
        return orphanPaths;
    }

    const wxString recordingsRoot = JoinControlPath(*g_PrivateDataDir, "NMEArecordings");

    if (!wxDirExists(recordingsRoot)) {
        return orphanPaths;
    }

    const wxArrayString referencedNames = CollectReferencedNmeaRecordingNames(observations);

    wxArrayString localRecordings;
    CollectLocalNmeaRecordingFiles(recordingsRoot, localRecordings);

    for (size_t i = 0; i < localRecordings.GetCount(); ++i) {
        wxFileName recordingFile(localRecordings[i]);
        const wxString recordingName = recordingFile.GetFullName();

        if (referencedNames.Index(recordingName) == wxNOT_FOUND) {
            orphanPaths.Add(localRecordings[i]);
        }
    }

    return orphanPaths;
}

ooControlDialogImpl::ooControlDialogImpl(wxWindow* parent) 
    : ooControlDialogDef(parent),
      m_MiniPanel(nullptr),
      m_Observations(nullptr),
      m_ObservationsTable(nullptr),
      m_isScanningNmea(false),
      m_viewScale(1.0),
      m_observationsChoiceCount(1),
      m_markIconsLoaded(false),
      m_showObservationMarks(false)
{
    // The full Open Observer control window is a wxFrame, like OpenCPN's
    // Route & Mark Manager. wxDialog layout adaptation is not used here.

    // The full control dialog intentionally does not use wxSTAY_ON_TOP.
    // The mini dialog keeps wxSTAY_ON_TOP.
   

    // Make the observations/project switcher wider and easier to read.
    m_choiceObservations->SetMinSize(wxSize(240, -1));
    m_choiceObservations->SetInitialSize(wxSize(240, -1));
    m_choiceObservations->GetParent()->Layout();

    m_MiniPanel = new ooMiniPanel(m_panelObservations);
    m_MiniPanel->SetToggleWindowButtonLabel("Minimize");
    this->Connect(wxEVT_SHOW, wxShowEventHandler(ooMiniPanel::OnShow), NULL, m_MiniPanel);
    
    std::function<void(wxCommandEvent&)> refreshHandler = [&](wxCommandEvent& event)
    {
        if (event.GetEventType() == OBSERVATION_STOPPED &&
            m_showObservationMarks &&
            m_Observations) {
            // When the user wants observations on the chart, each completed
            // observation should quietly receive its mark.
            m_Observations->AddMarks(0);
        }

        RefreshGridAppearance(m_ObservationsTable);
        event.Skip();
    };

    m_MiniPanel->Bind(OBSERVATION_STARTED, refreshHandler);
    m_MiniPanel->Bind(OBSERVATION_STOPPED, refreshHandler);

    bSizerTopButtons->Add(m_MiniPanel, 1, wxEXPAND, 5);
    m_panelObservations->Layout();
    m_fgSizerObservations->Fit(m_panelObservations);

    // Initialize on 'Observations' tab
    m_notebookControl->SetSelection(1);

    OnNmeaFieldUpdate();
    RefreshListings();
    OnProjectGridSelectionChange();

    m_colourProject->Bind(wxEVT_COLOURPICKER_CHANGED,
                          &ooControlDialogImpl::OnProjectColourChanged,
                          this);

    m_gridProject->Bind(wxEVT_GRID_CELL_CHANGED,
                          &ooControlDialogImpl::OnProjectGridCellChange,
                          this);

    ApplyStandardBoldGridLabelFont(m_gridProject);

    // Do not call GetIconNameArray() here.
    // On macOS/OpenCPN 5.14 this can crash during plugin initialization.
    // The full OpenCPN icon list is loaded later when the user enters project edit mode.
    SelectMarkIconOrFallback(DEFAULT_PROJECT_ICON);

    m_currentObservationsIndex = 0;

    // bind backup timer (started in RestoreBackupObservations)
    m_BackupTimer.Bind(wxEVT_TIMER, &ooControlDialogImpl::OnBackupTimer, this, m_BackupTimer.GetId());
    // start timer to backup observations every 30 seconds
    m_BackupTimer.Start(30000);  // 30'000 ms = 30 s
}

ooControlDialogImpl::~ooControlDialogImpl()
{
    if (m_ObservationsTable) {
        m_ObservationsTable->Disconnect(
            wxEVT_GRID_SELECT_CELL,
            wxGridEventHandler(ooControlDialogImpl::OnObservationsGridCellSelect),
            NULL, this);
        m_ObservationsTable->Disconnect(
            wxEVT_GRID_RANGE_SELECTED,
            wxGridRangeSelectEventHandler(
                ooControlDialogImpl::OnObservationsGridRangeSelect),
            NULL, this);
        m_ObservationsTable->Disconnect(wxEVT_GRID_CELL_CHANGED,
                               wxGridEventHandler(ooControlDialogImpl::OnObservationsGridCellChange),
                               NULL, this);
    }

    this->Disconnect(wxEVT_SHOW, wxShowEventHandler(ooMiniPanel::OnShow), NULL, m_MiniPanel);

    m_BackupTimer.Stop();

    if (!m_Observations) return;

    SaveObservations(GetBackupFilename(m_currentObservationsIndex));
}

void ooControlDialogImpl::EnsureMarkIconChoiceContains(const wxString& iconName)
{
    if (!m_listMarkIcons) {
        return;
    }

    if (iconName.IsEmpty()) {
        return;
    }

    if (m_listMarkIcons->FindString(iconName) == wxNOT_FOUND) {
        m_listMarkIcons->Append(iconName);
    }
}

void ooControlDialogImpl::SelectMarkIconOrFallback(const wxString& iconName)
{
    if (!m_listMarkIcons) {
        return;
    }

    wxString selectedIcon = iconName;

    if (selectedIcon.IsEmpty()) {
        selectedIcon = DEFAULT_PROJECT_ICON;
    }

    EnsureMarkIconChoiceContains(selectedIcon);

    int index = m_listMarkIcons->FindString(selectedIcon);

    if (index == wxNOT_FOUND) {
        EnsureMarkIconChoiceContains(DEFAULT_PROJECT_ICON);
        index = m_listMarkIcons->FindString(DEFAULT_PROJECT_ICON);
    }

    if (index != wxNOT_FOUND) {
        m_listMarkIcons->SetSelection(index);
    }
}

void ooControlDialogImpl::LoadMarkIconsIfNeeded(const wxString& preferredIconName)
{
    if (!m_listMarkIcons) {
        return;
    }

    wxString selectedIcon = preferredIconName;

    if (selectedIcon.IsEmpty()) {
        selectedIcon = m_listMarkIcons->GetStringSelection();
    }

    if (selectedIcon.IsEmpty()) {
        selectedIcon = DEFAULT_PROJECT_ICON;
    }

    if (m_markIconsLoaded) {
        SelectMarkIconOrFallback(selectedIcon);
        return;
    }

    wxArrayString icons = GetIconNameArray();

    if (icons.GetCount() == 0) {
        SelectMarkIconOrFallback(selectedIcon);
        return;
    }

    m_listMarkIcons->Clear();
    m_listMarkIcons->Append(icons);

    m_markIconsLoaded = true;

    SelectMarkIconOrFallback(selectedIcon);
}

void ooControlDialogImpl::NewProject()
{
    // Prefer the user-editable default project template.
    // This keeps the first Open Observer experience welcoming without hiding
    // the project structure inside C++ code.
    const wxString templatePath = GetDefaultProjectTemplatePath();

    if (!templatePath.IsEmpty() && wxFileExists(templatePath)) {
        int fileVersion = 0;
        wxXmlDocument xmlDoc;
        wxXmlNode* root = nullptr;
        ooProject templateProject;

        if (ooObservations::ReadFromXML(
                templatePath,
                fileVersion,
                templateProject,
                xmlDoc,
                root,
                CreateFallbackDefaultProject())) {
            LoadProject(templateProject);
            return;
        }
    }

    // Keep a tiny, dependable fallback in code.
    // It is not meant to be beautiful; it only keeps New Project usable if the
    // external template is missing, damaged, or not copied yet.
    LoadProject(CreateFallbackDefaultProject());
}

void ooControlDialogImpl::UpdateProjectCellEditors()
{
    for (int c = 0; c < m_gridProject->GetNumberCols(); ++c) {
        wxGridCellChoiceEditor* observationFieldTypeEditor =
            new wxGridCellChoiceEditor(ooObservations::GetObservationFieldTypes());
        m_gridProject->SetCellEditor(1, c, observationFieldTypeEditor);
    }

    HideInternalProjectColumns();
}

bool ooControlDialogImpl::LoadProject(const ooProject& project)
{
    // delete columns
    if (m_gridProject->GetNumberCols() > 0)
        m_gridProject->DeleteCols(0, m_gridProject->GetNumberCols());

    m_textProjectName->SetValue(project.GetName());
    m_textProjectDescription->SetValue(project.GetDescription());
    m_colourProject->SetColour(project.GetColor());
    SelectMarkIconOrFallback(project.GetMarkIcon());

    const int C = project.GetColCount();
    for (int c = 0; c < C; c++) {
        // expand number of columns as needed to accommodate fields
        if (c >= m_gridProject->GetNumberCols()) {
            m_gridProject->AppendCols(c - m_gridProject->GetNumberCols() + 1);
        }

        m_gridProject->SetColSize(c, project.GetColSizes().GetSize(c));
        // set label and field type
        m_gridProject->SetCellValue(0, c, project.GetColLabels()[c]);
        m_gridProject->SetCellValue(1, c, project.GetColFieldTypes()[c]);
    }

    // set the column labels and cell editors
    for (int c = 0; c < m_gridProject->GetNumberCols(); ++c) {
        m_gridProject->SetColLabelValue(c, "");
    }

    UpdateProjectCellEditors();
    ApplyProjectGridReadabilityStyle(m_gridProject);
    HideInternalProjectColumns();

    m_CurrentProject = project;

    return true;
}

ooProject ooControlDialogImpl::GenerateProject() const
{
    const int C = m_gridProject->GetNumberCols();

    wxArrayString labels, fieldTypes;
    wxGridSizesInfo colSizes;

    for (int c = 0; c < C; ++c) {
        labels.push_back(m_gridProject->GetCellValue(0, c));
        fieldTypes.push_back(m_gridProject->GetCellValue(1, c));
        colSizes.m_customSizes[c] = m_gridProject->GetColSize(c);
    }

    wxString markIcon = m_listMarkIcons->GetStringSelection();

    if (markIcon.IsEmpty()) {
        markIcon = DEFAULT_PROJECT_ICON;
    }

    return ooProject(
    m_textProjectName->GetValue(),
    m_textProjectDescription->GetValue(),
    colSizes,
    fieldTypes,
    labels,
    m_colourProject->GetColour(),
    markIcon);
}

void ooControlDialogImpl::CreateObservationsTable(ooObservations *observations)
{
    m_Observations = observations;

    m_ObservationsTable = new wxGrid(m_panelObservations, wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize, 0);
    m_ObservationsTable->Connect(wxEVT_GRID_SELECT_CELL,
                           wxGridEventHandler(ooControlDialogImpl::OnObservationsGridCellSelect), NULL,
                           this);
    m_ObservationsTable->Connect(
        wxEVT_GRID_RANGE_SELECTED,
        wxGridRangeSelectEventHandler(
            ooControlDialogImpl::OnObservationsGridRangeSelect),
        NULL, this);
    m_ObservationsTable->Connect(wxEVT_GRID_CELL_CHANGED,
                                 wxGridEventHandler(ooControlDialogImpl::OnObservationsGridCellChange),
                                 NULL,
                           this);

    m_ObservationsTable->SetMinSize(wxSize(1, 1));

    m_ObservationsTable->AssignTable(m_Observations);
    m_ObservationsTable->SetColSizes(m_Observations->GetColSizes());
    HideInternalObservationColumns();

	// Grid
	m_ObservationsTable->EnableEditing( true );
	m_ObservationsTable->EnableGridLines( true );
    m_ObservationsTable->EnableDragGridSize(true);
	m_ObservationsTable->SetMargins( 0, 0 );

	// Columns
	m_ObservationsTable->EnableDragColMove( false );
	m_ObservationsTable->EnableDragColSize( true );
	m_ObservationsTable->SetColLabelSize( wxGRID_AUTOSIZE );
	m_ObservationsTable->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows 
	m_ObservationsTable->EnableDragRowSize(true);
	m_ObservationsTable->SetRowLabelSize( 0 );
	m_ObservationsTable->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

    const wxColour projectColour = m_Observations->GetProject().GetColor();
    m_ObservationsTable->SetLabelBackgroundColour(projectColour);
    m_ObservationsTable->SetLabelTextColour(ContrastTextColour(projectColour));

	// Cell Defaults
    m_ObservationsTable->SetDefaultCellFont(GetFont());
    m_ObservationsTable->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
    m_ObservationsTable->SetDefaultRowSize(32, true);

    wxGridCellAutoWrapStringEditor *editor = new wxGridCellAutoWrapStringEditor();
    m_ObservationsTable->SetDefaultEditor(editor);
    
    wxGridCellAutoWrapStringRenderer *renderer = new wxGridCellAutoWrapStringRenderer();
    m_ObservationsTable->SetDefaultRenderer(renderer);

    // m_ObservationsTable is a wxGrid, ultimately derived from wxWindow
    m_fgSizerObservations->Add(m_ObservationsTable, 1, wxALL|wxEXPAND, 5);

    m_ObservationsTable->EnableDragColSize(true);
    ApplyModernGridStyle(m_ObservationsTable);
}

void ooControlDialogImpl::SetObservationsChoiceCount(int observationsChoiceCount)
{
  wxArrayString choices = {};
  for (int i = 0; i < observationsChoiceCount; i++) {
    wxXmlDocument xmlDoc;
    wxXmlNode* root;
    int fileVersion;
    ooProject project;
    wxString name = wxString::Format(wxT("Empty (%i)"), i + 1);
    if (ooObservations::ReadFromXML(GetBackupFilename(i), fileVersion, project,
                                    xmlDoc, root, ooProject()) &&
        !project.GetName().IsEmpty())
      name = project.GetName();
    choices.Add(name);
  }
  m_choiceObservations->Append(choices);
  m_choiceObservations->GetParent()->Layout();
}

bool ooControlDialogImpl::SaveObservations(const wxString& filename, bool stopObservation)
{
    wxString savePath = filename;
    if (savePath.IsEmpty()) {
        wxFileDialog saveFileDialog(this, _("Save entire project with observations to XML file"), "",
                                    m_Observations->GetProject().GetName(),
                                    "XML file (*.xml)|*.xml",
                                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        
        if (saveFileDialog.ShowModal() == wxID_CANCEL) return false;

        savePath = saveFileDialog.GetPath();

        wxFileName saveFileName(savePath);

        if (saveFileName.GetExt().Lower() != wxT("xml")) {
            saveFileName.SetExt(wxT("xml"));
            savePath = saveFileName.GetFullPath();
        }
    }
    
    wxFileOutputStream output_stream(savePath);
    if (!output_stream.IsOk()) {
        wxMessageBox("Unable to save project to file " + savePath + ".",
            "Error", wxOK, this);
        return false;
    }

    // Update the user-visible column sizes without saving hidden technical columns.
    m_Observations->SetColSizes(GetUserVisibleObservationColSizes());

    if (stopObservation) {
        m_Observations->StopObservation();
    }
    const bool stripMarkGuid = filename.IsEmpty();

    m_Observations->SaveToXML(output_stream.GetFile(), stripMarkGuid);
    return true;
}

bool ooControlDialogImpl::LoadObservations(const wxString& filename, bool updateFromMarks)
{
    if (!m_Observations) return false;

    m_Observations->StopObservation();

    if (!m_Observations->ReadFromXML(filename, m_CurrentProject)) {
        wxMessageBox("Error loading observations file " + filename + ".",
                     "Error", wxOK, this);
        return false;
    }

    if (updateFromMarks) {
        m_Observations->UpdateObservationsFromMarks();
    }

    SetupObservationsForProject();
    RefreshGridAppearance(m_ObservationsTable);

    LoadProject(m_Observations->GetProject());
    g_openobserver_pi->SetProject(m_textProjectName->GetValue(),
                                  m_colourProject->GetColour(),
                                  m_currentObservationsIndex);
    m_MiniPanel->SetProjectInfo(m_textProjectName->GetValue(),
                                m_colourProject->GetColour());
    m_choiceObservations->SetString(m_currentObservationsIndex,
                                    m_Observations->GetProject().GetName());
    m_choiceObservations->GetParent()->Layout();

    SyncShowObservationMarksCheckboxWithData();

    SetProjectEditable(false);

    return true;
}

bool ooControlDialogImpl::RestoreBackupObservations(int observationsIndex)
{
    wxString backupFilename = GetBackupFilename(observationsIndex);
    if (observationsIndex < 0 ||
        observationsIndex >= (int)m_choiceObservations->GetCount())
        observationsIndex = 0;

    m_currentObservationsIndex = observationsIndex;
    m_choiceObservations->SetSelection(observationsIndex);
    bool result = (wxFile::Exists(backupFilename) && // We do not want to show an error if the file does not exists.
                   LoadObservations(backupFilename, false));   
    return result;
}

static bool IsDarkColour(const wxColour& colour)
{
    // Perceived brightness formula.
    const int brightness =
        (colour.Red() * 299 + colour.Green() * 587 + colour.Blue() * 114) / 1000;

    return brightness < 128;
}

static wxColour BlendColour(const wxColour& base, const wxColour& overlay, double ratio)
{
    ratio = std::max(0.0, std::min(1.0, ratio));

    const unsigned char r = static_cast<unsigned char>(
        base.Red() * (1.0 - ratio) + overlay.Red() * ratio);
    const unsigned char g = static_cast<unsigned char>(
        base.Green() * (1.0 - ratio) + overlay.Green() * ratio);
    const unsigned char b = static_cast<unsigned char>(
        base.Blue() * (1.0 - ratio) + overlay.Blue() * ratio);

    return wxColour(r, g, b);
}

static wxColour ContrastTextColour(const wxColour& background)
{
    return IsDarkColour(background) ? *wxWHITE : *wxBLACK;
}

static wxColour AlternateRowColour(const wxColour& base)
{
    if (IsDarkColour(base)) {
        return BlendColour(base, *wxWHITE, 0.06);
    }

    return BlendColour(base, *wxBLACK, 0.04);
}

static void ApplyStandardBoldGridLabelFont(wxGrid* grid)
{
    if (!grid) return;

    wxFont labelFont = grid->GetDefaultCellFont();

    if (!labelFont.IsOk()) {
        labelFont = grid->GetFont();
    }

    if (!labelFont.IsOk()) {
        labelFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    }

    labelFont.SetWeight(wxFONTWEIGHT_BOLD);

    grid->SetLabelFont(labelFont);
    grid->SetColLabelAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
    grid->SetRowLabelAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
}

static bool IsTextLikeObservationFieldType(const wxString& fieldType)
{
    return fieldType.IsSameAs("Text", false) ||
           fieldType.IsSameAs("Comments", false) ||
           fieldType.IsSameAs("Notes", false);
}

void ooControlDialogImpl::RefreshGridAppearance(wxGrid* grid)
{
    if (!grid) return;

     ApplyStandardBoldGridLabelFont(grid);

    const wxColour baseBackground =
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour alternateBackground =
        AlternateRowColour(baseBackground);

    const wxColour baseTextColour =
        ContrastTextColour(baseBackground);
    const wxColour alternateTextColour =
        ContrastTextColour(alternateBackground);

    const wxColour selectionBackground =
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    const wxColour selectionText =
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);

    grid->BeginBatch();

    grid->SetDefaultCellBackgroundColour(baseBackground);
    grid->SetDefaultCellTextColour(baseTextColour);
    grid->SetSelectionBackground(selectionBackground);
    grid->SetSelectionForeground(selectionText);

    const int rows = grid->GetNumberRows();

    for (int row = 0; row < rows; row++) {
        const wxColour rowBackground =
            (row % 2 == 0) ? baseBackground : alternateBackground;
        const wxColour rowTextColour =
            ContrastTextColour(rowBackground);

        wxGridCellAttr* attr = new wxGridCellAttr();
        attr->SetBackgroundColour(rowBackground);
        attr->SetTextColour(rowTextColour);

        grid->SetRowAttr(row, attr);
    }

    grid->EndBatch();
    grid->ForceRefresh();
}

void ooControlDialogImpl::ApplyModernGridStyle(wxGrid* grid)
{
    if (!grid) return;

    ApplyStandardBoldGridLabelFont(grid);

    const wxColour baseBackground =
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour baseTextColour =
        ContrastTextColour(baseBackground);

    const wxColour labelBackground =
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    const wxColour labelTextColour =
        ContrastTextColour(labelBackground);

    const wxColour selectionBackground =
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    const wxColour selectionText =
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);

    grid->SetDefaultCellFont(GetFont());
    grid->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
    grid->SetDefaultRowSize(36, true);  

    grid->SetDefaultCellBackgroundColour(baseBackground);
    grid->SetDefaultCellTextColour(baseTextColour);

    grid->SetLabelBackgroundColour(labelBackground);
    grid->SetLabelTextColour(labelTextColour);

    grid->SetSelectionBackground(selectionBackground);
    grid->SetSelectionForeground(selectionText);

    grid->SetRowLabelSize(40);
    grid->ForceRefresh();
}

void ooControlDialogImpl::ApplyProjectGridReadabilityStyle(wxGrid* grid)
{
    if (!grid) return;

    ApplyStandardBoldGridLabelFont(grid);

    const wxColour baseBackground =
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour alternateBackground =
        AlternateRowColour(baseBackground);

    const wxColour baseTextColour =
        ContrastTextColour(baseBackground);

    const wxColour labelBackground =
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    const wxColour labelTextColour =
        ContrastTextColour(labelBackground);

    const wxColour selectionBackground =
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    const wxColour selectionText =
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);

    grid->BeginBatch();

    grid->SetDefaultCellFont(GetFont());
    grid->SetDefaultCellAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
    grid->SetDefaultRowSize(42, true);

    grid->SetDefaultCellBackgroundColour(baseBackground);
    grid->SetDefaultCellTextColour(baseTextColour);

    grid->SetLabelBackgroundColour(labelBackground);
    grid->SetLabelTextColour(labelTextColour);

    grid->SetSelectionBackground(selectionBackground);
    grid->SetSelectionForeground(selectionText);

    grid->SetRowLabelSize(58);
    grid->SetRowSize(0, 42);
    grid->SetRowSize(1, 42);

    const int cols = grid->GetNumberCols();

    for (int col = 0; col < cols; ++col) {
        const wxColour columnBackground = baseBackground;
        const wxColour columnTextColour = baseTextColour;

        wxGridCellAttr* attr = new wxGridCellAttr();
        attr->SetBackgroundColour(columnBackground);
        attr->SetTextColour(columnTextColour);
        attr->SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);

        grid->SetColAttr(col, attr);

        wxFont titleFont = grid->GetDefaultCellFont();
        if (!titleFont.IsOk()) {
            titleFont = grid->GetFont();
        }
        if (!titleFont.IsOk()) {
            titleFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
        }
        titleFont.SetWeight(wxFONTWEIGHT_BOLD);

        const wxColour titleBackground = m_colourProject->GetColour();

        const wxColour typeBackground =
            IsDarkColour(columnBackground)
                ? BlendColour(columnBackground, *wxWHITE, 0.04)
                : BlendColour(columnBackground, *wxBLACK, 0.025);

        grid->SetCellFont(0, col, titleFont);
        grid->SetCellBackgroundColour(0, col, titleBackground);
        grid->SetCellTextColour(0, col, ContrastTextColour(titleBackground));
        grid->SetCellAlignment(0, col, wxALIGN_CENTER, wxALIGN_CENTER);

        grid->SetCellBackgroundColour(1, col, typeBackground);
        grid->SetCellTextColour(1, col, ContrastTextColour(typeBackground));
        grid->SetCellAlignment(1, col, wxALIGN_CENTER, wxALIGN_CENTER);
    }

    grid->EndBatch();
    grid->ForceRefresh();
}

void ooControlDialogImpl::SetupObservationsForProject()
{
    if (!m_Observations) return;
    if (!m_ObservationsTable) return;

    // update table color
    const wxColour projectColour = m_Observations->GetProject().GetColor();
    m_ObservationsTable->SetLabelBackgroundColour(projectColour);
    m_ObservationsTable->SetLabelTextColour(ContrastTextColour(projectColour));

    // update column sizes of table to match
    m_ObservationsTable->SetColSizes(m_Observations->GetColSizes());
    HideInternalObservationColumns();

    SetupListingEditors();
}

void ooControlDialogImpl::HideInternalObservationColumns()
{
    if (!m_Observations) return;
    if (!m_ObservationsTable) return;

    const wxArrayString& fieldTypes = m_Observations->GetColFieldTypes();

    for (int c = 0; c < (int)fieldTypes.GetCount(); ++c) {
        if (!ooObservations::IsInternalObservationFieldType(fieldTypes[c])) continue;

        // Mark GUID is our quiet bridge to OpenCPN marks.
        // It keeps observations linked to the chart without asking the user to care about IDs.
        m_ObservationsTable->SetColSize(c, 0);
    }
}

void ooControlDialogImpl::HideInternalProjectColumns()
{
    if (!m_gridProject) return;

    for (int c = 0; c < m_gridProject->GetNumberCols(); ++c) {
        if (!ooObservations::IsInternalObservationFieldType(
                m_gridProject->GetCellValue(1, c))) continue;

        // Mark GUID belongs to the chart link, not to the user protocol.
        // Keep it in the project quietly, but do not ask the user to manage it.
        m_gridProject->SetColSize(c, 0);
    }
}

wxGridSizesInfo ooControlDialogImpl::GetUserVisibleObservationColSizes() const
{
    if (!m_ObservationsTable) return wxGridSizesInfo();
    if (!m_Observations) return m_ObservationsTable->GetColSizes();

    wxGridSizesInfo colSizes = m_ObservationsTable->GetColSizes();

    const wxArrayString& fieldTypes = m_Observations->GetColFieldTypes();
    const wxGridSizesInfo& storedSizes = m_Observations->GetColSizes();

    for (int c = 0; c < (int)fieldTypes.GetCount(); ++c) {
        if (!ooObservations::IsInternalObservationFieldType(fieldTypes[c])) continue;

        // Keep the technical column hidden on screen, but do not save that hidden width
        // as if it were a user layout choice.
        colSizes.m_customSizes[c] = storedSizes.GetSize(c);
    }

    return colSizes;
}

void ooControlDialogImpl::SetupListingEditors()
{
    if (m_Observations == NULL) return;
    if (m_ObservationsTable == NULL) return;

    // Setup listings editors
    const int C = m_Observations->GetColsCount();
    
    for (int c = 0; c < C; ++c) {
        const wxString field_type = m_Observations->GetColFieldTypes()[c];
        wxArrayString items;
        wxGridCellAttr* attr = new wxGridCellAttr();
        if (field_type.IsSameAs("Checkbox")) {
            attr->SetEditor(new wxGridCellBoolEditor());
            attr->SetRenderer(new wxGridCellBoolRenderer());
        } else if (ooObservations::GetListing(field_type, items)) {
            attr->SetEditor(new wxGridCellChoiceEditor(items, true));
        } else {
            attr->SetEditor(NULL);
        }

        if (IsTextLikeObservationFieldType(field_type)) {
            attr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
        } else {
            attr->SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
        }

        m_ObservationsTable->SetColAttr(c, attr);
    }
}

void ooControlDialogImpl::SetPositionFix(time_t fixTime, double lat, double lon)
{
    wxString dateString = m_Observations->GetUtcTimeFromNMEA(ooObservations::UTC_TIME_DATE);
    wxString timeString = m_Observations->GetUtcTimeFromNMEA(ooObservations::UTC_TIME_TIME);
    wxString timeSource = m_Observations->GetUtcTimeSourceLabel();

    m_ObservationsDate->SetValue(dateString);
    m_ObservationsTime->SetValue(timeString);

    m_ObservationsUtcSource->SetLabel(timeSource);
    m_ObservationsUtcSource->GetParent()->Layout();

    m_ObservationsLat->SetValue(toSDMM_PlugIn(1, lat));
    m_ObservationsLon->SetValue(toSDMM_PlugIn(2, lon));
}

void ooControlDialogImpl::SetNmeaSentence(const wxString& sentence)
{
    if (!m_isScanningNmea) return;

    if (sentence.Length() < 6 || sentence[0] != '$') return;

    wxArrayString fields = wxSplit(sentence, ',');
    if (fields.GetCount() == 0) return;

    // Sentence ID is in first split, skipping 3 chars ($GP).
    wxString id = fields[0].Mid(3);
    if (id.IsEmpty()) return;
    
    // Skip first split (sentence ID), and count all remaining splits.
    for (size_t i = 1; i < fields.GetCount(); i++) {
        m_scannedNmeaFields[id].insert(static_cast<int>(i));
    }
    OnNmeaFieldUpdate();
}

int ooControlDialogImpl::GetNmeaFieldCount() const
{
    int count = 0;
    if (m_isScanningNmea) {
        for (auto it : m_scannedNmeaFields) {
            count += it.second.size();
        }
    } else {
        count = ooObservations::GetNMEAFields().size();
    }
    return count;
}

void ooControlDialogImpl::OnNmeaFieldUpdate()
{
    m_staticTextNMEA->SetLabel(
      wxString::Format(wxT("%i fields"), GetNmeaFieldCount()));
}

void ooControlDialogImpl::OnButtonClickCleanNmeaRecordings(wxCommandEvent& event)
{
    if (m_Observations && m_Observations->IsObserving()) {
        wxMessageBox(
            _("Please stop the current observation before cleaning NMEA recordings."),
            _("Clean NMEA recordings"),
            wxOK | wxICON_INFORMATION,
            this);
        return;
    }

    const wxArrayString orphanRecordings =
        CollectOrphanNmeaRecordingFiles(m_Observations);

    if (orphanRecordings.IsEmpty()) {
        wxMessageBox(
            _("No orphan NMEA recordings found."),
            _("Clean NMEA recordings"),
            wxOK | wxICON_INFORMATION,
            this);
        return;
    }

    const int response = wxMessageBox(
        wxString::Format(
            _("Found %i orphan NMEA recording(s).\n\nDelete orphan recordings?"),
            (int)orphanRecordings.GetCount()),
        _("Clean NMEA recordings"),
        wxYES_NO | wxNO_DEFAULT | wxICON_WARNING,
        this);

    if (response != wxYES) return;

    const int deletedCount = DeleteExistingFiles(orphanRecordings);

    int deletedFolderCount = 0;
    if (g_PrivateDataDir && !g_PrivateDataDir->IsEmpty()) {
        const wxString recordingsRoot = JoinControlPath(*g_PrivateDataDir, "NMEArecordings");
        deletedFolderCount = DeleteEmptyFoldersRecursively(recordingsRoot);
    }

    wxMessageBox(
        wxString::Format(
            _("Deleted %i orphan NMEA recording(s).\nRemoved %i empty folder(s)."),
            deletedCount,
            deletedFolderCount),
        _("Clean NMEA recordings"),
        wxOK | wxICON_INFORMATION,
        this);
}

void ooControlDialogImpl::OnButtonClickScanNmea(wxCommandEvent& event)
{
    if (m_isScanningNmea) {
        // Stop scan
        g_openobserver_pi->WriteNmeaXML(m_scannedNmeaFields);
        ooObservations::SetNMEAFields(g_openobserver_pi->ReadNmeaXML());
        UpdateProjectCellEditors();
    } else {
        // Start scan
        m_scannedNmeaFields.clear();
    }
    m_isScanningNmea = !m_isScanningNmea;
    m_buttonScanNmea->SetLabel(m_isScanningNmea ? "Stop NMEA Scan" : "Start NMEA Scan");
    OnNmeaFieldUpdate();
}

void ooControlDialogImpl::OnCheckBoxShowAdvancedNmeaFields(wxCommandEvent& event)
{
    ooObservations::SetShowAdvancedNMEAFields(m_checkShowAdvancedNmeaFields->GetValue());
    UpdateProjectCellEditors();
    event.Skip();
}

void ooControlDialogImpl::ooControlDialogActivate(wxActivateEvent& event)
{
    static bool inActivate = false;
    if (!inActivate) {
        inActivate = true;
        if (int count = m_Observations->UpdateObservationsFromMarks()) {
          // wxMessageBox(wxString::Format(wxT("The position of %i observation(s)
          // has been updated !"), count),
          //              "Position updated", wxOK, this);
          RefreshGridAppearance(m_ObservationsTable);
        }
    }
    event.Skip();
    inActivate = false;
}

void ooControlDialogImpl::OnButtonClickOpenResourcesFolder(wxCommandEvent& event)
{
    // Open the main Open Observer user folder, which contains Listings,
    // NMEArecordings, NMEAFields.xml, templates, icons and user resources.
    if (!g_PrivateDataDir || g_PrivateDataDir->IsEmpty()) return;

    wxLaunchDefaultApplication(*g_PrivateDataDir);
}

void ooControlDialogImpl::OnButtonClickRefreshListings(wxCommandEvent& event)
{
    RefreshListings();
}

void ooControlDialogImpl::RefreshListings()
{
    openobserver_pi::RefreshListings();
    int count = ooObservations::GetListings().size();
    m_staticTextListings->SetLabel(wxString::Format(wxT("%i fields"), count));
  
    SetupListingEditors();
    UpdateProjectCellEditors();
}

void ooControlDialogImpl::UseProject()
{
    const ooProject project = GenerateProject();

    g_openobserver_pi->SetProject(project.GetName(),
                                  project.GetColor(),
                                  m_currentObservationsIndex);
    m_MiniPanel->SetProjectInfo(project.GetName(),
                                project.GetColor());

    m_Observations->SetProject(project);
    m_CurrentProject = project;

    // update the project tab interface
    SetProjectEditable(false);

    // setup observations for the project
    SetupObservationsForProject();

    m_choiceObservations->SetString(m_currentObservationsIndex,
                                    project.GetName());
    m_choiceObservations->GetParent()->Layout();
}

void ooControlDialogImpl::EnsureProjectHasFieldType(const wxString& field_type, const wxString& label)
{
  bool has_field_type_col = false;
  for (int c = 0; c < m_gridProject->GetNumberCols(); ++c) {
    if (m_gridProject->GetCellValue(1, c).IsSameAs(field_type)) {
      has_field_type_col = true;
      break;
    }
  }
  if (!has_field_type_col) {
    m_gridProject->AppendCols();
    m_gridProject->SetColLabelValue(m_gridProject->GetNumberCols() - 1, "");
    m_gridProject->SetCellValue(0, m_gridProject->GetNumberCols() - 1, label);
    m_gridProject->SetCellValue(1, m_gridProject->GetNumberCols() - 1, field_type);

    if (!ooObservations::IsInternalObservationFieldType(field_type)) {
        wxMessageBox(
            wxString::Format(
                wxT("Column \"%s: %s\" has been automatically added to the project."),
                label, field_type),
            "Added column", wxOK_DEFAULT, this);
    }

    HideInternalProjectColumns();
  }
}

void ooControlDialogImpl::OnButtonClickProjectEditUse(wxCommandEvent& event)
{
    if (m_gridProject->IsEnabled()) 
    {
        // exit edit mode and use project
         
        // first, ensure that there is a Mark GUID column and, if not, add one as
        // the last column
        EnsureProjectHasFieldType("Start Timestamp UTC", "Start Timestamp UTC");
        EnsureProjectHasFieldType("Start Longitude", "Lon");
        EnsureProjectHasFieldType("Start Latitude", "Lat");
        EnsureProjectHasFieldType("Mark GUID", "Mark GUID");

        // second, warn user when project structure changes may affect data consistency
        if (m_Observations &&
            m_Observations->GetRowsCount() > 0 &&
            !m_Observations->GetProject().IsUpdatable(GenerateProject()))
        {
            const int response = wxMessageBox(
                "Attention! Your latest edits to the project structure may affect data consistency.\n\n"
                "It is still time to revert your changes.\n\n"
                "Are you sure you want to continue?",
                "Project structure changed",
                wxYES_NO | wxCANCEL | wxICON_WARNING,
                this);

            if (response != wxYES) return;

            const int saveResponse = wxMessageBox(
                "Do you want to keep a safety copy of the current project & observations before applying these changes?",
                "Save safety copy?",
                wxYES_NO | wxCANCEL | wxICON_QUESTION,
                this);

            if (saveResponse == wxCANCEL) return;

            if (saveResponse == wxYES) {
                if (!SaveObservations(wxString(), false))
                    return;
            }
        }

        for (int c = m_gridProject->GetNumberCols() - 1; c >= 0; --c) {
            if (m_gridProject->GetCellValue(0, c).IsSameAs("")  ||
                m_gridProject->GetCellValue(1, c).IsSameAs("")) {
                m_gridProject->DeleteCols(c);
                wxMessageBox(wxString::Format(
                             wxT("Empty column %i has been automatically "
                                   "removed from the project."),
                             c + 1), "Removed column", wxOK_DEFAULT, this);
            }
        }

        UseProject();
        
    } else {
        // enter edit mode
        SetProjectEditable(true);
    }
}

void ooControlDialogImpl::OnProjectColourChanged(wxColourPickerEvent& event)
{
    ApplyProjectGridReadabilityStyle(m_gridProject);
    event.Skip();
}

void ooControlDialogImpl::SetProjectEditable(bool editable)
{
    if (editable) {
        LoadMarkIconsIfNeeded(m_CurrentProject.GetMarkIcon());

        m_ProjectEditUse->SetLabel("Use");
        m_gridProject->Enable();
        m_ProjectNew->Enable();
        m_ProjectNewColumn->Enable();
        m_ProjectDeleteColumn->Enable();
        m_textProjectName->Enable();
        m_textProjectDescription->Enable();
        m_colourProject->Enable();
        m_listMarkIcons->Enable();
    } else {
        m_ProjectEditUse->SetLabel("Edit");
        m_gridProject->Disable();
        m_ProjectNew->Disable();
        m_ProjectNewColumn->Disable();
        m_ProjectDeleteColumn->Disable();
        m_colourProject->Disable();
        m_listMarkIcons->Disable();

        // disable project name text field, first setting value in code to ensure it
        // stays
        m_textProjectName->SetValue(m_textProjectName->GetValue());
        m_textProjectName->Disable();

        m_textProjectDescription->SetValue(m_textProjectDescription->GetValue());
        m_textProjectDescription->Disable();
    }
}

void ooControlDialogImpl::ClearCurrentObservationsForNewProject()
{
    if (!m_Observations) return;

    // A new project should start with a clear deck.
    // Project edits may preserve rows, but replacing the project should not carry old observations along.
    m_Observations->DeleteMarks();

    if (m_Observations->GetNumberRows() > 0) {
        m_Observations->DeleteRows(0, m_Observations->GetNumberRows());
    }
    m_showObservationMarks = false;
    if (m_checkShowObservationMarks) {
        m_checkShowObservationMarks->SetValue(false);
    }
    RefreshObservationsGrid();
}

void ooControlDialogImpl::OnButtonClickProjectNew(wxCommandEvent& event)
{
    const int response = wxMessageBox(
        "Warning: your current project and observations will be cleared.\n\n"
        "Do you want to continue?",
        "New project",
        wxYES_NO | wxICON_WARNING,
        this);

    if (response != wxYES) return;

    ClearCurrentObservationsForNewProject();
    NewProject();
    UseProject();
}

void ooControlDialogImpl::OnButtonClickProjectNewColumn(wxCommandEvent& event)
{
    wxArrayInt selection = m_gridProject->GetSelectedCols();
    int pos = (selection.IsEmpty() ? 0 : selection[0]);

    m_gridProject->InsertCols(pos, 1);
    m_gridProject->SetColLabelValue(pos, "");

    wxGridCellChoiceEditor *observationFieldTypeEditor = new wxGridCellChoiceEditor(ooObservations::GetObservationFieldTypes());
    m_gridProject->SetCellEditor(1, pos, observationFieldTypeEditor);

    ApplyProjectGridReadabilityStyle(m_gridProject);
}

void ooControlDialogImpl::OnButtonClickProjectDeleteColumn(wxCommandEvent& event)
{
    if (m_gridProject->GetNumberCols() == 0) return;

    wxArrayInt selection = m_gridProject->GetSelectedCols();
    int pos = (selection.IsEmpty() ? 0 : selection[0]);

    for (auto it = selection.rbegin(); it != selection.rend(); it++) {
        m_gridProject->DeleteCols(*it);
    }

    ApplyProjectGridReadabilityStyle(m_gridProject);
}

void ooControlDialogImpl::OnButtonClickNewObservation( wxCommandEvent& event )
{
    if (!m_Observations) return;
    if (m_Observations->IsObserving()) return;

    m_MiniPanel->StartOrStopObservation();
    m_MiniPanel->StartOrStopObservation();
}

void ooControlDialogImpl::OnButtonClickDeleteObservation( wxCommandEvent& event )
{
    if (!m_Observations) return;
    if (!m_ObservationsTable) return;
    if (m_Observations->GetNumberRows() <= 0) return;

    wxArrayInt selectedRows = m_ObservationsTable->GetSelectedRows();
    selectedRows.Sort([](int * a, int* b) { return *a > *b  ? -1 :
                                                   *a == *b ? 0  :
                                                              1; });
    if (selectedRows.IsEmpty()) return;

    const int response = wxMessageBox(
        wxString::Format(wxT("Warning: %i selected observation(s) will be deleted. Do you want to "
        "continue?"), selectedRows.GetCount()),
        "Delete observation(s)?", wxYES_NO, this);
    if (response != wxYES) return;

    const wxArrayString nmeaRecordingPaths =
        CollectNmeaRecordingPathsForRows(m_Observations, selectedRows);

    bool deleteAssociatedNmeaRecordings = false;

    if (!nmeaRecordingPaths.IsEmpty()) {
        const int nmeaResponse = wxMessageBox(
            wxString::Format(
                wxT("%i associated NMEA recording file(s) were found.\n\n"
                    "Do you also want to delete these original NMEA recording file(s)?"),
                nmeaRecordingPaths.GetCount()),
            "Delete associated NMEA recording file(s)?",
            wxYES_NO | wxICON_QUESTION,
            this);

        deleteAssociatedNmeaRecordings = (nmeaResponse == wxYES);
    }

    for (int i = 0; i < (int)selectedRows.GetCount(); i++) {
        m_Observations->DeleteMarks(selectedRows[i]);
        m_Observations->DeleteRows(selectedRows[i]);
    }

    if (deleteAssociatedNmeaRecordings) {
        DeleteExistingFiles(nmeaRecordingPaths);
    }

    RefreshGridAppearance(m_ObservationsTable);
}

#include <wx/sysopt.h>

void ooControlDialogImpl::RefreshObservationsGrid()
{
    if (m_ObservationsTable) {
        RefreshGridAppearance(m_ObservationsTable);
        m_ObservationsTable->ForceRefresh();
    }

    if (m_MiniPanel) {
        m_MiniPanel->RefreshObservationDisplay();
    }
}

void ooControlDialogImpl::CommitCurrentObservationsGridEdit()
{
    if (m_ObservationsTable && m_ObservationsTable->IsCellEditControlShown()) {
        m_ObservationsTable->SaveEditControlValue();
        m_ObservationsTable->HideCellEditControl();
    }
}

void ooControlDialogImpl::OnButtonClickExportObservations( wxCommandEvent& event )
{
    CommitCurrentObservationsGridEdit();

    if (!m_Observations) return;

#ifdef __WXMAC__
    wxSystemOptions::SetOption(wxOSX_FILEDIALOG_ALWAYS_SHOW_TYPES, 1); // From wxFileDialog documentation about multiple file filters.
#endif
    wxFileDialog exportFileDialog(
        this,
        _("Export observations"), "",
        m_Observations->GetProject().GetName(),
        "GeoJSON file (*.geojson)|*.geojson|CSV file (*.csv)|*.csv",
        wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
 
    if (exportFileDialog.ShowModal() == wxID_CANCEL)
        return;
 
    wxFileOutputStream output_stream(exportFileDialog.GetPath());
    if (!output_stream.IsOk())
    {
        wxMessageBox("Unable to save observations to file " + exportFileDialog.GetPath() + ".", "Error", wxOK, this);
        return;
    }

    if (exportFileDialog.GetPath().EndsWith("geojson")) m_Observations->SaveToGeoJSON(output_stream);
else                                                m_Observations->SaveToCSV(output_stream.GetFile(), true);
}

static bool ExtractJsonArrayValues(
    const wxString& content,
    const wxString& key,
    wxArrayString& values)
{
    values.Clear();

    const wxString marker = "\"" + key + "\"";
    int keyPos = content.Find(marker);
    if (keyPos == wxNOT_FOUND) {
        return false;
    }

    int arrayStart = content.find("[", keyPos);
    if (arrayStart == wxNOT_FOUND) {
        return false;
    }

    int arrayEnd = content.find("]", arrayStart);
    if (arrayEnd == wxNOT_FOUND) {
        return false;
    }

    wxString arrayContent = content.Mid(arrayStart + 1, arrayEnd - arrayStart - 1);

    int pos = 0;
    while (pos < (int)arrayContent.Length()) {
        int quoteStart = arrayContent.find("\"", pos);
        if (quoteStart == wxNOT_FOUND) {
            break;
        }

        int quoteEnd = arrayContent.find("\"", quoteStart + 1);
        if (quoteEnd == wxNOT_FOUND) {
            break;
        }

        wxString value = arrayContent.Mid(quoteStart + 1, quoteEnd - quoteStart - 1);
        value.Trim(true);
        value.Trim(false);

        if (!value.IsEmpty()) {
            values.Add(value);
        }

        pos = quoteEnd + 1;
    }

    return true;
}

static bool ReadDataPackageSettings(
    const wxString& packageDir,
    wxArrayString& dailyFolders,
    wxArrayString& workingFolders,
    wxArrayString& rawDataFolders)
{
    dailyFolders.Clear();
    workingFolders.Clear();
    rawDataFolders.Clear();

    const wxString settingsPath =
        packageDir + wxFILE_SEP_PATH +
        "00_raw-data" + wxFILE_SEP_PATH +
        "metadata" + wxFILE_SEP_PATH +
        "data_package_settings.json";

    if (!wxFileExists(settingsPath)) {
        return false;
    }

    wxTextFile file(settingsPath);
    if (!file.Open()) {
        return false;
    }

    wxString content;
    for (size_t i = 0; i < file.GetLineCount(); ++i) {
        content += file.GetLine(i) + "\n";
    }

    file.Close();

    bool foundAny = false;
    foundAny = ExtractJsonArrayValues(content, "dailyMediaFolders", dailyFolders) || foundAny;
    foundAny = ExtractJsonArrayValues(content, "workingFolders", workingFolders) || foundAny;
    foundAny = ExtractJsonArrayValues(content, "rawDataFolders", rawDataFolders) || foundAny;

    return foundAny;
}

static void ApplyDataPackageSettingsToChecklist(
    wxCheckListBox* rawChecklist,
    wxCheckListBox* dailyChecklist,
    wxCheckListBox* workingChecklist,
    const wxArrayString& dailyFolders,
    const wxArrayString& workingFolders,
    const wxArrayString& rawDataFolders)
{
    if (!rawChecklist || !dailyChecklist || !workingChecklist) {
        return;
    }

    for (unsigned int i = 0; i < rawChecklist->GetCount(); ++i) {
        rawChecklist->Check(i, false);
    }

    for (unsigned int i = 0; i < dailyChecklist->GetCount(); ++i) {
        dailyChecklist->Check(i, false);
    }

    for (unsigned int i = 0; i < workingChecklist->GetCount(); ++i) {
        workingChecklist->Check(i, false);
    }

    rawChecklist->Check(0, rawDataFolders.Index("00_raw-data/observations") != wxNOT_FOUND);
    rawChecklist->Check(1, rawDataFolders.Index("00_raw-data/nmea-recordings") != wxNOT_FOUND);
    rawChecklist->Check(2, rawDataFolders.Index("00_raw-data/tracks") != wxNOT_FOUND);

    dailyChecklist->Check(0, dailyFolders.Index("photos") != wxNOT_FOUND);
    dailyChecklist->Check(1, dailyFolders.Index("audio") != wxNOT_FOUND);
    dailyChecklist->Check(2, dailyFolders.Index("video") != wxNOT_FOUND);
    dailyChecklist->Check(3, dailyFolders.Index("tracks") != wxNOT_FOUND);
    dailyChecklist->Check(4, dailyFolders.Index("nmea") != wxNOT_FOUND);
    dailyChecklist->Check(5, dailyFolders.Index("samples") != wxNOT_FOUND);

    workingChecklist->Check(0, workingFolders.Index("02_working") != wxNOT_FOUND);
}

static bool ShowDataPackageFolderDialog(
    wxWindow* parent,
    bool createMode,
    wxString& selectedPath,
    wxArrayString& selectedDailyFolders,
    wxArrayString& selectedWorkingFolders,
    wxArrayString& selectedRawDataFolders)
{
    wxDialog dialog(
        parent,
        wxID_ANY,
        createMode ? _("Create a new Data Package") : _("Update an existing Data Package"),
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxFRAME_FLOAT_ON_PARENT | wxSTAY_ON_TOP);

    dialog.SetMinSize(wxSize(560, 620));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* title = new wxStaticText(
        &dialog,
        wxID_ANY,
        createMode ? _("Create a new Data Package") : _("Update an existing Data Package"));

    wxFont titleFont = title->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);

    mainSizer->Add(title, 0, wxALL, 14);

    wxStaticText* explanation = new wxStaticText(
        &dialog,
        wxID_ANY,
        createMode
            ? _("Choose the folder where Open Observer will create the new Data Package.")
            : _("Choose the existing Data Package folder to update."));

    explanation->Wrap(520);
    mainSizer->Add(explanation, 0, wxLEFT | wxRIGHT | wxBOTTOM, 14);

    wxStaticText* folderIntro = new wxStaticText(
        &dialog,
        wxID_ANY,
        createMode
            ? _("Choose which folders Open Observer should prepare.\n\n"
                "You can keep only what is useful for you in this project.")
            : _("Choose which folders Open Observer should update.\n\n"
                "You can keep only what is useful for you in this project.\n"
                "Missing folders will be added."));

    folderIntro->Wrap(520);

    wxBoxSizer* pathSizer = new wxBoxSizer(wxHORIZONTAL);

    wxTextCtrl* pathText = new wxTextCtrl(
        &dialog,
        wxID_ANY,
        selectedPath,
        wxDefaultPosition,
        wxSize(420, -1));

    wxButton* browseButton = new wxButton(
        &dialog,
        wxID_ANY,
        _("Browse…"));

    pathSizer->Add(pathText, 1, wxRIGHT | wxEXPAND, 8);
    pathSizer->Add(browseButton, 0);

    wxBoxSizer* listsSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* projectTitle = new wxStaticText(
        &dialog,
        wxID_ANY,
        _("Project"));

    wxFont sectionFont = projectTitle->GetFont();
    sectionFont.SetWeight(wxFONTWEIGHT_BOLD);
    projectTitle->SetFont(sectionFont);

    wxStaticText* projectInfo = new wxStaticText(
        &dialog,
        wxID_ANY,
        _("✓ project XML in 00_raw-data/project"));

    wxStaticText* exportsTitle = new wxStaticText(
        &dialog,
        wxID_ANY,
        _("00_raw-data"));

    exportsTitle->SetFont(sectionFont);

    wxArrayString rawChoices;
    rawChoices.Add(_("observations"));
    rawChoices.Add(_("nmea-recordings"));
    rawChoices.Add(_("tracks"));

    wxCheckListBox* rawChecklist = new wxCheckListBox(
        &dialog,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(520, 76),
        rawChoices);

    wxStaticText* dailyTitle = new wxStaticText(
        &dialog,
        wxID_ANY,
        _("01_daily-media"));

    dailyTitle->SetFont(sectionFont);

    wxArrayString dailyChoices;
    dailyChoices.Add(_("photos"));
    dailyChoices.Add(_("audio"));
    dailyChoices.Add(_("video"));
    dailyChoices.Add(_("tracks"));
    dailyChoices.Add(_("nmea"));
    dailyChoices.Add(_("samples"));

    wxCheckListBox* dailyChecklist = new wxCheckListBox(
        &dialog,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(520, 118),
        dailyChoices);

    wxStaticText* workingTitle = new wxStaticText(
        &dialog,
        wxID_ANY,
        _("02_working"));

    workingTitle->SetFont(sectionFont);

    wxArrayString workingChoices;
    workingChoices.Add(_("working"));

    wxCheckListBox* workingChecklist = new wxCheckListBox(
        &dialog,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(520, 38),
        workingChoices);

    for (unsigned int i = 0; i < rawChoices.GetCount(); ++i) {
        rawChecklist->Check(i, true);
    }

    for (unsigned int i = 0; i < dailyChoices.GetCount(); ++i) {
        dailyChecklist->Check(i, true);
    }

    for (unsigned int i = 0; i < workingChoices.GetCount(); ++i) {
        workingChecklist->Check(i, true);
    }

    listsSizer->Add(projectTitle, 0, wxLEFT | wxRIGHT | wxTOP, 0);
    listsSizer->Add(projectInfo, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 6);

    listsSizer->AddSpacer(8);

    listsSizer->Add(exportsTitle, 0, wxLEFT | wxRIGHT | wxTOP, 0);
    listsSizer->Add(rawChecklist, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    listsSizer->AddSpacer(8);

    listsSizer->Add(dailyTitle, 0, wxLEFT | wxRIGHT | wxTOP, 0);
    listsSizer->Add(dailyChecklist, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    listsSizer->AddSpacer(8);

    listsSizer->Add(workingTitle, 0, wxLEFT | wxRIGHT | wxTOP, 0);
    listsSizer->Add(workingChecklist, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    if (createMode) {
        mainSizer->Add(folderIntro, 0, wxLEFT | wxRIGHT | wxBOTTOM, 14);
        mainSizer->Add(listsSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 14);
        mainSizer->Add(pathSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 14);
    } else {
        mainSizer->Add(pathSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 14);
        mainSizer->Add(folderIntro, 0, wxLEFT | wxRIGHT | wxBOTTOM, 14);
        mainSizer->Add(listsSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 14);
    }

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* actionButton = new wxButton(
        &dialog,
        wxID_OK,
        createMode ? _("Create Data Package") : _("Update Data Package"));

    wxButton* cancelButton = new wxButton(
        &dialog,
        wxID_CANCEL,
        _("Cancel"));

    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(actionButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 9);

    browseButton->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        wxDirDialog dirDialog(
            &dialog,
            createMode
                ? _("Choose the folder where Open Observer will create the new Data Package")
                : _("Choose the existing Data Package folder to update"),
            pathText->GetValue(),
            wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

        if (dirDialog.ShowModal() == wxID_OK) {
            pathText->SetValue(dirDialog.GetPath());

            if (!createMode) {
                wxArrayString savedDailyFolders;
                wxArrayString savedWorkingFolders;
                wxArrayString savedRawDataFolders;

                if (ReadDataPackageSettings(
                        dirDialog.GetPath(),
                        savedDailyFolders,
                        savedWorkingFolders,
                        savedRawDataFolders)) {
                    ApplyDataPackageSettingsToChecklist(
                        rawChecklist,
                        dailyChecklist,
                        workingChecklist,
                        savedDailyFolders,
                        savedWorkingFolders,
                        savedRawDataFolders);
                }
            }
        }
    });

    dialog.SetSizerAndFit(mainSizer);
    dialog.CentreOnParent();

    if (dialog.ShowModal() != wxID_OK) {
        return false;
    }

    selectedPath = pathText->GetValue();
    selectedPath.Trim(true);
    selectedPath.Trim(false);

    if (selectedPath.IsEmpty()) {
        wxMessageBox(
            _("Please choose a folder before continuing."),
            createMode ? _("Create Data Package") : _("Update Data Package"),
            wxOK | wxICON_WARNING,
            parent);
        return false;
    }

    selectedDailyFolders.Clear();
    selectedWorkingFolders.Clear();
    selectedRawDataFolders.Clear();

    if (rawChecklist->IsChecked(0)) selectedRawDataFolders.Add("00_raw-data/observations");
    if (rawChecklist->IsChecked(1)) selectedRawDataFolders.Add("00_raw-data/nmea-recordings");
    if (rawChecklist->IsChecked(2)) selectedRawDataFolders.Add("00_raw-data/tracks");

    if (dailyChecklist->IsChecked(0)) selectedDailyFolders.Add("photos");
    if (dailyChecklist->IsChecked(1)) selectedDailyFolders.Add("audio");
    if (dailyChecklist->IsChecked(2)) selectedDailyFolders.Add("video");
    if (dailyChecklist->IsChecked(3)) selectedDailyFolders.Add("tracks");
    if (dailyChecklist->IsChecked(4)) selectedDailyFolders.Add("nmea");
    if (dailyChecklist->IsChecked(5)) selectedDailyFolders.Add("samples");

    if (workingChecklist->IsChecked(0)) selectedWorkingFolders.Add("02_working");

    return true;
}

void ooControlDialogImpl::OnButtonClickDataPackage(wxCommandEvent& event)
{
    wxDialog dialog(
        this,
        wxID_ANY,
        _("Data Package"),
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxFRAME_FLOAT_ON_PARENT | wxSTAY_ON_TOP);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* title = new wxStaticText(
        &dialog,
        wxID_ANY,
        _("Prepare your Open Observer Data Package"));

    wxFont titleFont = title->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);

    mainSizer->Add(title, 0, wxALL, 14);

    wxButton* createButton = new wxButton(
        &dialog,
        wxID_YES,
        _("Create a new Data Package"),
        wxDefaultPosition,
        wxSize(360, 38));

    wxButton* updateButton = new wxButton(
        &dialog,
        wxID_NO,
        _("Update an existing Data Package"),
        wxDefaultPosition,
        wxSize(360, 38));

    mainSizer->Add(createButton, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 14);
    mainSizer->Add(updateButton, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 14);

    wxButton* cancelButton = new wxButton(
        &dialog,
        wxID_CANCEL,
        _("Cancel"));

    mainSizer->Add(cancelButton, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 14);

    createButton->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        dialog.EndModal(wxID_YES);
    });

    updateButton->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        dialog.EndModal(wxID_NO);
    });

    cancelButton->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        dialog.EndModal(wxID_CANCEL);
    });

    dialog.SetSizerAndFit(mainSizer);
    dialog.CentreOnParent();

    const int answer = dialog.ShowModal();

    if (answer == wxID_YES) {
        OnButtonClickCreateScientificPackage(event);
    } else if (answer == wxID_NO) {
        OnButtonClickUpdateScientificPackage(event);
    }
}


void ooControlDialogImpl::OnButtonClickCreateScientificPackage(wxCommandEvent& event)
{
    CommitCurrentObservationsGridEdit();

    if (!m_Observations) return;

    wxString destinationFolder;
    wxArrayString selectedDailyFolders;
    wxArrayString selectedWorkingFolders;
    wxArrayString selectedRawDataFolders;

    if (!ShowDataPackageFolderDialog(
            this,
            true,
            destinationFolder,
            selectedDailyFolders,
            selectedWorkingFolders,
            selectedRawDataFolders)) {
        return;
    }

    wxString createdPackagePath;
    wxString errorMessage;
    ooScientificPackage::RunSummary runSummary;

    if (!ooScientificPackage::Create(
            m_Observations,
            destinationFolder,
            createdPackagePath,
            errorMessage,
            selectedDailyFolders,
            selectedWorkingFolders,
            selectedRawDataFolders,
            runSummary)) {
        wxMessageBox(
            _("Unable to create data package:\n\n") + errorMessage,
            _("Create Data Package"),
            wxOK | wxICON_ERROR,
            this);
        return;
    }

    wxMessageBox(
        _("Data Package created successfully:\n\n") + createdPackagePath +
        _("\n\nSummary:\n") +
        wxString::Format(_("• %d export files created\n"), runSummary.exportFilesRefreshed) +
        wxString::Format(_("• %d folders created\n"), runSummary.foldersTouched) +
        wxString::Format(_("• %d NMEA recordings exported\n"), runSummary.nmeaRecordingsCopied) +
        wxString::Format(_("• %d daily GPX tracks exported\n"), runSummary.gpxDailyTracksExported) +
        wxString::Format(_("• %d raw OpenCPN GPX tracks exported\n"), runSummary.gpxCompiledTracksExported) +
        wxString::Format(_("• %d GPX track points exported\n"), runSummary.gpxTrackPointsExported) +
        _("\nA detailed log was written to:\n00_raw-data/metadata/data_package_last_run.txt"),
        _("Create Data Package"),
        wxOK | wxICON_INFORMATION,
        this);

    wxLaunchDefaultApplication(createdPackagePath);
}

void ooControlDialogImpl::OnButtonClickUpdateScientificPackage(wxCommandEvent& event)
{
    CommitCurrentObservationsGridEdit();

    if (!m_Observations) return;

    wxString packageFolder;
    wxArrayString selectedDailyFolders;
    wxArrayString selectedWorkingFolders;
    wxArrayString selectedRawDataFolders;

    if (!ShowDataPackageFolderDialog(
            this,
            false,
            packageFolder,
            selectedDailyFolders,
            selectedWorkingFolders,
            selectedRawDataFolders)) {
        return;
    }

    wxString errorMessage;
    ooScientificPackage::RunSummary runSummary;

    if (!ooScientificPackage::Update(
            m_Observations,
            packageFolder,
            errorMessage,
            selectedDailyFolders,
            selectedWorkingFolders,
            selectedRawDataFolders,
            runSummary)) {
        wxMessageBox(
            _("Unable to update data package:\n\n") + errorMessage,
            _("Update Data Package"),
            wxOK | wxICON_ERROR,
            this);
        return;
    }

    wxMessageBox(
        _("Data Package updated successfully:\n\n") + packageFolder +
        _("\n\nSummary:\n") +
        wxString::Format(_("• %d export files updated\n"), runSummary.exportFilesRefreshed) +
        wxString::Format(_("• %d folders updated\n"), runSummary.foldersTouched) +
        wxString::Format(_("• %d NMEA recordings exported\n"), runSummary.nmeaRecordingsCopied) +
        wxString::Format(_("• %d daily GPX tracks exported\n"), runSummary.gpxDailyTracksExported) +
        wxString::Format(_("• %d raw OpenCPN GPX tracks exported\n"), runSummary.gpxCompiledTracksExported) +
        wxString::Format(_("• %d GPX track points exported\n"), runSummary.gpxTrackPointsExported) +
        _("\nA detailed log was written to:\n00_raw-data/metadata/data_package_last_run.txt"),
        _("Update Data Package"),
        wxOK | wxICON_INFORMATION,
        this);

    wxLaunchDefaultApplication(packageFolder);
}

void ooControlDialogImpl::OnButtonClickImportObservations(wxCommandEvent& event)
{
    if (!m_Observations) return;

    wxFileDialog importFileDialog(this, _("Import observations from CSV file"), "", "",
                                "CSV file (*.csv)|*.csv",
                                wxFD_OPEN|wxFD_FILE_MUST_EXIST|wxFD_CHANGE_DIR);

    if (importFileDialog.ShowModal() == wxID_CANCEL) return;
    const wxString path = importFileDialog.GetPath();

    wxString err;
    if (!m_Observations->ReadFromCSV(path, err)) {
        wxMessageBox("Unable to import CSV file " + path + ": " + err,
                     "Error", wxOK, this);
    }
}

void ooControlDialogImpl::OnBackupTimer(wxTimerEvent& event)
{
    if (!m_Observations) return;

    SaveObservations(GetBackupFilename(m_currentObservationsIndex), false);
}

bool ooControlDialogImpl::ObservationMarksArePresent() const
{
    if (!m_Observations) return false;

    const int markCol = m_Observations->GetProject().GetMarkCol();
    if (markCol == wxNOT_FOUND) return false;

    for (int r = 0; r < m_Observations->GetRowsCount(); ++r) {
        if (!m_Observations->GetValue(r, markCol).IsEmpty()) {
            return true;
        }
    }

    return false;
}

void ooControlDialogImpl::SyncShowObservationMarksCheckboxWithData()
{
    m_showObservationMarks = ObservationMarksArePresent();

    if (m_checkShowObservationMarks) {
        m_checkShowObservationMarks->SetValue(m_showObservationMarks);
    }
}

void ooControlDialogImpl::OnCheckBoxShowObservationMarks(wxCommandEvent& event)
{
    if (!m_Observations) return;

    m_showObservationMarks = m_checkShowObservationMarks->GetValue();

    if (m_showObservationMarks) {
        // The checkbox expresses an intention: observations should be visible on the chart.
        // Missing marks can be recreated safely from the observation rows.
        m_Observations->AddMarks();
    } else {
        // Keep the chart calm without touching the observation data.
        m_Observations->DeleteMarks();
    }

    m_ObservationsTable->ForceRefresh();
    event.Skip();
}

void ooControlDialogImpl::OnButtonClickLoadObservation(wxCommandEvent& event)
{
    if (m_Observations->GetRowsCount() > 0) {
        const int response = wxMessageBox(
            "Warning: your current observations will be cleared. Do you want to continue?",
            "Warning", wxYES_NO, this);
        
        if (response != wxYES) return;
    }

    wxFileDialog loadObservationsDialog(
        this, _("Load entire project with observations from XML file"), "", "", "XML file (*.xml)|*.xml",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);
    
    if (loadObservationsDialog.ShowModal() == wxID_CANCEL) return;
    
    if (!LoadObservations(loadObservationsDialog.GetPath())) {
        wxMessageBox("Unable to load project from file " +
                     loadObservationsDialog.GetPath() + ".",
                     "Error", wxOK, this);
        return;
    }
}

void ooControlDialogImpl::OnButtonClickSaveObservation(wxCommandEvent& event)
{
    CommitCurrentObservationsGridEdit();

    SaveObservations();
}

void ooControlDialogImpl::ooControlCloseClick(wxCommandEvent& event)
{
    g_openobserver_pi->ToggleToolbarIcon();
}

void ooControlDialogImpl::ooControlDialogDefOnClose(wxCloseEvent& event)
{
    g_openobserver_pi->ToggleToolbarIcon();
}


void ooControlDialogImpl::OnNotebookPageChanged(wxNotebookEvent& event)
{
    if (event.GetSelection() == 0 &&
        m_Observations &&
        m_ObservationsTable &&
        m_gridProject &&
        !m_gridProject->IsEnabled()) {
        m_Observations->SetColSizes(GetUserVisibleObservationColSizes());

        const wxGridSizesInfo& colSizes = m_Observations->GetColSizes();

        for (int c = 0; c < m_gridProject->GetNumberCols(); ++c) {
            m_gridProject->SetColSize(c, colSizes.GetSize(c));
        }

        HideInternalProjectColumns();
        m_gridProject->ForceRefresh();
    }

    event.Skip();
}

void ooControlDialogImpl::OnChoiceObservationsChanged(wxCommandEvent& event)
{
    SaveObservations(GetBackupFilename(m_currentObservationsIndex));
    m_currentObservationsIndex = m_choiceObservations->GetSelection();
    wxString filename = GetBackupFilename(m_currentObservationsIndex);
    if (wxFile::Exists(filename)) {
        LoadObservations(filename);
    } else {
        ClearCurrentObservationsForNewProject();
        NewProject();
        UseProject();
    }

    event.Skip();
}

void ooControlDialogImpl::OnObservationsGridCellSelect(wxGridEvent& event)
{
    int lat_col = m_Observations->GetProject().GetLatCol();
    int lon_col = m_Observations->GetProject().GetLonCol();
    int mark_col = m_Observations->GetProject().GetMarkCol();

    int col = event.GetCol();
    int row = event.GetRow();
    bool hasMark = (mark_col != wxNOT_FOUND &&
                    !m_Observations->GetValue(row, mark_col).IsEmpty());
    if (hasMark) {
        if (lat_col != wxNOT_FOUND && lon_col != wxNOT_FOUND) {
            const double lat = fromDMM_Plugin(m_Observations->GetValue(row, lat_col));
            const double lon = fromDMM_Plugin(m_Observations->GetValue(row, lon_col));
            if (g_openobserver_pi) {
                g_openobserver_pi->JumpToObservationOnChart(lat, lon);
            } else {
                JumpToPosition(lat, lon, m_viewScale);
            }

            if (g_openobserver_pi) {
                g_openobserver_pi->HighlightObservationOnChart(
                    lat,
                    lon,
                    m_Observations->GetProject().GetColor());
            }
        }
    }

    m_ObservationsDelete->Enable(!m_ObservationsTable->GetSelectedRows().IsEmpty());

    event.Skip();
}

void ooControlDialogImpl::SetViewScale(double viewScale)
{
    m_viewScale = viewScale;
}

void ooControlDialogImpl::OnObservationsGridRangeSelect(
    wxGridRangeSelectEvent& event)
{
  m_ObservationsDelete->Enable(
      !m_ObservationsTable->GetSelectedRows().IsEmpty());

  event.Skip();
}

void ooControlDialogImpl::OnProjectGridSelectionChange()
{
    const wxArrayInt selectedCols = m_gridProject->GetSelectedCols();
    m_ProjectDeleteColumn->Enable(!selectedCols.IsEmpty());
    m_ProjectDeleteColumn->SetLabel("Delete Selected");
}

void ooControlDialogImpl::OnProjectGridCellSelect(wxGridEvent& event)
{
    OnProjectGridSelectionChange();
    event.Skip();
}

void ooControlDialogImpl::OnProjectGridRangeSelect(wxGridRangeSelectEvent& event)
{
    OnProjectGridSelectionChange();
    event.Skip();
}

void ooControlDialogImpl::OnProjectGridCellChange(wxGridEvent& event)
{
    const int row = event.GetRow();
    const int col = event.GetCol();

    if (row == 1) {
        const wxString value = m_gridProject->GetCellValue(row, col);

        wxString trimmedValue = value;
        trimmedValue.Trim(true);
        trimmedValue.Trim(false);

        if (trimmedValue.IsEmpty() || trimmedValue.EndsWith(":")) {
            // Category titles and blank separators help the eye,
            // but they are not real project fields.
            m_gridProject->SetCellValue(row, col, wxEmptyString);

            wxMessageBox(
                _("Please choose a valid field type."),
                _("Field type category"),
                wxOK | wxICON_INFORMATION,
                this);
        }
    }

    event.Skip();
}

void ooControlDialogImpl::OnObservationsGridCellChange(wxGridEvent& event)
{
    const int r = event.GetRow();
    const int markCol = m_Observations->GetProject().GetMarkCol();
    const bool hasMark = (!m_Observations->GetValue(r, markCol).IsEmpty());
    if (hasMark) {
        m_Observations->DeleteMarks(r);
        m_Observations->AddMarks(r);
    }

    if (g_openobserver_pi) {
        g_openobserver_pi->RefreshObservationDisplay();
    }

    event.Skip();
}

wxString ooControlDialogImpl::GetBackupFilename(int index)
{
  wxFileName backup(*g_pData,
                    index == -1 ? "observations.xml" : wxString::Format(wxT("observations_%i.xml"), index));
  return backup.GetFullPath();
}
