



#pragma once


class wzbuffer; // forward declaration
template <class T> class wzstack; // forward declaration
template <class T> class wzarray; // forward declaration



//// wzbuffer ////////////////////////////////////////////////////////////////////////////
//
// auto-resizable buffer
//
class wzbuffer
{

	// Members
protected:
	void	*m_pBuffer;
	long	m_nLength;
	long	m_nb1024bytes;
	int		m_nCnt;

	// Constructor/Destructor
public:
	wzbuffer();
	wzbuffer(const wzbuffer &buf);
	wzbuffer(void *pBuffer, long nLength);
	virtual ~wzbuffer();

	void AddRef();
	void Release();

	// Accessors
public:
    void empty();
	void free();
	void *getBuffer() const;
	long getLength() const;

	// Methods
public:
	BOOL addChar(char c);
	BOOL addWord(WORD w);
	BOOL addLong(long l);
	BOOL addDouble(double d);
	BOOL addString(char* szString);
	BOOL addStringWithEOL(char* szString);
	BOOL addBuffer(void *pBuffer, long nLength);
	BOOL addBuffer(wzbuffer &);
    BOOL addEOLIfRequired();
	
	void operator =(wzbuffer &);

	void dump();

	// Helpers
public:

};



typedef enum {_operator, _litteral} elementtype;

#define PARSERFUNC_BINARYBASE       0x200
#define PARSERFUNC_BINARYPLUS       (PARSERFUNC_BINARYBASE + 50)
#define PARSERFUNC_BINARYMINUS      (PARSERFUNC_BINARYBASE + 51)
#define PARSERFUNC_MULT             (PARSERFUNC_BINARYBASE + 52)
#define PARSERFUNC_DIV              (PARSERFUNC_BINARYBASE + 53)
#define PARSERFUNC_SIN              60
#define PARSERFUNC_COS              61
#define PARSERFUNC_SUM              62
#define PARENTHESISID               0x1000

#define PARSERERROR_SYNTAXERROR 1
#define PARSERERROR_OUTOFMEMORY 2
#define PARSERERROR_NOTENOUGHPARAMETERS 3
#define PARSERERROR_TOOMANYPARAMETERS 4
#define PARSERERROR_IMPAIREDPARENTHESIS 5

typedef struct
{
	LPSTR _operator;
	long _id;
	long _priority;
	long _nbparamsmin; // -1 if N/A
    long _nbparamsmax; // -1 if N/A
	BOOL _isfunction;
} _structoperator;


class wzelement
{
protected:
	elementtype m_type;

public:
	wzelement();
	virtual ~wzelement();

	void setType(elementtype t);
	elementtype getType();

};


class wzelementblock; // forward declaration
typedef wzelementblock* wzelementblockptr;

class wzoperator : public wzelement
{
protected:
	long m_nID;
	long m_nPriority;
    long m_nbParamsMin, m_nbParamsMax;
	BOOL m_bIsAfunction;
    wzelementblockptr m_elemBlock;

public:
    wzoperator();
    virtual ~wzoperator();

	void setID(long n);
	long getID();

	void setPriority(long lvl);
	long getPriority();

	void setNbParamsMin(long nb);
	long getNbParamsMin();

	void setNbParamsMax(long nb);
	long getNbParamsMax();

    void setElementBlock(wzelementblockptr elemBlock);
    wzelementblockptr getElementBlock();

	void setIsAFunction(BOOL bIsAFunction);
	BOOL getIsAFunction();

	BOOL isHigherPriorityThan(wzoperator* src); // TRUE if this is of higher priority than src
	BOOL isParenthesis();
};



// wzstring //////////////////////////////////////////////
//
// simple string storage implementation
//
class wzstring : public wzelement
{

	// Members
protected:
    wzbuffer m_origValue, m_curValue;

	// Construction
public:
	wzstring();
	virtual ~wzstring(); // frees the buffer

	void init();

	BOOL isEmpty();

	void empty();
	LPSTR setOrigString(LPSTR pString, long nLength);
	LPSTR setOrigString(wzstring* pString);
	LPSTR setString(LPSTR pString, long nLength);
	LPSTR setString(LPSTR pString);
    LPSTR setString(wzstring* pString);
	LPSTR getString();
	long getLength();

    wzbuffer* getOrigString();
    wzbuffer* getCurValue();

	BOOL isANumber(); // TRUE if the number is an integer
	BOOL isADouble(); // less restricting than isANumber()
	long getNumber();
	double getDouble();

