
#pragma once

#include <Arduino.h>
#include <unity.h>
#include "../src/RobotMotion/MotionControl/MotionHelper.h"
#include <ArduinoLog.h>

static const char* UnitTestHomingSeq_Test1_Config = R"strDelim( 
    {"robotType":"SandTableScaraPiHat3.6","cmdsAtStart":"","webui":"SandUI","evaluators":{"thrContinue":0},
    "robotGeom":{"model":"SingleArmScara",
    "homing":{"homingSeq":"A-10000n;B10000;#;A+10000N;B-10000;#;A+500;B-500;#;B+10000n;#;B-10000N;#;B-1050;#;A=h;B=h;$","maxHomingSecs":120},
    "blockDistanceMM":1,"allowOutOfBounds":0,"stepEnablePin":"21","stepEnLev":0,"stepDisableSecs":10,
    "axis0":{"maxSpeed":50,"maxAcc":50,"maxRPM":30,"stepsPerRot":9600,"maxVal":92.5,"stepPin":"27","dirnPin":"33","endStop0":{"sensePin":"39","actLvl":0,"inputType":"INPUT_PULLUP"}},
    "axis1":{"maxSpeed":50,"maxAcc":50,"maxRPM":30,"stepsPerRot":9600,"maxVal":92.5,"stepPin":"12","dirnPin":"16","endStop0":{"sensePin":"36","actLvl":0,"inputType":"INPUT_PULLUP"}}},
    "fileManager":{"spiffsEnabled":1,"spiffsFormatIfCorrupt":1,"sdEnabled":1,"sdMOSI":"23","sdMISO":"19","sdCLK":"18","sdCS":"4"},
    "wifiLed":{"hwPin":"14","onLevel":1,"onMs":200,"shortOffMs":200,"longOffMs":750},"ledStrip":{"ledPin":"25","sensorPin":"-1"}}
    )strDelim";

static char const* UnitTestHomingSeq_Test1_DebugCmdStrs[] = {
    "Move",
    "Move",
    "Move",
    "Move",
    "Move",
    "Move",
    "Done",
};
static char const* UnitTestHomingSeq_Test1_JsonRslts[] = {
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[-10000,10000,0],"F":4800.00,"mv":"rel","end":[[2,0],[0,0],[0,0]],"OoB":"Y","num":10001,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[10000,-10000,0],"F":4800.00,"mv":"rel","end":[[1,0],[0,0],[0,0]],"OoB":"Y","num":10002,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[500,-500,0],"F":4800.00,"mv":"rel","end":[[0,0],[0,0],[0,0]],"OoB":"Y","num":10003,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[0,10000,0],"F":4800.00,"mv":"rel","end":[[0,0],[2,0],[0,0]],"OoB":"Y","num":10004,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[0,-10000,0],"F":4800.00,"mv":"rel","end":[[0,0],[1,0],[0,0]],"OoB":"Y","num":10005,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[0,-1050,0],"F":4800.00,"mv":"rel","end":[[0,0],[0,0],[0,0]],"OoB":"Y","num":10006,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[0,0,0],"mv":"rel","end":[[0,0],[0,0],[0,0]],"OoB":"N","num":0,"Qd":0,"Hmd":1,"Homing":1,"pause":0}
    )strDelim"
};


static const char* UnitTestHomingSeq_Test2_Config = R"strDelim( 
    {"robotType":"SandTableScaraMatt","cmdsAtStart":"g28","webui":"SandUI","evaluators":{"thrContinue":0},
    "robotGeom":{"model":"SingleArmScara","homing":{"homingSeq":"FS1000;A-10000N;B+10000#;A-10000n;B+10000S2000;#;FR60;A+470;B-470;#;B-10000S2200N;#;B10000R120n;#;B+850R240;#;A=h;B=h;$","maxHomingSecs":120},
    "blockDistanceMM":1,"allowOutOfBounds":0,"stepEnablePin":"12","stepEnLev":0,"stepDisableSecs":10,
    "axis0":{"maxSpeed":50,"maxAcc":50,"maxRPM":20,"stepsPerRot":9600,"unitsPerRot":628.318,"maxVal":190,"stepPin":"14","dirnPin":"32","endStop0":{"sensePin":"36","actLvl":0,"inputType":"INPUT_PULLUP"}},
    "axis1":{"maxSpeed":50,"maxAcc":50,"stepsPerRot":9600,"unitsPerRot":628.318,"maxRPM":30,"maxVal":190,"stepPin":"33","dirnPin":"15","endStop0":{"sensePin":"39","actLvl":0,"inputType":"INPUT_PULLUP"}}},
    "fileManager":{"spiffsEnabled":1,"spiffsFormatIfCorrupt":1,"sdEnabled":1,"sdMOSI":"18","sdMISO":"19","sdCLK":"5","sdCS":"21"},
    "wifiLed":{"hwPin":"14","onLevel":1,"onMs":200,"shortOffMs":200,"longOffMs":750},"ledStrip":{"ledPin":"4","sensorPin":"34"}}
    )strDelim";
    
