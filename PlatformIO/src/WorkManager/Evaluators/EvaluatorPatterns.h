// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "EvaluatorPattern_Vars.h"
#include "tinyexpr.h"
#include <vector>
#include "AxisValues.h"

class WorkManager;
class WorkItem;
class FileManager;

class EvaluatorPatterns
{
public:
    EvaluatorPatterns(FileManager& fileManager, WorkManager& WorkManager);
    ~EvaluatorPatterns();
    void cleanUp();

    // Config
    void setConfig(const char* configStr, const char* robotAttributes);
    const char* getConfig();

    // Is Busy
    bool isBusy();
    
    // Check valid
    bool isValid(WorkItem& workItem);

    // Expressions
    void addExpression(const char* exprStr, bool isInitialValue);
    void evalExpressions(bool procInitialValues, bool procLoopValues);

    // Get XY coordinates of point
    // Return false if invalid
    bool getPoint(AxisFloats &pt);
    bool getStopVar(bool& stopVar);

    // Control
    void start();
    void stop();

    // Call frequently
    void service();

    // Process WorkItem
    bool execWorkItem(WorkItem& workItem);

private:
    // Full configuration JSON
    String _jsonConfigStr;

    // File manager & work manager
    FileManager& _fileManager;
    WorkManager& _workManager;

    // Robot attributes (size, etc)
    String _robotAttribStr;

    // All variables used in the pattern
    EvaluatorPattern_Vars _patternVars;

    // Store for a variable index and compiled expression that evaluates to that variable
    typedef struct VarIdxAndCompiledExpr
    {
        te_expr* _pCompExpr;
        int _varIdx;
        bool _isInitialValue;
    } VarIdxAndCompiledExpr;

    // List of variable indices and compiled expressions
    std::vector<VarIdxAndCompiledExpr> _varIdxAndCompiledExprs;

    // Indicator that the current pattern is running
    bool _isRunning;

    // Current pattern name
    String _curPattern;
};
