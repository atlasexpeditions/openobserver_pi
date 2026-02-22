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

#include "tpUtils.h"
#include "ocpn_plugin.h"

bool ooProject::ReadFromXML(const wxXmlNode* project)
{
    if (project == NULL ||
        project->GetName() != "project") {
        return false;
    }

    m_col_labels.clear();
    m_col_field_types.clear();
    m_col_sizes = wxGridSizesInfo();

    m_name = project->GetAttribute("name");

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

    return true;
}

wxXmlNode* ooProject::SaveToXML(wxXmlNode* parent)
{
    const int C = m_col_field_types.size();

    wxXmlNode* project = new wxXmlNode(parent, wxXML_ELEMENT_NODE, "project");
    project->AddAttribute("name", m_name);
    
    for (int c = 0; c < C; ++c) {
        wxXmlNode* field = new wxXmlNode(project, wxXML_ELEMENT_NODE, "field");
        field->AddAttribute("id", wxString::Format(wxT("%i"), c));
        field->AddAttribute("label", m_col_labels[c]);
        field->AddAttribute("field_type", m_col_field_types[c]);
        field->AddAttribute(
            "col_size", wxString::Format(wxT("%i"), m_col_sizes.GetSize(c)));
    }

    return project;
}

std::unordered_map<wxString, wxArrayString> ooObservations::m_listings;

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

void ooObservations::SetPositionFix(time_t fixTime, double lat, double lon)
{
    m_position_fix_time = fixTime;
    m_position_fix_lat = lat;
    m_position_fix_lon = lon;
}

////////////VPE NMEA//////////////
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
                ComputeTrueWind(m_SOG, m_apparentWindSpeed, m_apparentWindAngle, m_trueWindSpeed,
                                m_trueWindAngle);
            }
        }
        if (nmea.LastSentenceIDParsed == "RMC") {
             m_SOG = nmea.Rmc.SpeedOverGroundKnots;
             m_COG = nmea.Rmc.TrackMadeGoodDegreesTrue;
             //wxLogMessage("SOG: %.2f kn  COG: %.2f°", m_SOG, m_COG);

             wxString sog = GetNMEAField(m_sentenceNMEA, "RMC", 7);  // SOG
             wxString cog = GetNMEAField(m_sentenceNMEA, "RMC", 8);  // COG
             //wxLogMessage("SOG=%s COG=%s", sog, cog);
        }
    }
}

