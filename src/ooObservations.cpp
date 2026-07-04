/**************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Open Observer Plugin Observations
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

#include "../opencpn-libs/nmea0183/src/nmea0183.h"

#include "ooObservations.h"

#include <wx/log.h>
#include <wx/sstream.h>
#include <wx/xml/xml.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/msgdlg.h>

#include <vector>

#include "tpUtils.h"
#include "ocpn_plugin.h"
#include <openobserver_pi.h>

void ComputeTrueWind(double sog, double cog, double apparentWindSpeed,
                     double apparentWindAngle, double& trueWindSpeed,
                     double& trueWindDirection);

    std::vector<NMEAField> ooObservations::m_nmeaFields;
    bool ooObservations::m_showAdvancedNMEAFields = false;

bool ooProject::ReadFromXML(const wxXmlNode* project)
{
    if (project == NULL ||
        project->GetName() != "project") {
        return false;
    }

    m_col_labels.clear();
    m_col_field_types.clear();
    m_col_sizes = wxGridSizesInfo();
    m_color = DEFAULT_PROJECT_COLOUR;
    m_description = "";

    m_name = project->GetAttribute("name");

    wxArrayString colorStr = wxSplit(project->GetAttribute("color"), ',');
    int r, g, b;
    if (colorStr.GetCount() == 3 &&
        colorStr[0].ToInt(&r) &&
        colorStr[1].ToInt(&g) &&
        colorStr[2].ToInt(&b)) {
        m_color = wxColor(r, g, b);
    }

    m_mark_icon = project->GetAttribute("mark_icon", DEFAULT_PROJECT_ICON);

    // Optional project description.
    // This keeps compatibility with older project XML files that do not have it.
    for (wxXmlNode* child = project->GetChildren(); child != NULL; child = child->GetNext()) {
        if (child->GetName() == "description") {
            m_description = child->GetNodeContent();
            break;
        }
    }

    for (wxXmlNode* field = project->GetChildren(); field != NULL ; field = field->GetNext()) {
        int c = -1;
        if (!field->GetAttribute("id").ToInt(&c) || c < 0) continue;

        m_col_labels.SetCount(c + 1);
        m_col_field_types.SetCount(c + 1);

        int col_size = -1;
        if (field->GetAttribute("col_size").ToInt(&col_size) && col_size >= 0) {
            m_col_sizes.m_customSizes[c] = col_size;
        }

        m_col_labels[c] = field->GetAttribute("label");
        m_col_field_types[c] = field->GetAttribute("field_type");

        if (m_col_labels[c].IsEmpty()) {
            m_col_labels[c] = wxString("Data");
        }

        if (m_col_field_types[c].IsEmpty()) {
            m_col_field_types[c] = wxString("Text");
        }
    }

    OnFieldTypeChanged();

    return true;
}

wxXmlNode* ooProject::SaveToXML(wxXmlNode* parent)
{
    const int C = m_col_field_types.size();

    wxXmlNode* project = new wxXmlNode(parent, wxXML_ELEMENT_NODE, "project");

    project->AddAttribute("name", m_name);
    project->AddAttribute(
        "color",
        wxString::Format(
            "%i,%i,%i",
            (int)m_color.Red(),
            (int)m_color.Green(),
            (int)m_color.Blue())
    );
    project->AddAttribute("mark_icon", m_mark_icon);

    // Project description is stored as a child node to support multiline text.
    wxXmlNode* description = new wxXmlNode(project, wxXML_ELEMENT_NODE, "description");
    description->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", m_description));

    for (int c = 0; c < C; ++c) {
        wxXmlNode* field = new wxXmlNode(project, wxXML_ELEMENT_NODE, "field");
        field->AddAttribute("id", wxString::Format(wxT("%i"), c));
        field->AddAttribute("label", m_col_labels[c]);
        field->AddAttribute("field_type", m_col_field_types[c]);
        field->AddAttribute(
            "col_size",
            wxString::Format(wxT("%i"), m_col_sizes.GetSize(c)));
    }

    return project;
}

bool IsTextOrListing(const wxString& fieldType, const wxArrayString& listings)
{
    if (fieldType.IsSameAs("Text"))               return true;
    if (listings.Index(fieldType) != wxNOT_FOUND) return true;
    return false;
}

bool ooProject::IsUpdatable(const ooProject& other) const
{
    bool res = (this->GetColCount() == other.GetColCount());
    int c = 0;
    const int C = GetColCount();
    const wxArrayString listings = ooObservations::GetListingKeys();
    while (res && c < C) {
        const bool otherIsTextOrListing =
            IsTextOrListing(other.GetColFieldTypes()[c], listings);
        const bool thisIsTextOrListing =
            IsTextOrListing(this->GetColFieldTypes()[c], listings);
    
        if (otherIsTextOrListing || thisIsTextOrListing) {
            res = (otherIsTextOrListing && thisIsTextOrListing);
        } else {
            res = (other.GetColFieldTypes()[c].IsSameAs(
                   this->GetColFieldTypes()[c]));
        }
        c++;
    }
    return res;
}

int ooProject::FindFieldTypeColumn(const wxString& field_type) const
{
    const int C = m_col_field_types.size();
    return m_col_field_types.Index(field_type);
}

void ooProject::OnFieldTypeChanged()
{
    m_lat_col = FindFieldTypeColumn("Start Latitude");
    m_lon_col = FindFieldTypeColumn("Start Longitude");
    m_mark_col = FindFieldTypeColumn("Mark GUID");
}

std::unordered_map<wxString, wxArrayString> ooObservations::m_listings;
wxArrayString ooObservations::m_icons;
wxString ooObservations::m_iconsListing;

ooObservations::ooObservations() : wxGridStringTable(0, 0), m_IsObserving(false)
{
}

ooObservations::~ooObservations()
{
}

const wxGridSizesInfo& ooObservations::GetColSizes() const
{
    return m_project.GetColSizes();
}

void ooObservations::SetColSizes(const wxGridSizesInfo& colSize)
{
    m_project.SetColSizes(colSize);
}

const wxArrayString& ooObservations::GetColFieldTypes() const
{
    return m_project.GetColFieldTypes();
}

static bool IsReasonableUtcDateTime(const wxDateTime& dateTime);

void ooObservations::SetPositionFix(time_t fixTime, double lat, double lon)
{
    m_position_fix_time = fixTime;
    m_position_fix_lat = lat;
    m_position_fix_lon = lon;
}

void ooObservations::SetNmeaSentFix(wxString sentenceNmea)
{
    m_sentenceNMEA = sentenceNmea;
    NMEA0183 nmea;
    nmea << sentenceNmea;

    if (nmea.Parse())
    {
        if (nmea.LastSentenceIDParsed == "MWV") {
            m_apparentWindSpeed = nmea.Mwv.WindSpeed;
            m_apparentWindAngle = nmea.Mwv.WindAngle;
            if (m_SOG != 0 && m_COG != 0) {
                ComputeTrueWind(m_SOG, m_COG, m_apparentWindSpeed, m_apparentWindAngle, m_trueWindSpeed,
                                m_trueWindDirection);
            }
        }
        else if (nmea.LastSentenceIDParsed == "RMC") {
            if (nmea.Rmc.IsDataValid == NTrue) {
                wxString dt = nmea.Rmc.Date + nmea.Rmc.UTCTime;
                m_utcTime.ParseFormat(dt.c_str(), _T("%d%m%y%H%M%S"));
                if (IsReasonableUtcDateTime(m_utcTime)) {
                    m_lastUtcNmeaUpdate = wxDateTime::Now();
                }
            }
            if (nmea.Rmc.TrackMadeGoodDegreesTrue >= 0 && nmea.Rmc.TrackMadeGoodDegreesTrue <= 360)
            {
                m_COG = nmea.Rmc.TrackMadeGoodDegreesTrue;
            }
             m_SOG = nmea.Rmc.SpeedOverGroundKnots;
             
             //wxLogMessage("SOG: %.2f kn  COG: %.2f?", m_SOG, m_COG);

             wxString sog = GetNMEAField(m_sentenceNMEA, "RMC", 7);  // SOG
             wxString cog = GetNMEAField(m_sentenceNMEA, "RMC", 8);  // COG
             //wxLogMessage("SOG=%s COG=%s", sog, cog);
        }
        else if (nmea.LastSentenceIDReceived == _T("ZDA")) {
            wxString dt;
            dt.Printf(_T("%4d%02d%02d"), nmea.Zda.Year, nmea.Zda.Month,
                      nmea.Zda.Day);
            dt.Append(nmea.Zda.UTCTime);
            m_utcTime.ParseFormat(dt.c_str(), _T("%d%m%y%H%M%S"));
            if (IsReasonableUtcDateTime(m_utcTime)) {
                m_lastUtcNmeaUpdate = wxDateTime::Now();
            }
        }
        for (auto& item : ooObservations::m_nmeaFields) {
            if (nmea.LastSentenceIDParsed.IsSameAs(item.m_sentenceId)) {
                item.m_value = GetNMEAField(m_sentenceNMEA,
                                            item.m_sentenceId,
                                            item.m_fieldIndex);
            }
        }


    }
}

// Computes TrueWindSpeed and TrueWindAngle using the Apparent Wind and the GPS SOG.
void ComputeTrueWind(double sog, double cog,
                                     double apparentWindSpeed,
                                     double apparentWindAngle,
                                     double& trueWindSpeed,
                                     double& trueWindDirection)
{
    double awa_rad = apparentWindAngle * M_PI / 180.0;
    
    double Vx = apparentWindSpeed * cos(awa_rad);
    double Vy = apparentWindSpeed * sin(awa_rad);
    
    double Vtx = Vx - sog;
    double Vty = Vy;
    
    trueWindSpeed = sqrt(Vtx * Vtx + Vty * Vty);
    trueWindDirection = (int)(cog + atan2(Vty, Vtx) * 180.0 / M_PI) % 360;
    if (trueWindDirection < 0) trueWindDirection += 360.0;
}

std::vector<wxString> ooObservations::SplitNMEAFields(const wxString& sentence,
                                                      wxChar sep)
{
    std::vector<wxString> fields;
    size_t start = 0, end = 0;
    
    while ((end = sentence.find(sep, start)) != wxString::npos) {
        fields.push_back(sentence.Mid(start, end - start));
        start = end + 1;
    }
    fields.push_back(sentence.Mid(start));
    return fields;
}

// Get an NMEA field by ID and index
wxString ooObservations::GetNMEAField(const wxString& sentence,
                                      const wxString& sentenceID,
                                      int fieldIndex)
{
    if (sentence.IsEmpty()) return "";
    
    if (!sentence.Mid(3, sentenceID.Length()).IsSameAs(sentenceID)) return "";
    
    std::vector<wxString> fields = SplitNMEAFields(sentence);
    
    if (fieldIndex < 0 || fieldIndex >= (int)fields.size()) return "";
    
    return fields[fieldIndex];
}


////////////VPE NMEA//////////////

static bool IsReasonableUtcDateTime(const wxDateTime& dateTime)
{
    if (!dateTime.IsValid()) {
        return false;
    }

    const int year = dateTime.GetYear();

    // Simple safety range to reject uninitialized or absurd dates.
    return year >= 2000 && year <= 2100;
}

static wxDateTime GetComputerUtcDateTime()
{
    return wxDateTime::Now().ToUTC();
}

wxString ooObservations::GetUtcTimeFromNMEA(int dateFormat) const
{
    wxDateTime utcTime = m_utcTime;

    const wxTimeSpan maxGpsAge = wxTimeSpan::Seconds(5);
    const bool hasFreshNmeaUtc =
        IsReasonableUtcDateTime(utcTime) &&
        m_lastUtcNmeaUpdate.IsValid() &&
        (wxDateTime::Now() - m_lastUtcNmeaUpdate) <= maxGpsAge;

    if (!hasFreshNmeaUtc) {
        utcTime = GetComputerUtcDateTime();
    }

    wxDateTime::Tm tm = utcTime.GetTm();
    wxString res;

    switch (dateFormat) {
        case UTC_TIME_DATE:
            res = wxString::Format(
                "%04d/%02d/%02d",
                (int)tm.year,
                (int)tm.mon + 1,
                (int)tm.mday);
            break;

        case UTC_TIME_TIME:
            res = wxString::Format(
                "%02d:%02d:%02d",
                (int)tm.hour,
                (int)tm.min,
                (int)tm.sec);
            break;

        case UTC_TIMESTAMP:
            res = wxString::Format(
                "%04d-%02d-%02dT%02d:%02d:%02dZ",
                (int)tm.year,
                (int)tm.mon + 1,
                (int)tm.mday,
                (int)tm.hour,
                (int)tm.min,
                (int)tm.sec);
            break;
    }

    return res;
}

wxString ooObservations::GetUtcTimeSourceLabel() const
{
    const wxTimeSpan maxGpsAge = wxTimeSpan::Seconds(5);
    const bool hasFreshNmeaUtc =
        IsReasonableUtcDateTime(m_utcTime) &&
        m_lastUtcNmeaUpdate.IsValid() &&
        (wxDateTime::Now() - m_lastUtcNmeaUpdate) <= maxGpsAge;

    if (hasFreshNmeaUtc) {
        return "GPS UTC";
    }

    return "Computer UTC";
}

wxString ooObservations::GenerateObservationId(const wxString& dateString)
{
    // Expected date format from GetUtcTimeFromNMEA(UTC_TIME_DATE): YYYY/MM/DD
    wxString compactDate = "000000";

    if (dateString.Length() >= 10) {
        compactDate =
            dateString.Mid(2, 2) +  // YY
            dateString.Mid(5, 2) +  // MM
            dateString.Mid(8, 2);   // DD
    }

    const wxString prefix = "OBS-" + compactDate + "-";

    int observationIdCol = -1;
    const int C = m_project.GetColCount();

    for (int c = 0; c < C; ++c) {
        if (m_project.GetColFieldTypes()[c].IsSameAs("Observation ID")) {
            observationIdCol = c;
            break;
        }
    }

    int maxIndex = 0;

    if (observationIdCol >= 0) {
        const int R = GetNumberRows();

        for (int r = 0; r < R; ++r) {
            const wxString existingId = GetValue(r, observationIdCol);

            if (!existingId.StartsWith(prefix)) {
                continue;
            }

            wxString suffix = existingId.Mid(prefix.Length());
            long index = 0;

            if (suffix.ToLong(&index) && index > maxIndex) {
                maxIndex = (int)index;
            }
        }
    }

    return wxString::Format(
        wxT("%s%03d"),
        prefix,
        maxIndex + 1);
}

void ooObservations::StartObservation()
{
    if (m_IsObserving) return;

    // start duration stopwatch
    m_ObservationDurationStopWatch.Start(0);

    // get date and time
    //char dateString[16];
    //strftime(dateString, 16, "%F", gmtime(&m_position_fix_time));
    //char timeString[16];
    //strftime(timeString, 16, "%T", gmtime(&m_position_fix_time));
    //char utcTimestampString[64];
    //strftime(utcTimestampString, 64, "%FT%TZ", gmtime(&m_position_fix_time));
    wxString dateString         = GetUtcTimeFromNMEA(UTC_TIME_DATE);
    wxString timeString         = GetUtcTimeFromNMEA(UTC_TIME_TIME);
    wxString utcTimestampString = GetUtcTimeFromNMEA(UTC_TIMESTAMP);
    wxString observationIdString = GenerateObservationId(dateString);

    
    // Create new observations at the end of the table so the internal order
    // follows the natural field log: oldest observation first, newest last.
    AppendRows(1);
    const int currentObservationRow = GetCurrentObservationRow();
    const int C = GetNumberCols();

    if (m_project.GetColCount() == C)
    {
        for (int c=0; c<C; ++c)
        {
            wxString field_type = m_project.GetColFieldTypes()[c];

            if (field_type.IsSameAs("Observation ID"))
                SetValue(currentObservationRow, c, observationIdString);
            else if (field_type.IsSameAs("Start Date"))
                SetValue(currentObservationRow, c, dateString);
            else if (field_type.IsSameAs("Start Time"))
                SetValue(currentObservationRow, c, timeString);
            else if (field_type.IsSameAs("Start Timestamp UTC"))
                SetValue(currentObservationRow, c, utcTimestampString);
            else if (field_type.IsSameAs("Start Latitude"))
              {
                SetValue(currentObservationRow, c, toSDMM_PlugIn(1, m_position_fix_lat));
                StartLatSave = m_position_fix_lat;
              }
            else if (field_type.IsSameAs("Start Longitude"))
              {
                 SetValue(currentObservationRow, c, toSDMM_PlugIn(2, m_position_fix_lon));
                 StartLongSave = m_position_fix_lon;
              }
            else if (field_type.IsSameAs("NMEA AWS"))
                SetValue(currentObservationRow, c, wxString::Format("%d", (int)round(m_apparentWindSpeed)));
            else if (field_type.IsSameAs("NMEA AWA"))
                SetValue(currentObservationRow, c, wxString::Format("%d", (int)round(m_apparentWindAngle)));
            else if (field_type.IsSameAs("NMEA TWS"))
                SetValue(currentObservationRow, c, wxString::Format("%d", (int)round(m_trueWindSpeed)));
            else if (field_type.IsSameAs("NMEA TWD"))
                SetValue(currentObservationRow, c, wxString::Format("%d", (int)round(m_trueWindDirection)));
            else if (field_type.IsSameAs("NMEA COG"))
                SetValue(currentObservationRow, c, wxString::Format("%d", (int)round(m_COG)));
            else if (field_type.IsSameAs("NMEA SOG"))
                SetValue(currentObservationRow, c, wxString::Format("%.1f", m_SOG));
            else if (field_type.IsSameAs("Distance"))
              SetValue(currentObservationRow, c, "...");
            else {
                for (const auto& item : ooObservations::m_nmeaFields) {
                    if (field_type.IsSameAs(item.m_description)) {
                        SetValue(currentObservationRow, c, item.m_value);
                        break;
                    }
                }
            }
        }
    } else {
        wxLogError("m_col_field_types.GetCount() does not match number of observation columns");
    }

    m_IsObserving = true;
}

void ooObservations::StopObservation()
{
    if (!m_IsObserving) return;

    m_ObservationDurationStopWatch.Pause();

    // get date and time
    // char dateString[16];
    // strftime(dateString, 16, "%F", gmtime(&m_position_fix_time));
    // char timeString[16];
    // strftime(timeString, 16, "%T", gmtime(&m_position_fix_time));
    // char utcTimestampString[64];
    // strftime(utcTimestampString, 64, "%FT%TZ", gmtime(&m_position_fix_time));
    wxString dateString = GetUtcTimeFromNMEA(UTC_TIME_DATE);
    wxString timeString = GetUtcTimeFromNMEA(UTC_TIME_TIME);
    wxString utcTimestampString = GetUtcTimeFromNMEA(UTC_TIMESTAMP);
    
    // get duration
    const long duration_ms = GetObservationDuration();
    const unsigned int hours = duration_ms / 3600000;
    const unsigned int minutes = (duration_ms % 3600000) / 60000;
    const unsigned int seconds = (duration_ms % 60000) / 1000;

    char durationString[16];
    sprintf(durationString, "%02u:%02u:%02u", hours, minutes, seconds);

    // fill in fields
    const int currentObservationRow = GetCurrentObservationRow();
    const int C = GetNumberCols();

    if (m_project.GetColCount() == C)
    {
        for (int c=0; c<C; ++c)
        {
            wxString field_type = m_project.GetColFieldTypes()[c];

            if (field_type.IsSameAs("End Date"))
                SetValue(currentObservationRow, c, dateString);
            else if (field_type.IsSameAs("End Time"))
                SetValue(currentObservationRow, c, timeString);
            else if (field_type.IsSameAs("End Timestamp UTC"))
                SetValue(currentObservationRow, c, utcTimestampString);
            else if (field_type.IsSameAs("End Latitude"))
                SetValue(currentObservationRow, c, toSDMM_PlugIn(1, m_position_fix_lat));
            else if (field_type.IsSameAs("End Longitude"))
                SetValue(currentObservationRow, c, toSDMM_PlugIn(2, m_position_fix_lon));
            else if (field_type.IsSameAs("Distance")) {
                double dist = HaversineDistance(StartLatSave, StartLongSave, m_position_fix_lat, m_position_fix_lon);
                SetValue(currentObservationRow, c, wxString::Format("%.3f", dist));
            }
            else if (field_type.IsSameAs("Observation Duration"))
                SetValue(currentObservationRow, c, durationString);
        }
    } else {
        wxLogError("m_col_field_types.GetCount() does not match number of observation columns");
    }

    m_IsObserving = false;
}

bool ooObservations::HasNmeaRecordingField()
{
    const int C = GetNumberCols();
    if (m_project.GetColCount() != C) {
        return false;
    }

    for (int c = 0; c < C; ++c) {
        if (m_project.GetColFieldTypes()[c].IsSameAs("NMEA Recording")) {
            return true;
        }
    }

    return false;
}

void ooObservations::SetCurrentObservationNmeaRecording(const wxString& recordingPath)
{
    if (recordingPath.IsEmpty()) return;
    if (GetNumberRows() <= 0) return;

    const int C = GetNumberCols();
    if (m_project.GetColCount() != C) {
        wxLogError("m_col_field_types.GetCount() does not match number of observation columns");
        return;
    }

    const int currentObservationRow = GetCurrentObservationRow();

    for (int c = 0; c < C; ++c) {
        const wxString field_type = m_project.GetColFieldTypes()[c];

        if (field_type.IsSameAs("NMEA Recording")) {
            SetValue(currentObservationRow, c, recordingPath);
            return;
        }
    }
}

int ooObservations::GetCurrentObservationRow()
{
    if (GetNumberRows() <= 0) {
        return wxNOT_FOUND;
    }

    // The active observation is the newest row, stored at the end of the table.
    return GetNumberRows() - 1;
}

void ooObservations::AddObservation(double lat, double lon)
{
    if (m_IsObserving) return;
    
    double ex_position_fix_lon = m_position_fix_lon,
           ex_position_fix_lat = m_position_fix_lat;
    m_position_fix_lon = lon;
    m_position_fix_lat = lat;
    StartObservation();
    StopObservation();
    m_position_fix_lon = ex_position_fix_lon,
    m_position_fix_lat = ex_position_fix_lat;
}

bool ooObservations::SetObservationMarkGuid(int row, const wxString& markGuid)
{
    if (row < 0 || row >= GetNumberRows() || markGuid.IsEmpty()) {
        return false;
    }

    const int markCol = GetProject().GetMarkCol();

    if (markCol == wxNOT_FOUND) {
        return false;
    }

    SetValue(row, markCol, markGuid);
    return true;
}

const double R_EARTH_KM = 6371.0;  // rayon Terre en km
double ooObservations::DegToRad(double deg) { return deg * M_PI / 180.0; }
constexpr double KM_TO_NAUTICAL_MILES = 1.0 / 1.8520;

// Returns the distance in NM between two GPS positions
double ooObservations::HaversineDistance(double lat1, double lon1, double lat2,
                                         double lon2) {
  double dLat = DegToRad(lat2 - lat1);
  double dLon = DegToRad(lon2 - lon1);

  double radLat1 = DegToRad(lat1);
  double radLat2 = DegToRad(lat2);
  
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(radLat1) * cos(radLat2) * sin(dLon / 2) * sin(dLon / 2);

  double c = 2 * atan2(sqrt(a), sqrt(1 - a));

  double distance = R_EARTH_KM * c * KM_TO_NAUTICAL_MILES;
  return distance;
}

bool ooObservations::IsObserving() const
{
    return m_IsObserving;
}

long ooObservations::GetObservationDuration()
{
    if (IsObserving())
        return m_ObservationDurationStopWatch.Time();
    else
        return 0;
}

wxString ooObservations::GetRowDescription(int row)
{
    wxString res;
    const int C = m_project.GetColCount();

    for (int c = 0; c < C; ++c) {
        wxString field_type = m_project.GetColFieldTypes()[c];
        if (field_type.IsSameAs("Mark GUID") ||
            field_type.IsSameAs("Start Date") ||
            field_type.IsSameAs("Start Time") ||
            field_type.IsSameAs("Start Timestamp UTC") ||
            field_type.IsSameAs("End Date") ||
            field_type.IsSameAs("End Time") ||
            field_type.IsSameAs("End Timestamp UTC") ||
            field_type.IsSameAs("Start Latitude") ||
            field_type.IsSameAs("Start Longitude") ||
            field_type.IsSameAs("End Latitude") ||
            field_type.IsSameAs("End Longitude") ||
            field_type.IsSameAs("Observation Duration") ||
            field_type.IsSameAs("Distance"))
            continue;
        const wxString cell = GetValue(row, c);
        if (cell.Length() == 0) continue;
        res += wxString::Format(
            wxT("%s: %s\n"),
            m_project.GetColLabels()[c],
            cell
        );
    }

    return res;
}

void ooObservations::AddMarks(int targetRow)
{
    // Get column indices
    int markGUIDCol = -1;
    int observationIdCol = -1;
    int dateCol = -1;
    int timeCol = -1;
    int tstpCol = -1;
    int latCol = -1;
    int lonCol = -1;
    int nameCol = -1;
    int descriptionCol = -1;
    int iconCol = -1;
    const int C = m_project.GetColCount();

    for (int c=0; c<C; ++c)
    {
        wxString field_type = m_project.GetColFieldTypes()[c];
        if (field_type.IsSameAs("Mark GUID")) {
            if (markGUIDCol < 0) markGUIDCol = c;
        } else if (field_type.IsSameAs("Observation ID")) {
            if (observationIdCol < 0) observationIdCol = c;
        } else if (field_type.IsSameAs("Start Date")) {
            if (dateCol < 0) dateCol = c;
        } else if (field_type.IsSameAs("Start Time")) {
            if (timeCol < 0) timeCol = c;
        } else if (field_type.IsSameAs("Start Timestamp UTC")) {
            if (tstpCol < 0) tstpCol = c;
        } else if (field_type.IsSameAs("Start Latitude")) {
            if (latCol < 0) latCol = c;
        } else if (field_type.IsSameAs("Start Longitude")) {
            if (lonCol < 0) lonCol = c;
        } else if (field_type.IsSameAs(m_iconsListing)) {
            if (iconCol < 0) iconCol = c;
        } else if (field_type.IsSameAs("Text")) {
            // use first text column as name and second as description
            if (nameCol < 0) nameCol = c;
            else if (descriptionCol < 0) descriptionCol = c;
        }
    }

    if (markGUIDCol < 0) {
        wxLogError("Could not find a column for storing the Mark GUID");
        return;
    }

    if ((latCol < 0) || (lonCol < 0)) {
        wxLogError("Could not find columns storing the latitude and longitude, so unable to make marks");
        return;
    }

    const int R = GetNumberRows();
    for (int r=0; r<R; ++r)
    {
        if (targetRow != -1 && targetRow != r) continue;

        if (GetValue(r, markGUIDCol).IsEmpty())
        {
            const double lat = fromDMM_Plugin(GetValue(r, latCol));
            const double lon = fromDMM_Plugin(GetValue(r, lonCol));

            wxDateTime datetime;
            if (dateCol>=0) datetime.ParseISODate(GetValue(r, dateCol));
            if (timeCol>=0) datetime.ParseISOTime(GetValue(r, timeCol));
            if (tstpCol>=0) datetime.ParseISOCombined(GetValue(r, tstpCol));

            wxString description = GetRowDescription(r);
            wxString name;

            if (observationIdCol >= 0 && !GetValue(r, observationIdCol).IsEmpty()) {
                name = wxString::Format(wxT("%s (OO)"), GetValue(r, observationIdCol));
            } else {
                name = wxString::Format(wxT("%s (OO)"), datetime.FormatISOCombined());
            }

            wxString guid = GetNewGUID();

            wxString icon;
            if (iconCol != -1) {
                wxString iconValue = GetValue(r, iconCol);
                int iconIndex = m_listings[m_iconsListing].Index(iconValue);
                if (iconIndex != wxNOT_FOUND) icon = m_icons[iconIndex];
            }
            if (icon.IsEmpty()) icon = m_project.GetMarkIcon();
            
            // add waypoint
            PlugIn_Waypoint wp(lat, lon, icon, name, guid);
            wp.m_MarkDescription = description;
            wp.m_CreateTime = datetime;
            AddSingleWaypoint(&wp);

            // store guid in table
            SetValue(r, markGUIDCol, guid);
        }
    }
}

void ooObservations::DeleteMarks(int targetRow)
{
    // Get column indices
    int markGUIDCol = -1;
    
    const int C = GetNumberCols();
    if (m_project.GetColCount() == C)
    {
        for (int c=0; c<C; ++c)
        {
            wxString field_type = m_project.GetColFieldTypes()[c];
            if (field_type.IsSameAs("Mark GUID")) {
                if (markGUIDCol < 0) markGUIDCol = c;
            }
        }
    } else {
        wxLogError("m_col_field_types.GetCount() does not match number of observation columns");
        return;
    }

    if (markGUIDCol < 0) {
        wxLogError("Could not find a column for storing the Mark GUID");
        return;
    }

    const int R = GetNumberRows();
    for (int r=0; r<R; ++r)
    {
        if (targetRow != -1 && targetRow != r) continue;

        wxString guid = GetValue(r, markGUIDCol);
        if (guid.IsEmpty()) continue;

        // delete waypoint
        DeleteSingleWaypoint(guid);

        // remove guid from table
        guid.Clear();
        SetValue(r, markGUIDCol, guid);
    }
}

int ooObservations::UpdateObservationsFromMarks()
{
    int res = 0;
    const int R = GetRowsCount();
    const int markCol = GetProject().GetMarkCol();
    const int latCol = GetProject().GetLatCol();
    const int lonCol = GetProject().GetLonCol();

    if (latCol == wxNOT_FOUND ||
        lonCol == wxNOT_FOUND ||
        markCol == wxNOT_FOUND)
      return 0;

    for (int r = R - 1; r >= 0; r--) {
        wxString guid = GetValue(r, markCol);
        if (guid.IsEmpty()) continue;
        const wxString latStr = GetValue(r, latCol);
        const wxString lonStr = GetValue(r, lonCol);
        
        const double lat = fromDMM_Plugin(latStr);
        const double lon = fromDMM_Plugin(lonStr);
        
        PlugIn_Waypoint wp;
        if (GetSingleWaypoint(guid, &wp)) {
            double aLat = wxRound(wp.m_lat * 1000);
            double bLat = wxRound(lat * 1000);
            
            double aLon = wxRound(wp.m_lon * 1000);
            double bLon = wxRound(lon * 1000);
            
            if (wxRound(wp.m_lat*1000) != wxRound(lat*1000) ||
                wxRound(wp.m_lon*1000) != wxRound(lon*1000)) {
                // Update needed
                SetValue(r, latCol, toSDMM_PlugIn(1, wp.m_lat));
                SetValue(r, lonCol, toSDMM_PlugIn(2, wp.m_lon));
                res++;
            }
        } else {
            // The chart mark is only the visible handle.
            // If it disappears, keep the observation safe and simply forget the old link.
            SetValue(r, markCol, wxEmptyString);
            res++;
        }
    }
    return res;
}

static wxString EscapeCSVCell(const wxString& value)
{
    wxString escaped = value;

    // Keep each CSV record on a single line.
    // Multiline cell content is flattened for CSV export only.
    escaped.Replace("\r\n", " ");
    escaped.Replace("\n", " ");
    escaped.Replace("\r", " ");

    // CSV rule: internal quotes must be doubled.
    escaped.Replace("\"", "\"\"");

    // Always wrap in quotes.
    // This safely supports commas, semicolons, quotes,
    // and leading/trailing spaces inside cells.
    return "\"" + escaped + "\"";
}

void ooObservations::SaveToCSV(wxFile *file, bool stripMarkGuid)
{
    const int C = GetNumberCols();
    const int R = GetNumberRows();

    wxArrayInt exportedColumns;

    for (int c = 0; c < C; ++c) {
        const bool isInternalColumn =
            stripMarkGuid &&
            m_project.GetColCount() == C &&
            IsInternalObservationFieldType(m_project.GetColFieldTypes()[c]);

        if (!isInternalColumn) {
            exportedColumns.Add(c);
        }
    }

    for (size_t i = 0; i < exportedColumns.GetCount(); ++i)
    {
        file->Write(EscapeCSVCell(GetColLabelValue(exportedColumns[i])));

        if (i < exportedColumns.GetCount() - 1)
            file->Write(",");
    }
    file->Write("\n");

    for (int r = 0; r < R; ++r)
    {
        for (size_t i = 0; i < exportedColumns.GetCount(); ++i)
        {
            file->Write(EscapeCSVCell(GetValue(r, exportedColumns[i])));

            if (i < exportedColumns.GetCount() - 1)
                file->Write(",");
        }
        file->Write("\n");
    }
}

void ooObservations::SaveToCSVForDate(wxFile *file, const wxString& date, bool stripMarkGuid)
{
    const int C = GetNumberCols();
    const int R = GetNumberRows();

    wxArrayInt exportedColumns;
    int dateCol = -1;
    int timestampCol = -1;

    if (m_project.GetColCount() == C) {
        for (int c = 0; c < C; ++c) {
            const wxString fieldType = m_project.GetColFieldTypes()[c];

            const bool isInternalColumn =
                stripMarkGuid &&
                IsInternalObservationFieldType(fieldType);

            if (!isInternalColumn) {
                exportedColumns.Add(c);
            }

            if (fieldType.IsSameAs("Start Date")) {
                dateCol = c;
            } else if (fieldType.IsSameAs("Start Timestamp UTC")) {
                timestampCol = c;
            }
        }
    } else {
        for (int c = 0; c < C; ++c) {
            exportedColumns.Add(c);
        }
    }

    wxString normalizedDate = date;
    normalizedDate.Trim(true);
    normalizedDate.Trim(false);
    normalizedDate.Replace("/", "-");

    for (size_t i = 0; i < exportedColumns.GetCount(); ++i)
    {
        file->Write(EscapeCSVCell(GetColLabelValue(exportedColumns[i])));

        if (i < exportedColumns.GetCount() - 1)
            file->Write(",");
    }
    file->Write("\n");

    for (int r = 0; r < R; ++r)
    {
        wxString rowDate;

        if (dateCol >= 0) {
            rowDate = GetValue(r, dateCol);
            rowDate.Trim(true);
            rowDate.Trim(false);
        }

        if (rowDate.IsEmpty() && timestampCol >= 0) {
            wxString timestamp = GetValue(r, timestampCol);
            timestamp.Trim(true);
            timestamp.Trim(false);

            if (timestamp.Length() >= 10) {
                rowDate = timestamp.Left(10);
            }
        }

        if (rowDate.Length() >= 10) {
            rowDate = rowDate.Left(10);
            rowDate.Replace("/", "-");
        }

        if (!rowDate.IsSameAs(normalizedDate)) {
            continue;
        }

        for (size_t i = 0; i < exportedColumns.GetCount(); ++i)
        {
            file->Write(EscapeCSVCell(GetValue(r, exportedColumns[i])));

            if (i < exportedColumns.GetCount() - 1)
                file->Write(",");
        }
        file->Write("\n");
    }
}

static wxArrayString ParseCSVLine(const wxString& line)
{
    wxArrayString cells;
    wxString current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.Length(); ++i) {
        wxChar ch = line.GetChar(i);

        if (ch == '"') {
            if (inQuotes && i + 1 < line.Length() && line.GetChar(i + 1) == '"') {
                // Escaped quote inside a quoted cell.
                current += '"';
                ++i;
            } else {
                // Toggle quoted section.
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            cells.Add(current);
            current.clear();
        } else {
            current += ch;
        }
    }

    cells.Add(current);
    return cells;
}

bool ooObservations::ReadFromCSV(const wxString& filename, wxString& err)
{
    wxFileInputStream file_input(filename);
    wxTextInputStream input(file_input);

    // CSV exports intentionally omit internal project fields such as Mark GUID.
    // Build the matching list of user-visible project columns before importing.
    wxArrayInt importedColumns;

    for (int c = 0; c < m_project.GetColCount(); ++c) {
        if (!IsInternalObservationFieldType(m_project.GetColFieldTypes()[c])) {
            importedColumns.Add(c);
        }
    }

    wxString line;
    int r = GetRowsCount();
    bool skipHeader = true;

    while ((line = input.ReadLine()).Length() > 0)
    {
        if (skipHeader) {
            skipHeader = false;
            continue;
        }

        wxArrayString cells = ParseCSVLine(line);
        const int cellCount = cells.GetCount();

        if (cellCount != static_cast<int>(importedColumns.GetCount()))
        {
            err = wxString::Format(
                wxT("CSV contains %i columns, but the current project expects %i user-visible columns"),
                cellCount,
                static_cast<int>(importedColumns.GetCount()));
            return false;
        }

        AppendRows(1);

        for (size_t i = 0; i < cells.GetCount(); ++i) {
            SetValue(r, importedColumns[i], cells[i]);
        }

        ++r;
    }

    return true;
}

void ooObservations::SaveToXML(wxFile *file, bool stripMarkGuid)
{
    const int C = GetNumberCols();
    const int R = GetNumberRows();

    int markGuidCol = -1;

    if (stripMarkGuid && m_project.GetColCount() == C) {
        for (int c = 0; c < C; ++c) {
            if (IsInternalObservationFieldType(m_project.GetColFieldTypes()[c])) {
                markGuidCol = c;
                break;
            }
        }
    }

    wxXmlDocument xmlDoc;
    wxXmlNode* observations = new wxXmlNode(NULL, wxXML_ELEMENT_NODE, "observations");
    observations->AddAttribute("creator", "Open Observer for OpenCPN");
    observations->AddAttribute("file_version", wxString::FromDouble(XML_FILE_VERSION_OBSERVATIONS));
    observations->AddAttribute("row_order", "chronological");
    xmlDoc.SetRoot(observations);

    // xml item 1: save project
    m_project.SaveToXML(observations);

    // xml item 2: save data
    wxXmlNode* data = new wxXmlNode(observations, wxXML_ELEMENT_NODE, "data");

    // wxXmlNode(parent, ...) inserts new nodes before existing children.
    // Iterate backwards so the saved XML remains human-readable:
    // observation id 0 first, newest observation last.
    for (int r = R - 1; r >= 0; --r) {
        wxXmlNode* observation = new wxXmlNode(data, wxXML_ELEMENT_NODE, "observation");
        observation->AddAttribute("id", wxString::Format(wxT("%i"), r));

        for (int c=0; c<C; ++c) {
            wxXmlNode* field = new wxXmlNode(observation, wxXML_ELEMENT_NODE, "field");
            field->AddAttribute("id", wxString::Format(wxT("%i"), c));

            wxString cellValue = GetValue(r, c);

            if (stripMarkGuid && c == markGuidCol) {
                cellValue.Clear();
            }

            field->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", cellValue));
        }
    }

    // write the output to a wxString
    wxStringOutputStream stream;
    xmlDoc.Save(stream);

    // write the string to the file
    file->Write(stream.GetString());
}

bool ooObservations::ReadListingFromXML(const wxString& filename, wxArrayString& result, wxArrayString& icons)
{
    wxXmlDocument xmlDoc;
    int fileVersion = -1;
    if (filename.IsEmpty() || (!xmlDoc.Load(filename)) ||
        (xmlDoc.GetRoot()->GetName() != "listing") ||
        !xmlDoc.GetRoot()->GetAttribute("file_version").ToInt(&fileVersion) ||
         fileVersion > XML_FILE_VERSION_LISTING) {
        return false;
    }

    bool bHasIcon = false;
    wxXmlNode* item = xmlDoc.GetRoot()->GetChildren();
    while (item) {
        int r = -1;
        wxString label = item->GetAttribute("label");
        wxString icon = item->GetAttribute("icon");
        if (label.length() > 0) {
            result.Add(label);
            icons.Add(icon);
        }
        bHasIcon |= (icon.length() > 0);
        
        item = item->GetNext();
    }

    if (!bHasIcon) icons.clear();

    return true;
}

wxXmlNode* FindChild(const wxXmlNode* parent, const wxString& name)
{
    wxXmlNode * child = parent->GetChildren();
    while (child) {
        if (child->GetName() == name) return child;
        child = child->GetNext();
    }
    return NULL;
}

bool ooObservations::ReadFromXML(const wxString& filename, int& fileVersion,
                                 ooProject& project, wxXmlDocument& xmlDoc, wxXmlNode*& root,
                                 const ooProject& defaultProject) {
    if (filename.IsEmpty() || (!xmlDoc.Load(filename))) {
        return false;
    }
    root = xmlDoc.GetRoot();
    fileVersion = -1;

    wxXmlNode* xmlProject = NULL;
    if (root->GetName() == "observations") {
        if (!root->GetAttribute("file_version").ToInt(&fileVersion) ||
            fileVersion > XML_FILE_VERSION_OBSERVATIONS) {
            return false;
        }
        xmlProject = FindChild(root, "project");
    } else if (root->GetName() == "project") {
        if (!root->GetAttribute("file_version").ToInt(&fileVersion) ||
            fileVersion > XML_FILE_VERSION_PROJECT) {
            return false;
        }
        xmlProject = root;
    } else {
        return false;
    }

    if (!project.ReadFromXML(xmlProject)) project = defaultProject;

    return true;
}

bool ooObservations::ReadFromXML(const wxString& filename, const ooProject& defaultProject)
{
    int fileVersion = 0;
    wxXmlNode * root = nullptr;
    wxXmlDocument xmlDoc;
    ooProject project;
    if (!ooObservations::ReadFromXML(filename, fileVersion, project, xmlDoc, root,
                                     defaultProject))
        return false;

    // Make sure the table is cleared before loading any new data
    // (loading blank data would let the old data still present)
    DeleteRows(0, GetNumberRows());

    SetProject(project);

    const int C = GetNumberCols();
    const bool convertLegacyNewestFirstOrder =
        root->GetName() == "observations" &&
        !root->HasAttribute("row_order");

    wxXmlNode* data = (root->GetName() == "project" ? NULL // Loading old project file without data
                                                    : fileVersion == 1 ? root
                                                    : FindChild(root, "data"));
    wxXmlNode* observation = (data ? data->GetChildren() : NULL);
    while (observation)
    {
        int r = -1;
        if (observation->GetAttribute("id").ToInt(&r) && (r>=0)) {

            // expand number of rows as needed to accommodate observations
            if (r >= GetNumberRows()) {
                AppendRows(r - GetNumberRows() + 1);
            }

            wxXmlNode *field = observation->GetChildren();
            while (field) {
                if (field->GetChildren()) {
                    int c = -1;
                    if (field->GetAttribute("id").ToInt(&c) && (c>=0) && (c<C)) {
                        SetValue(r, c, field->GetChildren()->GetContent());
                    }
                }

                field = field->GetNext();
            }
        }

        observation = observation->GetNext();
    }

    if (convertLegacyNewestFirstOrder) {
        const int R = GetNumberRows();

        for (int top = 0, bottom = R - 1; top < bottom; ++top, --bottom) {
            for (int c = 0; c < C; ++c) {
                const wxString topValue = GetValue(top, c);
                SetValue(top, c, GetValue(bottom, c));
                SetValue(bottom, c, topValue);
            }
        }
    }

    return true;
}

#include <wx/jsonwriter.h>
bool ooObservations::SaveToGeoJSON(wxOutputStream& out)
{
    const int R = GetRowsCount();
    const int C = GetColsCount();
    const int latCol = m_project.GetLatCol();
    const int lonCol = m_project.GetLonCol();

    wxJSONValue root;
    root["type"] = wxString("FeatureCollection");
    
    wxJSONValue features;
    for (int r = 0; r < R; r++) {
        const double lat = fromDMM_Plugin(GetValue(r, latCol));
        const double lon = fromDMM_Plugin(GetValue(r, lonCol));
        const wxString des = GetRowDescription(r);

        features[r]["type"] = wxString("Feature");
        features[r]["geometry"]["type"] = wxString("Point");
        features[r]["geometry"]["coordinates"][0] = lon;
        features[r]["geometry"]["coordinates"][1] = lat;

        for (int c = 0; c < C; c++) {
            const wxString field_type = m_project.GetColFieldTypes()[c];
            if (IsInternalObservationFieldType(field_type) ||
                field_type.IsSameAs("End Date") ||
                field_type.IsSameAs("End Time") ||
                field_type.IsSameAs("End Timestamp UTC") ||
                field_type.IsSameAs("Start Latitude") ||
                field_type.IsSameAs("Start Longitude") ||
                field_type.IsSameAs("End Latitude") ||
                field_type.IsSameAs("End Longitude") ||
                field_type.IsSameAs("Observation Duration") ||
                field_type.IsSameAs("Distance"))
                continue;
            const wxString field_label = m_project.GetColLabels()[c];

            features[r]["properties"][field_label] =
                  wxString(GetValue(r,c));
        }
    }
    
    root["features"] = features;

    wxJSONWriter().Write(root, out);

    return true;
}

static std::vector<int> BuildColumnMigrationMap(
    const ooProject& oldProject,
    const ooProject& newProject)
{
    const int oldColCount = oldProject.GetColCount();
    const int newColCount = newProject.GetColCount();

    std::vector<int> oldToNewCol(oldColCount, -1);
    std::vector<bool> newColUsed(newColCount, false);

    // First pass: exact match by label and field type.
    // This preserves columns that were only moved left/right.
    for (int oldC = 0; oldC < oldColCount; ++oldC) {
        for (int newC = 0; newC < newColCount; ++newC) {
            if (newColUsed[newC]) continue;

            if (oldProject.GetColLabels()[oldC].IsSameAs(newProject.GetColLabels()[newC]) &&
                oldProject.GetColFieldTypes()[oldC].IsSameAs(newProject.GetColFieldTypes()[newC])) {
                oldToNewCol[oldC] = newC;
                newColUsed[newC] = true;
                break;
            }
        }
    }

    // Second pass: fallback match by field type only.
    // This preserves data when the user renamed a column but kept the same type.
    for (int oldC = 0; oldC < oldColCount; ++oldC) {
        if (oldToNewCol[oldC] != -1) continue;

        for (int newC = 0; newC < newColCount; ++newC) {
            if (newColUsed[newC]) continue;

            if (oldProject.GetColFieldTypes()[oldC].IsSameAs(newProject.GetColFieldTypes()[newC])) {
                oldToNewCol[oldC] = newC;
                newColUsed[newC] = true;
                break;
            }
        }
    }

    // Third pass: fallback match by label only.
    // The user's column name often carries the real intent. If they change a listing
    // behind the same label, keep the field notes safe and let them clean values later.
    for (int oldC = 0; oldC < oldColCount; ++oldC) {
        if (oldToNewCol[oldC] != -1) continue;

        for (int newC = 0; newC < newColCount; ++newC) {
            if (newColUsed[newC]) continue;

            if (oldProject.GetColLabels()[oldC].IsSameAs(newProject.GetColLabels()[newC])) {
                oldToNewCol[oldC] = newC;
                newColUsed[newC] = true;
                break;
            }
        }
    }

    return oldToNewCol;
}

void ooObservations::SetProject(const ooProject& project)
{
    const ooProject oldProject = m_project;
    const bool structureChanged = !oldProject.IsUpdatable(project);

    const int oldRowCount = GetNumberRows();
    const int oldColCount = GetNumberCols();
    const int newColCount = project.GetColCount();

    std::vector<std::vector<wxString>> oldValues;
    oldValues.resize(oldRowCount);

    for (int r = 0; r < oldRowCount; ++r) {
        oldValues[r].resize(oldColCount);
        for (int c = 0; c < oldColCount; ++c) {
            oldValues[r][c] = GetValue(r, c);
        }
    }

    const std::vector<int> oldToNewCol =
        BuildColumnMigrationMap(oldProject, project);

    if (structureChanged && IsObserving()) {
        StopObservation();
    }

    if (oldRowCount > 0) {
        DeleteRows(0, oldRowCount);
    }

    if (oldColCount > 0) {
        DeleteCols(0, oldColCount);
    }

    m_project = project;

    if (newColCount > 0) {
        InsertCols(0, newColCount);
    }

    if (oldRowCount > 0) {
        InsertRows(0, oldRowCount);
    }

    for (int oldC = 0; oldC < oldColCount && oldC < static_cast<int>(oldToNewCol.size()); ++oldC) {
        const int newC = oldToNewCol[oldC];
        if (newC < 0 || newC >= newColCount) continue;

        for (int r = 0; r < oldRowCount; ++r) {
            SetValue(r, newC, oldValues[r][oldC]);
        }
    }

    // set the column labels
    for (int c = 0; c < newColCount; ++c) {
        SetColLabelValue(c, m_project.GetColLabels()[c]);
    }
}

const ooProject& ooObservations::GetProject() const
{
    return m_project;
}

wxArrayString ooObservations::GetObservationFieldTypes()
{
    wxArrayString observationFieldTypes;

    // Keep one simple field type list for the project grid, but make it readable.
    // The category titles and blank lines are visual helpers only; the project
    // editor refuses them if the user selects them by mistake.
    //
    // Internal fields, such as Mark GUID, are intentionally not listed here.
    // They are kept quietly in the project when needed, hidden from the user,
    // and handled through IsInternalObservationFieldType().
    observationFieldTypes.Add("Common fields :");
    observationFieldTypes.Add(" ");
    observationFieldTypes.Add("Observation ID");
    observationFieldTypes.Add("Start Date");
    observationFieldTypes.Add("Start Time");
    observationFieldTypes.Add("Start Timestamp UTC");
    observationFieldTypes.Add("Start Latitude");
    observationFieldTypes.Add("Start Longitude");
    observationFieldTypes.Add("Distance");
    observationFieldTypes.Add("End Date");
    observationFieldTypes.Add("End Time");
    observationFieldTypes.Add("End Timestamp UTC");
    observationFieldTypes.Add("End Latitude");
    observationFieldTypes.Add("End Longitude");
    observationFieldTypes.Add("Observation Duration");
    observationFieldTypes.Add("NMEA Recording");
    observationFieldTypes.Add("Checkbox");
    observationFieldTypes.Add("Text");

    observationFieldTypes.Add(" ");
    observationFieldTypes.Add("Listings :");
    observationFieldTypes.Add(" ");
    wxArrayString listingNames;
    for (auto it : m_listings) {
        listingNames.Add(it.first);
    }
    listingNames.Sort();
    for (const auto& listingName : listingNames) {
        observationFieldTypes.Add(listingName);
    }

    observationFieldTypes.Add(" ");
    observationFieldTypes.Add("Standard NMEA :");
    observationFieldTypes.Add(" ");
    observationFieldTypes.Add("NMEA AWS");
    observationFieldTypes.Add("NMEA AWA");
    observationFieldTypes.Add("NMEA TWS");
    observationFieldTypes.Add("NMEA TWD");
    observationFieldTypes.Add("NMEA COG");
    observationFieldTypes.Add("NMEA SOG");

    observationFieldTypes.Add(" ");
    observationFieldTypes.Add("Detected NMEA :");
    observationFieldTypes.Add(" ");
    wxArrayString detectedNmeaFields;
    for (const auto& item : ooObservations::m_nmeaFields) {
        if (m_showAdvancedNMEAFields || IsSimpleNMEAField(item)) {
            detectedNmeaFields.Add(item.m_description);
        }
    }
    detectedNmeaFields.Sort();
    for (const auto& fieldName : detectedNmeaFields) {
        observationFieldTypes.Add(fieldName);
    }

    return observationFieldTypes;
}

bool ooObservations::IsInternalObservationFieldType(const wxString& fieldType)
{
    // Internal fields quietly keep Open Observer linked to OpenCPN.
    // They are stored when needed, but are not part of the user protocol.
    return fieldType.IsSameAs("Mark GUID");
}

void ooObservations::ClearListings()
{
    m_listings.clear();
    m_icons.clear();
    m_iconsListing.clear();
}

void ooObservations::AddListing(const wxString& listing, const wxArrayString& items)
{
    m_listings[listing] = items;
}

bool ooObservations::GetListing(const wxString& listing, wxArrayString& items)
{
    if (m_listings.find(listing) == m_listings.end()) return false;

    items = m_listings.at(listing);
    return true;
}

const std::unordered_map<wxString, wxArrayString>& ooObservations::GetListings()
{
    return m_listings;
}

wxArrayString ooObservations::GetListingKeys()
{
    wxArrayString res;
    for (auto it : m_listings) {
        res.Add(it.first);
    }
    return res;
}

void ooObservations::SetIcons(const wxString& listing, const wxArrayString& icons)
{
    m_icons = icons;
    m_iconsListing = listing;
}

void ooObservations::SetNMEAFields(const std::vector<NMEAField>& fields)
{
    m_nmeaFields = fields;
}

void ooObservations::SetShowAdvancedNMEAFields(bool show)
{
    m_showAdvancedNMEAFields = show;
}

bool ooObservations::GetShowAdvancedNMEAFields()
{
    return m_showAdvancedNMEAFields;
}

bool ooObservations::IsSimpleNMEAField(const NMEAField& field)
{
    const wxString& sentenceId = field.m_sentenceId;
    const int fieldIndex = field.m_fieldIndex;

    if (sentenceId == "RMC" && fieldIndex == 9) return true; // UTC date
    if (sentenceId == "RMC" && fieldIndex == 1) return true; // UTC time
    if (sentenceId == "RMC" && fieldIndex == 3) return true; // GPS latitude
    if (sentenceId == "RMC" && fieldIndex == 5) return true; // GPS longitude
    if (sentenceId == "RMC" && fieldIndex == 7) return true; // SOG
    if (sentenceId == "RMC" && fieldIndex == 8) return true; // COG
    if (sentenceId == "RMC" && fieldIndex == 2) return true; // Navigation status

    if (sentenceId == "GGA" && fieldIndex == 1) return true; // UTC time
    if (sentenceId == "GGA" && fieldIndex == 2) return true; // GPS latitude
    if (sentenceId == "GGA" && fieldIndex == 4) return true; // GPS longitude
    if (sentenceId == "GGA" && fieldIndex == 6) return true; // GPS fix quality
    if (sentenceId == "GGA" && fieldIndex == 7) return true; // Satellites in use
    if (sentenceId == "GGA" && fieldIndex == 8) return true; // HDOP
    if (sentenceId == "GGA" && fieldIndex == 9) return true; // Altitude

    if (sentenceId == "GLL" && fieldIndex == 1) return true; // GPS latitude
    if (sentenceId == "GLL" && fieldIndex == 3) return true; // GPS longitude
    if (sentenceId == "GLL" && fieldIndex == 5) return true; // UTC time
    if (sentenceId == "GLL" && fieldIndex == 6) return true; // Navigation status

    if (sentenceId == "VTG" && fieldIndex == 1) return true; // COG true
    if (sentenceId == "VTG" && fieldIndex == 5) return true; // SOG knots
    if (sentenceId == "VTG" && fieldIndex == 7) return true; // SOG km/h

    if (sentenceId == "HDG" && fieldIndex == 1) return true; // Magnetic heading
    if (sentenceId == "HDT" && fieldIndex == 1) return true; // True heading
    if (sentenceId == "HDM" && fieldIndex == 1) return true; // Magnetic heading

    if (sentenceId == "MWV" && fieldIndex == 1) return true; // Apparent wind angle
    if (sentenceId == "MWV" && fieldIndex == 3) return true; // Apparent wind speed

    if (sentenceId == "MWD" && fieldIndex == 1) return true; // True wind direction
    if (sentenceId == "MWD" && fieldIndex == 5) return true; // Wind speed knots

    if (sentenceId == "DPT" && fieldIndex == 1) return true; // Water depth
    if (sentenceId == "DBT" && fieldIndex == 3) return true; // Depth meters
    if (sentenceId == "MTW" && fieldIndex == 1) return true; // Water temperature

    return false;
}

const std::vector<NMEAField>& ooObservations::GetNMEAFields()
{
    return m_nmeaFields;
}
