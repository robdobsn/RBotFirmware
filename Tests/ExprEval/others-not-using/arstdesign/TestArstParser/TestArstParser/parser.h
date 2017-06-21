
#pragma once


// parser //////////////////////////////////////




class wzparser
{
	// Members
protected:
    wzelementblock m_topLevel;
	long m_nLastError;
	char m_cArgumentSeparator;

	// Construction
public:
	wzparser();
	virtual ~wzparser();

	// Methods

	// compiles a formula into byte-code
	BOOL parse(LPSTR szFormula);

	// eval a parsed formula (values being passed replace the variable placeholders)
	BOOL eval(wzarray<wzvariable*> &arrVariables, wzstring &result);


	// Accessors

	void setArgumentSeparator(TCHAR c); // used between function arguments. Default is ';'
	TCHAR getArgumentSeparator();

	long getSize();
	wzelement* get(long i); // ith element

	long getLastError(); // valid if parse() returns FALSE
	void dump(); // output content in the debug console

protected:
    void dump(wzelementblock* block, int level);
    BOOL createChildBlock(wzelementblockptr& curlevel);

	void free();
	long add(wzelement* );

	BOOL isOperator(/*in*/LPSTR* pp, /*out*/wzoperator** ppop);
	BOOL isStringLitteral(/*in*/LPSTR pin, /*in*/LPSTR pout);

	BOOL evalOperator(long &i, wzstring &result);

    BOOL preEval(wzarray<wzvariable*> &arrVariables);
    BOOL doeval(wzelementblockptr curlevel, wzarray<wzvariable*> &arrVariables, wzstring &result);
    BOOL postEval(wzarray<wzvariable*> &arrVariables);

    BOOL replaceVariables(wzelementblockptr curlevel, wzarray<wzvariable*> &arrVariables);
};
