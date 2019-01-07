// RBotFirmware
// Rob Dobson 2017

#pragma once

class WorkManager;
class WorkItem;
class FileManager;

class EvaluatorSequences
{
public:
    static const int MAX_SEQUENCE_FILE_LEN = 2000;

    EvaluatorSequences(FileManager& fileManager, WorkManager& workManager);

    // Config
    void setConfig(const char* configStr);
    const char* getConfig();

    // Modes
    bool _shuffleMode;
    bool _repeatMode;
    bool _defaultShuffleMode;
    bool _defaultRepeatMode;
    int _lineCount;

    // Is Busy
    bool isBusy();
    
    // Check valid
    bool isValid(WorkItem& workItem);

    // Process WorkItem
    bool execWorkItem(WorkItem& workItem);

    // Call frequently
    void service();

    // Control
    void stop();
    
private:
    // Count lines
    int countLines(String& lines);

    // Full configuration JSON
    String _jsonConfigStr;

    // File manager & work manager
    FileManager& _fileManager;
    WorkManager& _workManager;

    // List of commands to add to workflow - delimited string
    String _commandList;

    // Busy and current line
    int _inProgress;
    int _reqLineIdx;
    int _linesDone;
};
