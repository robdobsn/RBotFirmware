
#include <stdio.h>
#include <math.h> // for sin and cos
#include <windows.h>

#include "util.h"
#include "parser.h"


///////////////////////////////////////////////////////////////////


wzparser::wzparser()
{
	setArgumentSeparator(';'); // default argument separator, eg. max(x;y)
}
wzparser::~wzparser()
{
}

// Methods

void wzparser::free()
{
}


void wzparser::setArgumentSeparator(char c) // used between function arguments. Default is ';'
{
	m_cArgumentSeparator = c;
}

TCHAR wzparser::getArgumentSeparator()
{
	return m_cArgumentSeparator;
}


long wzparser::getSize()
{
	return m_topLevel.m_arrelements.GetSize();
}

long wzparser::add(wzelement* p)
{
	m_topLevel.m_arrelements.Add( p );
	return getSize() - 1;
}

wzelement* wzparser::get(long i)
{
	return m_topLevel.m_arrelements.GetAt( i );
}


BOOL wzparser::parse(LPSTR szFormula)
{
	free(); // allow re-entrance

	//
	LPSTR p = szFormula;
	m_nLastError = 0;

	if (!p)
	{
		m_nLastError = PARSERERROR_SYNTAXERROR; // empty formula
		return FALSE;
	}

	// pass the possible left-hand equal sign and spaces
	while ( *p && (*p <= 32 || *p == '=') )
		p++;

	LPSTR q = p;

    wzelementblock* curlevel = &m_topLevel;


	//
	// main iteration
	//
	while ( *p )
	{

		if ( isStringLitteral(q, p) )
		{
			p++; // no need to wonk about any further
			continue;
		}

		// is is an operator?
		wzoperator* op = NULL;
		LPSTR lastp = p;
		if ( isOperator(&p, &op) )
		{
			// do we have any litteral to store before that?
			if (lastp > q)
			{
				wzstring* pstring = new wzstring();
				if (!pstring)
                {
		            m_nLastError = PARSERERROR_OUTOFMEMORY;
		            return FALSE;
                }

				pstring->setType( _litteral );
				pstring->setOrigString(q /*pointer to litteral*/, lastp - q /*length*/);
				curlevel->m_arrelements.Add( pstring );
			}


            // ( ==> create a functional block and another child block ; proceed processing from there
            // ; ==> create a new child block ; proceed parameter processing from there
            // ) ==> leave the functional block

            if (op->getID() == PARENTHESISID)
            {
                curlevel->m_arrelements.Add( op );

                wzelementblock* child = curlevel->createChildBlock();
                if (!child)
                {
		            m_nLastError = PARSERERROR_OUTOFMEMORY;
		            return FALSE;
                }

                op->setElementBlock(child);

                // move to child level
                curlevel = child;

                if (!createChildBlock(curlevel))
                    return FALSE;

            }
            else if (op->getID() == PARENTHESISID+1)
            {
	            // popup all remaining operators
	            while ( !curlevel->m_arroperators.IsEmpty() )
		            curlevel->m_arrelements.Add( curlevel->m_arroperators.Pop() );

                // move one level up
                curlevel = curlevel->m_cpParent;
                if (!curlevel)
                {
                    m_nLastError = PARSERERROR_IMPAIREDPARENTHESIS;
                    return FALSE;
                }

                // we have reached the intermediate block, the block where parameters are listed


	            // popup all remaining operators
	            while ( !curlevel->m_arroperators.IsEmpty() )
		            curlevel->m_arrelements.Add( curlevel->m_arroperators.Pop() );

                // move one level up
                curlevel = curlevel->m_cpParent;
                if (!curlevel)
                {
                    m_nLastError = PARSERERROR_IMPAIREDPARENTHESIS;
                    return FALSE;
                }

            }
            else
            {
			    // do we have to pop any operator from the stack?
			    wzoperator* existingop = curlevel->m_arroperators.GetTop();
			    if (!existingop)
			    {
                    if (op->getID() != PARENTHESISID+1)
				        curlevel->m_arroperators.Push( op ); // stack is empty, so fill it
			    }
			    else
			    {
				    // priority is lower, or is it an open parenthesis?
				    if ( op->isHigherPriorityThan(existingop) || existingop->getID() == PARENTHESISID )
				    {
					    // just add the new operator on top
                        if (op->getID() != PARENTHESISID+1)
					        curlevel->m_arroperators.Push( op );
				    }
				    else
				    {
					    existingop = curlevel->m_arroperators.Pop();
	    
					    curlevel->m_arrelements.Add( existingop );

                        if (op->getID() != PARENTHESISID+1)
					        curlevel->m_arroperators.Push( op );
				    }
			    } // end if (existingop)

            }

			// pass spaces
			while (*p && *p <= 32)
				p++;

			q = p;
		}
        else if ( *(p - 1) == getArgumentSeparator() )
		{

			// do we have any litteral to store before that?
			if (lastp > q)
			{
				wzstring* pstring = new wzstring();
				if (!pstring)
                {
		            m_nLastError = PARSERERROR_OUTOFMEMORY;
		            return FALSE;
                }

				pstring->setType( _litteral );
				pstring->setOrigString(q, //pointer to litteral
                                   lastp - q //length
                                   );
				curlevel->m_arrelements.Add( pstring );
			}


	        // popup all remaining operators
	        while ( !curlevel->m_arroperators.IsEmpty() )
		        curlevel->m_arrelements.Add( curlevel->m_arroperators.Pop() );


            // move one level up
            curlevel = curlevel->m_cpParent;
            if (!curlevel)
            {
                m_nLastError = PARSERERROR_IMPAIREDPARENTHESIS;
                return FALSE;
            }

            if (!createChildBlock(curlevel))
                return FALSE;

			// pass spaces
			while (*p && *p <= 32)
				p++;

			q = p;

		}

	} // end of main iteration loop

	// do we have a remaining litteral to store?
	if (p > q)
	{
		wzstring* pstring = new wzstring();
		if (!pstring)
        {
		    m_nLastError = PARSERERROR_OUTOFMEMORY;
		    return FALSE;
        }

		pstring->setType( _litteral );
		pstring->setOrigString(q /*pointer to litteral*/, p - q /*length*/);
		curlevel->m_arrelements.Add( pstring );
	}

	// popup all remaining operators
	while ( !curlevel->m_arroperators.IsEmpty() )
		curlevel->m_arrelements.Add( curlevel->m_arroperators.Pop() );

	return TRUE;
}

