// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "PatternEvaluator_Vars.h"
#include "tinyexpr.h"
#include <vector>

class PatternEvaluator
{
public:
    PatternEvaluator()
    {
        _isRunning = false;
    }
    ~PatternEvaluator()
    {
        cleanUp();
    }

    void setConfig(const char* configStr)
    {
        // Store the config string
        _jsonConfigStr = configStr;
    }

    const char* getConfig()
    {
        return _jsonConfigStr.c_str();
    }

    void addExpression(const char* exprStr, bool isInitialValue)
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
            int varIdx = _patternVars.addAssignment(inExpr, outExpr);

            // Store the compiled expression, variable index and whether an initial value
            if (varIdx >= 0)
            {
                // Compile expression
                int err = 0;
                te_variable* vars = _patternVars.getVars();
                te_expr* compiledExpr = te_compile(outExpr.c_str(), vars, _patternVars.getNumVars(), &err);
                // Log.trace("PatternEval compile %s hex %02x%02x%02x%02x%02x%02x result %ld err %d", outExpr.c_str(),
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
                    Log.trace("PatternEval addLoop addedCompiledExpr (Count=%d)", _varIdxAndCompiledExprs.size());
                }
            }

            // Next expression - skip separator (in case of escaped chars need to skip two chars)
            if (*pExpr == '\\')
                ++pExpr;
            if (*pExpr)
                ++pExpr;
        }
    }

    void cleanUp()
    {
        _patternVars.cleanUp();
        for (unsigned int i = 0; i < _varIdxAndCompiledExprs.size(); i++)
            te_free(_varIdxAndCompiledExprs[i]._pCompExpr);
        _varIdxAndCompiledExprs.clear();
        _isRunning = false;
    }

    void evalExpressions(bool procInitialValues, bool procLoopValues)
    {
        Log.trace("PatternEval numExprs %d", _varIdxAndCompiledExprs.size());
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
			// Log.trace("EVAL %d: %s varIdx %d exprRslt %f isInitialValue=%d",
            //                     i, _patternVars.getVariableName(varIdx).c_str(), varIdx, val, isInitialValue);
		}
    }

    // Get XY coordinates of point
    // Return false if invalid
    bool getPoint(PointND &pt)
    {
        bool xValid = false;
        bool yValid = false;
		pt._pt[0] = _patternVars.getVal("x", xValid, true);
		pt._pt[1] = _patternVars.getVal("y", yValid, true);
        return xValid && yValid;
    }

    bool getStopVar(bool& stopVar)
    {
        bool stopValid = false;
        stopVar = _patternVars.getVal("stop", stopValid, true) != 0.0;
        return stopValid;
    }

    void start()
    {
        _isRunning = true;
        // Re-evaluate starting conditions
        evalExpressions(true, false);
    }

    void stop()
    {
        _isRunning = false;
    }

    void service(CommandInterpreter* pCommandInterpreter)
    {
        // Check running
        if (!_isRunning)
            return;

        // Check if the command interpreter can accept new stuff
        if (!pCommandInterpreter->canAcceptCommand())
            return;

        // Evaluate expressions
        evalExpressions(false, true);

        // Get next point and send to commandInterpreter
        PointND pt;
        bool isValid = getPoint(pt);
        if (!isValid)
        {
            Log.info("PatternEval stopped X and Y must be specified");
            _isRunning = false;
            return;
        }
        String cmdStr = String::format("G0 X%0.2f Y%0.2f", pt._pt[0], pt._pt[1]);
        Log.trace("PatternEval ->cmdInterp %s", cmdStr.c_str());
        pCommandInterpreter->process(cmdStr.c_str());

        // Check if we reached a limit
        bool stopReqd = 0;
        isValid = getStopVar(stopReqd);
        if (!isValid)
        {
            Log.info("PatternEval stopped STOP variable not specified");
            _isRunning = false;
            return;
        }
        if (stopReqd)
        {
            Log.info("PatternEval stopped STOP = TRUE");
            _isRunning = false;
            return;
        }
    }

    bool procCommand(const char* cmdStr)
    {
        // Find the pattern matching the command
        bool isValid = false;
        String patternJson = ConfigManager::getString(cmdStr, "{}", _jsonConfigStr, isValid);
        Log.trace("PatternEval::procCmd cmdStr %s seqStr %s", cmdStr, patternJson.c_str());
        if (isValid)
        {
            // Remove existing pattern
            cleanUp();

            // Get pattern details
            _curPattern = cmdStr;
            String setupExprs = ConfigManager::getString("setup", "", patternJson);
            String loopExprs = ConfigManager::getString("loop", "", patternJson);
            Log.trace("PatternEval patternName %s setup %s",
                            _curPattern.c_str(), setupExprs.c_str());
            Log.trace("PatternEval patternName %s loop %s",
                            _curPattern.c_str(), loopExprs.c_str());

            // Add to the pattern evaluator expressions
            addExpression(setupExprs.c_str(), true);
            addExpression(loopExprs.c_str(), false);

            // Start the pattern evaluation process
            start();

        }
        return isValid;
    }

private:
    // Full configuration JSON
    String _jsonConfigStr;

    // All variables used in the pattern
    PatternEvaluator_Vars _patternVars;

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
