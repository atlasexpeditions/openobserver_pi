///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/button.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <wx/dialog.h>

#include "wxWTranslateCatalog.h"

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class ooControlDialogDef
///////////////////////////////////////////////////////////////////////////////
class ooControlDialogDef : public wxDialog
{
	private:

	protected:
		wxBoxSizer* m_SizerControl;
		wxNotebook* m_notebookControl;
		wxPanel* m_panelProject;
		wxButton* m_ProjectEditUse;
		wxButton* m_ProjectNewColumn;
		wxButton* m_ProjectDeleteColumn;
		wxButton* m_ProjectNew;
		wxStaticLine* m_staticline3;
		wxStaticText* m_staticText6;
		wxTextCtrl* m_textProjectName;
		wxGrid* m_gridProject;
		wxPanel* m_panelObservations;
		wxFlexGridSizer* m_fgSizerObservations;
		wxBoxSizer* bSizerTopButtons;
		wxStaticLine* m_staticline1;
		wxStaticText* m_ObservationsDateLabel1;
		wxStaticText* m_ObservationsDateLabel;
		wxTextCtrl* m_ObservationsDate;
		wxStaticText* m_ObservationsTimeLabel;
		wxTextCtrl* m_ObservationsTime;
		wxStaticText* m_ObservationsLatLabel;
		wxTextCtrl* m_ObservationsLat;
		wxStaticText* m_ObservationsLonLabel;
		wxTextCtrl* m_ObservationsLon;
		wxStaticLine* m_staticline11;
		wxButton* m_ObservationsNew;
		wxButton* m_ObservationsDelete;
		wxButton* m_ObservationsDeleteAll;
		wxButton* m_ObservationsAddMarks;
		wxButton* m_ObservationsDeleteMarks;
		wxButton* m_buttonLoadObs;
		wxButton* m_buttonSaveObs;
		wxButton* m_ObservationsExportObservations;
		wxButton* m_ObservationsImportObservations;
		wxChoice* m_choiceObservations;

		// Virtual event handlers, override them in your derived class
		virtual void ooControlDialogDefOnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnNotebookPageChanged( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnButtonClickProjectEditUse( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickProjectNewColumn( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickProjectDeleteColumn( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickProjectNew( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickNewObservation( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickDeleteObservation( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickDeleteAllObservations( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickObservationsAddMarks( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickObservationsDeleteMarks( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickLoadObservation( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickSaveObservation( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickExportObservations( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClickImportObservations( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChoiceObservationsChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		ooControlDialogDef();
		ooControlDialogDef( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Open Observer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1925,716 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );
		bool Create( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Open Observer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1925,716 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );

		~ooControlDialogDef();

};

