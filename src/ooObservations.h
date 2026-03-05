/***************************************************************************
* Project:  OpenCPN
* Purpose:  Open Observer Plugin Observations
* Author:   Alex Mansfield
*
* Project:  OpenCPN
*
***************************************************************************
*   Copyright (C) 2013 by David S. Register                               *
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

#pragma once

#include <wx/file.h>
#include <wx/grid.h>
#include <wx/stopwatch.h>
#include <wx/xml/xml.h>
#include <unordered_map>
#include <vector>

#define XML_FILE_VERSION_PROJECT      1
#define XML_FILE_VERSION_OBSERVATIONS 2
#define XML_FILE_VERSION_LISTING      1

class ooProject {
public:
  ooProject() : m_name("Empty") {}
  ooProject(const wxString& name, const wxGridSizesInfo& colSizes,
            const wxArrayString& colFieldTypes, const wxArrayString& colLabels)
      : m_name(name),
        m_col_sizes(colSizes),
        m_col_field_types(colFieldTypes),
        m_col_labels(colLabels) {}

  bool ReadFromXML(const wxXmlNode* node);
  wxXmlNode* SaveToXML(wxXmlNode* parent = NULL);

  int GetColCount() const { return m_col_labels.GetCount(); }

  const wxString& GetName() const { return m_name; }
  void SetName(const wxString& name) { m_name = name; }

  const wxGridSizesInfo& GetColSizes() const { return m_col_sizes; }
  void SetColSizes(const wxGridSizesInfo& sizeInfo) { m_col_sizes = sizeInfo; }

  const wxArrayString& GetColFieldTypes() const { return m_col_field_types; }
  void SetColFieldTypes(const wxArrayString& colFieldTypes) {
    m_col_field_types = colFieldTypes;
  }

  const wxArrayString& GetColLabels() const { return m_col_labels; }
  void SetColLabels(const wxArrayString& colLabels) { m_col_labels = colLabels; }

protected:
  wxString m_name;
  wxGridSizesInfo m_col_sizes;
  wxArrayString m_col_field_types;
  wxArrayString m_col_labels;
};

///////////////////////////////////////////////////////////////////////////////
/// Class ooObservations
///////////////////////////////////////////////////////////////////////////////
class ooObservations : public wxGridStringTable
{
public:
    ooObservations();
    ~ooObservations();

    void SetColSizes(const wxGridSizesInfo& colSize);
    const wxGridSizesInfo& GetColSizes() const;
    const wxArrayString& GetColFieldTypes() const;

    void SetPositionFix(time_t fixTime, double lat, double lon);
    void SetNmeaSentFix(wxString sentenceNmea);

    void StartObservation();
    void StopObservation();

    bool IsObserving() const;

    long GetObservationDuration();

    void AddMarks();
    void DeleteMarks();

    void SetProject(const ooProject& project);
    const ooProject& GetProject() const;

    void SaveToCSV(wxFile *file);
    bool ReadFromCSV(const wxString& filename, wxString& err);
    void SaveToXML(wxFile *file);
    bool ReadFromXML(const wxString& filename, const ooProject& defaultProject);

    wxString GetRowDescription(int row);

    double HaversineDistance(double lat1, double lon1, double lat2,double lon2);
    double DegToRad(double deg);
    static wxArrayString GetObservationFieldTypes();
    static void AddListing(const wxString& listing, const wxArrayString& items);
    static bool GetListing(const wxString& listing, wxArrayString& items);
    static bool ReadListingFromXML(const wxString& filename,
                                   wxArrayString& result,
                                   wxArrayString& icons);
    static bool ReadFromXML(const wxString& filename, int& fileVersion,
                            ooProject& project,
                            wxXmlDocument& xmlDoc, wxXmlNode*& root,
                            const ooProject& defaultProject);
    static void SetIcons(const wxString& listing, const wxArrayString& icons);
    static void ComputeTrueWind(double sog,
                                double apparentWindSpeed, double apparentWindAngle,
                                double& trueWindSpeed, double& trueWindAngle);
    static wxString GetNMEAField(const wxString& sentence, const wxString& sentenceID,
                                 int fieldIndex);
    static std::vector<wxString> SplitNMEAFields(const wxString& sentence, wxChar sep = ',');

private:
    ooProject m_project;
    static std::unordered_map<wxString, wxArrayString> m_listings;
    static wxArrayString m_icons;
    static wxString m_iconsListing;

    time_t m_position_fix_time;
    double m_position_fix_lat;
    double m_position_fix_lon;

    wxString m_sentenceNMEA;
    double m_apparentWindSpeed = 0;
    double m_apparentWindAngle = 0;
    double m_trueWindSpeed = 0;
    double m_trueWindAngle = 0;

    double m_SOG = 0;
    double m_COG = 0;

    double StartLongSave;
    double StartLatSave;

    bool m_IsObserving;
    wxStopWatch m_ObservationDurationStopWatch;
};
