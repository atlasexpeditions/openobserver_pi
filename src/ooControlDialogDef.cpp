///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "ooControlDialogDef.h"

///////////////////////////////////////////////////////////////////////////

ooControlDialogDef::ooControlDialogDef()
{
}

ooControlDialogDef::ooControlDialogDef( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
{
	this->Create( parent, id, title, pos, size, style );
}

bool ooControlDialogDef::Create( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
{
	if ( !wxFrame::Create( parent, id, title, pos, size, style ) )
	{
		return false;
	}

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_SizerControl = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 2, 1, 0, 0 );
	fgSizer3->AddGrowableCol( 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->AddGrowableRow( 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	m_notebookControl = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelProject = new wxPanel( m_notebookControl, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelProject->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
	m_panelProject->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	wxFlexGridSizer* fgSizerProject;
	fgSizerProject = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizerProject->AddGrowableCol( 0 );
	fgSizerProject->AddGrowableRow( 4 );
	fgSizerProject->SetFlexibleDirection( wxBOTH );
	fgSizerProject->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	wxFlexGridSizer* fgSizerProjectButtons;
	fgSizerProjectButtons = new wxFlexGridSizer( 1, 6, 0, 0 );
	fgSizerProjectButtons->AddGrowableCol( 0 );
	fgSizerProjectButtons->SetFlexibleDirection( wxBOTH );
	fgSizerProjectButtons->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ProjectEditUse = new wxButton( m_panelProject, wxID_ANY, _("Edit"), wxDefaultPosition, wxDefaultSize, 0 );

	m_ProjectEditUse->SetDefault();
	fgSizerProjectButtons->Add( m_ProjectEditUse, 0, wxALL, 5 );

	m_ProjectNew = new wxButton( m_panelProject, wxID_ANY, _("New Project"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ProjectNew->Enable( false );

	fgSizerProjectButtons->Add( m_ProjectNew, 0, wxALL, 5 );

	m_ProjectNewColumn = new wxButton( m_panelProject, wxID_ANY, _("Insert Column"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ProjectNewColumn->Enable( false );

	fgSizerProjectButtons->Add( m_ProjectNewColumn, 0, wxALL, 5 );

	m_ProjectDeleteColumn = new wxButton( m_panelProject, wxID_ANY, _("Delete Selected"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ProjectDeleteColumn->Enable( false );

	fgSizerProjectButtons->Add( m_ProjectDeleteColumn, 0, wxALL, 5 );


	fgSizerProject->Add( fgSizerProjectButtons, 0, 0, 5 );

	m_staticline3 = new wxStaticLine( m_panelProject, wxID_ANY, wxDefaultPosition, wxSize( 10,1 ), wxLI_HORIZONTAL|wxLI_VERTICAL );
	fgSizerProject->Add( m_staticline3, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 2, 4, 0, 0 );
	fgSizer8->AddGrowableCol( 1 );
	fgSizer8->AddGrowableCol( 3 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText6 = new wxStaticText( m_panelProject, wxID_ANY, _("Name"), wxDefaultPosition, wxSize( 100,-1 ), 0 );
	m_staticText6->Wrap( -1 );
	fgSizer8->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textProjectName = new wxTextCtrl( m_panelProject, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
fgSizer8->Add( m_textProjectName, 1, wxALL|wxEXPAND, 5 );

fgSizer8->Add( 0, 0, 1, wxEXPAND, 5 );
fgSizer8->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText9 = new wxStaticText( m_panelProject, wxID_ANY, _("Color"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer8->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_colourProject = new wxColourPickerCtrl( m_panelProject, wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE );
	fgSizer8->Add( m_colourProject, 0, wxALL|wxEXPAND, 5 );

	m_staticText10 = new wxStaticText( m_panelProject, wxID_ANY, _("Mark icon"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer8->Add( m_staticText10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_listMarkIconsChoices;
	m_listMarkIcons = new wxChoice( m_panelProject, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_listMarkIconsChoices, 0 );
	m_listMarkIcons->SetSelection( 0 );
	fgSizer8->Add( m_listMarkIcons, 0, wxALL|wxEXPAND, 5 );


	fgSizerProject->Add( fgSizer8, 0, wxEXPAND, 5 );
	wxBoxSizer* bSizerProjectDescription;
	bSizerProjectDescription = new wxBoxSizer( wxVERTICAL );

	m_staticTextProjectDescription = new wxStaticText( m_panelProject, wxID_ANY, _("Description"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextProjectDescription->Wrap( -1 );
	bSizerProjectDescription->Add( m_staticTextProjectDescription, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_textProjectDescription = new wxTextCtrl(
    m_panelProject,
    wxID_ANY,
    wxEmptyString,
    wxDefaultPosition,
    wxSize( -1,80 ),
    wxTE_MULTILINE | wxTE_PROCESS_ENTER
);
m_textProjectDescription->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
m_textProjectDescription->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
bSizerProjectDescription->Add( m_textProjectDescription, 0, wxALL|wxEXPAND, 5 );

fgSizerProject->Add( bSizerProjectDescription, 0, wxEXPAND, 5 );
	
m_gridProject = new wxGrid( m_panelProject, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0 );

	// Grid
	m_gridProject->CreateGrid( 2, 0 );
	m_gridProject->EnableEditing( true );
	m_gridProject->EnableGridLines( true );
	m_gridProject->EnableDragGridSize( false );
	m_gridProject->SetMargins( 0, 0 );

	// Columns
	m_gridProject->EnableDragColMove( false );
	m_gridProject->EnableDragColSize( true );
	m_gridProject->SetColLabelSize( 20 );
	m_gridProject->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridProject->EnableDragRowSize( false );
	m_gridProject->SetRowLabelValue( 0, _("Label") );
	m_gridProject->SetRowLabelValue( 1, _("Type") );
	m_gridProject->SetRowLabelSize( wxGRID_AUTOSIZE );
	m_gridProject->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridProject->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_gridProject->Enable( false );
	m_gridProject->SetMinSize( wxSize( 1,1 ) );

	fgSizerProject->Add( m_gridProject, 1, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText7 = new wxStaticText( m_panelProject, wxID_ANY, _("NMEA :"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer9->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTextNMEA = new wxStaticText( m_panelProject, wxID_ANY, _("0 fields"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNMEA->Wrap( -1 );
	m_staticTextNMEA->SetMinSize( wxSize( 100,-1 ) );

	fgSizer9->Add( m_staticTextNMEA, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_buttonScanNmea = new wxButton( m_panelProject, wxID_ANY, _("Start NMEA Scan"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_buttonScanNmea, 0, wxALL, 5 );

	m_checkShowAdvancedNmeaFields = new wxCheckBox(
    	m_panelProject,
    	wxID_ANY,
    	_("Show advanced NMEA fields"),
    	wxDefaultPosition,
    	wxDefaultSize,
    	0
	);
	m_checkShowAdvancedNmeaFields->SetValue(false);
	fgSizer9->Add( m_checkShowAdvancedNmeaFields, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText11 = new wxStaticText( m_panelProject, wxID_ANY, _("Listings :"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer9->Add( m_staticText11, 0, wxALL, 5 );

	m_staticTextListings = new wxStaticText( m_panelProject, wxID_ANY, _("0 fields"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextListings->Wrap( -1 );
	fgSizer9->Add( m_staticTextListings, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_buttonRefreshListings = new wxButton( m_panelProject, wxID_ANY, _("Refresh Listings"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_buttonRefreshListings, 0, wxALL, 5 );

	m_buttonOpenResourcesFolder = new wxButton( m_panelProject, wxID_ANY, _("Open Resources Folder"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_buttonOpenResourcesFolder, 0, wxALL|wxEXPAND, 5 );


	fgSizerProject->Add( fgSizer9, 0, wxEXPAND, 5 );


	m_panelProject->SetSizer( fgSizerProject );
	m_panelProject->Layout();
	fgSizerProject->Fit( m_panelProject );
	m_notebookControl->AddPage( m_panelProject, _("Project"), true );
	m_panelObservations = new wxPanel( m_notebookControl, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_fgSizerObservations = new wxFlexGridSizer( 6, 1, 0, 0 );
	m_fgSizerObservations->AddGrowableCol( 0 );
	m_fgSizerObservations->AddGrowableRow( 5 );
	m_fgSizerObservations->SetFlexibleDirection( wxBOTH );
	m_fgSizerObservations->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	bSizerTopButtons = new wxBoxSizer( wxHORIZONTAL );


	m_fgSizerObservations->Add( bSizerTopButtons, 0, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( m_panelObservations, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_fgSizerObservations->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgSizerObservationsLabels;
	fgSizerObservationsLabels = new wxFlexGridSizer( 1, 11, 0, 0 );
	fgSizerObservationsLabels->SetFlexibleDirection( wxBOTH );
	fgSizerObservationsLabels->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ObservationsDateLabel1 = new wxStaticText( m_panelObservations, wxID_ANY, _("Current Data: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_ObservationsDateLabel1->Wrap( -1 );
	fgSizerObservationsLabels->Add( m_ObservationsDateLabel1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 5 );

	m_ObservationsUtcSource = new wxStaticText( m_panelObservations, wxID_ANY, _("Computer UTC"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ObservationsUtcSource->Wrap( -1 );
	{
		wxFont utcSourceFont = m_ObservationsUtcSource->GetFont();
		utcSourceFont.SetWeight(wxFONTWEIGHT_BOLD);
		m_ObservationsUtcSource->SetFont(utcSourceFont);
	}
	fgSizerObservationsLabels->Add( m_ObservationsUtcSource, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5 );

	m_ObservationsDateLabel = new wxStaticText( m_panelObservations, wxID_ANY, _("Date"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ObservationsDateLabel->Wrap( -1 );
	fgSizerObservationsLabels->Add( m_ObservationsDateLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ObservationsDate = new wxTextCtrl( m_panelObservations, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizerObservationsLabels->Add( m_ObservationsDate, 1, wxALL, 5 );

	m_ObservationsTimeLabel = new wxStaticText( m_panelObservations, wxID_ANY, _("Time"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ObservationsTimeLabel->Wrap( -1 );
	fgSizerObservationsLabels->Add( m_ObservationsTimeLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ObservationsTime = new wxTextCtrl( m_panelObservations, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizerObservationsLabels->Add( m_ObservationsTime, 1, wxALL, 5 );

	m_ObservationsLatLabel = new wxStaticText( m_panelObservations, wxID_ANY, _("Lat"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ObservationsLatLabel->Wrap( -1 );
	fgSizerObservationsLabels->Add( m_ObservationsLatLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ObservationsLat = new wxTextCtrl( m_panelObservations, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 120,-1 ), wxTE_READONLY | wxTE_CENTER );
	m_ObservationsLat->SetMinSize( wxSize( 120,-1 ) );
	fgSizerObservationsLabels->Add( m_ObservationsLat, 0, wxALL, 5 );

	m_ObservationsLonLabel = new wxStaticText( m_panelObservations, wxID_ANY, _("Lon"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ObservationsLonLabel->Wrap( -1 );
	fgSizerObservationsLabels->Add( m_ObservationsLonLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ObservationsLon = new wxTextCtrl( m_panelObservations, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 120,-1 ), wxTE_READONLY | wxTE_CENTER );
	m_ObservationsLon->SetMinSize( wxSize( 120,-1 ) );
	fgSizerObservationsLabels->Add( m_ObservationsLon, 0, wxALL, 5 );


	m_fgSizerObservations->Add( fgSizerObservationsLabels, 0, wxEXPAND, 5 );

	m_staticline11 = new wxStaticLine( m_panelObservations, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_fgSizerObservations->Add( m_staticline11, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgSizerObservationsButtons;
	fgSizerObservationsButtons = new wxFlexGridSizer( 1, 6, 0, 0 );
	fgSizerObservationsButtons->SetFlexibleDirection( wxBOTH );
	fgSizerObservationsButtons->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ObservationsNew = new wxButton( m_panelObservations, wxID_ANY, _("Add Observation"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerObservationsButtons->Add( m_ObservationsNew, 0, wxALL, 5 );

	m_ObservationsDelete = new wxButton( m_panelObservations, wxID_ANY, _("Delete selected"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerObservationsButtons->Add( m_ObservationsDelete, 0, wxALL, 5 );

	m_checkShowObservationMarks = new wxCheckBox( m_panelObservations, wxID_ANY, _("Show Observation Marks"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerObservationsButtons->Add( m_checkShowObservationMarks, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_fgSizerObservations->Add( fgSizerObservationsButtons, 0, wxEXPAND, 5 );


	m_panelObservations->SetSizer( m_fgSizerObservations );
	m_panelObservations->Layout();
	m_fgSizerObservations->Fit( m_panelObservations );
	m_notebookControl->AddPage( m_panelObservations, _("Observations"), false );

	fgSizer3->Add( m_notebookControl, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerBottomBar;
	fgSizerBottomBar = new wxFlexGridSizer( 1, 9, 0, 0 );
	fgSizerBottomBar->AddGrowableCol( 7 );
	fgSizerBottomBar->SetFlexibleDirection( wxHORIZONTAL );
	fgSizerBottomBar->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

	m_buttonLoadObs = new wxButton( this, wxID_ANY, _("Load Project..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBottomBar->Add( m_buttonLoadObs, 0, wxALL, 5 );

	m_buttonSaveObs = new wxButton( this, wxID_ANY, _("Save Project..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBottomBar->Add( m_buttonSaveObs, 0, wxALL, 5 );

	wxStaticText* bottomSeparatorProjectData = new wxStaticText( this, wxID_ANY, _("|"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBottomBar->Add( bottomSeparatorProjectData, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );

	m_ObservationsImportObservations = new wxButton( this, wxID_ANY, _("Import Data..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBottomBar->Add( m_ObservationsImportObservations, 0, wxALL, 5 );

	m_ObservationsExportObservations = new wxButton( this, wxID_ANY, _("Export Data..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBottomBar->Add( m_ObservationsExportObservations, 0, wxALL, 5 );

	wxStaticText* bottomSeparatorDataPackage = new wxStaticText( this, wxID_ANY, _("|"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBottomBar->Add( bottomSeparatorDataPackage, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );

	m_buttonCreateScientificPackage = new wxButton( this, wxID_ANY, _("Data Package..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBottomBar->Add( m_buttonCreateScientificPackage, 0, wxALL, 5 );

	fgSizerBottomBar->Add( 0, 0, 1, wxEXPAND, 5 );

	wxArrayString m_choiceObservationsChoices;
	m_choiceObservations = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceObservationsChoices, 0 );
	m_choiceObservations->SetSelection( 0 );
	fgSizerBottomBar->Add( m_choiceObservations, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT, 20 );


	fgSizer3->Add( fgSizerBottomBar, 1, wxEXPAND, 5 );


	m_SizerControl->Add( fgSizer3, 1, wxEXPAND, 5 );


	this->SetSizer( m_SizerControl );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_ACTIVATE, wxActivateEventHandler( ooControlDialogDef::ooControlDialogActivate ) );
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ooControlDialogDef::ooControlDialogDefOnClose ) );
	m_notebookControl->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( ooControlDialogDef::OnNotebookPageChanged ), NULL, this );
	m_ProjectEditUse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectEditUse ), NULL, this );
	m_ProjectNew->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectNew ), NULL, this );
	m_ProjectNewColumn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectNewColumn ), NULL, this );
	m_ProjectDeleteColumn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectDeleteColumn ), NULL, this );
	m_gridProject->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( ooControlDialogDef::OnProjectGridSelect ), NULL, this );
	m_gridProject->Connect( wxEVT_GRID_RANGE_SELECT, wxGridRangeSelectEventHandler( ooControlDialogDef::OnProjectGridRangeSelect ), NULL, this );
	m_buttonScanNmea->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickScanNmea ), NULL, this );
	m_checkShowAdvancedNmeaFields->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnCheckBoxShowAdvancedNmeaFields ), NULL, this );
	m_buttonOpenResourcesFolder->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickOpenResourcesFolder ), NULL, this );
	m_buttonRefreshListings->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickRefreshListings ), NULL, this );
	m_ObservationsNew->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickNewObservation ), NULL, this );
	m_ObservationsDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickDeleteObservation ), NULL, this );
	m_checkShowObservationMarks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnCheckBoxShowObservationMarks ), NULL, this );
	m_buttonLoadObs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickLoadObservation ), NULL, this );
	m_buttonSaveObs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickSaveObservation ), NULL, this );
	m_ObservationsImportObservations->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickImportObservations ), NULL, this );
	m_ObservationsExportObservations->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickExportObservations ), NULL, this );
	m_buttonCreateScientificPackage->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickDataPackage ), NULL, this );
	m_choiceObservations->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ooControlDialogDef::OnChoiceObservationsChanged ), NULL, this );

	return true;
}

ooControlDialogDef::~ooControlDialogDef()
{
	// Disconnect Events
	this->Disconnect( wxEVT_ACTIVATE, wxActivateEventHandler( ooControlDialogDef::ooControlDialogActivate ) );
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ooControlDialogDef::ooControlDialogDefOnClose ) );
	m_notebookControl->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( ooControlDialogDef::OnNotebookPageChanged ), NULL, this );
	m_ProjectEditUse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectEditUse ), NULL, this );
	m_ProjectNew->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectNew ), NULL, this );
	m_ProjectNewColumn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectNewColumn ), NULL, this );
	m_ProjectDeleteColumn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectDeleteColumn ), NULL, this );
	m_gridProject->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( ooControlDialogDef::OnProjectGridSelect ), NULL, this );
	m_gridProject->Disconnect( wxEVT_GRID_RANGE_SELECT, wxGridRangeSelectEventHandler( ooControlDialogDef::OnProjectGridRangeSelect ), NULL, this );
	m_buttonScanNmea->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickScanNmea ), NULL, this );
	m_checkShowAdvancedNmeaFields->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnCheckBoxShowAdvancedNmeaFields ), NULL, this );
	m_buttonOpenResourcesFolder->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickOpenResourcesFolder ), NULL, this );
	m_buttonRefreshListings->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickRefreshListings ), NULL, this );
	m_ObservationsNew->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickNewObservation ), NULL, this );
	m_ObservationsDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickDeleteObservation ), NULL, this );
	m_checkShowObservationMarks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnCheckBoxShowObservationMarks ), NULL, this );
	m_buttonLoadObs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickLoadObservation ), NULL, this );
	m_buttonSaveObs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickSaveObservation ), NULL, this );
	m_ObservationsImportObservations->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickImportObservations ), NULL, this );
	m_ObservationsExportObservations->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickExportObservations ), NULL, this );
	m_buttonCreateScientificPackage->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickDataPackage ), NULL, this );
	m_choiceObservations->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ooControlDialogDef::OnChoiceObservationsChanged ), NULL, this );

}
