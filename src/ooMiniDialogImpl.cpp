/**************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Open Observer Plugin Mini Dialog
 * Author:   Alex Mansfield
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

#include "ooMiniDialogImpl.h"

#include "openobserver_pi.h"


extern openobserver_pi *g_openobserver_pi;

ooMiniDialogImpl::ooMiniDialogImpl(wxWindow* parent) : ooMiniDialogDef(parent), m_MiniPanel(nullptr)
{
#if wxCHECK_VERSION(3,0,0)
    SetLayoutAdaptationMode(wxDIALOG_ADAPTATION_MODE_ENABLED);
#endif // wxCHECK_VERSION(3,0,0)

    m_MiniPanel = new ooMiniPanel(this);
    m_MiniPanel->SetToggleWindowButtonLabel("Maximize");
    this->Connect(wxEVT_SHOW, wxShowEventHandler(ooMiniPanel::OnShow), NULL, m_MiniPanel);
    m_fgSizer->Add(m_MiniPanel, 1, wxEXPAND, 5);
}

ooMiniDialogImpl::~ooMiniDialogImpl()
{
    this->Disconnect(wxEVT_SHOW, wxShowEventHandler(ooMiniPanel::OnShow), NULL, m_MiniPanel);
}

void ooMiniDialogImpl::SetProjectInfo(
    const wxString& projectName,
    const wxColor& projectColor)
{
    m_MiniPanel->SetProjectInfo(projectName, projectColor);
}

void ooMiniDialogImpl::ooMiniDialogDefOnClose(wxCloseEvent& event)
{
    g_openobserver_pi->ToggleToolbarIcon();
}
