
typedef struct TEST_POSN_TYPE { double x; double y; };
static TEST_POSN_TYPE testPositions[] =
    {
        {0,0}, {10,10}, {10,0}, {0,10}, {-10,0}, {-10,-10}, {-10,10}, {10,-10}
    };

class TestWorkflowGCode
{
public:

    long millisRateIn = 6000;
    long millisRateOut = 1000;
    long lastMillisIn = 0;
    long lastMillisOut = 0;
    long lastMillisFlip = 0;
    bool hasHomed = false;
    bool busyNotified = false;
    long initialMemory = System.freeMemory();
    long lowestMemory = System.freeMemory();
    double curX = 0;
    double curY = 0;
    int testPosIdx = 0;

    static const bool TEST_ADD_TO_QUEUE = false;
    static const bool TEST_REMOVE_FROM_QUEUE = false;
    static const bool TEST_FLIP_ADD_REMOVE_RATES = false;

    void testLoop(WorkItemQueue& workflowManager, RobotController& robotController)
    {
        if (!hasHomed)
        {
            String workItemStr = "G28 XYZ";
            bool rslt = false;
            rslt = workflowManager.add(workItemStr);
            Log.notice("Add %s %d", workItemStr.c_str(), rslt);
            hasHomed = true;
        }

        if (TEST_ADD_TO_QUEUE && (millis() > lastMillisIn + millisRateIn))
        {
            curX = testPositions[testPosIdx].x;
            curY = testPositions[testPosIdx].y;
            testPosIdx++;
            if (testPosIdx >= (sizeof(testPositions)/sizeof(TEST_POSN_TYPE)))
                testPosIdx = 0;
            String workItemStr = "G01 X" + String(curX) + "Y" + String(curY);
            bool rslt = workflowManager.add(workItemStr);
            Log.notice("Add %d", rslt);
            lastMillisIn = millis();
            if (lowestMemory > System.freeMemory())
                lowestMemory = System.freeMemory();
        }

        if (TEST_REMOVE_FROM_QUEUE && (millis() > lastMillisOut + millisRateOut))
        {
            if (robotController.canAcceptCommand())
            {
                WorkItem workItem;
                bool rslt = workflowManager.get(workItem);
                if (rslt)
                {
                    Log.notice("");
                    Log.notice("Get %d = %s, initMem %d, mem %d, lowMem %d", rslt,
                                    workItem.getString().c_str(), initialMemory,
                                    System.freeMemory(), lowestMemory);
                    EvaluatorGCode::interpretGcode(workItem, robotController, true);
                }
                else
                {
                    //Log.notice("Get none available");
                }
                lastMillisOut = millis();
                busyNotified = false;
            }
            else if (!busyNotified)
            {
                Log.notice("Robot queue full");
                busyNotified = true;
            }
        }

        if (TEST_FLIP_ADD_REMOVE_RATES && (millis() > lastMillisFlip + 30000))
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
