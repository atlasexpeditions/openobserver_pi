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
	if ( !wxDialog::Create( parent, id, title, pos, size, style ) )
	{
		return false;
	}

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_SizerControl = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 2, 1, 0, 0 );
	fgSizer3->AddGrowableCol( 0 );
	fgSizer3->AddGrowableRow( 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_notebookControl = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelProject = new wxPanel( m_notebookControl, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizerProject;
	fgSizerProject = new wxFlexGridSizer( 4, 1, 0, 0 );
	fgSizerProject->SetFlexibleDirection( wxBOTH );
	fgSizerProject->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxFlexGridSizer* fgSizerProjectButtons;
	fgSizerProjectButtons = new wxFlexGridSizer( 1, 6, 0, 0 );
	fgSizerProjectButtons->SetFlexibleDirection( wxBOTH );
	fgSizerProjectButtons->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ProjectEditUse = new wxButton( m_panelProject, wxID_ANY, _("Edit"), wxDefaultPosition, wxDefaultSize, 0 );

	m_ProjectEditUse->SetDefault();
	fgSizerProjectButtons->Add( m_ProjectEditUse, 0, wxALL, 5 );

	m_ProjectNew = new wxButton( m_panelProject, wxID_ANY, _("New"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ProjectNew->Enable( false );

	fgSizerProjectButtons->Add( m_ProjectNew, 0, wxALL, 5 );

	m_ProjectLoad = new wxButton( m_panelProject, wxID_ANY, _("Load"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ProjectLoad->Enable( false );

	fgSizerProjectButtons->Add( m_ProjectLoad, 0, wxALL, 5 );

	m_ProjectSave = new wxButton( m_panelProject, wxID_ANY, _("Save"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ProjectSave->Enable( false );

	fgSizerProjectButtons->Add( m_ProjectSave, 0, wxALL, 5 );

	m_ProjectNewColumn = new wxButton( m_panelProject, wxID_ANY, _("Insert Column"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ProjectNewColumn->Enable( false );

	fgSizerProjectButtons->Add( m_ProjectNewColumn, 0, wxALL, 5 );

	m_ProjectDeleteColumn = new wxButton( m_panelProject, wxID_ANY, _("Delete First Column"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ProjectDeleteColumn->Enable( false );

	fgSizerProjectButtons->Add( m_ProjectDeleteColumn, 0, wxALL, 5 );


	fgSizerProject->Add( fgSizerProjectButtons, 1, wxEXPAND, 5 );

	m_staticline3 = new wxStaticLine( m_panelProject, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerProject->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText61 = new wxStaticText( m_panelProject, wxID_ANY, _("File"), wxDefaultPosition, wxSize( 100,-1 ), 0 );
	m_staticText61->Wrap( -1 );
	fgSizer8->Add( m_staticText61, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textProjectFile = new wxTextCtrl( m_panelProject, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 600,-1 ), wxTE_NO_VSCROLL|wxTE_READONLY );
	m_textProjectFile->Enable( false );

	fgSizer8->Add( m_textProjectFile, 0, wxALL, 5 );

	m_staticText6 = new wxStaticText( m_panelProject, wxID_ANY, _("Project Name"), wxDefaultPosition, wxSize( 100,-1 ), 0 );
	m_staticText6->Wrap( -1 );
	fgSizer8->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textProjectName = new wxTextCtrl( m_panelProject, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 600,-1 ), 0 );
	m_textProjectName->Enable( false );

	fgSizer8->Add( m_textProjectName, 0, wxALL, 5 );


	fgSizerProject->Add( fgSizer8, 1, wxEXPAND, 5 );

	m_gridProject = new wxGrid( m_panelProject, wxID_ANY, wxDefaultPosition, wxSize( 740,100 ), 0 );

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

	fgSizerProject->Add( m_gridProject, 0, wxALL, 5 );


	m_panelProject->SetSizer( fgSizerProject );
	m_panelProject->Layout();
	fgSizerProject->Fit( m_panelProject );
	m_notebookControl->AddPage( m_panelProject, _("Project"), false );
	m_panelObservations = new wxPanel( m_notebookControl, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_fgSizerObservations = new wxFlexGridSizer( 6, 1, 0, 0 );
	m_fgSizerObservations->SetFlexibleDirection( wxBOTH );
	m_fgSizerObservations->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	bSizerTopButtons = new wxBoxSizer( wxHORIZONTAL );


	m_fgSizerObservations->Add( bSizerTopButtons, 1, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( m_panelObservations, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_fgSizerObservations->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_ObservationsDateLabel1 = new wxStaticText( m_panelObservations, wxID_ANY, _("Current Data (timezone UTC)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ObservationsDateLabel1->Wrap( -1 );
	bSizer6->Add( m_ObservationsDateLabel1, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerObservationsLabels;
	fgSizerObservationsLabels = new wxFlexGridSizer( 1, 8, 0, 0 );
	fgSizerObservationsLabels->SetFlexibleDirection( wxBOTH );
	fgSizerObservationsLabels->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

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

	m_ObservationsLatLabel = new wxStaticText( m_panelObservations, wxID_ANY, _("Latitude"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ObservationsLatLabel->Wrap( -1 );
	fgSizerObservationsLabels->Add( m_ObservationsLatLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ObservationsLat = new wxTextCtrl( m_panelObservations, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizerObservationsLabels->Add( m_ObservationsLat, 1, wxALL, 5 );

	m_ObservationsLonLabel = new wxStaticText( m_panelObservations, wxID_ANY, _("Longitude"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ObservationsLonLabel->Wrap( -1 );
	fgSizerObservationsLabels->Add( m_ObservationsLonLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ObservationsLon = new wxTextCtrl( m_panelObservations, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizerObservationsLabels->Add( m_ObservationsLon, 1, wxALL, 5 );


	bSizer6->Add( fgSizerObservationsLabels, 1, wxEXPAND, 5 );


	m_fgSizerObservations->Add( bSizer6, 1, wxEXPAND, 5 );

	m_staticline11 = new wxStaticLine( m_panelObservations, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_fgSizerObservations->Add( m_staticline11, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgSizerObservationsButtons;
	fgSizerObservationsButtons = new wxFlexGridSizer( 1, 6, 0, 0 );
	fgSizerObservationsButtons->SetFlexibleDirection( wxBOTH );
	fgSizerObservationsButtons->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ObservationsNew = new wxButton( m_panelObservations, wxID_ANY, _("Insert Row"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerObservationsButtons->Add( m_ObservationsNew, 0, wxALL, 5 );

	m_ObservationsDelete = new wxButton( m_panelObservations, wxID_ANY, _("Delete Top Row"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerObservationsButtons->Add( m_ObservationsDelete, 0, wxALL, 5 );

	m_ObservationsDeleteAll = new wxButton( m_panelObservations, wxID_ANY, _("Delete All Rows"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerObservationsButtons->Add( m_ObservationsDeleteAll, 0, wxALL, 5 );

	m_ObservationsAddMarks = new wxButton( m_panelObservations, wxID_ANY, _("Create Marks"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerObservationsButtons->Add( m_ObservationsAddMarks, 0, wxALL, 5 );

	m_ObservationsDeleteMarks = new wxButton( m_panelObservations, wxID_ANY, _("Delete Marks"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerObservationsButtons->Add( m_ObservationsDeleteMarks, 0, wxALL, 5 );

	m_ObservationsExportObservations = new wxButton( m_panelObservations, wxID_ANY, _("Export CSV"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerObservationsButtons->Add( m_ObservationsExportObservations, 0, wxALL, 5 );


	m_fgSizerObservations->Add( fgSizerObservationsButtons, 1, 0, 5 );


	m_panelObservations->SetSizer( m_fgSizerObservations );
	m_panelObservations->Layout();
	m_fgSizerObservations->Fit( m_panelObservations );
	m_notebookControl->AddPage( m_panelObservations, _("Observations"), true );

	fgSizer3->Add( m_notebookControl, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgSizerBottomBar;
	fgSizerBottomBar = new wxFlexGridSizer( 1, 1, 0, 0 );
	fgSizerBottomBar->SetFlexibleDirection( wxBOTH );
	fgSizerBottomBar->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxBoxSizer* bSizerOKCancel;
	bSizerOKCancel = new wxBoxSizer( wxHORIZONTAL );

	m_buttonClose = new wxButton( this, wxID_ANY, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerOKCancel->Add( m_buttonClose, 0, wxALL, 5 );

	m_buttonLoadObs = new wxButton( this, wxID_ANY, _("Load..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerOKCancel->Add( m_buttonLoadObs, 0, wxALL, 5 );

	m_buttonSaveObs = new wxButton( this, wxID_ANY, _("Save..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerOKCancel->Add( m_buttonSaveObs, 0, wxALL, 5 );


	fgSizerBottomBar->Add( bSizerOKCancel, 1, wxEXPAND, 5 );


	fgSizer3->Add( fgSizerBottomBar, 1, wxEXPAND, 5 );


	m_SizerControl->Add( fgSizer3, 1, wxEXPAND, 5 );


	this->SetSizer( m_SizerControl );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ooControlDialogDef::ooControlDialogDefOnClose ) );
	m_ProjectEditUse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectEditUse ), NULL, this );
	m_ProjectNew->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectNew ), NULL, this );
	m_ProjectLoad->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectLoad ), NULL, this );
	m_ProjectSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectSave ), NULL, this );
	m_ProjectNewColumn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectNewColumn ), NULL, this );
	m_ProjectDeleteColumn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectDeleteColumn ), NULL, this );
	m_ObservationsNew->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickNewObservation ), NULL, this );
	m_ObservationsDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickDeleteObservation ), NULL, this );
	m_ObservationsDeleteAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickDeleteAllObservations ), NULL, this );
	m_ObservationsAddMarks->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickObservationsAddMarks ), NULL, this );
	m_ObservationsDeleteMarks->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickObservationsDeleteMarks ), NULL, this );
	m_ObservationsExportObservations->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickExportObservations ), NULL, this );
	m_buttonClose->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::ooControlCloseClick ), NULL, this );
	m_buttonLoadObs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickLoadObservation ), NULL, this );
	m_buttonSaveObs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickSaveObservation ), NULL, this );

	return true;
}

ooControlDialogDef::~ooControlDialogDef()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ooControlDialogDef::ooControlDialogDefOnClose ) );
	m_ProjectEditUse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectEditUse ), NULL, this );
	m_ProjectNew->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectNew ), NULL, this );
	m_ProjectLoad->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectLoad ), NULL, this );
	m_ProjectSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectSave ), NULL, this );
	m_ProjectNewColumn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectNewColumn ), NULL, this );
	m_ProjectDeleteColumn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickProjectDeleteColumn ), NULL, this );
	m_ObservationsNew->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickNewObservation ), NULL, this );
	m_ObservationsDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickDeleteObservation ), NULL, this );
	m_ObservationsDeleteAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickDeleteAllObservations ), NULL, this );
	m_ObservationsAddMarks->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickObservationsAddMarks ), NULL, this );
	m_ObservationsDeleteMarks->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickObservationsDeleteMarks ), NULL, this );
	m_ObservationsExportObservations->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickExportObservations ), NULL, this );
	m_buttonClose->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::ooControlCloseClick ), NULL, this );
	m_buttonLoadObs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickLoadObservation ), NULL, this );
	m_buttonSaveObs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ooControlDialogDef::OnButtonClickSaveObservation ), NULL, this );

}
