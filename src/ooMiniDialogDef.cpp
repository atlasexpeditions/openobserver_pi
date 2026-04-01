///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "ooMiniDialogDef.h"

///////////////////////////////////////////////////////////////////////////

ooMiniDialogDef::ooMiniDialogDef()
{
}

ooMiniDialogDef::ooMiniDialogDef( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
{
	this->Create( parent, id, title, pos, size, style );
}

bool ooMiniDialogDef::Create( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
{
	if ( !wxDialog::Create( parent, id, title, pos, size, style ) )
	{
		return false;
	}

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_boxSizer = new wxBoxSizer( wxVERTICAL );

	m_fgSizer = new wxFlexGridSizer( 0, 1, 0, 0 );
	m_fgSizer->AddGrowableCol( 0 );
	m_fgSizer->SetFlexibleDirection( wxBOTH );
	m_fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	m_boxSizer->Add( m_fgSizer, 1, wxEXPAND, 5 );


	this->SetSizer( m_boxSizer );
	this->Layout();
	m_boxSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ooMiniDialogDef::ooMiniDialogDefOnClose ) );

	return true;
}

ooMiniDialogDef::~ooMiniDialogDef()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ooMiniDialogDef::ooMiniDialogDefOnClose ) );

}
