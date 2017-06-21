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
	~PatternEvaluator_Vars()
    {
        cleanUp();
    }
    void splitAssignmentExpr(const char* inStr, String& varName, String& expr)
    {
        varName = "";
        expr = "";
		// Split string on equals sign
        const char* eqPos = strchr(inStr, '=');
        if (!eqPos)
            return;
        varName = inStr;
        varName.remove(eqPos-inStr);
        varName = varName.trim();
        expr = eqPos+1;
    }

    int getVariableIdx(const char* name, bool caseInsensitive = false)
    {
        for (int i = 0; i < _numTeVars; i++)
        {
            if ((caseInsensitive && _stricmp(name, _teVars[i].name) == 0) || 
				(!caseInsensitive && strcmp(name, _teVars[i].name) == 0))
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
			// Add first variable to array
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
            return -1;
        }
        // Add the variable name
        char* newName = new char[strlen(name)+1];
        strcpy(newName, name);
        _teVars[newVarIdx].name = newName;
        // Add the value reference
        _teVars[newVarIdx].address = pVal;
		_teVars[newVarIdx].type = 0;
		_teVars[newVarIdx].context = NULL;

		// Show vars
		printf("NumVars = %d\r\n", _numTeVars);
		for (int i = 0; i < _numTeVars; i++)
		{
			printf("Var %d %s = %f type %d context %ld\r\n", i, _teVars[i].name, *((double*)(_teVars[i].address)), _teVars[i].type, _teVars[i].context);
		}
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
                    *((double*)(_teVars[varIdx].address)) = 0;
                else
                    *((double*)(_teVars[varIdx].address)) = te_interp(expr, &err);
            }
			else
			{
				// Create the value for this variable
				double* pVal = new double;
				*pVal = 0;
				int err = 0;
				if (!zeroInitialValue)
					*pVal = te_interp(expr, &err);
				// Create the variable using this value
				varIdx = addVariable(varName.c_str(), pVal);
				// Flag that the variable address needs to be freed
				if (_numTeVars > 0)
					_teVars[_numTeVars - 1].type |= TEVARS_FREE_VALUE_ADDR_REQD;
				Serial.printlnf("AddVar %s, expr %s, val %f, err %d, numVars %d",
					varName.c_str(), expr.c_str(), *pVal, err, _numTeVars);
			}

            return varIdx;
        }
        return -1;
    }

    te_variable* getVars()
    {
        return _teVars;
    }

	int getNumVars()
	{
		return _numTeVars;
	}

	double getVal(char* varName, bool caseInsensitive = false)
	{
		int varIdx = getVariableIdx(varName, caseInsensitive);
		if (varIdx == -1)
			return 0;
		return *((double*)(_teVars[varIdx].address));
	}

	void setVal(char* varName, double val, bool caseInsensitive = false)
	{
		int varIdx = getVariableIdx(varName, caseInsensitive);
		if (varIdx == -1)
			return;
		*((double*)(_teVars[varIdx].address)) = val;
	}

	void setValByIdx(int varIdx, double val)
	{
		*((double*)(_teVars[varIdx].address)) = val;
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
			const char* pExprStart = pExpr;
			String inExpr = pExpr;

            // Find the beginning of the next assignment
            while(*pExpr && (*pExpr != ';') && (*pExpr != '\n'))
                ++pExpr;
			int exprLen = pExpr - pExprStart;
			inExpr = inExpr.substring(0, exprLen);

			// Add the assignment
			_patternVars.addAssignment(inExpr.c_str(), outExpr);
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
			const char* pExprStart = pExpr;
			String inExpr = pExpr;
			// Find the beginning of the next assignment
			while (*pExpr && (*pExpr != ';') && (*pExpr != '\n'))
				++pExpr;
			int exprLen = pExpr - pExprStart;
			inExpr = inExpr.substring(0, exprLen);
			int varIdx = _patternVars.addAssignment(inExpr, outExpr, true);

            // Store the compiled expression and variable index
            if (varIdx >= 0)
            {
                // Compile expression
                int err = 0;
                te_variable* vars = _patternVars.getVars();
                te_expr* compiledExpr = te_compile(outExpr.c_str(), vars, _patternVars.getNumVars(), &err);
				printf("Compile %s result %ld err %d\r\n", outExpr.c_str(), compiledExpr, err);

                // Store the expression and assigned variable index
                if (compiledExpr)
                {
                    VarIdxAndCompiledExpr varIdxAndCompExpr;
                    varIdxAndCompExpr._pCompExpr = compiledExpr;
                    varIdxAndCompExpr._varIdx = varIdx;
                    _varIdxAndCompiledExprs.push_back(varIdxAndCompExpr);
					printf("addLoop addedCompiledExpr (Count=%d)\r\n", _varIdxAndCompiledExprs.size());
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
    }

    void procLoop(double &x, double &y)
    {
		printf("NumCompiledExpr %d\r\n", _varIdxAndCompiledExprs.size());
		// Go through all the expressions and evaluate
		for (unsigned int i = 0; i < _varIdxAndCompiledExprs.size(); i++)
		{
			int varIdx = _varIdxAndCompiledExprs[i]._varIdx;
			double val = te_eval(_varIdxAndCompiledExprs[i]._pCompExpr);
			_patternVars.setValByIdx(varIdx, val);
			printf("EVAL %d: varIdx %d exprRslt %f a= %f\r\n", i, varIdx, val, _patternVars.getVal("a"));
		}

		x = _patternVars.getVal("x", true);
		y = _patternVars.getVal("y", true);

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
