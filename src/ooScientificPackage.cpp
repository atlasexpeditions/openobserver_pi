/**************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Open Observer Data Package Export
 *
 **************************************************************************/

#include "ooScientificPackage.h"
#include "ooGpxTrackExport.h"
#include "ooObservations.h"

#include <wx/filename.h>
#include <wx/file.h>
#include <wx/wfstream.h>
#include <wx/sstream.h>
#include <wx/xml/xml.h>
#include <wx/datetime.h>

extern wxString* g_PrivateDataDir;

static wxString JoinPath(const wxString& a, const wxString& b)
{
    wxFileName fn(a, b);
    return fn.GetFullPath();
}

wxString ooScientificPackage::SanitizeFileName(const wxString& value)
{
    wxString res = value;
    res.Trim(true);
    res.Trim(false);

    if (res.IsEmpty()) {
        res = "Untitled";
    }

    const wxString forbidden = wxT("/\\:*?\"<>|");
    for (size_t i = 0; i < forbidden.Length(); ++i) {
        res.Replace(forbidden.Mid(i, 1), "-");
    }

    res.Replace(" ", "-");

    while (res.Contains("--")) {
        res.Replace("--", "-");
    }

    return res;
}

wxString ooScientificPackage::GetCurrentUtcDateString()
{
    wxDateTime now = wxDateTime::Now().ToUTC();
    return now.Format("%Y-%m-%d");
}

wxString ooScientificPackage::GetCurrentUtcTimestampString()
{
    wxDateTime now = wxDateTime::Now().ToUTC();
    return now.Format("%Y-%m-%dT%H:%M:%SZ");
}

wxString ooScientificPackage::GetProjectName(ooObservations* observations)
{
    if (!observations) {
        return "OpenObserver";
    }

    wxString name = observations->GetProject().GetName();
    if (name.IsEmpty()) {
        name = "OpenObserver";
    }

    return name;
}

wxArrayString ooScientificPackage::GetDefaultDailyFolders()
{
    wxArrayString folders;
    folders.Add("photos");
    folders.Add("audio");
    folders.Add("video");
    folders.Add("tracks");
    folders.Add("nmea");
    folders.Add("samples");
    return folders;
}

wxArrayString ooScientificPackage::GetDefaultWorkingFolders()
{
    wxArrayString folders;
    folders.Add("02_working");
    return folders;
}

wxArrayString ooScientificPackage::GetDefaultRawDataFolders()
{
    wxArrayString folders;
    folders.Add("00_raw-data");
    folders.Add("00_raw-data/project");
    folders.Add("00_raw-data/observations");
    folders.Add("00_raw-data/nmea-recordings");
    folders.Add("00_raw-data/tracks");
    return folders;
}

wxString ooScientificPackage::GetDefaultTemplatePath()
{
    if (g_PrivateDataDir) {
        wxFileName userTemplate(
            *g_PrivateDataDir,
            "ScientificPackageTemplates/Default.xml");

        if (userTemplate.FileExists()) {
            return userTemplate.GetFullPath();
        }
    }

    wxFileName sourceTemplate(
        wxFileName::GetCwd(),
        "../data/ScientificPackageTemplates/Default.xml");

    if (sourceTemplate.FileExists()) {
        return sourceTemplate.GetFullPath();
    }

    wxFileName buildTemplate(
        wxFileName::GetCwd(),
        "data/ScientificPackageTemplates/Default.xml");

    if (buildTemplate.FileExists()) {
        return buildTemplate.GetFullPath();
    }

    return wxEmptyString;
}

static wxArrayString ReadFolderNamesFromTemplateSection(
    const wxString& templatePath,
    const wxString& sectionName,
    const wxString& attributeName,
    const wxArrayString& fallbackFolders)
{
    wxArrayString folders;

    if (templatePath.IsEmpty()) {
        return fallbackFolders;
    }

    wxXmlDocument xmlDoc;
    if (!xmlDoc.Load(templatePath)) {
        return fallbackFolders;
    }

    wxXmlNode* root = xmlDoc.GetRoot();
    if (!root || root->GetName() != "data_package_template") {
        return fallbackFolders;
    }

    for (wxXmlNode* sectionNode = root->GetChildren();
         sectionNode;
         sectionNode = sectionNode->GetNext()) {
        if (sectionNode->GetName() != sectionName) {
            continue;
        }

        for (wxXmlNode* folderNode = sectionNode->GetChildren();
             folderNode;
             folderNode = folderNode->GetNext()) {
            if (folderNode->GetName() != "folder") {
                continue;
            }

            wxString name = folderNode->GetAttribute(attributeName, "");
            name.Trim(true);
            name.Trim(false);

            if (!name.IsEmpty()) {
                folders.Add(name);
            }
        }
    }

    if (folders.IsEmpty()) {
        return fallbackFolders;
    }

    return folders;
}

