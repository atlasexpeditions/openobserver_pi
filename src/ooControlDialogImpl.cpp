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

#include <wx/xml/xml.h>

#include <wx/base64.h>
#include <wx/mstream.h>

#include "ooControlDialogImpl.h"
#include "openobserver_pi.h"

#include "ocpn_plugin.h"

#if wxCHECK_VERSION(3,0,0)
#include <wx/valnum.h>
#endif

#include <wx/fontdlg.h>

extern openobserver_pi *g_openobserver_pi;
extern wxString *g_pData;

ooControlDialogImpl::ooControlDialogImpl(wxWindow* parent) 
    : ooControlDialogDef(parent),
      m_MiniPanel(nullptr),
      m_Observations(nullptr),
      m_ObservationsTable(nullptr),
      m_isScanningNmea(false),
      m_viewScale(1.0),
      m_observationsChoiceCount(1)
{
#if wxCHECK_VERSION(3,0,0)
    SetLayoutAdaptationMode(wxDIALOG_ADAPTATION_MODE_ENABLED);
#endif // wxCHECK_VERSION(3,0,0)

    m_MiniPanel = new ooMiniPanel(m_panelObservations);
    m_MiniPanel->SetToggleWindowButtonLabel("Minimize");
    this->Connect(wxEVT_SHOW, wxShowEventHandler(ooMiniPanel::OnShow), NULL, m_MiniPanel);
    
    std::function<void(wxCommandEvent&)> refreshHandler = [&](wxCommandEvent& event)
        {
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

    UpdateProjectCellEditors();
    OnNmeaFieldUpdate();

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
    }
    this->Disconnect(wxEVT_SHOW, wxShowEventHandler(ooMiniPanel::OnShow), NULL, m_MiniPanel);

    m_BackupTimer.Stop();

    if (!m_Observations) return;

    SaveObservations(GetBackupFilename(m_currentObservationsIndex));
}

void ooControlDialogImpl::NewProject()
{
    // delete columns
    if (m_gridProject->GetNumberCols() > 0)
        m_gridProject->DeleteCols(0, m_gridProject->GetNumberCols());

    // add columns
    m_gridProject->InsertCols(0, 7);

    // set the column sizes
    wxArrayInt allColSizes;
    allColSizes.Add(200);
    allColSizes.Add(90);
    allColSizes.Add(90);
    allColSizes.Add(90);
    allColSizes.Add(100);
    allColSizes.Add(200);
    allColSizes.Add(70);
    wxGridSizesInfo colSizes = wxGridSizesInfo(70, allColSizes);
    m_gridProject->SetColSizes(colSizes);

    // set the column labels and cell editors
    for (int c=0; c<m_gridProject->GetNumberCols(); ++c)
    {
        m_gridProject->SetColLabelValue(c, "");
    }
    UpdateProjectCellEditors();

    // fill the table
    m_gridProject->SetCellValue(0, 0, "Start timestamp");
    m_gridProject->SetCellValue(1, 0, "Start Timestamp UTC");
    m_gridProject->SetCellValue(0, 1, "Lat");
    m_gridProject->SetCellValue(1, 1, "Start Latitude");
    m_gridProject->SetCellValue(0, 2, "Lon");
    m_gridProject->SetCellValue(1, 2, "Start Longitude");
    m_gridProject->SetCellValue(0, 3, "Duration");
    m_gridProject->SetCellValue(1, 3, "Observation Duration");
    m_gridProject->SetCellValue(0, 4, "Species");
    m_gridProject->SetCellValue(1, 4, "Text");
    m_gridProject->SetCellValue(0, 5, "Notes");
    m_gridProject->SetCellValue(1, 5, "Text");
    m_gridProject->SetCellValue(0, 6, "Mark GUID");
    m_gridProject->SetCellValue(1, 6, "Mark GUID");

    m_textProjectName->SetValue(wxString::Format(wxT("Default Project %i"), m_currentObservationsIndex + 1));
}

