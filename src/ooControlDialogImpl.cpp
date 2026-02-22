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
    : ooControlDialogDef(parent), m_MiniPanel(nullptr), m_Observations(nullptr), m_ObservationsTable(nullptr)
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

    for (int c=0; c<m_gridProject->GetNumberCols(); ++c)
    {
        wxGridCellChoiceEditor *observationFieldTypeEditor = new wxGridCellChoiceEditor(ooObservations::GetObservationFieldTypes());
        m_gridProject->SetCellEditor(1, c, observationFieldTypeEditor);
    }

    wxFileName backup(*g_pData, "observations.xml");
    m_BackupFilename = backup.GetFullPath();

    // bind backup timer (started in RestoreBackupObservations)
    m_BackupTimer.Bind(wxEVT_TIMER, &ooControlDialogImpl::OnBackupTimer, this, m_BackupTimer.GetId());
    // start timer to backup observations every 30 seconds
    m_BackupTimer.Start(30000);  // 30'000 ms = 30 s
}

ooControlDialogImpl::~ooControlDialogImpl()
{
    this->Disconnect(wxEVT_SHOW, wxShowEventHandler(ooMiniPanel::OnShow), NULL, m_MiniPanel);

    m_BackupTimer.Stop();

    if (!m_Observations) return;

    SaveObservations(m_BackupFilename);
}

void ooControlDialogImpl::NewProject()
{
    // delete columns
    if (m_gridProject->GetNumberCols() > 0)
        m_gridProject->DeleteCols(0, m_gridProject->GetNumberCols());

    // add columns
    m_gridProject->InsertCols(0, 8);

    // set the column sizes
    wxArrayInt allColSizes;
    allColSizes.Add(70);
    allColSizes.Add(70);
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

        wxGridCellChoiceEditor *observationFieldTypeEditor = new wxGridCellChoiceEditor(ooObservations::GetObservationFieldTypes());
        m_gridProject->SetCellEditor(1, c, observationFieldTypeEditor);
    }

    // fill the table
    m_gridProject->SetCellValue(0, 0, "Date");
    m_gridProject->SetCellValue(1, 0, "Start Date");
    m_gridProject->SetCellValue(0, 1, "Time");
    m_gridProject->SetCellValue(1, 1, "Start Time");
    m_gridProject->SetCellValue(0, 2, "Lat");
    m_gridProject->SetCellValue(1, 2, "Start Latitude");
    m_gridProject->SetCellValue(0, 3, "Lon");
    m_gridProject->SetCellValue(1, 3, "Start Longitude");
    m_gridProject->SetCellValue(0, 4, "Duration");
    m_gridProject->SetCellValue(1, 4, "Observation Duration");
    m_gridProject->SetCellValue(0, 5, "Species");
    m_gridProject->SetCellValue(1, 5, "Text");
    m_gridProject->SetCellValue(0, 6, "Notes");
    m_gridProject->SetCellValue(1, 6, "Text");
    m_gridProject->SetCellValue(0, 7, "Mark GUID");
    m_gridProject->SetCellValue(1, 7, "Mark GUID");

    m_textProjectFile->SetValue("");
    m_textProjectName->SetValue("Default Project");
}

bool ooControlDialogImpl::LoadProject(const ooProject& project)
{
    // delete columns
    if (m_gridProject->GetNumberCols() > 0)
        m_gridProject->DeleteCols(0, m_gridProject->GetNumberCols());

    m_textProjectName->SetValue(project.GetName());
    m_textProjectFile->SetValue(wxString());

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
        
        wxGridCellChoiceEditor* observationFieldTypeEditor =
            new wxGridCellChoiceEditor(
                ooObservations::GetObservationFieldTypes());
        m_gridProject->SetCellEditor(1, c, observationFieldTypeEditor);
    }

    m_CurrentProject = project;

    return true;
}

bool ooControlDialogImpl::LoadProject(const wxString& filename)
{
    wxXmlDocument xmlDoc;
    if (!xmlDoc.Load(filename)) {
        return false;
    }

    ooProject project;
    int fileVersion = -1;
    bool res = (xmlDoc.GetRoot()->GetAttribute("file_version").ToInt(&fileVersion) &&
               fileVersion == XML_FILE_VERSION_PROJECT) &&
               project.ReadFromXML(xmlDoc.GetRoot()) &&
               LoadProject(project);
    if (res) {
        m_textProjectFile->SetValue(filename);
    }

    return res;
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
      labels);
}