	void fromNumber(long n);
	void fromDouble(double d);

//	void fromNumberOrDouble(double d, BOOL bArg1IsANumber, BOOL bArg2IsANumber);
	void fromNumberOrDouble(double d, ...); // var args implementation

};



class wzvariable : public wzelement
{

	// Members
protected:
	wzstring m_varname;

public:
	void setVar(LPSTR name, LPSTR value);
	void setVarname(LPSTR name);
	void setVarvalue(LPSTR value);

	LPSTR getVarname();
	LPSTR getVarvalue();

	BOOL isNameMatching(wzstring* litteral); // returns TRUE if the litteral being passed matches the variable name
};





//// wzstack ////////////////////////////////////////////////////////////////////////////
//
// template LIFO stack
//
template <class T>
class wzstack
{

	// Members
protected:
	wzarray<T> m_arrElements;
    long m_nIterator;

	// Construction
public:
	wzstack();
	virtual ~wzstack();

	// Accessors

	BOOL IsEmpty();
	long GetSize();
	T GetTop(); // element not moved from the stack
	T GetBottom(); // element not moved from the stack

	// methods

	long Push(T newElement); // returns the new size of the stack
	T Pop();

    // fake iterator
    BOOL IsIteratorEmpty();
    BOOL GetIteratorSize();
    T GetIteratorTop();
    T GetIteratorBottom();
    T IteratorPop();

};


template <class T> wzstack<T>::wzstack()
{
    m_nIterator = 0;
}

template <class T> wzstack<T>::~wzstack()
{
}

template <class T> BOOL wzstack<T>::IsEmpty()
{ 
	return m_arrElements.GetSize() == 0; 
}

template <class T> long wzstack<T>::GetSize()
{ 
	return m_arrElements.GetSize(); 
}

template <class T> T wzstack<T>::GetTop()
{ 
	T empty = NULL;
	return !IsEmpty() ? m_arrElements.GetAt( m_arrElements.GetSize() - 1 ) : empty; 
}

template <class T> T wzstack<T>::GetBottom()
{ 
	T empty = NULL;
	return !IsEmpty() ? m_arrElements.GetAt(0) : empty; 
}


template <class T> long wzstack<T>::Push(T newElement)
{
	m_arrElements.Add( newElement );
    m_nIterator = m_arrElements.GetSize();
	return GetSize();
}

template <class T> T wzstack<T>::Pop()
{
	T empty = NULL;

	if ( IsEmpty() )
		return empty;

	T elem = GetTop();

	m_arrElements.RemoveAt( m_arrElements.GetSize() - 1 );

    m_nIterator = m_arrElements.GetSize();

	return elem;
}


// fake iterator
template <class T> BOOL wzstack<T>::IsIteratorEmpty()
{
    return (m_nIterator == 0);
}

template <class T> BOOL wzstack<T>::GetIteratorSize()
{
    return m_nIterator;
}

template <class T> T wzstack<T>::GetIteratorTop()
{
    T empty = NULL;

    if (m_nIterator > m_arrElements.GetSize())
        m_nIterator = m_arrElements.GetSize();

    if (m_nIterator == 0)
        return empty;

    return m_arrElements.GetAt(m_nIterator - 1);
}

template <class T> T wzstack<T>::GetIteratorBottom()
{
    T empty = NULL;

    if (m_nIterator > m_arrElements.GetSize())
        m_nIterator = m_arrElements.GetSize();

    if (m_nIterator == 0)
        return empty;

    return m_arrElements.GetAt(0);
}

template <class T> T wzstack<T>::IteratorPop()
{
    T empty = NULL;

    if (m_nIterator == 0)
        return empty;

    if (m_nIterator > m_arrElements.GetSize())
        m_nIterator = m_arrElements.GetSize();

    if (m_nIterator == 0)
        return empty;

    return m_arrElements.GetAt(m_nIterator-- - 1);
}





//// wzarray ////////////////////////////////////////////////////////////////////////////
//
// MFC collection source code
//
template <class T>
class wzarray
{
	// Members
protected:
	T*  m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount
    long m_nIterator;

	// Construction

public:
	wzarray();
	~wzarray();

	// Accessors

	int		GetSize() const;
	int		GetUpperBound() const;
	void	SetSize(int nNewSize, int nGrowBy = -1);

	// Methods

	void	FreeExtra();
	void	RemoveAll();

	T		GetAt(int nIndex) const;
	void	SetAt(int nIndex, T newElement);
	T&		ElementAt(int nIndex);

	// Direct Access to the element data (may return NULL)
	const T* GetData() const;
	T*		 GetData();

