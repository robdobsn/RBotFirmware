// RBotFirmware
// Rob Dobson 2016-2018

#include "RobotConfigurations.h"

const char *RobotConfigurations::_robotConfigs[] = {

    "{\"robotType\":\"MugBot\",\"xMaxMM\":150,\"yMaxMM\":120,"
    "\"homingSeq\":\"B-x;B+r9X;B-1.0;B=h$\","
    "\"cmdsAtStart\":\"\","
    "\"stepEnablePin\":\"A2\",\"stepEnLev\":1,\"stepDisableSecs\":60.0,"
    "\"axis0\":{\"stepPin\":\"D2\",\"dirnPin\":\"D3\",\"maxSpeed\":10.0,\"maxAcc\":5.0,"
    "\"stepsPerRot\":6400,\"unitsPerRot\":360,\"minVal\":-360,\"maxVal\":360},"
    "\"axis1\":{\"stepPin\":\"D4\",\"dirnPin\":\"D5\",\"maxSpeed\":5.0,\"maxAcc\":5.0,"
    "\"stepsPerRot\":3200,\"unitsPerRot\":2.0,\"minVal\":0,\"maxVal\":100,"
    "\"homeOffsetVal\":100,"
    "\"endStop1\":{\"sensePin\":\"A7\",\"actLvl\":0,\"inputType\":\"INPUT_PULLUP\"}},"
    "\"axis2\":{\"servoPin\":\"D0\",\"isServoAxis\":1,\"isPrimaryAxis\": 0,\"homeOffsetVal\":180,\"homeOffSteps\":2500,"
    "\"minVal\":0,\"maxVal\":180,\"stepsPerRot\":2000,\"unitsPerRot\":360}"
    "}",

    // static const char* ROBOT_CONFIG_STR_GEISTBOT =
    //     "{\"robotType\":\"GeistBot\",\"xMaxMM\":400,\"yMaxMM\":400, "
    //     " \"stepEnablePin\":\"A2\",\"stepEnLev\":1,\"stepDisableSecs\":1.0,"
    //     " \"maxHomingSecs\":120,\"homingLinOffsetDegs\":70,\"homingCentreOffsetMM\":4,"
    //     " \"homingRotCentreDegs\":3.7,\"cmdsAtStart\":\"G28;ModSpiral\", "
    //     " \"axis0\": { \"stepPin\":\"D2\",\"dirnPin\":\"D3\",\"maxSpeed\":75.0,\"maxAcc\":5.0,"
    //     " \"stepsPerRot\":12000,\"unitsPerRot\":360,\"isDominantAxis\":1,"
    //     " \"endStop0\": { \"sensePin\":\"A6\",\"actLvl\":1,\"inputType\":\"INPUT_PULLUP\"}},"
    //     " \"axis1\": { \"stepPin\":\"D4\",\"dirnPin\":\"D5\",\"maxSpeed\":75.0,\"maxAcc\":5.0,"
    //     " \"stepsPerRot\":12000,\"unitsPerRot\":44.8,\"minVal\":0,\"maxVal\":195, "
    //     " \"endStop0\": { \"sensePin\":\"A7\",\"actLvl\":0,\"inputType\":\"INPUT_PULLUP\"}},"
    //     "}";

    "{\"robotType\":\"SandTableScara\",\"xMaxMM\":195,\"yMaxMM\":195, "
    "\"cmdsAtStart\":\"\","
    "\"homingSeq\":\"A-10000n;B10000;#;A+10000N;B-10000;#;A+500;B-500;#;B+10000n;#;B-10000N;#;B-1050;#;A=h;B=h;$\","
    // "\"homingSeq\":\"A-10000n;B10000;#;A=h;B=h;$\","
    "\"maxHomingSecs\":120,"
    "\"stepEnablePin\":\"4\",\"stepEnLev\":1,\"stepDisableSecs\":1,"
    "\"blockDistanceMM\":1,"
    "\"axis0\":{\"stepPin\":\"14\",\"dirnPin\":\"13\",\"maxSpeed\":75,\"maxAcc\":50,"
    "\"stepsPerRot\":9600,\"unitsPerRot\":628.318,"
    "\"endStop0\":{\"sensePin\":\"36\",\"actLvl\":0,\"inputType\":\"INPUT_PULLUP\"}},"
    "\"axis1\":{\"stepPin\":\"15\",\"dirnPin\":\"21\",\"maxSpeed\":75,\"maxAcc\":50,"
    "\"stepsPerRot\":9600,\"unitsPerRot\":628.318,"
    "\"endStop0\":{\"sensePin\":\"39\",\"actLvl\":0,\"inputType\":\"INPUT_PULLUP\"}}"
    "}",

    // static const char* ROBOT_CONFIG_STR_AIRHOCKEY =
    //     "{\"robotType\":\"HockeyBot\",\"xMaxMM\":350,\"yMaxMM\":400, "
    //     " \"stepEnablePin\":\"A2\",\"stepEnLev\":1,\"stepDisableSecs\":1.0,"
    //     " \"cmdsAtStart\":\"\", "
    //     " \"axis0\": { \"stepPin\":\"D2\",\"dirnPin\":\"D3\",\"maxSpeed\":5000.0,\"maxAcc\":5000.0,"
    //     " \"stepsPerRot\":3200,\"unitsPerRot\":62},"
    //     " \"axis1\": { \"stepPin\":\"D4\",\"dirnPin\":\"D5\",\"maxSpeed\":5000.0,\"maxAcc\":5000.0,"
    //     " \"stepsPerRot\":3200,\"unitsPerRot\":62}"
    //     "}";

    "{\"robotType\":\"XYBot\",\"xMaxMM\":500,\"yMaxMM\":500, "
    "\"stepEnablePin\":\"A2\",\"stepEnLev\":1,\"stepDisableSecs\":1.0,"
    "\"cmdsAtStart\":\"\", "
    "\"axis0\":{\"stepPin\":\"D2\",\"dirnPin\":\"D3\",\"maxSpeed\":100.0,\"maxAcc\":10.0,"
    "\"stepsPerRot\":3200,\"unitsPerRot\":32},"
    "\"axis1\":{\"stepPin\":\"D4\",\"dirnPin\":\"D5\",\"maxSpeed\":100.0,\"maxAcc\":10.0,"
    "\"stepsPerRot\":3200,\"unitsPerRot\":32},"
    "\"workItemQueue\":{\"maxLen\":50}"
    "}"};

const int RobotConfigurations::_numRobotConfigurations = sizeof(RobotConfigurations::_robotConfigs) / sizeof(const char *);