void ooControlDialogImpl::SaveProject(wxFile *file) const
{
    ooProject project = GenerateProject();

    wxXmlDocument xmlDoc;
    wxXmlNode* xmlProject = project.SaveToXML();
    xmlProject->AddAttribute("creator", "Open Observer for OpenCPN");
    xmlProject->AddAttribute("file_version", wxString::FromDouble(XML_FILE_VERSION_PROJECT));
    
    xmlDoc.SetRoot(xmlProject);

    // write the output to a wxString
    wxStringOutputStream stream;
    xmlDoc.Save(stream);

    // write the string to the file
    file->Write(stream.GetString());
}

void ooControlDialogImpl::CreateObservationsTable(ooObservations *observations)
{
    m_Observations = observations;

    m_ObservationsTable = new wxGrid(m_panelObservations, wxID_ANY, wxDefaultPosition, wxSize(740, 465), 0);
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

	// Cell Defaults
	m_ObservationsTable->SetDefaultCellFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_ObservationsTable->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );

    wxGridCellAutoWrapStringEditor *editor = new wxGridCellAutoWrapStringEditor();
    m_ObservationsTable->SetDefaultEditor(editor);
    
    wxGridCellAutoWrapStringRenderer *renderer = new wxGridCellAutoWrapStringRenderer();
    m_ObservationsTable->SetDefaultRenderer(renderer);

    // m_ObservationsTable is a wxGrid, ultimately derived from wxWindow
	m_fgSizerObservations->Add(m_ObservationsTable, 0, wxALL|wxEXPAND, 5);
    
    m_ObservationsTable->EnableDragColSize(true);
    ApplyModernGridStyle(m_ObservationsTable);
}