	void	SetAtGrow(int nIndex, T newElement);
	int		Add(T newElement);
	int		Append(const wzarray& src);
	void	Copy(const wzarray& src);

	T		operator[](int nIndex) const;
	T&		operator[](int nIndex);

	void	InsertAt(int nIndex, T newElement, int nCount = 1);
	void	RemoveAt(int nIndex, int nCount = 1);
	void	InsertAt(int nStartIndex, wzarray* pNewArray);


    // fake iterator
    BOOL IsIteratorEmpty();
    BOOL GetIteratorSize();
    T GetIteratorTop();
    T GetIteratorBottom();
    T IteratorPop();


protected:
	// local typedefs for class templates
	typedef void* BASE_TYPE;
	typedef void* BASE_ARG_TYPE;
};



template <class T> wzarray<T>::wzarray()
{
	m_pData = NULL;
	m_nSize = m_nMaxSize = m_nGrowBy = m_nIterator = 0;
}

template <class T> wzarray<T>::~wzarray()
{
	delete[] (BYTE*)m_pData;
}

template <class T> int wzarray<T>::GetSize() const
{ 
	return m_nSize; 
}

template <class T> int wzarray<T>::GetUpperBound() const
{ 
	return m_nSize-1; 
}

template <class T> void wzarray<T>::SetSize(int nNewSize, int nGrowBy)
{
	if (nNewSize < 0)
		return;

	if (nGrowBy != -1)
		m_nGrowBy = nGrowBy;  // set new size

	if (nNewSize == 0)
	{
		// shrink to nothing
		delete[] (BYTE*)m_pData;
		m_pData = NULL;
		m_nSize = m_nMaxSize = 0;
	}
	else if (m_pData == NULL)
	{
		// create one with exact size
		m_pData = (T*) new BYTE[nNewSize * sizeof(T)];

		memset(m_pData, 0, nNewSize * sizeof(T));  // zero fill

		m_nSize = m_nMaxSize = nNewSize;
	}
	else if (nNewSize <= m_nMaxSize)
	{
		// it fits
		if (nNewSize > m_nSize)
		{
			// initialize the new elements

			memset(&m_pData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(T));

		}

		m_nSize = nNewSize;
	}
	else
	{
		// otherwise, grow array
		int nGrowBy = m_nGrowBy;
		if (nGrowBy == 0)
		{
			// heuristically determine growth when nGrowBy == 0
			//  (this avoids heap fragmentation in many situations)
			nGrowBy = (m_nSize/8 > 4)? m_nSize/8 : 4;
		}
		int nNewMax;
		if (nNewSize < m_nMaxSize + nGrowBy)
			nNewMax = m_nMaxSize + nGrowBy;  // granularity
		else
			nNewMax = nNewSize;  // no slush

		T* pNewData = (T*) new BYTE[nNewMax * sizeof(T)];

		// copy new data from old
		memcpy(pNewData, m_pData, m_nSize * sizeof(T));

		memset(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(T));


		// get rid of old stuff (note: no destructors called)
		delete[] (BYTE*)m_pData;
		m_pData = pNewData;
		m_nSize = nNewSize;
		m_nMaxSize = nNewMax;
	}

    m_nIterator = m_nSize;
}

template <class T> void wzarray<T>::FreeExtra()
{
	if (m_nSize != m_nMaxSize)
	{
		// shrink to desired size
		T* pNewData = NULL;
		if (m_nSize != 0)
		{
			pNewData = (T*) new BYTE[m_nSize * sizeof(T)];
			// copy new data from old
			memcpy(pNewData, m_pData, m_nSize * sizeof(T));
		}

		// get rid of old stuff (note: no destructors called)
		delete[] (BYTE*)m_pData;
		m_pData = pNewData;
		m_nMaxSize = m_nSize;
	}
}

template <class T> void wzarray<T>::RemoveAll()
{ 
	SetSize(0); 
}

template <class T> T wzarray<T>::GetAt(int nIndex) const
{ 
	if (nIndex<0 || nIndex>=m_nSize)
		return NULL;
	else
		return m_pData[nIndex]; 
}

template <class T> void wzarray<T>::SetAt(int nIndex, T newElement)
{ 
	if (nIndex >= 0 && nIndex < m_nSize);
		m_pData[nIndex] = newElement; 
}

template <class T> T& wzarray<T>::ElementAt(int nIndex)
{ 
	static T pVoid=NULL;

	if (nIndex<0 || nIndex>=m_nSize)
		return pVoid;
	else
		return m_pData[nIndex]; 
}