static wxArrayString ReadFolderPathsFromTemplateSection(
    const wxString& templatePath,
    const wxString& sectionName,
    const wxArrayString& fallbackFolders)
{
    wxArrayString folders;

    if (templatePath.IsEmpty()) {
        return fallbackFolders;
    }

    wxXmlDocument xmlDoc;
    if (!xmlDoc.Load(templatePath)) {
        return fallbackFolders;
    }

    wxXmlNode* root = xmlDoc.GetRoot();
    if (!root || root->GetName() != "data_package_template") {
        return fallbackFolders;
    }

    for (wxXmlNode* sectionNode = root->GetChildren();
         sectionNode;
         sectionNode = sectionNode->GetNext()) {
        if (sectionNode->GetName() != sectionName) {
            continue;
        }

        for (wxXmlNode* folderNode = sectionNode->GetChildren();
             folderNode;
             folderNode = folderNode->GetNext()) {
            if (folderNode->GetName() != "folder") {
                continue;
            }

            wxString path = folderNode->GetAttribute("path", "");
            path.Trim(true);
            path.Trim(false);

            if (!path.IsEmpty()) {
                folders.Add(path);
            }
        }
    }

    if (folders.IsEmpty()) {
        return fallbackFolders;
    }

    return folders;
}

wxArrayString ooScientificPackage::ReadDailyFoldersFromTemplate(const wxString& templatePath)
{
    return ReadFolderNamesFromTemplateSection(
        templatePath,
        "daily_media",
        "name",
        GetDefaultDailyFolders());
}

wxArrayString ooScientificPackage::ReadWorkingFoldersFromTemplate(const wxString& templatePath)
{
    return ReadFolderPathsFromTemplateSection(
        templatePath,
        "working",
        GetDefaultWorkingFolders());
}

wxArrayString ooScientificPackage::ReadRawDataFoldersFromTemplate(const wxString& templatePath)
{
    return ReadFolderPathsFromTemplateSection(
        templatePath,
        "raw_data",
        GetDefaultRawDataFolders());
}