BOOL wzparser::createChildBlock(wzelementblockptr& curlevel)
{
    // create new block, move to new block
    wzoperator* childop = new wzoperator();
    if (!childop)
    {
		m_nLastError = PARSERERROR_OUTOFMEMORY;
		return FALSE;
    }

	childop->setType( _operator );
	childop->setID( PARENTHESISID );
	childop->setPriority( 100 );
	childop->setNbParamsMin( 1 );
    childop->setNbParamsMax( 1 );
	childop->setIsAFunction( FALSE );

    curlevel->m_arrelements.Add( childop );

    wzelementblock* childchild = curlevel->createChildBlock();
    if (!childchild)
    {
		m_nLastError = PARSERERROR_OUTOFMEMORY;
		return FALSE;
    }

    childop->setElementBlock(childchild);

    // move to child level
    curlevel = childchild;

    return TRUE;
}



_structoperator operators[] = {
	{ "+"/*label*/, PARSERFUNC_BINARYPLUS/*id*/, 10/*priority*/, 2/*nbparmin*/, 2/*nbparmax*/,FALSE/*is a function*/ },
	{ "-",  PARSERFUNC_BINARYMINUS,              10,             2,             2,            FALSE },
	{ "*",  PARSERFUNC_MULT,                     20,             2,             2,            FALSE },
	{ "/",  PARSERFUNC_DIV,                      20,             2,             2,            FALSE },
	{ "(",  PARENTHESISID,                      100,            1,             1,            FALSE },
	{ ")",  PARENTHESISID+1,                    100,            1,             1,            FALSE },
	{ "SIN",PARSERFUNC_SIN,                      30,             1,             1,            TRUE},
	{ "COS",PARSERFUNC_COS,                      30,             1,             1,            TRUE},
    { "SUM",PARSERFUNC_SUM,                      30,             1,            -1,            TRUE},
	{ NULL,              0,                       0,             0,              0,              0}
};



