


#include "stdafx.h"
#include "util.h"





////////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////// wzbuffer class ////////////////////////////////////
//
//
//
wzbuffer::wzbuffer() : m_nCnt(0)
{ 
	m_pBuffer = NULL; 
	m_nLength = m_nb1024bytes = 0; 
}

wzbuffer::wzbuffer(const wzbuffer &buf) : m_nCnt(0)
{
	m_pBuffer = NULL; 
	m_nLength = m_nb1024bytes = 0; 

	addBuffer( buf.getBuffer(), buf.getLength() );
}

wzbuffer::wzbuffer(void *pBuffer, long nLength) : m_nCnt(0)
{
	m_pBuffer = pBuffer; 
	m_nLength = nLength;
	m_nb1024bytes = m_nLength / 1024 + 1; 
}

wzbuffer::~wzbuffer()
{
	free();
}

void wzbuffer::AddRef()
{
	m_nCnt++;
}

void wzbuffer::Release()
{
	m_nCnt--;
}

void wzbuffer::free()
{
	Release(); // decr count

	if (m_pBuffer && m_nCnt == 0)
	{
		::free( (void*)m_pBuffer );
	}

	m_pBuffer	= NULL;
	m_nLength	= m_nb1024bytes = 0; 
	m_nCnt		= 0;
}

void wzbuffer::empty()
{
	m_nLength = 0; 
}

void *wzbuffer::getBuffer() const
{ 
	return m_pBuffer; 
}

long wzbuffer::getLength() const
{
	return m_nLength;
}


BOOL wzbuffer::addChar(char c)
{
	return addBuffer( &c, 1 );
}

BOOL wzbuffer::addWord(WORD w)
{
	return addBuffer( &w, 2 );
}

BOOL wzbuffer::addLong(long l)
{
	return addBuffer( &l, 4 );
}

BOOL wzbuffer::addDouble(double d)
{
	return addBuffer( &d, 8 );
}

BOOL wzbuffer::addString(char* szString)
{
	return szString ? addBuffer( szString, strlen(szString) ) : NULL;
}

BOOL wzbuffer::addStringWithEOL(char* szString)
{
    return szString ? addBuffer( szString, 1 + strlen(szString) ) : NULL;
}

BOOL wzbuffer::addEOLIfRequired()
{
    if (m_nLength == 0)
        return addChar(0);

    if (((char*)m_pBuffer)[m_nLength - 1] != 0)
        return addChar(0);

    return TRUE;
}

BOOL wzbuffer::addBuffer(void *pBuffer, long nLength) 
{ 
	if ( !pBuffer || nLength <= 0)
		return FALSE;

	if (m_nCnt == 0)
		AddRef();

	if ( m_nb1024bytes == 0 )
	{
		m_nb1024bytes = nLength / 1024 + 1;
		m_pBuffer = (char*) malloc(m_nb1024bytes * 1024); // result of malloc tested later
		if (!m_pBuffer)
		{
			m_nb1024bytes = 0;
			return FALSE;
		}
	}
	else if ( (m_nLength + nLength) >= (m_nb1024bytes * 1024) )
	{
		long nOldnb1024bytes = m_nb1024bytes;

		m_nb1024bytes = (m_nLength+nLength) / 1024 + 1;

		m_pBuffer = (void*) realloc(m_pBuffer, m_nb1024bytes * 1024);

		if (!m_pBuffer)
		{
			m_nb1024bytes = nOldnb1024bytes; // get back old value
			return FALSE;
		}
	}

	// copy [in] buffer (size = nLength bytes)
	memcpy ( ((char*)m_pBuffer)+m_nLength, pBuffer, nLength);
	m_nLength += nLength;
	((char*)m_pBuffer)[m_nLength]=0;

	return TRUE;

}

BOOL wzbuffer::addBuffer(wzbuffer &w)
{
	return addBuffer(w.getBuffer(), w.getLength());
}

void wzbuffer::dump()
{
	char sztemp[MAX_PATH];
	sprintf(sztemp,"\n--- dump (ptr:%ld,length:%ld) ---\n",(long)getBuffer(),getLength());
	OutputDebugString(sztemp);

	char *pBuf = (char*) getBuffer();
	long nLength = getLength();
	
	for (long i=0; i<nLength; i++)
	{
		sprintf(sztemp,"%02X ",(unsigned char)*pBuf++);
		OutputDebugString(sztemp);
	}
	OutputDebugString("\n");

}