bool ooScientificPackage::EnsureDirectory(const wxString& path, wxString& errorMessage)
{
    if (wxDirExists(path)) {
        return true;
    }

    if (!wxFileName::Mkdir(path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
        errorMessage = "Unable to create directory: " + path;
        return false;
    }

    return true;
}

bool ooScientificPackage::CopyFileIfNewOrModified(
    const wxString& sourcePath,
    const wxString& destinationPath,
    const wxString& logLabel,
    wxString& errorMessage,
    RunSummary& runSummary)
{
    if (!wxFileExists(sourcePath)) {
        runSummary.logLines.Add(logLabel + " missing source: " + sourcePath);
        return true;
    }

    bool shouldCopy = true;

    if (wxFileExists(destinationPath)) {
        wxFileName sourceFile(sourcePath);
        wxFileName destinationFile(destinationPath);

        const wxULongLong sourceSize = sourceFile.GetSize();
        const wxULongLong destinationSize = destinationFile.GetSize();

        wxDateTime sourceModified;
        wxDateTime destinationModified;

        sourceFile.GetTimes(NULL, &sourceModified, NULL);
        destinationFile.GetTimes(NULL, &destinationModified, NULL);

        shouldCopy =
            sourceSize != destinationSize ||
            !sourceModified.IsValid() ||
            !destinationModified.IsValid() ||
            sourceModified.IsLaterThan(destinationModified);
    }

    if (!shouldCopy) {
        runSummary.logLines.Add(logLabel + " unchanged: " + destinationPath);
        return true;
    }

    if (!wxCopyFile(sourcePath, destinationPath, true)) {
        errorMessage = "Unable to copy file to: " + destinationPath;
        return false;
    }

    runSummary.nmeaRecordingsCopied++;
    runSummary.logLines.Add(logLabel + " exported or updated: " + destinationPath);

    return true;
}

bool ooScientificPackage::IsScientificPackageFolder(
    ooObservations* observations,
    const wxString& packageDir)
{
    if (packageDir.IsEmpty() || !wxDirExists(packageDir)) {
        return false;
    }

    const wxString rawDataDir = JoinPath(packageDir, "00_raw-data");
    const wxString dailyMediaDir = JoinPath(packageDir, "01_daily-media");
    const wxString metadataDir = JoinPath(rawDataDir, "metadata");
    const wxString packageMetadataFile = JoinPath(metadataDir, "package_metadata.json");

    if (!wxDirExists(rawDataDir) ||
        !wxDirExists(dailyMediaDir) ||
        !wxDirExists(metadataDir) ||
        !wxFileExists(packageMetadataFile)) {
        return false;
    }

    if (!observations) {
        return false;
    }

    const wxString expectedPrefix =
        SanitizeFileName(GetProjectName(observations));

    wxFileName selectedDir(packageDir, "");
    selectedDir.Normalize();

    wxString folderName = selectedDir.GetDirs().IsEmpty()
        ? wxString()
        : selectedDir.GetDirs().Last();

    return folderName.StartsWith(expectedPrefix);
}

wxArrayString ooScientificPackage::GetObservationDates(ooObservations* observations)
{
    wxArrayString dates;

    if (!observations) {
        return dates;
    }

    int dateCol = -1;
    int timestampCol = -1;

    const int C = observations->GetProject().GetColCount();

    for (int c = 0; c < C; ++c) {
        const wxString fieldType = observations->GetProject().GetColFieldTypes()[c];

        if (fieldType.IsSameAs("Start Date")) {
            dateCol = c;
        } else if (fieldType.IsSameAs("Start Timestamp UTC")) {
            timestampCol = c;
        }
    }

    const int R = observations->GetNumberRows();

    for (int r = 0; r < R; ++r) {
        wxString value;

        if (dateCol >= 0) {
            value = observations->GetValue(r, dateCol);
            value.Trim(true);
            value.Trim(false);
        }

        if (value.IsEmpty() && timestampCol >= 0) {
            wxString timestamp = observations->GetValue(r, timestampCol);
            timestamp.Trim(true);
            timestamp.Trim(false);

            // Expected timestamp format: YYYY-MM-DDTHH:MM:SSZ
            if (timestamp.Length() >= 10) {
                value = timestamp.Left(10);
            }
        }

        // Expected date formats:
        // Start Date: YYYY/MM/DD
        // Timestamp date: YYYY-MM-DD
        if (value.Length() >= 10) {
            value = value.Left(10);
            value.Replace("/", "-");
        }

        if (value.IsEmpty()) {
            continue;
        }

        if (dates.Index(value) == wxNOT_FOUND) {
            dates.Add(value);
        }
    }

    if (dates.IsEmpty()) {
        dates.Add(GetCurrentUtcDateString());
    }

    dates.Sort();

    return dates;
}

wxArrayString ooScientificPackage::GetObservationIdsOrFallbacks(ooObservations* observations)
{
    wxArrayString ids;

    if (!observations) {
        return ids;
    }

    int observationIdCol = -1;
    int timestampCol = -1;

    const int C = observations->GetProject().GetColCount();

    for (int c = 0; c < C; ++c) {
        const wxString fieldType = observations->GetProject().GetColFieldTypes()[c];

        if (fieldType.IsSameAs("Observation ID")) {
            observationIdCol = c;
        } else if (fieldType.IsSameAs("Start Timestamp UTC")) {
            timestampCol = c;
        }
    }

    const int R = observations->GetNumberRows();

    for (int r = 0; r < R; ++r) {
        wxString id;

        if (observationIdCol >= 0) {
            id = observations->GetValue(r, observationIdCol);
            id.Trim(true);
            id.Trim(false);
        }

        if (id.IsEmpty() && timestampCol >= 0) {
            wxString ts = observations->GetValue(r, timestampCol);
            ts.Trim(true);
            ts.Trim(false);

            // 2026-05-21T13:54:22Z -> UTC-20260521-135422
            if (ts.Length() >= 20) {
                id = "UTC-" +
                     ts.Mid(0, 4) +
                     ts.Mid(5, 2) +
                     ts.Mid(8, 2) +
                     "-" +
                     ts.Mid(11, 2) +
                     ts.Mid(14, 2) +
                     ts.Mid(17, 2);
            }
        }

        if (!id.IsEmpty() && ids.Index(id) == wxNOT_FOUND) {
            ids.Add(id);
        }
    }

    ids.Sort();

    return ids;
}

bool ooScientificPackage::CreateDailyFolders(
    const wxString& packageDir,
    const wxArrayString& dates,
    const wxArrayString& dailyFolders,
    wxString& errorMessage,
    RunSummary& runSummary)
{
    const wxString dailyRoot = JoinPath(packageDir, "01_daily-media");

    if (!EnsureDirectory(dailyRoot, errorMessage)) {
        return false;
    }

    for (size_t i = 0; i < dates.GetCount(); ++i) {
        const wxString dayDir = JoinPath(dailyRoot, dates[i]);

        if (!EnsureDirectory(dayDir, errorMessage)) {
            return false;
        }

        runSummary.foldersTouched++;
        runSummary.logLines.Add("Folder ensured: 01_daily-media/" + dates[i]);

        for (size_t j = 0; j < dailyFolders.GetCount(); ++j) {
            const wxString subDir = JoinPath(dayDir, dailyFolders[j]);

            if (!EnsureDirectory(subDir, errorMessage)) {
                return false;
            }

            runSummary.foldersTouched++;
            runSummary.logLines.Add(
                "Folder ensured: 01_daily-media/" + dates[i] + "/" + dailyFolders[j]);
        }
    }

    return true;
}

bool ooScientificPackage::CreateStaticFolders(
    const wxString& packageDir,
    const wxArrayString& folderPaths,
    wxString& errorMessage,
    RunSummary& runSummary)
{
    for (size_t i = 0; i < folderPaths.GetCount(); ++i) {
        wxString folderPath = folderPaths[i];
        folderPath.Trim(true);
        folderPath.Trim(false);

        if (folderPath.IsEmpty()) {
            continue;
        }

        folderPath.Replace("\\", "/");

        wxString currentPath = packageDir;
        wxArrayString parts = wxSplit(folderPath, '/');

        for (size_t j = 0; j < parts.GetCount(); ++j) {
            wxString part = parts[j];
            part.Trim(true);
            part.Trim(false);

            if (part.IsEmpty()) {
                continue;
            }

            currentPath = JoinPath(currentPath, part);

            if (!EnsureDirectory(currentPath, errorMessage)) {
                return false;
            }
        }

        runSummary.foldersTouched++;
        runSummary.logLines.Add("Folder ensured: " + folderPath);
    }

    return true;
}

bool ooScientificPackage::ExportObservations(
    ooObservations* observations,
    const wxString& packageDir,
    const wxArrayString& rawDataFolders,
    wxString& errorMessage,
    RunSummary& runSummary)
{
    if (!observations) {
        errorMessage = "No observations table available.";
        return false;
    }

    const wxString exportBaseName = SanitizeFileName(GetProjectName(observations));

    // The project XML is generated raw data and is always updated.
    {
        const wxString projectDir = JoinPath(
            JoinPath(packageDir, "00_raw-data"),
            "project");

        if (!EnsureDirectory(projectDir, errorMessage)) {
            return false;
        }

        const wxString xmlPath = JoinPath(projectDir, exportBaseName + ".xml");

        wxFileOutputStream output(xmlPath);
        if (!output.IsOk()) {
            errorMessage = "Unable to write 00_raw-data/project/" + exportBaseName + ".xml";
            return false;
        }

        observations->SaveToXML(output.GetFile(), true);
        runSummary.exportFilesRefreshed++;
        runSummary.logLines.Add(
            "Project XML updated: 00_raw-data/project/" + exportBaseName + ".xml");
    }

    // CSV and GeoJSON observation exports are optional and live under 00_raw-data/observations.
    if (rawDataFolders.Index("00_raw-data/observations") == wxNOT_FOUND) {
        runSummary.logLines.Add("Observation CSV/GeoJSON exports skipped: observations folder not selected.");
        return true;
    }

    const wxString observationsDir = JoinPath(
        JoinPath(packageDir, "00_raw-data"),
        "observations");

    if (!EnsureDirectory(observationsDir, errorMessage)) {
        return false;
    }

    {
        const wxString csvPath = JoinPath(observationsDir, exportBaseName + ".csv");

        wxFileOutputStream output(csvPath);
        if (!output.IsOk()) {
            errorMessage = "Unable to write " + exportBaseName + ".csv";
            return false;
        }

        observations->SaveToCSV(output.GetFile(), true);
        runSummary.exportFilesRefreshed++;
        runSummary.logLines.Add(
            "Observation export updated: 00_raw-data/observations/" +
            exportBaseName + ".csv");
    }

    {
        const wxString geojsonPath = JoinPath(observationsDir, exportBaseName + ".geojson");

        wxFileOutputStream output(geojsonPath);
        if (!output.IsOk()) {
            errorMessage = "Unable to write " + exportBaseName + ".geojson";
            return false;
        }

        observations->SaveToGeoJSON(output);
        runSummary.exportFilesRefreshed++;
        runSummary.logLines.Add(
            "Observation export updated: 00_raw-data/observations/" +
            exportBaseName + ".geojson");
    }

    return true;
}

bool ooScientificPackage::CopyNmeaRecordings(
    ooObservations* observations,
    const wxString& packageDir,
    wxString& errorMessage,
    RunSummary& runSummary)
{
    if (!observations) {
        return true;
    }

    if (!g_PrivateDataDir || g_PrivateDataDir->IsEmpty()) {
        runSummary.logLines.Add("NMEA recordings skipped: private data folder is not available.");
        return true;
    }

    int dateCol = -1;
    int timestampCol = -1;
    int nmeaRecordingCol = -1;

    const int C = observations->GetProject().GetColCount();

    for (int c = 0; c < C; ++c) {
        const wxString fieldType = observations->GetProject().GetColFieldTypes()[c];

        if (fieldType.IsSameAs("Start Date")) {
            dateCol = c;
        } else if (fieldType.IsSameAs("Start Timestamp UTC")) {
            timestampCol = c;
        } else if (fieldType.IsSameAs("NMEA Recording")) {
            nmeaRecordingCol = c;
        }
    }

    if (nmeaRecordingCol < 0) {
        runSummary.logLines.Add("NMEA recordings skipped: no NMEA Recording column in project.");
        return true;
    }

    const int R = observations->GetNumberRows();

    for (int r = 0; r < R; ++r) {
        wxString recordingName = observations->GetValue(r, nmeaRecordingCol);
        recordingName.Trim(true);
        recordingName.Trim(false);

        if (recordingName.IsEmpty() || recordingName.IsSameAs("no data")) {
            continue;
        }

        wxFileName recordingFile(recordingName);
        recordingName = recordingFile.GetFullName();

        if (recordingName.IsEmpty()) {
            continue;
        }

        wxString dateValue;

        if (dateCol >= 0) {
            dateValue = observations->GetValue(r, dateCol);
            dateValue.Trim(true);
            dateValue.Trim(false);
        }

        if (dateValue.IsEmpty() && timestampCol >= 0) {
            wxString timestamp = observations->GetValue(r, timestampCol);
            timestamp.Trim(true);
            timestamp.Trim(false);

            if (timestamp.Length() >= 10) {
                dateValue = timestamp.Left(10);
            }
        }

        if (dateValue.Length() >= 10) {
            dateValue = dateValue.Left(10);
            dateValue.Replace("/", "-");
        }

        if (dateValue.IsEmpty()) {
            runSummary.logLines.Add("NMEA recording skipped, no date available: " + recordingName);
            continue;
        }

        const wxString sourcePath = JoinPath(
            JoinPath(
                JoinPath(*g_PrivateDataDir, "NMEArecordings"),
                dateValue),
            recordingName);

        if (!wxFileExists(sourcePath)) {
            runSummary.logLines.Add("NMEA recording missing: " + sourcePath);
            continue;
        }

        const wxString dailyNmeaDir = JoinPath(
            JoinPath(
                JoinPath(packageDir, "01_daily-media"),
                dateValue),
            "nmea");

        const wxString rawNmeaDir = JoinPath(
            JoinPath(packageDir, "00_raw-data"),
            "nmea-recordings");

        if (!EnsureDirectory(dailyNmeaDir, errorMessage)) {
            return false;
        }

        if (!EnsureDirectory(rawNmeaDir, errorMessage)) {
            return false;
        }

        const wxString dailyDestination = JoinPath(dailyNmeaDir, recordingName);
        const wxString rawDestination = JoinPath(rawNmeaDir, recordingName);

        if (!CopyFileIfNewOrModified(
                sourcePath,
                dailyDestination,
                "NMEA daily export: 01_daily-media/" + dateValue + "/nmea/" + recordingName,
                errorMessage,
                runSummary)) {
            return false;
        }

        if (!CopyFileIfNewOrModified(
                sourcePath,
                rawDestination,
                "NMEA raw export: 00_raw-data/nmea-recordings/" + recordingName,
                errorMessage,
                runSummary)) {
            return false;
        }
    }

    return true;
}

bool ooScientificPackage::WriteRunLog(
    const wxString& packageDir,
    const wxString& actionLabel,
    RunSummary& runSummary,
    wxString& errorMessage)
{
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_raw-data"), "metadata");

    if (!EnsureDirectory(metadataDir, errorMessage)) {
        return false;
    }

    const wxString logPath = JoinPath(metadataDir, "data_package_last_run.txt");

    wxFile file(logPath, wxFile::write);
    if (!file.IsOpened()) {
        errorMessage = "Unable to write data_package_last_run.txt";
        return false;
    }

    runSummary.logPath = logPath;

    file.Write("Open Observer Data Package run log\n");
    file.Write("==================================\n\n");
    file.Write("Action: " + actionLabel + "\n");
    file.Write("Generated UTC: " + GetCurrentUtcTimestampString() + "\n\n");

    file.Write("Summary\n");
    file.Write("-------\n");
    file.Write(wxString::Format("Folders touched: %d\n", runSummary.foldersTouched));
    file.Write(wxString::Format("NMEA recordings exported: %d\n", runSummary.nmeaRecordingsCopied));
    file.Write(wxString::Format("Export files updated: %d\n", runSummary.exportFilesRefreshed));
    file.Write(wxString::Format("Daily GPX tracks exported: %d\n", runSummary.gpxDailyTracksExported));
    file.Write(wxString::Format("Compiled GPX tracks exported: %d\n", runSummary.gpxCompiledTracksExported));
    file.Write(wxString::Format("GPX track points exported: %d\n\n", runSummary.gpxTrackPointsExported));

    file.Write("Details\n");
    file.Write("-------\n");

    if (runSummary.logLines.IsEmpty()) {
        file.Write("No detailed entries.\n");
    } else {
        for (size_t i = 0; i < runSummary.logLines.GetCount(); ++i) {
            file.Write("- " + runSummary.logLines[i] + "\n");
        }
    }

    return true;
}

bool ooScientificPackage::WriteGeneratedFilesWarning(
    const wxString& packageDir,
    wxString& errorMessage)
{
    const wxString warningPath =
        JoinPath(JoinPath(packageDir, "00_raw-data"), "do-not-edit-generated-files.txt");

    wxFile file(warningPath, wxFile::write);
    if (!file.IsOpened()) {
        errorMessage = "Unable to write generated files warning.";
        return false;
    }

    file.Write(
        "Files in this folder are generated by Open Observer and may be updated "
        "when updating the Data Package.\n\n"
        "If you need to edit exported data manually, please copy the file to "
        "02_working/ first.\n\n"
        "Open Observer will not modify files placed in 01_daily-media/ or "
        "02_working/.\n");

    return true;
}

bool ooScientificPackage::WriteReadme(
    ooObservations* observations,
    const wxString& packageDir,
    wxString& errorMessage)
{
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_raw-data"), "metadata");

    if (!EnsureDirectory(metadataDir, errorMessage)) {
        return false;
    }

    wxFile file(JoinPath(metadataDir, "README.txt"), wxFile::write);
    if (!file.IsOpened()) {
        errorMessage = "Unable to write README.txt";
        return false;
    }

    wxString projectName = GetProjectName(observations);

        file.Write(
        "Folder structure\n"
        "----------------\n\n"
        "00_raw-data/\n"
        "  Generated raw data, exports and metadata. These files may be updated by\n"
        "  Open Observer when creating or updating the package.\n\n"
        "00_raw-data/project/\n"
        "  Generated Open Observer project XML export.\n\n"
        "00_raw-data/observations/\n"
        "  Generated CSV and GeoJSON observation exports.\n\n"
        "00_raw-data/nmea-recordings/\n"
        "  Raw NMEA recording files exported from Open Observer when available.\n\n"
        "00_raw-data/tracks/\n"
        "  Raw OpenCPN GPX track exports generated by Open Observer when available.\n\n"
        "01_daily-media/\n"
        "  One folder per observation day. Place photos, audio, video, daily tracks,\n"
        "  NMEA files and samples here.\n\n"
        "02_working/\n"
        "  User working folder for notes, maps, reports, tables and analysis files.\n\n"
        "Important rule\n"
        "--------------\n\n"
        "Open Observer may refresh generated files in 00_raw-data/.\n"
        "Open Observer will not delete, move or clean user media files in 01_daily-media/\n"
        "or user working files in 02_working/.\n\n"
        "Recommended file naming\n"
        "-----------------------\n\n"
        "Use the Observation ID when available, for example:\n"
        "OBS-260521-001_IMG_2045.jpg\n"
        "OBS-260521-001_hydrophone.wav\n\n"
        "If no Observation ID is available, use the compact UTC timestamp fallback,\n"
        "for example:\n"
        "UTC-20260521-135422_IMG_2045.jpg\n");

    return true;
}

bool ooScientificPackage::WriteObservationIds(
    ooObservations* observations,
    const wxString& packageDir,
    wxString& errorMessage)
{
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_raw-data"), "metadata");

    if (!EnsureDirectory(metadataDir, errorMessage)) {
        return false;
    }

    wxFile file(JoinPath(metadataDir, "observation_ids.txt"), wxFile::write);
    if (!file.IsOpened()) {
        errorMessage = "Unable to write observation_ids.txt";
        return false;
    }

    wxArrayString ids = GetObservationIdsOrFallbacks(observations);

    for (size_t i = 0; i < ids.GetCount(); ++i) {
        file.Write(ids[i] + "\n");
    }

    return true;
}

static wxString EscapeJsonString(const wxString& value)
{
    wxString escaped = value;
    escaped.Replace("\\", "\\\\");
    escaped.Replace("\"", "\\\"");
    return escaped;
}

static void WriteJsonStringArray(
    wxFile& file,
    const wxString& key,
    const wxArrayString& values,
    bool trailingComma)
{
    file.Write("  \"" + key + "\": [");

    for (size_t i = 0; i < values.GetCount(); ++i) {
        if (i > 0) {
            file.Write(", ");
        }

        file.Write("\"" + EscapeJsonString(values[i]) + "\"");
    }

    file.Write("]");

    if (trailingComma) {
        file.Write(",");
    }

    file.Write("\n");
}

bool ooScientificPackage::WriteDataPackageSettings(
    const wxString& packageDir,
    const wxArrayString& dailyFolders,
    const wxArrayString& workingFolders,
    const wxArrayString& rawDataFolders,
    wxString& errorMessage)
{
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_raw-data"), "metadata");

    if (!EnsureDirectory(metadataDir, errorMessage)) {
        return false;
    }

    wxFile file(JoinPath(metadataDir, "data_package_settings.json"), wxFile::write);
    if (!file.IsOpened()) {
        errorMessage = "Unable to write data_package_settings.json";
        return false;
    }

    file.Write("{\n");
    file.Write("  \"fileType\": \"Open Observer Data Package Settings\",\n");
    file.Write("  \"fileVersion\": 1,\n");
    file.Write("  \"updatedUTC\": \"" + GetCurrentUtcTimestampString() + "\",\n");
    WriteJsonStringArray(file, "dailyMediaFolders", dailyFolders, true);
    WriteJsonStringArray(file, "workingFolders", workingFolders, true);
    WriteJsonStringArray(file, "rawDataFolders", rawDataFolders, false);
    file.Write("}\n");

    return true;
}

bool ooScientificPackage::WritePackageMetadata(
    ooObservations* observations,
    const wxString& packageDir,
    wxString& errorMessage)
{
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_raw-data"), "metadata");

    if (!EnsureDirectory(metadataDir, errorMessage)) {
        return false;
    }

    wxFile file(JoinPath(metadataDir, "package_metadata.json"), wxFile::write);
    if (!file.IsOpened()) {
        errorMessage = "Unable to write package_metadata.json";
        return false;
    }

    wxString projectName = GetProjectName(observations);
    projectName.Replace("\\", "\\\\");
    projectName.Replace("\"", "\\\"");

    file.Write("{\n");
    file.Write("  \"packageType\": \"Open Observer Data Package\",\n");
    file.Write("  \"projectName\": \"" + projectName + "\",\n");
    file.Write("  \"updatedUTC\": \"" + GetCurrentUtcTimestampString() + "\",\n");
    file.Write("  \"observationIdField\": \"Observation ID\",\n");
    file.Write("  \"observationIdFallback\": \"Start Timestamp UTC\",\n");
    file.Write("  \"dailyMediaRoot\": \"01_daily-media\",\n");
    file.Write("  \"rawDataRoot\": \"00_raw-data\",\n");
    file.Write("  \"workingRoot\": \"02_working\"\n");
    file.Write("}\n");

    return true;
}

bool ooScientificPackage::CopyTemplateUsed(const wxString& packageDir, wxString& errorMessage)
{
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_raw-data"), "metadata");

    if (!EnsureDirectory(metadataDir, errorMessage)) {
        return false;
    }

    const wxString templatePath = GetDefaultTemplatePath();
    const wxString destination = JoinPath(metadataDir, "folder_template_used.xml");

    if (!templatePath.IsEmpty() && wxFileExists(templatePath)) {
        if (!wxCopyFile(templatePath, destination, true)) {
            errorMessage = "Unable to copy folder_template_used.xml";
            return false;
        }
        return true;
    }

    wxFile file(destination, wxFile::write);
    if (!file.IsOpened()) {
        errorMessage = "Unable to write folder_template_used.xml";
        return false;
    }

        file.Write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<data_package_template creator=\"Open Observer for OpenCPN\" file_version=\"1\" name=\"Default\">\n"
        "  <always>\n"
        "    <folder path=\"00_raw-data\"/>\n"
        "    <folder path=\"00_raw-data/metadata\"/>\n"
        "    <folder path=\"00_raw-data/project\"/>\n"
        "    <folder path=\"00_raw-data/observations\"/>\n"
        "    <folder path=\"00_raw-data/nmea-recordings\"/>\n"
        "    <folder path=\"00_raw-data/tracks\"/>\n"
        "    <folder path=\"01_daily-media\"/>\n"
        "    <folder path=\"02_working\"/>\n"
        "  </always>\n"
        "  <daily_media>\n"
        "    <folder name=\"photos\" default=\"true\"/>\n"
        "    <folder name=\"audio\" default=\"true\"/>\n"
        "    <folder name=\"video\" default=\"true\"/>\n"
        "    <folder name=\"tracks\" default=\"true\"/>\n"
        "    <folder name=\"nmea\" default=\"true\"/>\n"
        "    <folder name=\"samples\" default=\"true\"/>\n"
        "  </daily_media>\n"
        "  <raw_data>\n"
        "    <folder path=\"00_raw-data/observations\" default=\"true\"/>\n"
        "    <folder path=\"00_raw-data/nmea-recordings\" default=\"true\"/>\n"
        "    <folder path=\"00_raw-data/tracks\" default=\"true\"/>\n"
        "  </raw_data>\n"
        "  <working>\n"
        "    <folder path=\"02_working\" default=\"true\"/>\n"
        "  </working>\n"
        "</data_package_template>\n");

    return true;
}

