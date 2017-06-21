#include <iostream>
#include "shunting-yard.h"
#include "shunting-yard-exceptions.h"

struct codeBlock {
  static void interpret(const char* start, const char** end, TokenMap vars) {
    // Remove white spaces:
    while (isspace(*start)) ++start;

    while (*start) {
      calculator::calculate(start, vars, ";\n}", &start);

      // Find the beginning of the next expression:
      while(isspace(*start) || *start == ';')
        ++start;
    }

    if (*start == '\0') {
      *end = start+1;
    } else {
      Serial.println("Syntax error expected end of string");
      return;
    }
  }
};

int main() {
    Serial.begin(115200);
  GlobalScope vars;
  const char* code =
//    "  a = sin(10)\n"
//    "  b = 20 * 12\n"
//    "  c = a + b ";
    "tVal = 5;"
    "M_PI = 3.14159;"
    "_centreX = 0;"
    "_centreY = 0;"
    "_startRadius = 180;"
    "_endRadius = 20;"
    "_modulationAmp = 10;"
    "_modulationDampingFactor = 0.6;"
    "_modulationFreqMult = 20;"
    "_overRotationFactor = 1.03;"
    "_segmentsPerRev = 200;"
    "_outstepPerRev = -5;"
    "rRadians = tVal * 2 * M_PI * _overRotationFactor / _segmentsPerRev\n"
    "curModu = sin(tVal * _modulationFreqMult * 2 * M_PI / _segmentsPerRev)\n"
    "curRadius = _startRadius + _outstepPerRev * tVal / _segmentsPerRev;"
    "radiusProp = (curRadius-min(_startRadius,_endRadius))/abs(_startRadius-_endRadius);"
    "modDampFact = (1 - radiusProp) * _modulationDampingFactor;"
    "modAmpl = curRadius + curModu * _modulationAmp * (1 - modDampFact);"
    "X = _centreX + modAmpl * sin(rRadians);"
    "Y = _centreY + modAmpl * cos(rRadians);";

  codeBlock::interpret(code, &code, vars);

  //Serial.printlnf("X=%f, Y=%f", vars["X"], vars["Y"]);
  return 0;
}