BOOL wzparser::isOperator(/*in-out*/LPSTR *pp, /*out*/wzoperator** ppop)
{
	if (!pp) return FALSE;

	long icurop = 0;
	LPSTR curop = operators[icurop]._operator;

	//
	// loop all operators
	//
	while ( curop )
	{
		LPSTR p = *pp;
		while (*p && *curop) // current char matches?
		{
             char c1 = *p;
             char c2 = *curop;
             if (c1 >= 'a' && c1 <= 'z')
                 c1 -= 'a' - 'A';
             if (c2 >= 'a' && c2 <= 'z')
                 c2 -= 'a' - 'A';

             if (c1 != c2)
                 break;

			 p++;
			 curop++;
		}

		if (*curop == 0) // yes
		{
            // verify it's really a function

            bool bOpenParenthesisFunctionVerified = true;

            if (operators[icurop]._id >= 0 && operators[icurop]._id <= 0x0200)
            {
                LPSTR q = p;
                while (*q && *q <= ' ')
                    q++;

                if (*q == 0 || *q != '(')
                    bOpenParenthesisFunctionVerified = false;
            }

            if (bOpenParenthesisFunctionVerified)
            {
                // it is one
			    wzoperator* o = new wzoperator();
			    if (o)
			    {
				    o->setType( _operator );
				    o->setID( operators[icurop]._id );
				    o->setPriority( operators[icurop]._priority );
				    o->setNbParamsMin( operators[icurop]._nbparamsmin );
                    o->setNbParamsMax( operators[icurop]._nbparamsmax );
				    o->setIsAFunction( operators[icurop]._isfunction );
			    }

			    *ppop = o;

			    // move the cursor forward (move according to operator length)
			    *pp = p;

			    return TRUE;

            } // end if (bOpenParenthesisFunctionVerified)
		}

		curop = operators[++icurop]._operator;

	} // for all operators

	// move the cursor forward (1 char)
	LPSTR p = *pp;
	*pp = ++p;

	return FALSE;
}

// isStringLitteral
//
// returns TRUE if the string bound by the two pointers has exactly one quote char inside
// i.e. if we are inside a string litteral
//
// to escape a quote within a string litteral, just do like with plain C-code : \"
BOOL wzparser::isStringLitteral(/*in*/LPSTR pin, /*in*/LPSTR pout)
{
	if ((pin == pout) || (pin > pout))
		return FALSE; // trivia

	LPSTR p = pin;
	int nbQuotes = 0;

	// TODO : replace the following code with a full-fledged regular expression

	while (p <= pout)
	{
		if ( (*p == '\"') && (p==pin || *(p-1)!='\\') )
			nbQuotes++;

		p++;
	}

	return (nbQuotes == 1);
}






// eval a parsed formula (values being passed replace the variable placeholders)
BOOL wzparser::eval(wzarray<wzvariable*> &arrVariables, wzstring &result)
{

    preEval(arrVariables);


    wzelementblockptr curlevel = &m_topLevel;

    BOOL bRes = doeval(curlevel, arrVariables, result);

    postEval(arrVariables);

    return bRes;
}