bool ooScientificPackage::Create(
    ooObservations* observations,
    const wxString& rootDir,
    wxString& createdPackagePath,
    wxString& errorMessage,
    const wxArrayString& dailyFolders,
    const wxArrayString& workingFolders,
    const wxArrayString& rawDataFolders,
    RunSummary& runSummary)
{
    if (!observations) {
        errorMessage = "No observations table available.";
        return false;
    }

    if (rootDir.IsEmpty()) {
        errorMessage = "No destination folder selected.";
        return false;
    }

    const wxString projectName = SanitizeFileName(GetProjectName(observations));
    const wxString packageName = projectName;

    wxString packageDir = JoinPath(rootDir, packageName);

    int suffix = 2;
    while (wxDirExists(packageDir)) {
        packageDir = JoinPath(
            rootDir,
            packageName + wxString::Format(wxT("_%02d"), suffix));
        suffix++;
    }

    if (!EnsureDirectory(packageDir, errorMessage)) return false;
    runSummary.logLines.Add("Package folder ensured: " + packageDir);

    if (!EnsureDirectory(JoinPath(packageDir, "00_raw-data"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "00_raw-data"), "metadata"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(packageDir, "01_daily-media"), errorMessage)) return false;

    wxArrayString dates = GetObservationDates(observations);

    if (!CreateStaticFolders(packageDir, workingFolders, errorMessage, runSummary)) return false;
    if (!CreateStaticFolders(packageDir, rawDataFolders, errorMessage, runSummary)) return false;
    if (!CreateDailyFolders(packageDir, dates, dailyFolders, errorMessage, runSummary)) return false;
    if (dailyFolders.Index("tracks") != wxNOT_FOUND) {
    ooGpxExportResult gpxResult =
        ooGpxTrackExport::ExportDailyOpenCpnTracks(dates, packageDir);

    runSummary.gpxDailyTracksExported += gpxResult.exportedTrackCount;
    runSummary.gpxTrackPointsExported += gpxResult.exportedPointCount;

    if (gpxResult.success) {
        runSummary.logLines.Add(
            wxString::Format("Daily GPX tracks exported: %d", gpxResult.exportedTrackCount));
    } else if (!gpxResult.message.IsEmpty()) {
        runSummary.logLines.Add("Daily GPX tracks not exported: " + gpxResult.message);
    }
}

if (rawDataFolders.Index("00_raw-data/tracks") != wxNOT_FOUND) {
    ooGpxExportResult gpxResult =
        ooGpxTrackExport::ExportCompiledOpenCpnTrack(
            dates,
            packageDir,
            SanitizeFileName(GetProjectName(observations)));

    runSummary.gpxCompiledTracksExported += gpxResult.exportedTrackCount;
    runSummary.gpxTrackPointsExported += gpxResult.exportedPointCount;

    if (gpxResult.success) {
        runSummary.logLines.Add("Compiled GPX track exported.");
    } else if (!gpxResult.message.IsEmpty()) {
        runSummary.logLines.Add("Compiled GPX track not exported: " + gpxResult.message);
    }
}
    if (!CopyNmeaRecordings(observations, packageDir, errorMessage, runSummary)) return false;
    if (!ExportObservations(observations, packageDir, rawDataFolders, errorMessage, runSummary)) return false;
    if (!WriteGeneratedFilesWarning(packageDir, errorMessage)) return false;
    if (!WriteReadme(observations, packageDir, errorMessage)) return false;
    if (!WriteObservationIds(observations, packageDir, errorMessage)) return false;
    if (!WritePackageMetadata(observations, packageDir, errorMessage)) return false;
    if (!WriteDataPackageSettings(packageDir, dailyFolders, workingFolders, rawDataFolders, errorMessage)) return false;
    if (!CopyTemplateUsed(packageDir, errorMessage)) return false;
    if (!WriteRunLog(packageDir, "Create Data Package", runSummary, errorMessage)) return false;

    createdPackagePath = packageDir;
    return true;
}

