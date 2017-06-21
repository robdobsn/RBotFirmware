// evalparser.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include "util.h"
#include "parser.h"

// Global Variables:
HINSTANCE hInst;								// current instance


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

	wzparser* p = new wzparser();

	//p->parse("=5+3+SIN(x+4.5)");
    //p->parse("=5*3+2");
    //p->parse("=(5+3)*2");
    //p->parse("=(3)*2");
    //p->parse("=2*(3)");
    //p->parse("=((3)+4)*2*(3+3.5)");
    //p->parse("=2*(SUM(3;4))");
    p->parse("=3*SUM(x;1)");
	p->dump(); // for debug purpose only

	wzarray<wzvariable*> arrVariables;

	wzvariable* x = new wzvariable();
	x->setVar("x","13");
	arrVariables.Add( x );

	wzstring result;
	if ( p->eval(arrVariables, result) )
	{
		OutputDebugString( "result=" );
		OutputDebugString( result.getString() );
		OutputDebugString( "\r\n" );
	}

	x->setVarvalue("15");
	if ( p->eval(arrVariables, result) )
	{
		OutputDebugString( "result=" );
		OutputDebugString( result.getString() );
		OutputDebugString( "\r\n" );
	}


	// delete variables
	long nbVariables = arrVariables.GetSize();
	for (long iVars = 0; iVars < nbVariables; iVars++)
		delete arrVariables.GetAt(iVars);

	delete p;


	return 0;
}