template <class T> const T* wzarray<T>::GetData() const
{ 
	return (const T*)m_pData; 
}

template <class T> T* wzarray<T>::GetData()
{ 
	return (T*)m_pData; 
}

template <class T> void wzarray<T>::SetAtGrow(int nIndex, T newElement)
{
	if (nIndex < 0)
		return;

	if (nIndex >= m_nSize)
		SetSize(nIndex+1);
	m_pData[nIndex] = newElement;
}

template <class T> int wzarray<T>::Add(T newElement)
{ 
	int nIndex = m_nSize;
	SetAtGrow(nIndex, newElement);
	return nIndex; 
}

template <class T> int wzarray<T>::Append(const wzarray& src)
{
	int nOldSize = m_nSize;
	
	if (this != &src)
	{
		SetSize(m_nSize + src.m_nSize);

		memcpy(m_pData + nOldSize, src.m_pData, src.m_nSize * sizeof(T));
	}
	return nOldSize;
}

template <class T> void wzarray<T>::Copy(const wzarray& src)
{
	if (this == &src)
		return;

	SetSize(src.m_nSize);

	memcpy(m_pData, src.m_pData, src.m_nSize * sizeof(T));

}

template <class T> T wzarray<T>::operator[](int nIndex) const
{ 
	return GetAt(nIndex); 
}

template <class T> T& wzarray<T>::operator[](int nIndex)
{ 
	return ElementAt(nIndex); 
}


template <class T> void wzarray<T>::InsertAt(int nIndex, T newElement, int nCount)
{
	if (nIndex<0 || nCount<=0)
		return;

	if (nIndex >= m_nSize)
	{
		// adding after the end of the array
		SetSize(nIndex + nCount);  // grow so nIndex is valid
	}
	else
	{
		// inserting in the middle of the array
		int nOldSize = m_nSize;
		SetSize(m_nSize + nCount);  // grow it to new size
		// shift old data up to fill gap
		memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
			(nOldSize-nIndex) * sizeof(T));

		// re-init slots we copied from

		memset(&m_pData[nIndex], 0, nCount * sizeof(T));

	}

	// insert new value in the gap
	while (nCount-- && (nIndex + nCount <= m_nSize))
		m_pData[nIndex++] = newElement;
}

template <class T> void wzarray<T>::RemoveAt(int nIndex, int nCount)
{
	if (nIndex >= 0 || nCount >= 0 || (nIndex + nCount <= m_nSize) )
	{
		// just remove a range
		int nMoveCount = m_nSize - (nIndex + nCount);

		if (nMoveCount)
			memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
				nMoveCount * sizeof(T));
		m_nSize -= nCount;
	}
}

template <class T> void wzarray<T>::InsertAt(int nStartIndex, wzarray* pNewArray)
{
	if (pNewArray && nStartIndex>=0 && pNewArray->GetSize() > 0)
	{
		InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
		for (int i = 0; i < pNewArray->GetSize(); i++)
			SetAt(nStartIndex + i, pNewArray->GetAt(i));
	}
}

// fake iterator
template <class T> BOOL wzarray<T>::IsIteratorEmpty()
{
    return (m_nIterator == 0);
}

template <class T> BOOL wzarray<T>::GetIteratorSize()
{
    return m_nIterator;
}

template <class T> T wzarray<T>::GetIteratorTop()
{
    T empty = NULL;

    if (m_nIterator > m_nSize)
        m_nIterator = m_nSize;

    if (m_nIterator == 0)
        return empty;

    return m_pData[m_nIterator - 1];
}

template <class T> T wzarray<T>::GetIteratorBottom()
{
    T empty = NULL;

    if (m_nIterator > m_nSize)
        m_nIterator = m_nSize;

    if (m_nIterator == 0)
        return empty;

    return m_pData[0];
}

template <class T> T wzarray<T>::IteratorPop()
{
    T empty = NULL;

    if (m_nIterator == 0)
        return empty;

    if (m_nIterator > m_nSize)
        m_nIterator = m_nSize;

    if (m_nIterator == 0)
        return empty;

    return m_pData[m_nIterator-- - 1];
}


// two specializations

typedef wzarray<void*> XlsArrayLPVoid;
typedef wzarray<long>  XlsArrayLong;



class wzelementblock
{
public:
    wzelementblock();
    ~wzelementblock();

    wzelementblockptr m_cpParent;

	wzstack<wzoperator*> m_arroperators;
	wzarray<wzelement*> m_arrelements;

    wzelementblockptr createChildBlock();

};
