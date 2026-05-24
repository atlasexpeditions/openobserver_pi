#pragma once

#include <wx/string.h>
#include <wx/ffile.h>

class ooNmeaRecorder
{
public:
    ooNmeaRecorder();
    ~ooNmeaRecorder();

    bool StartRecording(const wxString& openObserverPrivateDataDir);
    void StopRecording();

    void WriteSentence(const wxString& sentence);

    bool IsRecording() const;
    wxString GetCurrentFilePath() const;

private:
    wxFFile m_file;
    wxString m_currentFilePath;
    bool m_isRecording;
};