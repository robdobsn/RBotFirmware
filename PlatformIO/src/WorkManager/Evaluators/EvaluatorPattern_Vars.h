// RBotFirmware
// Rob Dobson 2017

#pragma once

#include <Arduino.h>
#include "tinyexpr.h"

class EvaluatorPattern_Vars
{
public:
    EvaluatorPattern_Vars()
    {
        _pTeVars = NULL;
        _pTeVarFlags = NULL;
        _numTeVars = 0;
    }
	~EvaluatorPattern_Vars();

    void splitAssignmentExpr(const char* inStr, String& varName, String& expr);
    int getVariableIdx(const char* name, bool caseInsensitive = false);
    int getVariableFlags(int varIdx);
    String getVariableName(int varIdx);
    int addAssignment(const char* inStr, String& outExpr);
    int addConstant(const char* name, double val);
    te_variable* getVars();
	int getNumVars();
	double getVal(const char* varName, bool& isValid, bool caseInsensitive = false);
	void setVal(char* varName, double val, bool caseInsensitive = false);
	void setValByIdx(int varIdx, double val);
	void cleanUp();
    
private:
    int addVariablePtr(const char* name, const double* pVal, unsigned int flags);
    te_variable* _pTeVars;
    unsigned int* _pTeVarFlags;
    int _numTeVars;
    static constexpr int TEVARS_FREE_VALUE_ADDR_REQD = 1;
};
