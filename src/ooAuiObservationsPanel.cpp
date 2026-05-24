#include "ooAuiObservationsPanel.h"

#include "ooObservations.h"
#include "openobserver_pi.h"

#include <wx/menu.h>
#include <wx/settings.h>

extern openobserver_pi* g_openobserver_pi;

enum {
    ID_OO_AUI_OBSERVATIONS_UNDOCK_PANEL = wxID_HIGHEST + 3200
};

ooAuiObservationsPanel::ooAuiObservationsPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_titleText(nullptr),
      m_grid(nullptr),
      m_observations(nullptr),
      m_refreshTimer(this)
{
    wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);

    m_titleText = new wxStaticText(this, wxID_ANY,
                                   _("Open Observer observations"));

    wxFont titleFont = m_titleText->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_titleText->SetFont(titleFont);

    m_grid = new wxGrid(this, wxID_ANY);
    m_grid->CreateGrid(0, 0);
    m_grid->EnableEditing(false);
    m_grid->EnableGridLines(true);
    m_grid->EnableDragGridSize(true);
    m_grid->EnableDragColMove(false);
    m_grid->EnableDragColSize(true);
    m_grid->EnableDragRowSize(true);
    m_grid->SetMargins(0, 0);
    m_grid->SetRowLabelSize(0);
    m_grid->SetColLabelAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
    m_grid->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
    m_grid->SetDefaultRowSize(28, true);

    outerSizer->Add(m_titleText, 0,
                    wxLEFT | wxRIGHT | wxTOP | wxALIGN_CENTER_HORIZONTAL, 8);
    outerSizer->Add(m_grid, 1, wxALL | wxEXPAND, 8);

    SetSizer(outerSizer);
    SetMinSize(wxSize(640, 360));

    ApplyReadOnlyGridStyle();
    Layout();

    Bind(wxEVT_CONTEXT_MENU,
         &ooAuiObservationsPanel::OnContextMenu,
         this);
    Bind(wxEVT_MENU,
         &ooAuiObservationsPanel::OnUndockPanel,
         this,
         ID_OO_AUI_OBSERVATIONS_UNDOCK_PANEL);

    m_grid->Bind(wxEVT_CONTEXT_MENU,
                 &ooAuiObservationsPanel::OnContextMenu,
                 this);

    Bind(wxEVT_TIMER,
         &ooAuiObservationsPanel::OnRefreshTimer,
         this,
         m_refreshTimer.GetId());

    m_refreshTimer.Start(1000);
}

ooAuiObservationsPanel::~ooAuiObservationsPanel()
{
    m_refreshTimer.Stop();
}

void ooAuiObservationsPanel::SetObservations(ooObservations* observations)
{
    m_observations = observations;
    RebuildGrid();
}

void ooAuiObservationsPanel::RefreshObservations()
{
    RebuildGrid();
}

void ooAuiObservationsPanel::OnRefreshTimer(wxTimerEvent& event)
{
    RefreshObservations();
}

void ooAuiObservationsPanel::RebuildGrid()
{
    if (!m_grid) return;

    m_grid->ClearGrid();

    if (!m_observations) {
        if (m_grid->GetNumberRows() > 0) {
            m_grid->DeleteRows(0, m_grid->GetNumberRows());
        }

        if (m_grid->GetNumberCols() > 0) {
            m_grid->DeleteCols(0, m_grid->GetNumberCols());
        }

        return;
    }

    const int rows = m_observations->GetNumberRows();
    const int cols = m_observations->GetNumberCols();

    const int currentRows = m_grid->GetNumberRows();
    const int currentCols = m_grid->GetNumberCols();

    if (currentRows > rows) {
        m_grid->DeleteRows(rows, currentRows - rows);
    } else if (currentRows < rows) {
        m_grid->AppendRows(rows - currentRows);
    }

    if (currentCols > cols) {
        m_grid->DeleteCols(cols, currentCols - cols);
    } else if (currentCols < cols) {
        m_grid->AppendCols(cols - currentCols);
    }

    for (int c = 0; c < cols; ++c) {
        m_grid->SetColLabelValue(c, m_observations->GetColLabelValue(c));
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            m_grid->SetCellValue(r, c, m_observations->GetValue(r, c));
            m_grid->SetReadOnly(r, c, true);
        }
    }

    ApplyReadOnlyGridStyle();

    if (cols > 0) {
        m_grid->AutoSizeColumns(false);
    }

    m_grid->ForceRefresh();
    Layout();
}

void ooAuiObservationsPanel::ApplyReadOnlyGridStyle()
{
    if (!m_grid) return;

    wxColour labelBg =
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    wxColour labelText =
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);

    m_grid->SetLabelBackgroundColour(labelBg);
    m_grid->SetLabelTextColour(labelText);

    wxFont labelFont = m_grid->GetDefaultCellFont();

    if (!labelFont.IsOk()) {
        labelFont = m_grid->GetFont();
    }

    if (!labelFont.IsOk()) {
        labelFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    }

    labelFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_grid->SetLabelFont(labelFont);

    m_grid->SetCellHighlightPenWidth(0);
    m_grid->SetCellHighlightROPenWidth(0);
}

void ooAuiObservationsPanel::OnContextMenu(wxContextMenuEvent& event)
{
    wxMenu menu;
    menu.Append(ID_OO_AUI_OBSERVATIONS_UNDOCK_PANEL, _("Undock panel"));
    PopupMenu(&menu);
}

void ooAuiObservationsPanel::OnUndockPanel(wxCommandEvent& event)
{
    if (g_openobserver_pi) {
        g_openobserver_pi->UndockAuiObservationsPanel();
    }
}