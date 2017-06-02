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
        // Get pattern details
        String patternName = ConfigManager::getString("patternName", "NONE", configStr);
        String setupExprs = ConfigManager::getString("setupExprs", "", configStr);
        String loopExprs = ConfigManager::getString("loopExprs", "", configStr);
        Log.trace("PatternEvaluator patternName %s setup %s loop %s",
                        patternName.c_str(), setupExprs.c_str(), loopExprs.c_str());

        // Add to the pattern evaluator expressions
        addExpression(setupExprs.c_str(), true);
        addExpression(loopExprs.c_str(), false);
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
            // Find the beginning of the next assignment
            while ((*pExpr != '\0') && (*pExpr != ';') && (*pExpr != '\n'))
                ++pExpr;
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
                Log.trace("Compile %s result %ld err %d", outExpr.c_str(), compiledExpr, err);

                // Store the expression and assigned variable index
                if (compiledExpr)
                {
                    VarIdxAndCompiledExpr varIdxAndCompExpr;
                    varIdxAndCompExpr._pCompExpr = compiledExpr;
                    varIdxAndCompExpr._varIdx = varIdx;
                    varIdxAndCompExpr._isInitialValue = isInitialValue;
                    _varIdxAndCompiledExprs.push_back(varIdxAndCompExpr);
                    Log.trace("addLoop addedCompiledExpr (Count=%d)", _varIdxAndCompiledExprs.size());
                }
            }

            // Next expression
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
        Log.trace("NumCompiledExpr %d", _varIdxAndCompiledExprs.size());
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
			Log.trace("EVAL %d: %s varIdx %d exprRslt %f isInitialValue=%d",
                                i, _patternVars.getVariableName(varIdx).c_str(), varIdx, val, isInitialValue);
		}
    }

    void getPoint(PointND &pt)
    {
		pt._pt[0] = _patternVars.getVal("x", true);
		pt._pt[1] = _patternVars.getVal("y", true);
    }

    bool getStopVar()
    {
        return _patternVars.getVal("stop", true) != 0.0;
    }

    void start()
    {
        _isRunning = true;
        // Re-evaluate starting conditions
        evalExpressions(true, false);
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
        getPoint(pt);
        String cmdStr = String::format("G0 X%0.2f Y%0.2f", pt._pt[0], pt._pt[1]);
        pCommandInterpreter->process(cmdStr.c_str());
        Log.trace("CommandInterpreter process %s", cmdStr.c_str());

        // Check if we reached a limit
        if (getStopVar())
        {
            _isRunning = false;
        }
    }

private:
    typedef struct VarIdxAndCompiledExpr
    {
        te_expr* _pCompExpr;
        int _varIdx;
        bool _isInitialValue;
    } VarIdxAndCompiledExpr;
    PatternEvaluator_Vars _patternVars;
    std::vector<VarIdxAndCompiledExpr> _varIdxAndCompiledExprs;
    bool _isRunning;
};
