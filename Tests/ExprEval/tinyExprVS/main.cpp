// tinyExpr.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "PatternEvaluator.h"

//#include "tinyexpr.h"

PatternEvaluator patternEvaluator;

int main()
{

//	const char *c = "sqrt(5^2+7^2+11^2+(8-2)^2)";
//	double r = te_interp(c, 0);
//	printf("%s evaluates to %f", c, r);

	//const char *c = "max(10,2)\nc=5";
	//double r = te_interp(c, 0);
	//printf("%s evaluates to %f", c, r);

	//te_variable vars = { {"c",9} };
	//int err = 0;
	//te_expr* compiledExpr = te_compile("c", &vars, 1, &err);
	//printf("Compiling a=1 err = %d (%ld)\r\n", err, compiledExpr);

//	const char* setupExpr = "a=4;b=5;c=9;d=10";
	const char* setupExpr = "a=4==3;b=4>4;c=4<5;d=9.9>=10;e=12<=22;f=24<=24;g=99.9>=99.9;h=1=3;i=2<2;j=2<1;";
    patternEvaluator.addSetup(setupExpr);

 //   const char* loopExpr = "x=c;y=c-10";
 //   patternEvaluator.addLoop(loopExpr);

	//double x, y;
	//patternEvaluator.procLoop(x, y);
	//printf("Eval %0.2f %0.2f\r\n", x, y);
	int cg = getchar();
}
