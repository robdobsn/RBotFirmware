// RBotFirmware
// Rob Dobson 2017

#include "EvaluatorPattern_Vars.h"
#include "ArduinoLog.h"

//#define DEBUG_EVALUATOR_EXPRESSIONS 1

static const char* MODULE_PREFIX = "EvaluatorPattern_Vars: ";

EvaluatorPattern_Vars::~EvaluatorPattern_Vars()
{
    cleanUp();
}

void EvaluatorPattern_Vars::cleanUp()
{
    // Clean up variable names and values
    for (int i = 0; i < _numTeVars; i++)
    {
        delete[] _pTeVars[i].name;
        if (_pTeVarFlags[i] & TEVARS_FREE_VALUE_ADDR_REQD)
            delete ((double*)_pTeVars[i].address);
    }
    // Clean array of variables
    delete[] _pTeVars;
    _pTeVars = NULL;
    delete[] _pTeVarFlags;
    _pTeVarFlags = NULL;
    _numTeVars = 0;
}

void EvaluatorPattern_Vars::splitAssignmentExpr(const char* inStr, String& varName, String& expr)
{
    varName = "";
    expr = "";
    // Split string on equals sign
    const char* eqPos = strchr(inStr, '=');
    if (!eqPos)
        return;
    varName = inStr;
    varName.remove(eqPos-inStr);
    varName.trim();
    expr = eqPos+1;
}

int EvaluatorPattern_Vars::getVariableIdx(const char* name, bool caseInsensitive)
{
    for (int i = 0; i < _numTeVars; i++)
    {
        if ((caseInsensitive && strcasecmp(name, _pTeVars[i].name) == 0) ||
            (!caseInsensitive && strcmp(name, _pTeVars[i].name) == 0))
        {
            return i;
        }
    }
    return -1;
}

int EvaluatorPattern_Vars::getVariableFlags(int varIdx)
{
    if ((varIdx >= 0) && (varIdx < _numTeVars))
        return _pTeVarFlags[varIdx];
    return 0;
}

String EvaluatorPattern_Vars::getVariableName(int varIdx)
{
    if ((varIdx < 0) || (varIdx >= _numTeVars))
        return "";
    return _pTeVars[varIdx].name;
}

int EvaluatorPattern_Vars::addVariablePtr(const char* name, const double* pVal, unsigned int flags)
{
    // Check empty
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
        Log.error("%sfailed to add variable\n", MODULE_PREFIX);
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
#ifdef DEBUG_EVALUATOR_EXPRESSIONS
    Log.trace("%sNumVars = %d\n", MODULE_PREFIX, _numTeVars);
    for (int i = 0; i < _numTeVars; i++)
    {
    	Log.trace("%sVar %d %s = %F flags %x context %d\n", MODULE_PREFIX, i, _pTeVars[i].name,
                *((double*)(_pTeVars[i].address)), _pTeVarFlags[i], _pTeVars[i].context);
    }
#endif
    return newVarIdx;
}

int EvaluatorPattern_Vars::addConstant(const char* name, double val)
{
    // Check if already exists
    int varIdx = getVariableIdx(name);
    if (varIdx >= 0)
        return varIdx;

    // Create storage (will be freed on cleanup)
    double* pVal = new double;
    *pVal = val;
    unsigned int flags = TEVARS_FREE_VALUE_ADDR_REQD;
    varIdx = addVariablePtr(name, pVal, flags);
#ifdef DEBUG_EVALUATOR_EXPRESSIONS
    Log.trace("%sAddConst name %s val %F varIdx %d flags 0x%x numVars %d\n", MODULE_PREFIX,
            name, val, varIdx, flags, _numTeVars);   
#endif
    return varIdx;
}

int EvaluatorPattern_Vars::addAssignment(const char* inStr, String& outExpr)
{
    String varName;
    splitAssignmentExpr(inStr, varName, outExpr);
    if (varName.length() > 0)
    {
        // Check if it already exists
        int varIdx = getVariableIdx(varName.c_str());
        if (varIdx >= 0)
        {
            *((double*)(_pTeVars[varIdx].address)) = 0;
        }
        else
        {
            // Create the value for this variable
            double* pVal = new double;
            *pVal = 0;
            // Create the variable using this value
            unsigned int flags = TEVARS_FREE_VALUE_ADDR_REQD;
            varIdx = addVariablePtr(varName.c_str(), pVal, flags);
#ifdef DEBUG_EVALUATOR_EXPRESSIONS
            Log.trace("%sAddAssign var %s, expr %s, varIdx %d, flags 0x%x, numVars %d\n", MODULE_PREFIX,
                varName.c_str(), outExpr.c_str(), varIdx, flags, _numTeVars);
#endif
        }

        return varIdx;
    }
    return -1;
}

te_variable* EvaluatorPattern_Vars::getVars()
{
    return _pTeVars;
}

int EvaluatorPattern_Vars::getNumVars()
{
    return _numTeVars;
}

double EvaluatorPattern_Vars::getVal(const char* varName, bool& isValid, bool caseInsensitive)
{
    isValid = true;
    int varIdx = getVariableIdx(varName, caseInsensitive);
    if (varIdx == -1)
    {
        isValid = false;
        return 0;
    }
    return *((double*)(_pTeVars[varIdx].address));
}

void EvaluatorPattern_Vars::setVal(char* varName, double val, bool caseInsensitive)
{
    int varIdx = getVariableIdx(varName, caseInsensitive);
    if (varIdx == -1)
        return;
    *((double*)(_pTeVars[varIdx].address)) = val;
}

void EvaluatorPattern_Vars::setValByIdx(int varIdx, double val)
{
    *((double*)(_pTeVars[varIdx].address)) = val;
}