static char const* UnitTestHomingSeq_Test2_DebugCmdStrs[] = {
    "Move",
    "Move",
    "Move",
    "Move",
    "Move",
    "Move",
    "Done",
};
static char const* UnitTestHomingSeq_Test2_JsonRslts[] = {
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[-10000,10000,0],"F":1000.00,"mv":"rel","end":[[1,0],[0,0],[0,0]],"OoB":"Y","num":10007,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[-10000,10000,0],"F":2000.00,"mv":"rel","end":[[2,0],[0,0],[0,0]],"OoB":"Y","num":10008,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[470,-470,0],"F":9600.00,"mv":"rel","end":[[0,0],[0,0],[0,0]],"OoB":"Y","num":10009,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[0,-10000,0],"F":2200.00,"mv":"rel","end":[[0,0],[1,0],[0,0]],"OoB":"Y","num":10010,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[0,10000,0],"F":19200.00,"mv":"rel","end":[[0,0],[2,0],[0,0]],"OoB":"Y","num":10011,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[0,850,0],"F":38400.00,"mv":"rel","end":[[0,0],[0,0],[0,0]],"OoB":"Y","num":10012,"Qd":0,"Hmd":0,"Homing":1,"pause":0}
    )strDelim",
    R"strDelim(
        {"XYZ":[0.00,0.00,0.00],"ABC":[0,0,0],"mv":"rel","end":[[0,0],[0,0],[0,0]],"OoB":"N","num":0,"Qd":0,"Hmd":1,"Homing":1,"pause":0}
    )strDelim"
};

class UnitTestHomingSeq
{
public:
    

    void runTests()
    {
        MotionHelper* pMotionHelper = new MotionHelper();
        MotionHoming* pMotionHoming = pMotionHelper->testGetMotionHoming();

        TEST_ASSERT_FALSE(pMotionHoming->_commandInProgress);
        TEST_ASSERT_EQUAL_STRING("", pMotionHoming->_homingSequence.c_str());

        // Prep for tests
        String debugCmdStr;
        String homingJson;
        AxesParams axesParams;
        RobotCommandArgs homingCommand;
        homingCommand.setAxisValMM(0,0,true);
        homingCommand.setAxisValMM(1,0,true);

        /////////////////////////////////////////////////////////////////////////////////////
        //// Test Set 1
        /////////////////////////////////////////////////////////////////////////////////////

        Serial.println("UnitTestHomingSeq Test Set 1");

        // Set config
        pMotionHelper->configure(UnitTestHomingSeq_Test1_Config);
        axesParams = pMotionHelper->getAxesParams();

        // Test homing
        pMotionHoming->homingStart(homingCommand);

        // Loop through expected responses
        for (int i = 0; i < sizeof(UnitTestHomingSeq_Test1_DebugCmdStrs)/sizeof(const char*); i++)
        {
            String testRlstStr = UnitTestHomingSeq_Test1_JsonRslts[i];
            testRlstStr.trim();
            pMotionHoming->_curCommand.clear();
            TEST_ASSERT_TRUE(pMotionHoming->extractAndExecNextCmd(axesParams, debugCmdStr));
            homingJson = pMotionHoming->_curCommand.toJSON();
            Serial.println(homingJson);
            TEST_ASSERT_EQUAL_STRING(UnitTestHomingSeq_Test1_DebugCmdStrs[i], debugCmdStr.c_str());
            TEST_ASSERT_EQUAL_STRING(testRlstStr.c_str(), homingJson.c_str());
        }

        /////////////////////////////////////////////////////////////////////////////////////
        //// Test Set 2
        /////////////////////////////////////////////////////////////////////////////////////

        Serial.println("UnitTestHomingSeq Test Set 2");

        // Set config
        pMotionHelper->configure(UnitTestHomingSeq_Test2_Config);
        axesParams = pMotionHelper->getAxesParams();

        // Test homing
        pMotionHoming->homingStart(homingCommand);

        // Loop through expected responses
        for (int i = 0; i < sizeof(UnitTestHomingSeq_Test2_DebugCmdStrs)/sizeof(const char*); i++)
        {
            String testRlstStr = UnitTestHomingSeq_Test2_JsonRslts[i];
            testRlstStr.trim();
            pMotionHoming->_curCommand.clear();
            TEST_ASSERT_TRUE(pMotionHoming->extractAndExecNextCmd(axesParams, debugCmdStr));
            homingJson = pMotionHoming->_curCommand.toJSON();
            Serial.println(homingJson);
            TEST_ASSERT_EQUAL_STRING(UnitTestHomingSeq_Test2_DebugCmdStrs[i], debugCmdStr.c_str());
            TEST_ASSERT_EQUAL_STRING(testRlstStr.c_str(), homingJson.c_str());
        }

        // Cleanup
        delete pMotionHelper;
    }

};