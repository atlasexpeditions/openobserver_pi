/**************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Open Observer Scientific Package Export
 *
 **************************************************************************/

#include "ooScientificPackage.h"
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
    folders.Add("samples");
    folders.Add("documents");
    folders.Add("notes");
    folders.Add("other");
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

wxArrayString ooScientificPackage::ReadDailyFoldersFromTemplate(const wxString& templatePath)
{
    wxArrayString folders;

    if (templatePath.IsEmpty()) {
        return GetDefaultDailyFolders();
    }

    wxXmlDocument xmlDoc;
    if (!xmlDoc.Load(templatePath)) {
        return GetDefaultDailyFolders();
    }

    wxXmlNode* root = xmlDoc.GetRoot();
    if (!root || root->GetName() != "scientific_package_template") {
        return GetDefaultDailyFolders();
    }

    for (wxXmlNode* node = root->GetChildren(); node; node = node->GetNext()) {
        if (node->GetName() != "daily_media_folder") {
            continue;
        }

        wxString name = node->GetAttribute("name", "");
        name.Trim(true);
        name.Trim(false);

        if (!name.IsEmpty()) {
            folders.Add(SanitizeFileName(name));
        }
    }

    if (folders.IsEmpty()) {
        return GetDefaultDailyFolders();
    }

    return folders;
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

bool ooScientificPackage::IsScientificPackageFolder(
    ooObservations* observations,
    const wxString& packageDir)
{
    if (packageDir.IsEmpty() || !wxDirExists(packageDir)) {
        return false;
    }

    const wxString exportsDir = JoinPath(packageDir, "00_exports");
    const wxString dailyMediaDir = JoinPath(packageDir, "01_daily_media");
    const wxString metadataDir = JoinPath(exportsDir, "metadata");
    const wxString packageMetadataFile = JoinPath(metadataDir, "package_metadata.json");

    if (!wxDirExists(exportsDir) ||
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

    wxString selectedName;

    if (!selectedDir.GetDirs().IsEmpty()) {
        selectedName = selectedDir.GetDirs().Last();
    }

    return selectedName == expectedPrefix ||
           selectedName.StartsWith(expectedPrefix + "_");
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
    wxString& errorMessage)
{
    const wxString dailyRoot = JoinPath(packageDir, "01_daily_media");

    if (!EnsureDirectory(dailyRoot, errorMessage)) {
        return false;
    }

    for (size_t i = 0; i < dates.GetCount(); ++i) {
        const wxString dayDir = JoinPath(dailyRoot, dates[i]);

        if (!EnsureDirectory(dayDir, errorMessage)) {
            return false;
        }

        for (size_t j = 0; j < dailyFolders.GetCount(); ++j) {
            const wxString subDir = JoinPath(dayDir, dailyFolders[j]);

            if (!EnsureDirectory(subDir, errorMessage)) {
                return false;
            }
        }
    }

    return true;
}

bool ooScientificPackage::ExportObservations(
    ooObservations* observations,
    const wxString& packageDir,
    wxString& errorMessage)
{
    if (!observations) {
        errorMessage = "No observations table available.";
        return false;
    }

    const wxString exportsDir = JoinPath(packageDir, "00_exports");
    const wxString exportBaseName = SanitizeFileName(GetProjectName(observations));

    if (!EnsureDirectory(exportsDir, errorMessage)) {
        return false;
    }

    {
        const wxString csvPath = JoinPath(exportsDir, exportBaseName + ".csv");

        wxFileOutputStream output(csvPath);
        if (!output.IsOk()) {
            errorMessage = "Unable to write " + exportBaseName + ".csv";
            return false;
        }

        observations->SaveToCSV(output.GetFile(), true);
    }

    {
        const wxString geojsonPath = JoinPath(exportsDir, exportBaseName + ".geojson");

        wxFileOutputStream output(geojsonPath);
        if (!output.IsOk()) {
            errorMessage = "Unable to write " + exportBaseName + ".geojson";
            return false;
        }

        observations->SaveToGeoJSON(output);
    }

    {
        const wxString xmlPath = JoinPath(exportsDir, exportBaseName + ".xml");

        wxFileOutputStream output(xmlPath);
        if (!output.IsOk()) {
            errorMessage = "Unable to write " + exportBaseName + ".xml";
            return false;
        }

        observations->SaveToXML(output.GetFile(), true);
    }

    return true;
}

bool ooScientificPackage::WriteGeneratedFilesWarning(
    const wxString& packageDir,
    wxString& errorMessage)
{
    const wxString warningPath =
        JoinPath(JoinPath(packageDir, "00_exports"), "DO_NOT_EDIT_GENERATED_FILES.txt");

    wxFile file(warningPath, wxFile::write);
    if (!file.IsOpened()) {
        errorMessage = "Unable to write generated files warning.";
        return false;
    }

    file.Write(
        "Files in this folder are generated by Open Observer and may be refreshed "
        "when updating the Scientific Package.\n\n"
        "If you need to edit exported data manually, please copy the file to "
        "02_working/ first.\n\n"
        "Open Observer will not modify files placed in 01_daily_media/ or "
        "02_working/.\n");

    return true;
}

bool ooScientificPackage::WriteReadme(
    ooObservations* observations,
    const wxString& packageDir,
    wxString& errorMessage)
{
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_exports"), "metadata");

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
        "Open Observer Scientific Package\n"
        "================================\n\n");

    file.Write("Project: " + projectName + "\n");
    file.Write("Created or updated UTC: " + GetCurrentUtcTimestampString() + "\n\n");

    file.Write(
        "Folder structure\n"
        "----------------\n\n"
        "00_exports/\n"
        "  Generated observation exports and metadata. These files may be refreshed\n"
        "  by Open Observer when updating the package.\n\n"
        "01_daily_media/\n"
        "  One folder per observation day. Place photos, audio, video, tracks,\n"
        "  samples, documents, notes and other field media here.\n\n"
        "02_working/\n"
        "  User working folder for notes, maps, reports, tables and analysis files.\n\n"
        "Important rule\n"
        "--------------\n\n"
        "Open Observer may refresh files in 00_exports/.\n"
        "Open Observer will not delete, move or overwrite user media files,\n"
        "documents or custom folders in 01_daily_media/ or 02_working/.\n\n"
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
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_exports"), "metadata");

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

bool ooScientificPackage::WritePackageMetadata(
    ooObservations* observations,
    const wxString& packageDir,
    wxString& errorMessage)
{
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_exports"), "metadata");

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
    file.Write("  \"packageType\": \"Open Observer Scientific Package\",\n");
    file.Write("  \"projectName\": \"" + projectName + "\",\n");
    file.Write("  \"updatedUTC\": \"" + GetCurrentUtcTimestampString() + "\",\n");
    file.Write("  \"observationIdField\": \"Observation ID\",\n");
    file.Write("  \"observationIdFallback\": \"Start Timestamp UTC\",\n");
    file.Write("  \"dailyMediaRoot\": \"01_daily_media\",\n");
    file.Write("  \"exportsRoot\": \"00_exports\",\n");
    file.Write("  \"workingRoot\": \"02_working\"\n");
    file.Write("}\n");

    return true;
}

bool ooScientificPackage::CopyTemplateUsed(const wxString& packageDir, wxString& errorMessage)
{
    const wxString metadataDir = JoinPath(JoinPath(packageDir, "00_exports"), "metadata");

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
        "<scientific_package_template creator=\"Open Observer for OpenCPN\" file_version=\"1\" name=\"Default\">\n"
        "  <daily_media_folder name=\"photos\"/>\n"
        "  <daily_media_folder name=\"audio\"/>\n"
        "  <daily_media_folder name=\"video\"/>\n"
        "  <daily_media_folder name=\"tracks\"/>\n"
        "  <daily_media_folder name=\"samples\"/>\n"
        "  <daily_media_folder name=\"documents\"/>\n"
        "  <daily_media_folder name=\"notes\"/>\n"
        "  <daily_media_folder name=\"other\"/>\n"
        "</scientific_package_template>\n");

    return true;
}

bool ooScientificPackage::Create(
    ooObservations* observations,
    const wxString& rootDir,
    wxString& createdPackagePath,
    wxString& errorMessage)
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
    if (!EnsureDirectory(JoinPath(packageDir, "00_exports"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "00_exports"), "metadata"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(packageDir, "01_daily_media"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(packageDir, "02_working"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "02_working"), "notes"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "02_working"), "maps"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "02_working"), "reports"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "02_working"), "tables"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "02_working"), "working_files"), errorMessage)) return false;

    wxArrayString dates = GetObservationDates(observations);
    wxArrayString dailyFolders = ReadDailyFoldersFromTemplate(GetDefaultTemplatePath());

    if (!CreateDailyFolders(packageDir, dates, dailyFolders, errorMessage)) return false;
    if (!ExportObservations(observations, packageDir, errorMessage)) return false;
    if (!WriteGeneratedFilesWarning(packageDir, errorMessage)) return false;
    if (!WriteReadme(observations, packageDir, errorMessage)) return false;
    if (!WriteObservationIds(observations, packageDir, errorMessage)) return false;
    if (!WritePackageMetadata(observations, packageDir, errorMessage)) return false;
    if (!CopyTemplateUsed(packageDir, errorMessage)) return false;

    createdPackagePath = packageDir;
    return true;
}