void wzbuffer::operator =(wzbuffer &buf)
{
	free();

	addBuffer( buf.getBuffer(), buf.getLength() );
}










///////////////////////////////////////////////////////////////////

wzelement::wzelement()
{}

wzelement::~wzelement()
{}

void wzelement::setType(elementtype t)
{
	m_type = t;
}
elementtype wzelement::getType()
{
	return m_type;
}










///////////////////////////////////////////////////////////////////

wzoperator::wzoperator()
{
    setElementBlock(NULL);
}

wzoperator::~wzoperator()
{
    delete m_elemBlock;
}

void wzoperator::setID(long n)
{
	m_nID = n;
}

long wzoperator::getID()
{
	return m_nID;
}

void wzoperator::setPriority(long lvl)
{
	m_nPriority = lvl;
}

long wzoperator::getPriority()
{
	return m_nPriority;
}

void wzoperator::setNbParamsMin(long nb)
{
	m_nbParamsMin = nb;
}

long wzoperator::getNbParamsMin()
{
	return m_nbParamsMin;
}

void wzoperator::setNbParamsMax(long nb)
{
	m_nbParamsMax = nb;
}

long wzoperator::getNbParamsMax()
{
	return m_nbParamsMax;
}

void wzoperator::setElementBlock(wzelementblockptr elemBlock)
{
    m_elemBlock = elemBlock;
}

wzelementblockptr wzoperator::getElementBlock()
{
    return m_elemBlock;
}

void wzoperator::setIsAFunction(BOOL bIsAFunction)
{
	m_bIsAfunction = bIsAFunction;
}

BOOL wzoperator::getIsAFunction()
{
	return m_bIsAfunction;
}


BOOL wzoperator::isHigherPriorityThan(wzoperator* src) // TRUE if this is of higher priority than src
{
	if (!src) return TRUE;

	return m_nPriority > src->getPriority();
}

BOOL wzoperator::isParenthesis()
{
	return m_nID = PARENTHESISID;
}




///////////////////////////////////////////////////////////////////

// wzstring //////////////////////////////////////////////
//
// simple string storage implementation
//

wzstring::wzstring()
{
}

wzstring::~wzstring() // frees the buffer
{
}

void wzstring::init()
{
}

BOOL wzstring::isEmpty()
{
	return getLength() == 0;
}

void wzstring::empty()
{
    m_origValue.empty();
    m_curValue.empty();
}

LPSTR wzstring::setOrigString(LPSTR pString, long nLength) // allocates a buffer
{
    empty();

    m_origValue.addBuffer(pString, nLength);
    m_origValue.addEOLIfRequired();

    m_curValue.addBuffer(pString, nLength);
    m_curValue.addEOLIfRequired();

    return getString();
}

LPSTR wzstring::setOrigString(wzstring* pString) // allocates a buffer
{
	return setOrigString( pString->getString(), pString->getLength() );
}

LPSTR wzstring::setString(LPSTR pString, long nLength) // allocates a buffer
{
    m_curValue.empty();

    m_curValue.addBuffer(pString, nLength);
    m_curValue.addEOLIfRequired();

    return getString();
}

LPSTR wzstring::setString(LPSTR pString) // allocates a buffer
{
    if (!pString)
        return NULL;
	return setString( pString, strlen(pString));
}

LPSTR wzstring::setString(wzstring* pString)
{
    if (!pString)
        return NULL;
    return setString( pString->getString() );
}

LPSTR wzstring::getString()
{
	return (LPSTR)m_curValue.getBuffer();
}

long wzstring::getLength()
{
	return m_curValue.getLength();
}

wzbuffer* wzstring::getOrigString()
{
    return &m_origValue;
}

wzbuffer* wzstring::getCurValue()
{
    return &m_curValue;
}


BOOL wzstring::isANumber() // TRUE if the number is an integer
{
	long n = getLength();
	LPSTR p = getString();
	if (!p) return FALSE;

	for (long i = 0; i < n ; i++)
	{
		char c = *(p++);
		if (c >= '0' && c <= '9')
			continue;
	
		if (c == '.')
			return FALSE; // decimal separator not allowed

		if (c == '+' || c == '-' || c == ' ' || c == ',')
			continue;

		return FALSE;
	}

	return TRUE;
}