bool ooControlDialogImpl::SaveObservations(const wxString& filename, bool stopObservation)
{
    wxString savePath = filename;
    if (savePath.IsEmpty()) {
        wxFileDialog saveFileDialog(this, _("Save observations to XML file"), "",
                                    m_ObservationsDate->GetValue(),
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
    
    SetupObservationsForProject();
    RefreshGridAppearance(m_ObservationsTable);

    LoadProject(m_Observations->GetProject());
    g_openobserver_pi->SetProject(m_textProjectFile->GetValue(),
                                  m_textProjectName->GetValue());

    return true;
}

void ooControlDialogImpl::RestoreBackupObservations()
{
    LoadObservations(m_BackupFilename);
    return;
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
    char dateString[16];
    strftime(dateString, 16, "%F", gmtime(&fixTime));
    char timeString[16];
    strftime(timeString, 16, "%T", gmtime(&fixTime));

    m_ObservationsDate->SetValue(dateString);
    m_ObservationsTime->SetValue(timeString);
    m_ObservationsLat->SetValue(toSDMM_PlugIn(1, lat));
    m_ObservationsLon->SetValue(toSDMM_PlugIn(2, lon));
}

void ooControlDialogImpl::UseProject()
{
    // TODO Remove this function once we can create various ooObservations

    // update the project tab interface
    m_ProjectEditUse->SetLabel("Edit");
    m_gridProject->Disable();
    m_ProjectNew->Disable();
    m_ProjectLoad->Disable();
    m_ProjectSave->Disable();
    m_ProjectNewColumn->Disable();
    m_ProjectDeleteColumn->Disable();

    g_openobserver_pi->SetProject(m_textProjectFile->GetValue(), m_textProjectName->GetValue());

    // disable project name text field, first setting value in code to ensure it stays
    m_textProjectName->SetValue(m_textProjectName->GetValue());
    m_textProjectName->Disable();

    m_Observations->SetProject(GenerateProject());

    // setup observations for the project
    SetupObservationsForProject();
}

void ooControlDialogImpl::OnButtonClickProjectEditUse(wxCommandEvent& event)
{
    if (m_gridProject->IsEnabled()) 
    {
        // exit edit mode and use project

        // first, prompt user to export observations
        if (m_Observations)
        {
            const int response = wxMessageBox("Warning: your current observations will be cleared. Do you want to save them first?", "Export your observations?", wxYES_NO, this);
            if (response == wxYES)
            {
                if (!SaveObservations())
                    return;
            }
        }

        // second, ensure that there is a Mark GUID column and, if not, add one as the last column
        bool has_mark_guid_col = false;
        for (int c=0; c<m_gridProject->GetNumberCols(); ++c)
        {
            if(m_gridProject->GetCellValue(1, c).IsSameAs("Mark GUID"))
            {
                has_mark_guid_col = true;
                break;
            }
        }
        if (!has_mark_guid_col)
        {
            m_gridProject->AppendCols();
            m_gridProject->SetColLabelValue(m_gridProject->GetNumberCols()-1, "");
            m_gridProject->SetCellValue(1, m_gridProject->GetNumberCols()-1, "Mark GUID");
        }

        UseProject();
        
    } else {
        // enter edit mode
        m_ProjectEditUse->SetLabel("Use");
        m_gridProject->Enable();
        m_ProjectNew->Enable();
        m_ProjectLoad->Enable();
        m_ProjectSave->Enable();
        m_ProjectNewColumn->Enable();
        m_ProjectDeleteColumn->Enable();
        m_textProjectName->Enable();
    }
}

void ooControlDialogImpl::OnButtonClickProjectNew(wxCommandEvent& event)
{
    const int response = wxMessageBox("Warning: your current project will be cleared. Do you want to continue?", "Warning", wxYES_NO, this);
    
    if (response == wxYES)
        NewProject();
}

void ooControlDialogImpl::OnButtonClickProjectLoad(wxCommandEvent& event)
{
    const int response = wxMessageBox("Warning: your current project will be cleared. Do you want to continue?", "Warning", wxYES_NO, this);
    
    if (response != wxYES)
        return;

    wxFileDialog loadProjectDialog(this, _("Load project from XML file"), "", "", "XML file (*.xml)|*.xml", wxFD_OPEN|wxFD_FILE_MUST_EXIST|wxFD_CHANGE_DIR);

    if (loadProjectDialog.ShowModal() == wxID_CANCEL)
        return;

    if (!LoadProject(loadProjectDialog.GetPath()))
    {
        wxMessageBox("Unable to load project from file " + loadProjectDialog.GetPath() + ".", "Error", wxOK, this);
        return;
    }
}

void ooControlDialogImpl::OnButtonClickProjectSave(wxCommandEvent& event)
{
    wxFileDialog saveProjectDialog(this, _("Save project to XML file"), "", m_textProjectName->GetValue(), "XML file (*.xml)|*.xml", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
 
    if (saveProjectDialog.ShowModal() == wxID_CANCEL)
        return;
 
    wxFileOutputStream output_stream(saveProjectDialog.GetPath());
    if (!output_stream.IsOk())
    {
        wxMessageBox("Unable to save project to file " + saveProjectDialog.GetPath() + ".", "Error", wxOK, this);
        return;
    }

    SaveProject(output_stream.GetFile());

    m_textProjectFile->SetValue(saveProjectDialog.GetPath());
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

    const int response = wxMessageBox(
        "Warning: your last observation will be deleted. Do you want to "
        "continue?",
        "Delete last observation?", wxYES_NO, this);
    if (response != wxYES) return;

    m_Observations->DeleteRows(0);
}

void ooControlDialogImpl::OnButtonClickDeleteAllObservations(wxCommandEvent& event)
{
    if (!m_Observations) return;

    if (m_Observations->GetNumberRows() <= 0) return;

    const int response = wxMessageBox("Warning: all your current observations will be deleted. Do you want to continue?", "Delete all observations?", wxYES_NO, this);

    if (response == wxYES)
    {
        m_Observations->DeleteRows(0, m_Observations->GetNumberRows());
    }
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

    SaveObservations(m_BackupFilename, false);
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
    const int response = wxMessageBox(
        "Warning: your current observations will be cleared. Do you want to continue?",
        "Warning", wxYES_NO, this);
    
    if (response != wxYES) return;
    
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
    bool bShowButtons = (event.GetSelection() == 1);
    m_buttonLoadObs->Show(bShowButtons);
    m_buttonSaveObs->Show(bShowButtons);
    m_ObservationsExportObservations->Show(bShowButtons);
    m_ObservationsImportObservations->Show(bShowButtons);
}