void ooControlDialogImpl::UpdateProjectCellEditors()
{
    for (int c = 0; c < m_gridProject->GetNumberCols(); ++c) {
      wxGridCellChoiceEditor* observationFieldTypeEditor =
          new wxGridCellChoiceEditor(ooObservations::GetObservationFieldTypes());
      m_gridProject->SetCellEditor(1, c, observationFieldTypeEditor);
    }
}

bool ooControlDialogImpl::LoadProject(const ooProject& project)
{
    // delete columns
    if (m_gridProject->GetNumberCols() > 0)
        m_gridProject->DeleteCols(0, m_gridProject->GetNumberCols());

    m_textProjectName->SetValue(project.GetName());
    m_colourProject->SetColour(project.GetColor());

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
  return ooProject(
      m_textProjectName->GetValue(),
      colSizes,
      fieldTypes,
      labels,
      m_colourProject->GetColour());
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

    m_ObservationsTable->SetMinSize(wxSize(1, 1));

    m_ObservationsTable->AssignTable(m_Observations);
    m_ObservationsTable->SetColSizes(m_Observations->GetColSizes());

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
	m_ObservationsTable->SetLabelFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
    m_ObservationsTable->SetLabelBackgroundColour(m_Observations->GetProject().GetColor());
    m_ObservationsTable->SetLabelTextColour(wxColour(255, 255, 255));

	// Cell Defaults
	m_ObservationsTable->SetDefaultCellFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_ObservationsTable->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );

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
        wxFileDialog saveFileDialog(this, _("Save observations to XML file"), "",
                                    m_Observations->GetProject().GetName(),
                                    "XML file (*.xml)|*.xml",
                                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        
        if (saveFileDialog.ShowModal() == wxID_CANCEL) return false;
        savePath = saveFileDialog.GetPath();
    }
    
    wxFileOutputStream output_stream(savePath);
    if (!output_stream.IsOk()) {
      wxMessageBox("Unable to save observations to file " + savePath + ".",
          "Error", wxOK, this);
      return false;
    }

    // Update the col size
    m_Observations->SetColSizes(m_ObservationsTable->GetColSizes());

    if (stopObservation) {
        m_Observations->StopObservation();
    }
    m_Observations->SaveToXML(output_stream.GetFile());
    return true;
}

bool ooControlDialogImpl::LoadObservations(const wxString& filename)
{
    if (!m_Observations) return false;

    m_Observations->StopObservation();

    if (!m_Observations->ReadFromXML(filename, m_CurrentProject)) {
        wxMessageBox("Error loading observations file " + filename + ".",
                     "Error", wxOK, this);
        return false;
    }

    m_Observations->UpdateObservationsFromMarks();
    SetupObservationsForProject();
    RefreshGridAppearance(m_ObservationsTable);

    LoadProject(m_Observations->GetProject());
    g_openobserver_pi->SetProject(m_textProjectName->GetValue(),
                                  m_currentObservationsIndex);
    m_choiceObservations->SetString(m_currentObservationsIndex,
                                    m_Observations->GetProject().GetName());
    m_choiceObservations->GetParent()->Layout();

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
                   LoadObservations(backupFilename));      
    return result;
}

void ooControlDialogImpl::RefreshGridAppearance(wxGrid* grid) {
  if (!grid) return;

  int rows = grid->GetNumberRows();

  wxColour evenColor(255, 255, 255);  // White
  wxColour oddColor(250, 250, 250);   // Very light gray

  grid->BeginBatch();

  for (int row = 0; row < rows; row++) {
    wxGridCellAttr* attr = new wxGridCellAttr();

    if (row % 2 == 0)
      attr->SetBackgroundColour(evenColor);
    else
      attr->SetBackgroundColour(oddColor);

    grid->SetRowAttr(row, attr);
  }

  grid->EndBatch();
  grid->ForceRefresh();
}

void ooControlDialogImpl::ApplyModernGridStyle(wxGrid* grid) {
  if (!grid) return;

  //grid->EnableGridLines(false);  // supprime lignes moches

  grid->SetDefaultCellFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                  wxFONTWEIGHT_NORMAL));

  grid->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_TOP);

  grid->SetSelectionBackground(wxColour(0, 120, 215));  // bleu Windows moderne
  grid->SetSelectionForeground(*wxWHITE);

  grid->SetRowLabelSize(40);
}