// Computes TrueWindSpeed and TrueWindAngle using the Apparent Wind and the GPS SOG.
void ooObservations::ComputeTrueWind(double sog,
                                     double apparentWindSpeed,
                                     double apparentWindAngle,
                                     double& trueWindSpeed,
                                     double& trueWindAngle)
{
    double awa_rad = apparentWindAngle * M_PI / 180.0;
    
    double Vx = apparentWindSpeed * cos(awa_rad);
    double Vy = apparentWindSpeed * sin(awa_rad);
    
    double Vtx = Vx + sog;
    double Vty = Vy;
    
    trueWindSpeed = sqrt(Vtx * Vtx + Vty * Vty);
    trueWindAngle = atan2(Vty, Vtx) * 180.0 / M_PI;
    if (trueWindAngle < 0) trueWindAngle += 360.0;
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

void ooObservations::StartObservation()
{
    if (m_IsObserving) return;

    // start duration stopwatch
    m_ObservationDurationStopWatch.Start(0);

    // get date and time
    char dateString[16];
    strftime(dateString, 16, "%F", gmtime(&m_position_fix_time));
    char timeString[16];
    strftime(timeString, 16, "%T", gmtime(&m_position_fix_time));
    char utcTimestampString[64];
    strftime(utcTimestampString, 64, "%FT%TZ", gmtime(&m_position_fix_time));

    // create new observation in table and fill in fields
    InsertRows(0, 1);
    const int C = GetNumberCols();
    if (m_project.GetColCount() == C)
    {
        for (int c=0; c<C; ++c)
        {
            wxString field_type = m_project.GetColFieldTypes()[c];
            if (field_type.IsSameAs("Start Date"))
                SetValue(0, c, dateString);
            else if (field_type.IsSameAs("Start Time"))
                SetValue(0, c, timeString);
            else if (field_type.IsSameAs("Start Timestamp UTC"))
                SetValue(0, c, utcTimestampString);
            else if (field_type.IsSameAs("Start Latitude"))
              {
               
                SetValue(0, c, toSDMM_PlugIn(1, m_position_fix_lat));
                StartLatSave = m_position_fix_lat;
              }
            else if (field_type.IsSameAs("Start Longitude"))
              {
                 SetValue(0, c, toSDMM_PlugIn(1, m_position_fix_lon));
                 StartLongSave = m_position_fix_lon;
              }
            else if (field_type.IsSameAs("NMEA TWS"))
                SetValue(0, c, wxString::Format("%d", (int)round(m_trueWindSpeed)));
            else if (field_type.IsSameAs("NMEA TWA"))
                SetValue(0, c, wxString::Format("%d", (int)round(m_trueWindAngle)));
            else if (field_type.IsSameAs("NMEA COG"))
                SetValue(0, c, wxString::Format("%d", (int)round(m_COG)));
            else if (field_type.IsSameAs("NMEA SOG"))
                SetValue(0, c, wxString::Format("%.1f", m_SOG));
            else if (field_type.IsSameAs("Distance"))
              SetValue(0, c, "...");
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
    char dateString[16];
    strftime(dateString, 16, "%F", gmtime(&m_position_fix_time));
    char timeString[16];
    strftime(timeString, 16, "%T", gmtime(&m_position_fix_time));
    char utcTimestampString[64];
    strftime(utcTimestampString, 64, "%FT%TZ", gmtime(&m_position_fix_time));

    // get duration
    const long duration_ms = GetObservationDuration();
    const unsigned int hours = duration_ms / 3600000;
    const unsigned int minutes = (duration_ms % 3600000) / 60000;
    const unsigned int seconds = (duration_ms % 60000) / 1000;

    char durationString[16];
    sprintf(durationString, "%02u:%02u:%02u", hours, minutes, seconds);

    // fill in fields
    const int C = GetNumberCols();
    if (m_project.GetColCount() == C)
    {
        for (int c=0; c<C; ++c)
        {
            wxString field_type = m_project.GetColFieldTypes()[c];
            if (field_type.IsSameAs("End Date"))
                SetValue(0, c, dateString);
            else if (field_type.IsSameAs("End Time"))
                SetValue(0, c, timeString);
            else if (field_type.IsSameAs("End Timestamp UTC"))
                SetValue(0, c, utcTimestampString);
            else if (field_type.IsSameAs("End Latitude"))
                SetValue(0, c, toSDMM_PlugIn(1, m_position_fix_lat));
            else if (field_type.IsSameAs("End Longitude"))
                SetValue(0, c, toSDMM_PlugIn(2, m_position_fix_lon));
          else if (field_type.IsSameAs("Distance")) {
                double dist = HaversineDistance(StartLatSave, StartLongSave, m_position_fix_lat, m_position_fix_lon);
                SetValue(0, c,wxString::Format("%.3f", dist));
          }
            else if (field_type.IsSameAs("Observation Duration"))
                SetValue(0, c, durationString);
        }
    } else {
        wxLogError("m_col_field_types.GetCount() does not match number of observation columns");
    }

    m_IsObserving = false;
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
        res += wxString::Format(wxT("%s: %s\n"), field_type, cell);
    }

    return res;
}

void ooObservations::AddMarks()
{
    // Get column indices
    int markGUIDCol = -1;
    int dateCol = -1;
    int timeCol = -1;
    int tstpCol = -1;
    int latCol = -1;
    int lonCol = -1;
    int nameCol = -1;
    int descriptionCol = -1;
    const int C = m_project.GetColCount();

    for (int c=0; c<C; ++c)
    {
        wxString field_type = m_project.GetColFieldTypes()[c];
        if (field_type.IsSameAs("Mark GUID")) {
            if (markGUIDCol < 0) markGUIDCol = c;
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
        if (GetValue(r, markGUIDCol).IsEmpty())
        {
            const double lat = fromDMM_Plugin(GetValue(r, latCol));
            const double lon = fromDMM_Plugin(GetValue(r, lonCol));

            wxDateTime datetime;
            if (dateCol>=0) datetime.ParseISODate(GetValue(r, dateCol));
            if (timeCol>=0) datetime.ParseISOTime(GetValue(r, timeCol));
            if (tstpCol>=0) datetime.ParseISOCombined(GetValue(r, tstpCol));

            wxString description = GetRowDescription(r);
            wxString name = wxString::Format(wxT("%s (OO)"), datetime.FormatISOCombined());

            wxString guid = GetNewGUID();

            // add waypoint
            PlugIn_Waypoint wp(lat, lon, "Info-Fish-Whale", name, guid);
            wp.m_MarkDescription = description;
            wp.m_CreateTime = datetime;
            AddSingleWaypoint(&wp);

            // store guid in table
            SetValue(r, markGUIDCol, guid);
        }
    }
}

void ooObservations::DeleteMarks()
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
        wxString guid = GetValue(r, markGUIDCol);
        if (guid.IsEmpty()) continue;

        // delete waypoint
        DeleteSingleWaypoint(guid);

        // remove guid from table
        guid.Clear();
        SetValue(r, markGUIDCol, guid);
    }
}

void ooObservations::SaveToCSV(wxFile *file)
{
    const int C = GetNumberCols();
    const int R = GetNumberRows();
    
    for (int c=0; c<C; ++c)
    {
        file->Write("\"" + GetColLabelValue(c) + "\"");

        if (c<(C - 1))
            file->Write(",");
    }
    file->Write("\n");

    for (int r=0; r<R; ++r)
    {
        for (int c=0; c<C; ++c)
        {
            file->Write("\"" + GetValue(r, c) + "\"");

            if (c<(C - 1))
                file->Write(",");
        }
        file->Write("\n");
    }

    file->Close();
}

static wxString FormatImportedCsvCell(const wxString& value)
{
    wxString res = value;
    if (value.Length() >= 2 &&
        value.GetChar(0) == '"' &&
        value.GetChar(value.Length() - 1) == '"') {
        int length = res.Length();
        res = res.Right(length - 1).Left(length - 2);
    }
    return res;
}

bool ooObservations::ReadFromCSV(const wxString& filename, wxString& err)
{
    wxFileInputStream file_input(filename);
    wxTextInputStream input(file_input);
    const int C = m_project.GetColCount();

    wxString line;
    int r = GetRowsCount();
    bool bSkip = true;
    while ((line = input.ReadLine()).Length() > 0)
    {
        if (bSkip) {
            bSkip = false;
            continue;
        }
        wxArrayString cells = wxSplit(line, ',');
        const int cellCount = cells.GetCount();
        if (cellCount != C)
        {
            err = wxString::Format(wxT("CSV contains %i columns, but the current project has %i"), cellCount, C);
            return false;
        }
        AppendRows(1);
        int c = 0;
        for (auto cell: cells)
        {
            SetValue(r, c, FormatImportedCsvCell(cell));
            c++;
        }
        r++;
    }

    return true;
}

void ooObservations::SaveToXML(wxFile *file)
{
    const int C = GetNumberCols();
    const int R = GetNumberRows();

    wxXmlDocument xmlDoc;
    wxXmlNode* observations = new wxXmlNode(NULL, wxXML_ELEMENT_NODE, "observations");
    observations->AddAttribute("creator", "Open Observer for OpenCPN");
    observations->AddAttribute("file_version", wxString::FromDouble(XML_FILE_VERSION_OBSERVATIONS));
    xmlDoc.SetRoot(observations);

    // xml item 1: save project
    m_project.SaveToXML(observations);

    // xml item 2: save data
    wxXmlNode* data = new wxXmlNode(observations, wxXML_ELEMENT_NODE, "data");

    for (int r=0; r<R; ++r) {
        wxXmlNode* observation = new wxXmlNode(data, wxXML_ELEMENT_NODE, "observation");
        observation->AddAttribute("id", wxString::Format(wxT("%i"), r));

        for (int c=0; c<C; ++c) {
            wxXmlNode* field = new wxXmlNode(observation, wxXML_ELEMENT_NODE, "field");
            field->AddAttribute("id", wxString::Format(wxT("%i"), c));
            field->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "",  GetValue(r, c)));
        }
    }
    
    // write the output to a wxString
    wxStringOutputStream stream;
    xmlDoc.Save(stream);

    // write the string to the file
    file->Write(stream.GetString());    
}

