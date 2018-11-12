// RBotFirmware
// Rob Dobson 2017

#include "EvaluatorPatterns.h"
#include "WorkManager.h"

static const char* MODULE_PREFIX = "EvaluatorPatterns: ";

EvaluatorPatterns::~EvaluatorPatterns()
{
    cleanUp();
}

void EvaluatorPatterns::setConfig(const char* configStr)
{
    // Store the config string
    _jsonConfigStr = configStr;
}

const char* EvaluatorPatterns::getConfig()
{
    return _jsonConfigStr.c_str();
}

void EvaluatorPatterns::addExpression(const char* exprStr, bool isInitialValue)
{
    const char* pExpr = exprStr;

    // Skip initial space
    while (isspace(*pExpr))
        ++pExpr;

    // Iterate through the expressions (a single line can contain many)
    while (*pExpr)
    {
        // Extract the variable name and expression
        String outExpr;
        const char* pExprStart = pExpr;
        String inExpr = pExpr;
        // Find the end of the expression
        while ((*pExpr != '\0') && (*pExpr != ';') && (*pExpr != '\n'))
        {
            // Catch a backslash followed by n
            if (*pExpr == '\\' && (*(pExpr+1) == 'n'))
            {
                break;
            }
            ++pExpr;
        }
        int exprLen = pExpr - pExprStart;
        inExpr = inExpr.substring(0, exprLen);

        // Add the assignment
        int varIdx = _patternVars.addAssignment(inExpr.c_str(), outExpr);

        // Store the compiled expression, variable index and whether an initial value
        if (varIdx >= 0)
        {
            // Compile expression
            int err = 0;
            te_variable* vars = _patternVars.getVars();
            te_expr* compiledExpr = te_compile(outExpr.c_str(), vars, _patternVars.getNumVars(), &err);
            // Log.verbose("%scompile %s hex %02x%02x%02x%02x%02x%02x result %ld err %d\n", MODULE_PREFIX, outExpr.c_str(),
            //             outExpr.c_str()[0], outExpr.c_str()[1], outExpr.c_str()[2],
            //             outExpr.c_str()[3], outExpr.c_str()[4], outExpr.c_str()[5],
            //             compiledExpr, err);

            // Store the expression and assigned variable index
            if (compiledExpr)
            {
                VarIdxAndCompiledExpr varIdxAndCompExpr;
                varIdxAndCompExpr._pCompExpr = compiledExpr;
                varIdxAndCompExpr._varIdx = varIdx;
                varIdxAndCompExpr._isInitialValue = isInitialValue;
                _varIdxAndCompiledExprs.push_back(varIdxAndCompExpr);
                Log.verbose("%addLoop addedCompiledExpr (Count=%d)\n", MODULE_PREFIX, _varIdxAndCompiledExprs.size());
            }
        }

        // Next expression - skip separator (in case of escaped chars need to skip two chars)
        if (*pExpr == '\\')
            ++pExpr;
        if (*pExpr)
            ++pExpr;
    }
}

void EvaluatorPatterns::cleanUp()
{
    _patternVars.cleanUp();
    for (unsigned int i = 0; i < _varIdxAndCompiledExprs.size(); i++)
        te_free(_varIdxAndCompiledExprs[i]._pCompExpr);
    _varIdxAndCompiledExprs.clear();
    _isRunning = false;
}

void EvaluatorPatterns::evalExpressions(bool procInitialValues, bool procLoopValues)
{
    // Log.verbose("%snumExprs %d\n", MODULE_PREFIX, _varIdxAndCompiledExprs.size());
    // Go through all the expressions and evaluate
    for (unsigned int i = 0; i < _varIdxAndCompiledExprs.size(); i++)
    {
        // Variable index and type
        int varIdx = _varIdxAndCompiledExprs[i]._varIdx;
        bool isInitialValue = _varIdxAndCompiledExprs[i]._isInitialValue;
        // Check if it is an initial value - in which case don't evaluate
        if (!((isInitialValue && procInitialValues) || (!isInitialValue && procLoopValues)))
            continue;
        // Compute value of expression
        double val = te_eval(_varIdxAndCompiledExprs[i]._pCompExpr);
        _patternVars.setValByIdx(varIdx, val);
        // Log.verbose("%sexpr %d: %s varIdx %d exprRslt %f isInitialValue=%d\n", MODULE_PREFIX,
        //                     i, _patternVars.getVariableName(varIdx).c_str(), varIdx, val, isInitialValue);
    }
}

