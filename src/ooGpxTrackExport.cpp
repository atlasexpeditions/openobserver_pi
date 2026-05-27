#include "ooGpxTrackExport.h"
#include "ocpn_plugin.h"

#include <wx/file.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <memory>

bool ooGpxTrackExport::WriteTrackToGpx(
    const std::vector<ooGpxTrackPoint>& points,
    const wxString& trackName,
    const wxFileName& outputFile,
    wxString* errorMessage)
{
    if (points.empty()) {
        if (errorMessage) {
            *errorMessage = "No track points to export.";
        }

        return false;
    }

    wxFileName outputDir(outputFile.GetPath(), "");

    if (!outputDir.DirExists()) {
        if (!outputDir.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
            if (errorMessage) {
                *errorMessage = "Failed to create GPX output directory.";
            }

            return false;
        }
    }

    wxTextFile file;

    if (outputFile.FileExists()) {
        file.Open(outputFile.GetFullPath());
        file.Clear();
    } else {
        file.Create(outputFile.GetFullPath());
        file.Open(outputFile.GetFullPath());
    }

    file.AddLine("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

    file.AddLine(
        "<gpx version=\"1.1\" creator=\"OpenObserver\" "
        "xmlns=\"http://www.topografix.com/GPX/1/1\">");

    file.AddLine("  <trk>");

    file.AddLine(
        wxString::Format(
            "    <name>%s</name>",
            trackName));

    file.AddLine("    <trkseg>");

    for (const auto& point : points) {

        const wxString timestamp =
            point.timestampUtc.FormatISOCombined('T') + "Z";

        file.AddLine(
            wxString::Format(
                "      <trkpt lat=\"%.8f\" lon=\"%.8f\">",
                point.latitude,
                point.longitude));

        file.AddLine(
            wxString::Format(
                "        <time>%s</time>",
                timestamp));

        file.AddLine("      </trkpt>");
    }

    file.AddLine("    </trkseg>");
    file.AddLine("  </trk>");
    file.AddLine("</gpx>");

    file.Write();
    file.Close();

    return true;
}

bool ooGpxTrackExport::WriteTracksToGpx(
    const std::vector<ooGpxTrack>& tracks,
    const wxString& documentName,
    const wxFileName& outputFile,
    wxString* errorMessage)
{
    if (tracks.empty()) {
        if (errorMessage) {
            *errorMessage = "No tracks to export.";
        }
        return false;
    }

    wxFileName outputDir(outputFile.GetPath(), "");

    if (!outputDir.DirExists()) {
        if (!outputDir.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
            if (errorMessage) {
                *errorMessage = "Failed to create GPX output directory.";
            }
            return false;
        }
    }

    wxTextFile file;

    if (outputFile.FileExists()) {
        file.Open(outputFile.GetFullPath());
        file.Clear();
    } else {
        file.Create(outputFile.GetFullPath());
        file.Open(outputFile.GetFullPath());
    }

    file.AddLine("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    file.AddLine(
        "<gpx version=\"1.1\" creator=\"OpenObserver\" "
        "xmlns=\"http://www.topografix.com/GPX/1/1\">");

    file.AddLine("  <metadata>");
    file.AddLine(wxString::Format("    <name>%s</name>", documentName));
    file.AddLine("  </metadata>");

    for (const auto& track : tracks) {
        if (track.points.empty()) {
            continue;
        }

        file.AddLine("  <trk>");

        wxString trackName = track.name;
        if (trackName.IsEmpty()) {
            trackName = "OpenCPN track";
        }

        file.AddLine(wxString::Format("    <name>%s</name>", trackName));
        file.AddLine("    <trkseg>");

        for (const auto& point : track.points) {
            const wxString timestamp =
                point.timestampUtc.FormatISOCombined('T') + "Z";

            file.AddLine(
                wxString::Format(
                    "      <trkpt lat=\"%.8f\" lon=\"%.8f\">",
                    point.latitude,
                    point.longitude));

            file.AddLine(
                wxString::Format(
                    "        <time>%s</time>",
                    timestamp));

            file.AddLine("      </trkpt>");
        }

        file.AddLine("    </trkseg>");
        file.AddLine("  </trk>");
    }

    file.AddLine("</gpx>");

    file.Write();
    file.Close();

    return true;
}

std::vector<ooGpxTrack>
ooGpxTrackExport::CollectOpenCpnTracks()
{
    std::vector<ooGpxTrack> collectedTracks;

    const wxArrayString trackGuids = GetTrackGUIDArray();

    for (size_t i = 0; i < trackGuids.GetCount(); ++i) {
        std::unique_ptr<PlugIn_Track> track =
            GetTrack_Plugin(trackGuids[i]);

        if (!track || !track->pWaypointList) {
            continue;
        }

        ooGpxTrack gpxTrack;
        gpxTrack.name = track->m_NameString;
        gpxTrack.guid = track->m_GUID;

        Plugin_WaypointList::compatibility_iterator node =
            track->pWaypointList->GetFirst();

        while (node) {
            PlugIn_Waypoint* waypoint = node->GetData();

            if (waypoint && waypoint->m_CreateTime.IsValid()) {
                ooGpxTrackPoint point;
                point.latitude = waypoint->m_lat;
                point.longitude = waypoint->m_lon;
                point.timestampUtc = waypoint->m_CreateTime;

                gpxTrack.points.push_back(point);
            }

            node = node->GetNext();
        }

        if (!gpxTrack.points.empty()) {
            collectedTracks.push_back(gpxTrack);
        }
    }

    return collectedTracks;
}

std::vector<ooGpxTrackPoint>
ooGpxTrackExport::CollectOpenCpnTrackPoints()
{
    std::vector<ooGpxTrackPoint> collectedPoints;

    const wxArrayString trackGuids = GetTrackGUIDArray();

    for (size_t i = 0; i < trackGuids.GetCount(); ++i) {

        std::unique_ptr<PlugIn_Track> track =
            GetTrack_Plugin(trackGuids[i]);

        if (!track) {
            continue;
        }

        if (!track->pWaypointList) {
            continue;
        }

        Plugin_WaypointList::compatibility_iterator node =
            track->pWaypointList->GetFirst();

        while (node) {

            PlugIn_Waypoint* waypoint = node->GetData();

            if (waypoint) {

                if (waypoint->m_CreateTime.IsValid()) {

                    ooGpxTrackPoint point;

                    point.latitude = waypoint->m_lat;
                    point.longitude = waypoint->m_lon;
                    point.timestampUtc = waypoint->m_CreateTime;

                    collectedPoints.push_back(point);
                }
            }

            node = node->GetNext();
        }
    }

    return collectedPoints;
}

static wxDateTime MakeUtcDateTimeForDay(const wxString& date, int hour, int minute, int second)
{
    wxDateTime dt;

    if (!dt.ParseISODate(date)) {
        return wxDateTime();
    }

    dt.SetHour(hour);
    dt.SetMinute(minute);
    dt.SetSecond(second);

    return dt;
}

ooGpxExportResult ooGpxTrackExport::ExportDailyOpenCpnTracks(
    const wxArrayString& dates,
    const wxString& packageDir)
{
    ooGpxExportResult result;

    const std::vector<ooGpxTrackPoint> allPoints = CollectOpenCpnTrackPoints();

    for (size_t i = 0; i < dates.GetCount(); ++i) {
        const wxString date = dates[i];

        const wxDateTime startUtc = MakeUtcDateTimeForDay(date, 0, 0, 0);
        const wxDateTime endUtc = MakeUtcDateTimeForDay(date, 23, 59, 59);

        if (!startUtc.IsValid() || !endUtc.IsValid()) {
            continue;
        }

        const std::vector<ooGpxTrackPoint> dayPoints =
            FilterPointsByTimeRange(allPoints, startUtc, endUtc);

        if (dayPoints.empty()) {
            continue;
        }

        wxFileName outputFile(
            packageDir + "/01_daily_media/" + date + "/tracks",
            date + "_openobserver_daily_track.gpx");

        wxString errorMessage;

        if (WriteTrackToGpx(
                dayPoints,
                "OpenObserver daily track " + date,
                outputFile,
                &errorMessage)) {

            result.exportedTrackCount++;
            result.exportedPointCount += static_cast<int>(dayPoints.size());
        } else if (!errorMessage.IsEmpty()) {
            result.message += errorMessage + "\n";
        }
    }

    result.success = result.exportedTrackCount > 0;

    return result;
}

ooGpxExportResult ooGpxTrackExport::ExportCompiledOpenCpnTrack(
    const wxArrayString& dates,
    const wxString& packageDir,
    const wxString& projectName)
{
    ooGpxExportResult result;

    if (dates.IsEmpty()) {
        result.message = "No dates available for compiled GPX export.";
        return result;
    }

    const wxString firstDate = dates[0];
    const wxString lastDate = dates[dates.GetCount() - 1];

    const wxDateTime startUtc = MakeUtcDateTimeForDay(firstDate, 0, 0, 0);
    const wxDateTime endUtc = MakeUtcDateTimeForDay(lastDate, 23, 59, 59);

    if (!startUtc.IsValid() || !endUtc.IsValid()) {
        result.message = "Invalid date range for compiled GPX export.";
        return result;
    }

const std::vector<ooGpxTrack> allTracks = CollectOpenCpnTracks();

std::vector<ooGpxTrack> filteredTracks;
int exportedPointCount = 0;

for (const auto& track : allTracks) {
    ooGpxTrack filteredTrack;
    filteredTrack.name = track.name;
    filteredTrack.guid = track.guid;
    filteredTrack.points = FilterPointsByTimeRange(track.points, startUtc, endUtc);

    if (!filteredTrack.points.empty()) {
        exportedPointCount += static_cast<int>(filteredTrack.points.size());
        filteredTracks.push_back(filteredTrack);
    }
}

if (filteredTracks.empty()) {
    result.message = "No OpenCPN tracks found for compiled GPX export.";
    return result;
}

    wxFileName outputFile(
        packageDir + "/00_exports/raw_data/tracks",
        "full_" + projectName + "_tracks.gpx");

    wxString errorMessage;

if (WriteTracksToGpx(
        filteredTracks,
        "OpenObserver compiled OpenCPN tracks " + firstDate + " to " + lastDate,
        outputFile,
        &errorMessage)) {

    result.success = true;
    result.exportedTrackCount = static_cast<int>(filteredTracks.size());
    result.exportedPointCount = exportedPointCount;
} else {
    result.message = errorMessage;
}

    return result;
}

std::vector<ooGpxTrackPoint>
ooGpxTrackExport::FilterPointsByTimeRange(
    const std::vector<ooGpxTrackPoint>& points,
    const wxDateTime& startUtc,
    const wxDateTime& endUtc)
{
    std::vector<ooGpxTrackPoint> filteredPoints;

    for (const auto& point : points) {

        if (!point.timestampUtc.IsValid()) {
            continue;
        }

        if (point.timestampUtc < startUtc) {
            continue;
        }

        if (point.timestampUtc > endUtc) {
            continue;
        }

        filteredPoints.push_back(point);
    }

    return filteredPoints;
}