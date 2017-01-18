
#pragma push_macro("RD_DEBUG_FNAME")
#define RD_DEBUG_FNAME "TestWorkflowGCode.h"
#include "RdDebugLevel.h"

class TestWorkflowGCode
{
public:

    long millisRateIn = 10000;
    long millisRateOut = 5000;
    long lastMillisIn = 0;
    long lastMillisOut = 0;
    long lastMillisFlip = 0;
    bool hasHomed = false;
    bool busyNotified = false;
    long initialMemory = System.freeMemory();
    long lowestMemory = System.freeMemory();
    double curX = 0;
    double curY = 0;

    void testLoop(WorkflowManager& workflowManager, RobotController& robotController)
    {
        if (!hasHomed)
        {
            String cmdStr = "G28 XYZ";
            bool rslt = workflowManager.add(cmdStr);
            Log.info("Add %s %d", cmdStr.c_str(), rslt);
            hasHomed = true;
        }
        if (millis() > lastMillisIn + millisRateIn)
        {
            String cmdStr = "G01 X" + String(curX) + "Y" + String(curY);
            curY += 100;
            bool rslt = workflowManager.add(cmdStr);
            Log.info("Add %d", rslt);
            lastMillisIn = millis();
            if (lowestMemory > System.freeMemory())
                lowestMemory = System.freeMemory();
        }

        if (millis() > lastMillisOut + millisRateOut)
        {
            if (!robotController.isBusy())
            {
                CommandElem cmdElem;
                bool rslt = workflowManager.get(cmdElem);
                if (rslt)
                {
                    Log.info("Get %d = %s, initMem %d, mem %d, lowMem %d", rslt,
                                    cmdElem.getString().c_str(), initialMemory,
                                    System.freeMemory(), lowestMemory);
                    GCodeInterpreter::interpretGcode(cmdElem, robotController, true);
                }
                else
                {
                    Log.info("Get none available");
                }
                lastMillisOut = millis();
                busyNotified = false;
            }
            else if (!busyNotified)
            {
                Log.info("Robot Busy");
                busyNotified = true;
            }
        }

        if (millis() > lastMillisFlip + 30000)
        {
            if (millisRateOut > 3000)
                millisRateOut = 1000;
            else
                millisRateOut = 4000;
            lastMillisFlip = millis();
        }
    }
};

TestWorkflowGCode __testWorkflowGCode;

#pragma pop_macro("RD_DEBUG_FNAME")
