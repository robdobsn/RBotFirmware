#include "application.h"
#include "tinyexpr.h"

#include "PatternEvaluator.h"

PatternEvaluator patternEvaluator;

void setup()
{
    Serial.begin(115200);
    Serial.println("TinyExprTest");
    delay(4000);
    /*const char *c = "sqrt(5^2+7^2+11^2+(8-2)^2)";
    double r = te_interp(c, 0);
    Serial.printlnf("%s evaluates to %f", c, r);

    c = "max(10,2)";
    r = te_interp(c, 0);
    Serial.printlnf("%s evaluates to %f", c, r);
    Serial.printlnf("Debug %s", debugStr);*/

    const char* setupExpr = "a=4;b=5\nc=9";
    patternEvaluator.addSetup(setupExpr);

    const char* loopExpr = "x=a+b+t;y=t-10";
    patternEvaluator.addLoop(loopExpr);

}

void loop()
{
    /*double x, y;
    patternEvaluator.procLoop(x, y);*/
}