void ooControlDialogImpl::SetupObservationsForProject()
{
    if (!m_Observations) return;
    if (!m_ObservationsTable) return;

    // update table color
    m_ObservationsTable->SetLabelBackgroundColour(
        m_Observations->GetProject().GetColor()
    );

    // update column sizes of table to match
    m_ObservationsTable->SetColSizes(m_Observations->GetColSizes());

    // Setup listings editors
    const int C = m_Observations->GetColsCount();

    for (int c = 0; c < C; ++c) {
        const wxString field_type = m_Observations->GetColFieldTypes()[c];
        wxArrayString items;
        if (ooObservations::GetListing(field_type, items)) {
            wxGridCellAttr* attr = new wxGridCellAttr();
            attr->SetEditor(new wxGridCellChoiceEditor(items, true));
            m_ObservationsTable->SetColAttr(c, attr);
        }
    }
}

void ooControlDialogImpl::SetPositionFix(time_t fixTime, double lat, double lon)
{
    //char dateString[16];
    //strftime(dateString, 16, "%F", gmtime(&fixTime));
    //char timeString[16];
    //strftime(timeString, 16, "%T", gmtime(&fixTime));
    wxString dateString = m_Observations->GetUtcTimeFromNMEA(ooObservations::UTC_TIME_DATE);
    wxString timeString = m_Observations->GetUtcTimeFromNMEA(ooObservations::UTC_TIME_TIME);

    m_ObservationsDate->SetValue(dateString);
    m_ObservationsTime->SetValue(timeString);
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

void ooControlDialogImpl::UseProject()
{
    // TODO Remove this function once we can create various ooObservations

    g_openobserver_pi->SetProject(m_textProjectName->GetValue(),
                                  m_currentObservationsIndex);

    // update the project tab interface
    SetProjectEditable(false);

    m_Observations->SetProject(GenerateProject());

    // setup observations for the project
    SetupObservationsForProject();

    m_choiceObservations->SetString(m_currentObservationsIndex,
                                    m_textProjectName->GetValue());
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
  }
}

void ooControlDialogImpl::OnButtonClickProjectEditUse(wxCommandEvent& event)
{
    if (m_gridProject->IsEnabled()) 
    {
        // exit edit mode and use project

        // first, prompt user to export observations
        if (m_Observations &&
            m_Observations->GetRowsCount() > 0 &&
            !m_Observations->GetProject().IsUpdatable(GenerateProject()))
        {
            const int response = wxMessageBox(
              "Warning: your current observations will be cleared. Do you want "
              "to save them first?",
              "Export your observations?", wxYES | wxNO | wxCANCEL, this);
            if (response == wxCANCEL) return;
            if (response == wxYES)
            {
                if (!SaveObservations())
                    return;
            }
        }

        // second, ensure that there is a Mark GUID column and, if not, add one as the last column        
        EnsureProjectHasFieldType("Start Timestamp UTC", "Start Timestamp UTC");
        EnsureProjectHasFieldType("Start Longitude", "Lon");
        EnsureProjectHasFieldType("Start Latitude", "Lat");
        EnsureProjectHasFieldType("Mark GUID", "Mark GUID");

        UseProject();
        
    } else {
        // enter edit mode
        SetProjectEditable(true);
    }
}

void ooControlDialogImpl::SetProjectEditable(bool editable)
{
    if (editable) {
        m_ProjectEditUse->SetLabel("Use");
        m_gridProject->Enable();
        m_ProjectNew->Enable();
        m_ProjectNewColumn->Enable();
        m_ProjectDeleteColumn->Enable();
        m_textProjectName->Enable();
        m_colourProject->Enable();
    } else {
        m_ProjectEditUse->SetLabel("Edit");
        m_gridProject->Disable();
        m_ProjectNew->Disable();
        m_ProjectNewColumn->Disable();
        m_ProjectDeleteColumn->Disable();
        m_colourProject->Disable();

        // disable project name text field, first setting value in code to ensure it
        // stays
        m_textProjectName->SetValue(m_textProjectName->GetValue());
        m_textProjectName->Disable();
    }
}