BOOL wzparser::doeval(wzelementblockptr curlevel,
                      wzarray<wzvariable*> &arrVariables, 
                      wzstring &result)
{
    if (!curlevel)
        return FALSE;


    bool bError = false;
    bool bResult = false;

    while ( !bError && !bResult && !curlevel->m_arrelements.IsIteratorEmpty() )
	{
		wzelement* e = curlevel->m_arrelements.IteratorPop();
		if (!e)
        {
            m_nLastError = PARSERERROR_SYNTAXERROR;
            bError = true;
            break;
        }

		if (e->getType() != _operator) // that's a litteral
		{
            result = *(wzstring*) e;
            bResult = true;
            break;
		}
    
        // that's an operator

		wzoperator* o = (wzoperator*) e;

		switch ( o->getID() )
		{
            case PARENTHESISID :
                {
                    // those are the parenthesis not following function tokens

                    wzelementblockptr child = o->getElementBlock();
                    if (!child)
                    {
                        m_nLastError = PARSERERROR_IMPAIREDPARENTHESIS;
                        bError = true;
                        break;
                    }

                    wzstring arg;
                    BOOL bResParam = doeval(child,
                                            arrVariables, 
                                            arg);

                    if (!bResParam)
                    {
                        bError = true;
                        break;
                    }

                    result = arg;
                    bResult = true;
                }
                break;
			case PARSERFUNC_BINARYPLUS : 
			case PARSERFUNC_BINARYMINUS : 
			case PARSERFUNC_MULT : 
			case PARSERFUNC_DIV : 
				{
					if (curlevel->m_arrelements.GetIteratorSize() < 2)
                    {
                        m_nLastError = PARSERERROR_NOTENOUGHPARAMETERS;
                        bError = true;
                        break;
                    }

                    wzstring arg2;

                    BOOL bResParam2 = doeval(curlevel,
                                             arrVariables, 
                                             arg2);
    
                    if (!bResParam2)
                    {
                        bError = true;
                        break;
                    }

                    wzstring arg1;

                    BOOL bResParam1 = doeval(curlevel,
                                             arrVariables, 
                                             arg1);
    
                    if (!bResParam1)
                    {
                        bError = true;
                        break;
                    }



					if (o->getID() == PARSERFUNC_BINARYPLUS)
					{
						double d2 = arg2.getDouble();
						double d1 = arg1.getDouble();
						double d = d1 + d2;
						result.fromNumberOrDouble( d, arg1.isANumber(),  arg2.isANumber(), -1 );
                        bResult = true;
					}
					else if (o->getID() == PARSERFUNC_BINARYMINUS)
					{
						double d2 = arg2.getDouble();
						double d1 = arg1.getDouble();
						double d = d1 - d2;
						result.fromNumberOrDouble( d, arg1.isANumber(),  arg2.isANumber(), -1 );
                        bResult = true;
					}
					else if (o->getID() == PARSERFUNC_MULT)
					{
						double d2 = arg2.getDouble();
						double d1 = arg1.getDouble();
						double d = d1 * d2;
						result.fromNumberOrDouble( d, arg1.isANumber(),  arg2.isANumber(), -1 );
                        bResult = true;
					}
					else if (o->getID() == PARSERFUNC_DIV)
					{
						double d2 = arg2.getDouble();
						double d1 = arg1.getDouble();
						if (d2 < 0.00000001 && d2 > -0.00000001)
						{
							m_nLastError = 4;
							d2 = 0.00000001; // hack meant to avoid a div by zero exception
						}
						double d = d1 / d2;
						result.fromNumberOrDouble( d, arg1.isANumber(),  arg2.isANumber(), -1 );
                        bResult = true;
					}


				}
                break;

			case PARSERFUNC_SIN:
			case PARSERFUNC_COS:
            case PARSERFUNC_SUM:
				{

                    // 1)pop the argument from the stack, it should be a parenthesis with a child block
                    if (curlevel->m_arrelements.IsIteratorEmpty())
	                {
                        m_nLastError = PARSERERROR_NOTENOUGHPARAMETERS;
                        bError = true;
                        break;
                    }

                    wzelement* childelement = curlevel->m_arrelements.IteratorPop();
		            if (!childelement)
                    {
                        m_nLastError = PARSERERROR_NOTENOUGHPARAMETERS;
                        bError = true;
                        break;
                    }

		            if (childelement->getType() != _operator)
		            {
                        m_nLastError = PARSERERROR_SYNTAXERROR;
                        bError = true;
                        break;
		            }
    
                    // implicitely, chidelement should be a parenthesis operator, with a child block
		            wzoperator* ochild = (wzoperator*) childelement;
                    wzelementblockptr child = ochild->getElementBlock();
                    if (!child)
                    {
                        m_nLastError = PARSERERROR_IMPAIREDPARENTHESIS;
                        bError = true;
                        break;
                    }

                    // 2)eval all args internally

                    long nbParams = child->m_arrelements.GetIteratorSize();
		            long nbParamsMin = o->getNbParamsMin();
                    long nbParamsMax = o->getNbParamsMax();

                    

                    bool bNotEnoughParameters = (nbParams < nbParamsMin);
                    bool bTooManyParameters = (nbParamsMax != -1) && (nbParams > nbParamsMax);

                    if (bNotEnoughParameters)
                    {
                        m_nLastError = PARSERERROR_NOTENOUGHPARAMETERS;
                        bError = true;
                        break;
                    }

                    if (bTooManyParameters)
                    {
                        m_nLastError = PARSERERROR_TOOMANYPARAMETERS;
                        bError = true;
                        break;
                    }

                    if (nbParams == 0)
                    {
                        // TODO : function without parameters (we have no one at the moment)

                        /* sample code

						if (o->getID() == PARSERFUNC_SIN)
						{
							double d1 = ...;
							result.fromDouble( d1 );
                            bResult = true;
						}

                        */
                    }
                    else
                    {
                        // 3)retrieve all args

                        wzstring* arrArgs = new wzstring[nbParams];
                        if (!arrArgs)
                        {
                            m_nLastError = PARSERERROR_OUTOFMEMORY;
                            bError = true;
                            break;
                        }


                        int iParam = nbParams - 1;

                        while ( !bError && !bResult && !child->m_arrelements.IsIteratorEmpty() )
	                    {
                            BOOL bResParam = doeval(child,
                                                    arrVariables, 
                                                    arrArgs[iParam--]);
                            if (!bResParam)
                            {
                                bError = true;
                                break;
                            }

                        } // end while all params

                        if (bError)
                        {
                            break;
                        }

                        // 4)eval the function now that all args are resolved

						if (o->getID() == PARSERFUNC_SIN)
						{
							double d1 = sin( arrArgs[0].getDouble() );
							result.fromDouble( d1 );
                            bResult = true;
						}
						else if (o->getID() == PARSERFUNC_COS)
						{
							double d1 = cos( arrArgs[0].getDouble() );
							result.fromDouble( d1 );
                            bResult = true;
						}
						else if (o->getID() == PARSERFUNC_SUM)
						{
                            double d = 0;
                            iParam = 0;
                            while (iParam < nbParams)
                                d += arrArgs[iParam++].getDouble();
							result.fromDouble( d );
                            bResult = true;
						}
                        else
                        {
                            bError = true;
                            m_nLastError = PARSERERROR_SYNTAXERROR;
                            break;
                        }

                        delete [] arrArgs;
                    }


				}
				break;
		} // end switch case

    } // end main while (1)
	

	return (bError == false);
}



