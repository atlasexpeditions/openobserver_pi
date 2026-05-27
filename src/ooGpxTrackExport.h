#ifndef _OO_GPX_TRACK_EXPORT_H_
#define _OO_GPX_TRACK_EXPORT_H_

#include <vector>

#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/string.h>

struct ooGpxTrackPoint {
    double latitude = 0.0;
    double longitude = 0.0;
    wxDateTime timestampUtc;
};

struct ooGpxTrack {
    wxString name;
    wxString guid;
    std::vector<ooGpxTrackPoint> points;
};

struct ooGpxExportResult {
    bool success = false;

    int exportedTrackCount = 0;
    int exportedPointCount = 0;

    wxString message;
};

class ooGpxTrackExport
{
public:
    static bool WriteTrackToGpx(
        const std::vector<ooGpxTrackPoint>& points,
        const wxString& trackName,
        const wxFileName& outputFile,
        wxString* errorMessage = nullptr,
        bool* fileChanged = nullptr);

    static std::vector<ooGpxTrackPoint> CollectOpenCpnTrackPoints();
    static std::vector<ooGpxTrack> CollectOpenCpnTracks();

    static bool WriteTracksToGpx(
        const std::vector<ooGpxTrack>& tracks,
        const wxString& documentName,
        const wxFileName& outputFile,
        wxString* errorMessage = nullptr,
        bool* fileChanged = nullptr);
    
    static ooGpxExportResult ExportDailyOpenCpnTracks(
        const wxArrayString& dates,
        const wxString& packageDir);

    static ooGpxExportResult ExportCompiledOpenCpnTrack(
        const wxArrayString& dates,
        const wxString& packageDir,
        const wxString& projectName);

    static std::vector<ooGpxTrackPoint> FilterPointsByTimeRange(
        const std::vector<ooGpxTrackPoint>& points,
        const wxDateTime& startUtc,
        const wxDateTime& endUtc);
};

#endif