bool ooScientificPackage::Update(
    ooObservations* observations,
    const wxString& packageDir,
    wxString& errorMessage)
{
    if (!observations) {
        errorMessage = "No observations table available.";
        return false;
    }

    if (packageDir.IsEmpty() || !wxDirExists(packageDir)) {
        errorMessage = "Selected scientific package folder does not exist.";
        return false;
    }
    if (!IsScientificPackageFolder(observations, packageDir)) {
        errorMessage =
        "The selected folder does not appear to be the Scientific Package for the current Open Observer project.\n\n"
        "Please select the main package folder created by Open Observer for this project.";
        return false;
    }


    if (!EnsureDirectory(JoinPath(packageDir, "00_exports"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "00_exports"), "metadata"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(packageDir, "01_daily_media"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(packageDir, "02_working"), errorMessage)) return false;
    if (!EnsureDirectory(JoinPath(JoinPath(packageDir, "02_working"), "notes"), errorMessage)) return false;

    wxArrayString dates = GetObservationDates(observations);
    wxArrayString dailyFolders = ReadDailyFoldersFromTemplate(GetDefaultTemplatePath());

    if (!CreateDailyFolders(packageDir, dates, dailyFolders, errorMessage)) return false;
    if (!ExportObservations(observations, packageDir, errorMessage)) return false;
    if (!WriteGeneratedFilesWarning(packageDir, errorMessage)) return false;
    if (!WriteReadme(observations, packageDir, errorMessage)) return false;
    if (!WriteObservationIds(observations, packageDir, errorMessage)) return false;
    if (!WritePackageMetadata(observations, packageDir, errorMessage)) return false;
    if (!CopyTemplateUsed(packageDir, errorMessage)) return false;

    return true;
}