BOOL wzparser::preEval(wzarray<wzvariable*> &arrVariables)
{
	m_nLastError = 0;

    replaceVariables(&m_topLevel, arrVariables);

    return TRUE;
}

BOOL wzparser::replaceVariables(wzelementblockptr curlevel, wzarray<wzvariable*> &arrVariables)
{
	// 
	// replace variables with actual values
	//

    if (curlevel == NULL)
        return FALSE;

	int nbSize = curlevel->m_arrelements.GetSize();

	for (int i = 0; i < nbSize; i++)
	{
		wzelement* e = curlevel->m_arrelements.GetAt(i);
		if (!e) continue;

		// replace all variables with values
		if (e->getType() != _operator)
		{
			wzstring* s = (wzstring*) e;
	
			long nbVariables = arrVariables.GetSize();
			for (long j = 0; j < nbVariables; j++)
			{
				wzvariable* v = arrVariables.GetAt(j);
				if (!v) continue;

				if ( v->isNameMatching(s) )
					s->setString( v->getVarvalue() ); // replace the variable name with its value
			}
		}
        else
        {
            wzoperator* op = (wzoperator*) e;
            if (op->getID() == PARENTHESISID)
                replaceVariables(op->getElementBlock(), arrVariables);
        }
    } // end for all elements at this level

    return TRUE;
}

BOOL wzparser::postEval(wzarray<wzvariable*> &arrVariables)
{
    return TRUE;
}






// known errors :
// 1 : impaired parenthesis
// 2 : empty formula
//
// 4 : division by zero error (during evaluation)
long wzparser::getLastError() // valid if parse() returns FALSE
{
	return m_nLastError;
}

void wzparser::dump()
{
    dump(&m_topLevel, 0);
}


void wzparser::dump(wzelementblock* block, int level)
{
    if (!block)
        return;

	char sztmp[256];
	long nbSize = block->m_arrelements.GetSize();

	for (long i = 0; i < nbSize; i++)
	{
        // indent level
        sztmp[0] = 0;
        for (int l = 0; l < level; l++)
            strcat(sztmp, "  ");
        OutputDebugString(sztmp);

		wzelement* e = block->m_arrelements.GetAt(i);
		if (!e) continue;

		if (e->getType() == _operator)
		{
			wzoperator* op = (wzoperator*) e;
			sprintf(sztmp,"%02d - operator (%d)\r\n", i, op->getID() );
		}
		else
		{
			wzstring* s = (wzstring*) e;
			sprintf(sztmp,"%02d - litteral (%s)\r\n", i, s->getString() );
		}

		OutputDebugString(sztmp);

		if (e->getType() == _operator)
		{
            wzoperator* op = (wzoperator*) e;
            if (op->getElementBlock())
                dump(op->getElementBlock(), level + 1);
        }

	}
}