void ooControlDialogImpl::OnButtonClickProjectNew(wxCommandEvent& event)
{
    const int response = wxMessageBox("Warning: your current project will be cleared. Do you want to continue?", "Warning", wxYES_NO, this);
    
    if (response == wxYES)
        NewProject();
}

void ooControlDialogImpl::OnButtonClickProjectNewColumn(wxCommandEvent& event)
{
    m_gridProject->InsertCols(0, 1);
    m_gridProject->SetColLabelValue(0, "");

    wxGridCellChoiceEditor *observationFieldTypeEditor = new wxGridCellChoiceEditor(ooObservations::GetObservationFieldTypes());
    m_gridProject->SetCellEditor(1, 0, observationFieldTypeEditor);
}

void ooControlDialogImpl::OnButtonClickProjectDeleteColumn(wxCommandEvent& event)
{
    if (m_gridProject->GetNumberCols() > 0)
        m_gridProject->DeleteCols(0);
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

    
    if (m_Observations->GetNumberRows() <= 0) return;

    wxArrayInt selectedRows = m_ObservationsTable->GetSelectedRows();
    selectedRows.Sort([](int * a, int* b) { return *a > *b  ? -1 :
                                                   *a == *b ? 0  :
                                                              1; });
    if (selectedRows.IsEmpty()) return;

    const int response = wxMessageBox(
        wxString::Format(wxT("Warning: %i selected observations will be deleted. Do you want to "
        "continue?"), selectedRows.GetCount()),
        "Delete observation(s)?", wxYES_NO, this);
    if (response != wxYES) return;

    for (int i = 0; i < (int)selectedRows.GetCount(); i++) {
        m_Observations->DeleteMarks(selectedRows[i]);
        m_Observations->DeleteRows(selectedRows[i]);
    }
    RefreshGridAppearance(m_ObservationsTable);
}

void ooControlDialogImpl::OnButtonClickExportObservations( wxCommandEvent& event )
{
    if (!m_Observations) return;

    wxFileDialog exportFileDialog(this, _("Export observations to CSV file"), "", m_ObservationsDate->GetValue(), "CSV file (*.csv)|*.csv", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
 
    if (exportFileDialog.ShowModal() == wxID_CANCEL)
        return;
 
    wxFileOutputStream output_stream(exportFileDialog.GetPath());
    if (!output_stream.IsOk())
    {
        wxMessageBox("Unable to save observations to file " + exportFileDialog.GetPath() + ".", "Error", wxOK, this);
        return;
    }

    m_Observations->SaveToCSV(output_stream.GetFile());
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

void ooControlDialogImpl::OnButtonClickObservationsAddMarks( wxCommandEvent& event )
{
    if (!m_Observations) return;

    m_Observations->AddMarks();

    m_ObservationsTable->ForceRefresh();
}

void ooControlDialogImpl::OnButtonClickObservationsDeleteMarks( wxCommandEvent& event )
{
    if (!m_Observations) return;

    m_Observations->DeleteMarks();

    m_ObservationsTable->ForceRefresh();
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
        this, _("Load observations from XML file"), "", "", "XML file (*.xml)|*.xml",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);
    
    if (loadObservationsDialog.ShowModal() == wxID_CANCEL) return;
    
    if (!LoadObservations(loadObservationsDialog.GetPath())) {
        wxMessageBox("Unable to load observations from file " +
                     loadObservationsDialog.GetPath() + ".",
                     "Error", wxOK, this);
        return;
    }
}

void ooControlDialogImpl::OnButtonClickSaveObservation(wxCommandEvent& event)
{
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
            JumpToPosition(lat, lon, m_viewScale);
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

wxString ooControlDialogImpl::GetBackupFilename(int index)
{
  wxFileName backup(*g_pData,
                    index == -1 ? "observations.xml" : wxString::Format(wxT("observations_%i.xml"), index));
  return backup.GetFullPath();
}
