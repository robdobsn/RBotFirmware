#pragma once

#using<system.dll> 
using namespace System;
using namespace System::IO;
using namespace System::Collections::Generic;
using namespace System::Text::RegularExpressions;

#include "RobotCommandArgs.h"

#undef String
#define String String

ref class TestCaseM
{
public:
	String^ _name;
	List<String^> _listOfIns;
	List<String^> _listOfOuts;
	void clear()
	{
		_listOfIns.Clear();
		_listOfOuts.Clear();
	}
	void setName(String^ name)
	{
		_name = name;
	}
	const char* getName()
	{
		static char buf[1000];
		int i = 0;
		for each (char c in _name)
		{
			buf[i++] = c;
		}
		buf[i] = 0;
		return buf;
	}
	int numIns()
	{
		return _listOfIns.Count;
	}
	void getIn(int inIdx, const char* &pStr)
	{
		static char buf[1000];
		String^ st = _listOfIns[inIdx];
		int i = 0;
		for each (char c in st)
		{
			buf[i++] = c;
		}
		buf[i] = 0;
		pStr = buf;
	}
	int numOuts()
	{
		return _listOfOuts.Count;
	}
	void getOut(int inIdx, char* &pStr)
	{
		static char buf[1000];
		String^ st = _listOfOuts[inIdx];
		int i = 0;
		for each (char c in st)
		{
			buf[i++] = c;
		}
		buf[i] = 0;
		pStr = buf;
	}
};

ref class TestCaseHandler
{
public:
	List<TestCaseM^> testCases;
	TestCaseM^ testCaseUnderConstruction;
	bool inInMode = false;
	bool inOutMode = false;

	TestCaseHandler();

	bool readTestFile(System::String^ fileName)
	{
		try
		{
			testCaseUnderConstruction = gcnew TestCaseM;
			Console::WriteLine("trying to open file {0}...", fileName);
			StreamReader^ din = File::OpenText(fileName);

			String^ str;
			int count = 0;
			while ((str = din->ReadLine()) != nullptr)
			{
				count++;
				Console::WriteLine("line {0}: {1}", count, str);
				parseFileLine(str);
			}
			// Show test case
			for each(TestCaseM^ tc in testCases)
			{
				Console::WriteLine("Test Case");
				Console::WriteLine("Inputs...");
				for each(String^ st in tc->_listOfIns)
					Console::WriteLine(st);
				Console::WriteLine("Outputs...");
				for each(String^ st in tc->_listOfOuts)
					Console::WriteLine(st);
			}
			return true;
		}
		catch (Exception^ e)
		{
			if (dynamic_cast<FileNotFoundException^>(e))
				Console::WriteLine("file '{0}' not found", fileName);
			else
				Console::WriteLine("problem reading file '{0}'", fileName);
		}
		return false;
	}

	void parseFileLine(String^ line)
	{
		//array<String^> ^tokArr = line->Split(' ');
		//for each(String^ temp in StringArray)
		//	Console::WriteLine(temp);

		line = line->Trim();
		if (line->StartsWith("#"))
		{
			return;
		}
		else if (line->StartsWith("TESTCASE"))
		{
			//TestCaseM^ testCase = gcnew TestCaseM;
			testCaseUnderConstruction = gcnew TestCaseM;
			testCaseUnderConstruction->clear();
			testCaseUnderConstruction->setName(line->Substring(9));
		}
		else if (line == "ENDTESTCASE")
		{
			testCases.Add(testCaseUnderConstruction);
		}
		else if (line == "IN")
		{
			inInMode = true;
			inOutMode = false;
		}
		else if (line == "OUT")
		{
			inInMode = false;
			inOutMode = true;
		}
		else if (line == "ENDIN" || line == "ENDOUT")
		{
			inInMode = false;
			inOutMode = false;
		}
		else
		{
			if (inInMode)
			{
				testCaseUnderConstruction->_listOfIns.Add(line);
			}
			else if (inOutMode)
			{
				testCaseUnderConstruction->_listOfOuts.Add(line);
			}
		}


	}

};

#undef String
#define String WiringString
