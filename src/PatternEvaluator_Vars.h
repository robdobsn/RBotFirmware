// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "application.h"
#include "tinyexpr.h"

class PatternEvaluator_Vars
{
public:
    PatternEvaluator_Vars()
    {
        _pTeVars = NULL;
        _pTeVarFlags = NULL;
        _numTeVars = 0;
    }
	void cleanUp()
	{
		// Clean up variable names and values
		for (int i = 0; i < _numTeVars; i++)
		{
			delete[] _pTeVars[i].name;
			if (_pTeVarFlags[i] & TEVARS_FREE_VALUE_ADDR_REQD)
				delete _pTeVars[i].address;
		}
		// Clean array of variables
		delete[] _pTeVars;
		_pTeVars = NULL;
        delete[] _pTeVarFlags;
        _pTeVarFlags = NULL;
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
            if ((caseInsensitive && stricmp(name, _pTeVars[i].name) == 0) ||
				(!caseInsensitive && strcmp(name, _pTeVars[i].name) == 0))
            {
                return i;
            }
        }
        return -1;
    }

    int getVariableFlags(int varIdx)
    {
        if ((varIdx >= 0) && (varIdx < _numTeVars))
            return _pTeVarFlags[varIdx];
        return 0;
    }

    String getVariableName(int varIdx)
    {
        if ((varIdx >= 0) && (varIdx < _numTeVars))
            return "";
        return _pTeVars[varIdx].name;
    }

    int addVariable(const char* name, const double* pVal, unsigned int flags)
    {
        // Check empty
        te_variable* new_pTeVars = NULL;
        unsigned int* new_pTeVarFlags = NULL;
        int newVarIdx = 0;
        if (_numTeVars == 0)
        {
			// Add first variable to array
            _pTeVars = new te_variable[1];
            _pTeVarFlags = new unsigned int[1];
            _numTeVars = 1;
            newVarIdx = 0;
        }
        else
        {
            // Copy existing
            te_variable* new_pTeVars = new te_variable[_numTeVars+1];
            unsigned int* new_pTeVarFlags = new unsigned int[_numTeVars+1];
            for (int i = 0; i < _numTeVars; i++)
            {
                new_pTeVars[i] = _pTeVars[i];
                new_pTeVarFlags[i] = _pTeVarFlags[i];
            }
            delete[] _pTeVars;
            delete[] _pTeVarFlags;
            _pTeVars = new_pTeVars;
            _pTeVarFlags = new_pTeVarFlags;
            newVarIdx = _numTeVars;
            _numTeVars++;
        }
        // Check added ok
        if (!_pTeVars)
        {
            _numTeVars = 0;
            Log.error("PatternEvaluator failed to add variable");
            return -1;
        }
        // Add the variable name
        char* newName = new char[strlen(name)+1];
        strcpy(newName, name);
        _pTeVars[newVarIdx].name = newName;
        // Add the value reference
        _pTeVars[newVarIdx].address = pVal;
		_pTeVars[newVarIdx].type = 0;
		_pTeVars[newVarIdx].context = NULL;
        _pTeVarFlags[newVarIdx] = flags;

		// Show vars
		Log.trace("NumVars = %d", _numTeVars);
		for (int i = 0; i < _numTeVars; i++)
		{
			Log.trace("Var %d %s = %f flags %04x context %ld", i, _pTeVars[i].name,
                    *((double*)(_pTeVars[i].address)), _pTeVarFlags[i], _pTeVars[i].context);
		}
        return newVarIdx;
    }

    int addAssignment(const char* inStr, String& expr)
    {
        String varName;
        splitAssignmentExpr(inStr, varName, expr);
        if (varName.length() > 0)
        {
            // Check if it already exists
            int varIdx = getVariableIdx(varName);
            if (varIdx >= 0)
            {
                *((double*)(_pTeVars[varIdx].address)) = 0;
            }
			else
			{
				// Create the value for this variable
				double* pVal = new double;
				*pVal = 0;
				int err = 0;
				// Create the variable using this value
                unsigned int flags = TEVARS_FREE_VALUE_ADDR_REQD;
				varIdx = addVariable(varName.c_str(), pVal, flags);
				Log.trace("AddVar %s, expr %s, val %f, flags %02x, err %d, numVars %d",
					varName.c_str(), expr.c_str(), *pVal, flags, err, _numTeVars);
			}

            return varIdx;
        }
        return -1;
    }

    te_variable* getVars()
    {
        return _pTeVars;
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
		return *((double*)(_pTeVars[varIdx].address));
	}

	void setVal(char* varName, double val, bool caseInsensitive = false)
	{
		int varIdx = getVariableIdx(varName, caseInsensitive);
		if (varIdx == -1)
			return;
		*((double*)(_pTeVars[varIdx].address)) = val;
	}

	void setValByIdx(int varIdx, double val)
	{
		*((double*)(_pTeVars[varIdx].address)) = val;
	}

private:
    te_variable* _pTeVars;
    unsigned int* _pTeVarFlags;
    int _numTeVars;
    static constexpr int TEVARS_FREE_VALUE_ADDR_REQD = 1;
};
