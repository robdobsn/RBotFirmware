#pragma once

#include "application.h"
#include "tinyexpr.h"
#include <vector>

class PatternEvaluator_Vars
{
public:
    PatternEvaluator_Vars()
    {
        _teVars = NULL;
        _numTeVars = 0;
    }
    ~PatternEvaluator_Vars()
    {
        cleanUp();
    }
    void splitAssignmentExpr(const char* inStr, String& varName, String& expr)
    {
        varName = "";
        expr = "";
        char* eqPos = strchr(inStr, '=');
        if (!eqPos)
            return;
        varName = inStr;
        varName.remove(eqPos-inStr);
        varName = varName.trim();
        expr = eqPos+1;
    }

    int getVariableIdx(const char* name)
    {
        for (int i = 0; i < _numTeVars; i++)
        {
            if (strcmp(name, _teVars[i].name) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    int addVariable(const char* name, const double* pVal)
    {
        // Check empty
        te_variable* new_teVars = NULL;
        int newVarIdx = 0;
        if (_numTeVars == 0)
        {
            _teVars = new te_variable[1];
            _numTeVars = 1;
            newVarIdx = 0;
        }
        else
        {
            // Copy existing
            te_variable* new_teVars = new te_variable[_numTeVars+1];
            for (int i = 0; i < _numTeVars; i++)
                new_teVars[i] = _teVars[i];
            delete[] _teVars;
            _teVars = new_teVars;
            newVarIdx = _numTeVars;
            _numTeVars++;
        }
        // Check added ok
        if (!_teVars)
        {
            _numTeVars = 0;
            return;
        }
        // Add the variable name
        char* newName = new char[strlen(name)+1];
        strcpy(newName, name);
        _teVars[newVarIdx].name = newName;
        // Add the value reference
        _teVars[newVarIdx].address = pVal;
        return newVarIdx;
    }

    int addAssignment(const char* inStr, String& expr, bool zeroInitialValue = false)
    {
        String varName;
        splitAssignmentExpr(inStr, varName, expr);
        if (varName.length() > 0)
        {
            // Check if it already exists
            int varIdx = getVariableIdx(varName);
            if (varIdx >= 0)
            {
                int err;
                if (zeroInitialValue)
                    *_teVars[varIdx].address = 0;
                else
                    *_teVars[varIdx].address = te_interp(expr, &err);
            }
            else
            {
                // Create the value for this variable
                double* pVal = new double;
                *pVal = 0;
                int err;
                if (!zeroInitialValue)
                    *pVal = te_interp(expr, &err);
                // Create the variable using this value
                varIdx = addVariable(varName.c_str(), pVal);
                // Flag that the variable address needs to be freed
                if (_numTeVars > 0)
                    _teVars[_numTeVars-1].type |= TEVARS_FREE_VALUE_ADDR_REQD;
                Serial.printlnf("AddVar %s, expr %s, val %f, err %d, numVars %d",
                        varName.c_str(), expr.c_str(), *pVal, err, _numTeVars);

            return varIdx;
        }
        return -1;
    }

    void cleanUp()
    {
        // Clean up names and constants
        for (int i = 0; i < _numTeVars; i++)
        {
            delete[] _teVars[i].name;
            if (_teVars[i].type & TEVARS_FREE_VALUE_ADDR_REQD)
                delete _teVars[i].address;
        }
        // Clean array of variables
        delete[] _teVars;
        _teVars = NULL;
        _numTeVars = 0;
    }
    te_variable* getVars()
    {
        return _teVars;
    }

private:
    te_variable* _teVars;
    int _numTeVars;
    const int TEVARS_FREE_VALUE_ADDR_REQD = 16384;
};

class PatternEvaluator
{
public:
    PatternEvaluator()
    {
    }
    ~PatternEvaluator()
    {
    }
    void addSetup(const char* exprStr)
    {
        const char* pExpr = exprStr;

        // Skip initial space
        while (isspace(*pExpr))
            ++pExpr;

        // Iterate through the assignments
        while (*pExpr)
        {
            // Add the constant
            String outExpr;
            _patternVars.addAssignment(pExpr, outExpr);

            // Find the beginning of the next assignment
            while(*pExpr && (*pExpr != ';') && (*pExpr != '\n'))
                ++pExpr;
            if (*pExpr)
                ++pExpr;
        }
    }

    void addLoop(const char* exprStr)
    {
        const char* pExpr = exprStr;

        // Skip initial space
        while (isspace(*pExpr))
            ++pExpr;

        // Iterate through the assignments
        while (*pExpr)
        {
            // Add the variable name and set value to 0
            String outExpr;
            int varIdx = _patternVars.addAssignment(pExpr, outExpr, true);

            // Store the compiled expression and variable index
            if (varIdx >= 0)
            {
                // Compile expression
                int err = 0;
                te_variable* vars = _patternVars.getVars();
                te_expr* compiledExpr = te_compile(outExpr.c_str(), vars, 2, &err);

                // Store the expression and assigned variable index
                if (compiledExpr)
                {
                    VarIdxAndCompiledExpr varIdxAndCompExpr;
                    varIdxAndCompExpr._pCompExpr = compiledExpr;
                    varIdxAndCompExpr._varIdx = varIdx;
                    _varIdxAndCompiledExprs.push_back(varIdxAndCompExpr);
                }
            }

            // Find the beginning of the next assignment
            while(*pExpr && (*pExpr != ';') && (*pExpr != '\n'))
                ++pExpr;
            if (*pExpr)
                ++pExpr;
        }


    }

    void cleanUp()
    {
        _patternVars.cleanUp();
        for (int i = 0; i < _varIdxAndCompiledExprs.size(); i++)
            te_free(_varIdxAndCompiledExprs[i]._pCompExpr);
        _varIdxAndCompiledExprs.clear();
    }

    void procLoop(double &x, double &y)
    {
        // if (_compiledExpr)
        // {
        //     double r = te_eval(_compiledExpr);
        // }
    }

private:
    typedef struct VarIdxAndCompiledExpr
    {
        te_expr* _pCompExpr;
        int _varIdx;
    } VarIdxAndCompiledExpr;
    PatternEvaluator_Vars _patternVars;
    std::vector<VarIdxAndCompiledExpr> _varIdxAndCompiledExprs;
};