bool ooScientificPackage::Update(
    ooObservations* observations,
    const wxString& packageDir,
    wxString& errorMessage,
    const wxArrayString& dailyFolders,
    const wxArrayString& workingFolders,
    const wxArrayString& rawDataFolders,
    RunSummary& runSummary)
{
    if (!observations) {
        errorMessage = "No observations table available.";
        return false;
    }

    if (packageDir.IsEmpty() || !wxDirExists(packageDir)) {
        errorMessage = "Selected data package folder does not exist.";
        return false;
    }

    if (!IsScientificPackageFolder(observations, packageDir)) {
        errorMessage =
        "The selected folder does not appear to be the Data Package for the current Open Observer project.\n\n"
        "Please select the main package folder created by Open Observer for this project.";
        return false;
    }

    if (!EnsureDirectory(JoinPath(packageDir, "00_raw-data"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "00_raw-data"), "metadata"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(packageDir, "01_daily-media"), errorMessage)) return false;

    wxArrayString dates = GetObservationDates(observations);

    if (!CreateStaticFolders(packageDir, workingFolders, errorMessage, runSummary)) return false;
    if (!CreateStaticFolders(packageDir, rawDataFolders, errorMessage, runSummary)) return false;
    if (!CreateDailyFolders(packageDir, dates, dailyFolders, errorMessage, runSummary)) return false;
    if (dailyFolders.Index("tracks") != wxNOT_FOUND) {
    ooGpxExportResult gpxResult =
        ooGpxTrackExport::ExportDailyOpenCpnTracks(dates, packageDir);

    runSummary.gpxDailyTracksExported += gpxResult.exportedTrackCount;
    runSummary.gpxTrackPointsExported += gpxResult.exportedPointCount;

    if (gpxResult.success) {
        runSummary.logLines.Add(
            wxString::Format("Daily GPX tracks exported: %d", gpxResult.exportedTrackCount));
    } else if (!gpxResult.message.IsEmpty()) {
        runSummary.logLines.Add("Daily GPX tracks not exported: " + gpxResult.message);
    }
}

if (rawDataFolders.Index("00_raw-data/tracks") != wxNOT_FOUND) {
    ooGpxExportResult gpxResult =
        ooGpxTrackExport::ExportCompiledOpenCpnTrack(
            dates,
            packageDir,
            SanitizeFileName(GetProjectName(observations)));

    runSummary.gpxCompiledTracksExported += gpxResult.exportedTrackCount;
    runSummary.gpxTrackPointsExported += gpxResult.exportedPointCount;

    if (gpxResult.success) {
        runSummary.logLines.Add("Compiled GPX track exported.");
    } else if (!gpxResult.message.IsEmpty()) {
        runSummary.logLines.Add("Compiled GPX track not exported: " + gpxResult.message);
    }
}
    if (!CopyNmeaRecordings(observations, packageDir, errorMessage, runSummary)) return false;
    if (!ExportObservations(observations, packageDir, rawDataFolders, errorMessage, runSummary)) return false;
    if (!WriteGeneratedFilesWarning(packageDir, errorMessage)) return false;
    if (!WriteReadme(observations, packageDir, errorMessage)) return false;
    if (!WriteObservationIds(observations, packageDir, errorMessage)) return false;
    if (!WritePackageMetadata(observations, packageDir, errorMessage)) return false;
    if (!WriteDataPackageSettings(packageDir, dailyFolders, workingFolders, rawDataFolders, errorMessage)) return false;
    if (!CopyTemplateUsed(packageDir, errorMessage)) return false;
    if (!WriteRunLog(packageDir, "Update Data Package", runSummary, errorMessage)) return false;

    return true;
}
