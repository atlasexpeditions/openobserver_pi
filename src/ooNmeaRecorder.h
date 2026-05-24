#pragma once

#include <wx/string.h>
#include <wx/ffile.h>

class ooNmeaRecorder
{
public:
    ooNmeaRecorder();
    ~ooNmeaRecorder();

    bool StartRecording(const wxString& openObserverPrivateDataDir);
    wxString StopRecording();

    void WriteSentence(const wxString& sentence);

    bool IsRecording() const;
    bool HasRecordedData() const;
    wxString GetCurrentFilePath() const;

private:
    wxFFile m_file;
    wxString m_tempFilePath;
    wxString m_finalFilePath;
    bool m_isRecording;
    bool m_hasRecordedData;
};