bool ooObservations::ReadListingFromXML(const wxString& filename, wxArrayString& result)
{
    wxXmlDocument xmlDoc;
    int fileVersion = -1;
    if (filename.IsEmpty() || (!xmlDoc.Load(filename)) ||
        (xmlDoc.GetRoot()->GetName() != "listing") ||
        !xmlDoc.GetRoot()->GetAttribute("file_version").ToInt(&fileVersion) ||
         fileVersion > XML_FILE_VERSION_LISTING) {
        return false;
    }
    
    wxXmlNode* item = xmlDoc.GetRoot()->GetChildren();
    while (item) {
        int r = -1;
        wxString label = item->GetAttribute("label");
        if (label.length() > 0) {
            result.Add(label);
        }
        
        item = item->GetNext();
    }
    
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

bool ooObservations::ReadFromXML(const wxString& filename, const ooProject& defaultProject)
{
    wxXmlDocument xmlDoc;
    if (filename.IsEmpty() || (!xmlDoc.Load(filename))) {
        return false;
    }
    wxXmlNode* root = xmlDoc.GetRoot();
    int fileVersion = -1;
    if (root->GetName() != "observations" || 
        !root->GetAttribute("file_version").ToInt(&fileVersion) ||
        fileVersion > XML_FILE_VERSION_OBSERVATIONS) {
          return false;
    }

    wxXmlNode* project = FindChild(root, "project");
    if (!m_project.ReadFromXML(project)) m_project = defaultProject;

    SetProject(m_project);

    const int C = GetNumberCols();
    wxXmlNode* data = (fileVersion == 1 ? root
                                        : FindChild(xmlDoc.GetRoot(), "data"));
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

    return true;
}

void ooObservations::SetProject(const ooProject& project)
{
    // TODO Remove this function once we can create various observations
    //      Project will be set at ooObservation creation
    //      with constructor ooObservations(const ooProject& p)

    m_project = project;

    // stop any observation, if one is running
    StopObservation();
    
    // delete table
    if (GetNumberRows() > 0)
        DeleteRows(0, GetNumberRows());
    
    if (GetNumberCols() > 0)
        DeleteCols(0, GetNumberCols());
    
    const int C = m_project.GetColCount();
    // add columns
    InsertCols(0, C);
    
    // set the column labels
    for (int c = 0; c < C; ++c) {
        SetColLabelValue(c, m_project.GetColLabels()[c]);
    }
}

const ooProject& ooObservations::GetProject() const
{
    return m_project;
}

/*

  // set the column field types
  wxArrayString colFieldTypes;
  for (int c = 0; c < m_gridProject->GetNumberCols(); ++c) {
    wxString fieldType = m_gridProject->GetCellValue(1, c);
    if (fieldType.IsEmpty()) fieldType = wxString("Text");

    colFieldTypes.Add(fieldType);
  }
  m_Observations->SetColFieldTypes(colFieldTypes);

*/

wxArrayString ooObservations::GetObservationFieldTypes()
{
    wxArrayString observationFieldTypes;
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
    observationFieldTypes.Add("Mark GUID");
    observationFieldTypes.Add("Text");
    observationFieldTypes.Add("NMEA TWS");
    observationFieldTypes.Add("NMEA TWA");
    observationFieldTypes.Add("NMEA COG");
    observationFieldTypes.Add("NMEA SOG");

    for (auto it : m_listings)
    {
      observationFieldTypes.Add(it.first);
    }

    return observationFieldTypes;
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