// Get XY coordinates of point
// Return false if invalid
bool EvaluatorPatterns::getPoint(AxisFloats &pt)
{
    bool xValid = false;
    bool yValid = false;
    pt._pt[0] = _patternVars.getVal("x", xValid, true);
    pt._pt[1] = _patternVars.getVal("y", yValid, true);
    return xValid && yValid;
}

bool EvaluatorPatterns::getStopVar(bool& stopVar)
{
    bool stopValid = false;
    stopVar = _patternVars.getVal("stop", stopValid, true) != 0.0;
    return stopValid;
}

void EvaluatorPatterns::start()
{
    _isRunning = true;
    // Re-evaluate starting conditions
    evalExpressions(true, false);
}

void EvaluatorPatterns::stop()
{
    _isRunning = false;
}

void EvaluatorPatterns::service(WorkManager* pWorkManager)
{
    // Check running
    if (!_isRunning)
        return;

    // Check if the command interpreter can accept new stuff
    if (!pWorkManager->canAcceptWorkItem())
        return;

    // Evaluate expressions
    evalExpressions(false, true);

    // Get next point
    AxisFloats pt;
    bool isValid = getPoint(pt);
    if (!isValid)
    {
        Log.notice("%sstopped X and Y must be specified\n", MODULE_PREFIX);
        _isRunning = false;
        return;
    }
    char cmdStr[100];
    sprintf(cmdStr, "G0 X%F Y%F", pt._pt[0], pt._pt[1]);
    // Log.verbose("%scmdInterp %s\n", MODULE_PREFIX, cmdStr);
    String retStr;
    WorkItem workItem(cmdStr);
    pWorkManager->addWorkItem(workItem, retStr);

    // Check if we reached a limit
    bool stopReqd = 0;
    isValid = getStopVar(stopReqd);
    if (!isValid)
    {
        Log.notice("%sstopped STOP variable not specified\n", MODULE_PREFIX);
        _isRunning = false;
        return;
    }
    if (stopReqd)
    {
        Log.notice("%sPatternEval stopped STOP = TRUE\n", MODULE_PREFIX);
        _isRunning = false;
        return;
    }
}

// Process WorkItem
bool EvaluatorPatterns::execWorkItem(WorkItem& workItem)
{
    // Find the pattern matching the command
    bool isValid = false;
    String patternName = workItem.getString();
    String patternJson = RdJson::getString(patternName.c_str(), "{}", _jsonConfigStr.c_str(), isValid);
    if (isValid)
    {
        // This is a valid pattern
        Log.verbose("%sprocCmd cmdStr %s seqStr %s\n", MODULE_PREFIX, patternName.c_str(), patternJson.c_str());

        // Remove existing pattern
        cleanUp();

        // Get pattern details
        _curPattern = patternName;
        String setupExprs = RdJson::getString("setup", "", patternJson.c_str());
        String loopExprs = RdJson::getString("loop", "", patternJson.c_str());
        Log.trace("%spatternName %s setup %s\n", MODULE_PREFIX,
                        _curPattern.c_str(), setupExprs.c_str());
        Log.trace("%spatternName %s loop %s\n", MODULE_PREFIX,
                        _curPattern.c_str(), loopExprs.c_str());

        // Add to the pattern evaluator expressions
        addExpression(setupExprs.c_str(), true);
        addExpression(loopExprs.c_str(), false);

        // Start the pattern evaluation process
        start();

    }
    return isValid;
}