BOOL wzstring::isADouble() // TRUE if the number is an floating number
{
	long n = getLength();
	LPSTR p = getString();
	if (!p) return FALSE;

	for (long i = 0; i < n ; i++)
	{
		char c = *(p++);
		if (c >= '0' && c <= '9')
			continue;

		if (c == '+' || c == '-' || c == ' ' || c == '.' || c == ',')
			continue;

		return FALSE;
	}

	return TRUE;
}


long wzstring::getNumber()
{
	return atoi( getString() );
}

double wzstring::getDouble()
{
	return atof( getString() );
}

void wzstring::fromNumber(long n)
{
	char sztmp[32];
	sprintf(sztmp,"%d",n);
	setString(sztmp,strlen(sztmp));
}

void wzstring::fromDouble(double d)
{
	char sztmp[32];
	sprintf(sztmp,"%f",d);
	setString(sztmp,strlen(sztmp));
}

/*void wzstring::fromNumberOrDouble(double d, BOOL bArg1IsANumber, BOOL bArg2IsANumber)
{
	if (bArg1IsANumber && bArg2IsANumber)
		fromNumber( (long)d );
	else
		fromDouble(d);
}*/

void wzstring::fromNumberOrDouble(double d, ...)  // var args implementation
{
	va_list argList;
	va_start(argList, d);

	BOOL bMain = TRUE;
	BOOL bCurrent;

	// list all BOOL args
	while ( (bCurrent = va_arg(argList, BOOL)) != -1 )
		bMain &= bCurrent;

	va_end(argList);

	if ( bMain ) // TRUE if all BOOL passed were true
		fromNumber( (long)d );
	else
		fromDouble(d);
}



///////////////////////////////////////////////////////////////////


void wzvariable::setVar(LPSTR name, LPSTR value)
{
	setVarname(name);
	setVarvalue(value);
}

void wzvariable::setVarname(LPSTR name)
{
	m_varname.setOrigString(name, strlen(name));
}

void wzvariable::setVarvalue(LPSTR value)
{
	m_varname.setString(value, strlen(value));
}

LPSTR wzvariable::getVarname()
{
    return m_varname.getOrigString() ? (LPSTR)m_varname.getOrigString()->getBuffer() : NULL;
}

LPSTR wzvariable::getVarvalue()
{
    return m_varname.getString();
}

BOOL wzvariable::isNameMatching(wzstring* litteral) // returns TRUE if the litteral being passed matches the variable name
{
	if ( !litteral || litteral->isEmpty() ) return FALSE;
	if ( m_varname.isEmpty() ) return FALSE;

    wzbuffer* b1 = litteral->getOrigString();
    if (!b1)
        return FALSE;
    LPSTR p = (LPSTR) b1->getBuffer();
	long n = b1->getLength();

    wzbuffer* b2 = m_varname.getOrigString();
    if (!b2)
        return FALSE;

	LPSTR q = (LPSTR) b2->getBuffer();
	long m = b2->getLength();

	// . matching can only fail because of existing spaces on the left and on the right
	// . other note of importance : matching is case sensitive, which means that variables
	//   must be passed appropriately

	// TODO : replace the following code with a full-fledged regular expression

	// pass spaces on the left
	while (*p && *p <= 32)
		p++;

	if (*p == 0)
		return FALSE;

	long i = 0;
	while (*p && i < m && i < n)
	{
		if (*p != *q)
			return FALSE;

		p++;
		q++;
		i++;
	}

    if (*q)
        return FALSE;

	// names seem to match, just make sure that litteral has no suffix
	while ( *p )
	{
		if (*p > 32)
			return FALSE; // it has a suffix, for instance litteral="xy", m_varname="x"

		p++;
	}

	return TRUE;
}








wzelementblock::wzelementblock()
{
}

wzelementblock::~wzelementblock()
{
	// popup all remaining operators
	while ( !m_arroperators.IsEmpty() )
		m_arrelements.Add( m_arroperators.Pop() );

    // delete everything (virtual destructors)
	long nbSize = m_arrelements.GetSize();
	for (long i = 0; i < nbSize; i++)
		delete m_arrelements.GetAt(i);
	m_arrelements.RemoveAll();
}


wzelementblockptr wzelementblock::createChildBlock()
{
    wzelementblockptr newBlock = new wzelementblock();
    if (newBlock)
        newBlock->m_cpParent = this;
    return newBlock;
}



