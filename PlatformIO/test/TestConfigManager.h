
#include "ConfigManager.h"

static const char *TEST_JSON_STRING_001 =
    "{\"user\": \"johndoe\", \"admin\": false, \"uid\": 1000,\n  "
    "\"things\": [\"users\", \"wheel\", \"audio\", \"video\"], "
    "\"groups\": {\"my-array\" : [1,2,3,4], \"my-str\":\"TESTSTR\"}}";

// For this JSON (above) the contents are as follows:
//     type  strt   end  size content
//   OBJECT     0   154     5 {"user": "johndoe", "admin" : false, "uid" : 1000, "thnigs" : ["users", "wheel", "audio", "video"], "groups" : {"my-array" : [1, 2, 3, 4], "my-str" : "TESTSTR"}}
//   STRING     2     6     1 user
//   STRING    10    17     0 johndoe
//   STRING    21    26     1 admin
//PRIMITIVE    29    34     0 false
//   STRING    37    40     1 uid
//PRIMITIVE    43    47     0 1000
//   STRING    52    58     1 thnigs
//    ARRAY    61    97     4["users", "wheel", "audio", "video"]
//   STRING    63    68     0 users
//   STRING    72    77     0 wheel
//   STRING    81    86     0 audio
//   STRING    90    95     0 video
//   STRING   100   106     1 groups
//   OBJECT   109   153     2 {"my-array" : [1, 2, 3, 4], "my-str" : "TESTSTR"}
//   STRING   111   119     1 my - array
//    ARRAY   123   132     4[1, 2, 3, 4]
//PRIMITIVE   124   125     0 1
//PRIMITIVE   126   127     0 2
//PRIMITIVE   128   129     0 3
//PRIMITIVE   130   131     0 4
//   STRING   135   141     1 my - str
//   STRING   144   151     0 TESTSTR


    static const char *TEST_JSON_STRING_002 =
    "{\n"
    "    \"_id\": \"5866af200999160ee078d8de\",\n"
    "    \"index\": 0,\n"
    "    \"guid\": \"20e5721c-4353-4b04-81e7-804fa072d9bd\",\n"
    "    \"isActive\": true,\n"
    "    \"balance\": \"$3,091.09\",\n"
    "    \"picture\": \"http://placehold.it/32x32\",\n"
    "    \"age\": 28,\n"
    "    \"eyeColor\": \"blue\",\n"
    "    \"name\": \"Doreen Todd\",\n"
    "    \"info\": {\n"
    "\t\"pet\": {\n"
    "\t    \"type\": \"wolf\",\n"
    "            \"breed\": \"grey\",\n"
    "\t    \"name\": \"colin\"\n"
    "\t\t}\n"
    "\t},\n"
    "    \"gender\": \"female\",\n"
    "    \"company\": \"TOURMANIA\",\n"
    "    \"email\": \"doreentodd@tourmania.com\",\n"
    "    \"phone\": \"+1 (984) 578-2853\",\n"
    "    \"address\": \"958 Seeley Street, Walton, District Of Columbia, 9376\",\n"
    "    \"about\": \"Nostrud quis ipsum excepteur esse minim irure ullamco deserunt qui laboris. Consequat nisi amet veniam dolor est quis minim excepteur ex pariatur dolor. Do ullamco enim minim quis nulla exercitation adipisicing culpa minim non labore. Elit ad esse tempor dolor consectetur ex proident.\\r\\n\",\n"
    "    \"registered\": \"2014-06-04T11:52:17 -01:00\",\n"
    "    \"latitude\": 46.992565,\n"
    "    \"longitude\": -48.927624,\n"
    "    \"tags\": [\n"
    "      \"ex\",\n"
    "      \"veniam\",\n"
    "      \"sint\",\n"
    "      \"Lorem\",\n"
    "      \"aute\",\n"
    "      \"elit\",\n"
    "      \"occaecat\"\n"
    "    ],\n"
    "    \"friends\": [\n"
    "      {\n"
    "        \"id\": 0,\n"
    "        \"name\": \"Walters Flores\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 1,\n"
    "        \"name\": \"Gilmore Bray\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 2,\n"
    "        \"name\": \"Pugh Macias\"\n"
    "      }\n"
    "    ],\n"
    "    \"greeting\": \"Hello, Doreen Todd! You have 4 unread messages.\",\n"
    "    \"favoriteFruit\": \"apple\"\n"
    "  }";

    //index            type  strt   end  size content
    //00000          OBJECT     0  1084    23 {"_id":"5866af200999160ee078d8de", "index ...
    //00001          STRING     2     5     1 _id
    //00002          STRING     8    32     0 5866af200999160ee078d8de
    //00003          STRING    35    40     1 index
    //00004       PRIMITIVE    42    43     0 0
    //00005          STRING    45    49     1 guid
    //00006          STRING    52    88     0 20e5721c - 4353 - 4b04 - 81e7 - 804fa072d9bd
    //00007          STRING    91    99     1 isActive
    //00008       PRIMITIVE   101   105     0 true
    //00009          STRING   107   114     1 balance
    //00010          STRING   117   126     0 $3, 091.09
    //00011          STRING   129   136     1 picture
    //00012          STRING   139   164     0 http://placehold.it/32x32
    //00013          STRING   167   170     1 age
    //00014       PRIMITIVE   172   174     0 28
    //00015          STRING   176   184     1 eyeColor
    //00016          STRING   187   191     0 blue
    //00017          STRING   194   198     1 name
    //00018          STRING   201   212     0 Doreen Todd
    //00019          STRING   215   219     1 info
    //00020          OBJECT   221   274     1 {"pet":{"type":"wolf", "breed" : "grey", "na ...
    //00021          STRING   223   226     1 pet
    //00022          OBJECT   228   273     3 {"type":"wolf", "breed" : "grey", "name" : "co ...
    //00023          STRING   230   234     1 type
    //00024          STRING   237   241     0 wolf
    //00025          STRING   244   249     1 breed
    //00026          STRING   252   256     0 grey
    //00027          STRING   259   263     1 name
    //00028          STRING   266   271     0 colin
    //00029          STRING   276   282     1 gender
    //00030          STRING   285   291     0 female
    //00031          STRING   294   301     1 company
    //00032          STRING   304   313     0 TOURMANIA
    //00033          STRING   316   321     1 email
    //00034          STRING   324   348     0 doreentodd@tourmania.com
    //00035          STRING   351   356     1 phone
    //00036          STRING   359   376     0 + 1 (984) 578 - 2853
    //00037          STRING   379   386     1 address
    //00038          STRING   389   442     0 958 Seeley Street, Walton, District Of C ...
    //00039          STRING   445   450     1 about
    //00040          STRING   453   742     0 Nostrud quis ipsum excepteur esse minim  ...
    //00041          STRING   745   755     1 registered
    //00042          STRING   758   784     0 2014 - 06 - 04T11:52 : 17 - 01 : 00
    //00043          STRING   787   795     1 latitude
    //00044       PRIMITIVE   797   806     0 46.992565
    //00045          STRING   808   817     1 longitude
    //00046       PRIMITIVE   819   829     0 - 48.927624
    //00047          STRING   831   835     1 tags
    //00048           ARRAY   837   892     7["ex", "veniam", "sint", "Lorem", "aute", "el ...
    //00049          STRING   839   841     0 ex
    //00050          STRING   844   850     0 veniam
    //00051          STRING   853   857     0 sint
    //00052          STRING   860   865     0 Lorem
    //00053          STRING   868   872     0 aute
    //00054          STRING   875   879     0 elit
    //00055          STRING   882   890     0 occaecat
    //00056          STRING   894   901     1 friends
    //00057           ARRAY   903   998     3[{"id":0, "name" : "Walters Flores"}, { "id": ...
    //00058          OBJECT   904   936     2 {"id":0,"name" : "Walters Flores"}
    //00059          STRING   906   908     1 id
    //00060       PRIMITIVE   910   911     0 0
    //00061          STRING   913   917     1 name
    //00062          STRING   920   934     0 Walters Flores
    //00063          OBJECT   937   967     2 {"id":1,"name" : "Gilmore Bray"}
    //00064          STRING   939   941     1 id
    //00065       PRIMITIVE   943   944     0 1
    //00066          STRING   946   950     1 name
    //00067          STRING   953   965     0 Gilmore Bray
    //00068          OBJECT   968   997     2 {"id":2,"name" : "Pugh Macias"}
    //00069          STRING   970   972     1 id
    //00070       PRIMITIVE   974   975     0 2
    //00071          STRING   977   981     1 name
    //00072          STRING   984   995     0 Pugh Macias
    //00073          STRING  1000  1008     1 greeting
    //00074          STRING  1011  1058     0 Hello, Doreen Todd!You have 4 unread me ...
    //00075          STRING  1061  1074     1 favoriteFruit
    //00076          STRING  1077  1082     0 apple

class TestConfigManager
{
public:

    static int dump(const char *js, jsmntok_t *t, size_t count, int indent)
    {
        int i, j, k;

        if (count == 0)
        {
            return 0;
        }
        if (t->type == JSMNR_PRIMITIVE)
        {
            Log.trace("#Found primitive size %d, start %d, end %d",
                    t->size, t->start, t->end);
            Log.trace("%.*s", t->end - t->start, js + t->start);
            return 1;
        }
        else if (t->type == JSMNR_STRING)
        {
            Log.trace("#Found string size %d, start %d, end %d",
                    t->size, t->start, t->end);
            Log.trace("'%.*s'", t->end - t->start, js + t->start);
            return 1;
        }
        else if (t->type == JSMNR_OBJECT)
        {
            Log.trace("#Found object size %d, start %d, end %d",
                    t->size, t->start, t->end);
            j = 0;
            for (i = 0; i < t->size; i++)
            {
                for (k = 0; k < indent; k++)
                {
                    Log.trace("  ");
                }
                j += dump(js, t + 1 + j, count - j, indent + 1);
                Log.trace(": ");
                j += dump(js, t + 1 + j, count - j, indent + 1);
                Log.trace("\n\r");
            }
            return j + 1;
        }
        else if (t->type == JSMNR_ARRAY)
        {
            Log.trace("#Found array size %d, start %d, end %d",
                    t->size, t->start, t->end);
            j = 0;
            Log.trace("-----");
            for (i = 0; i < t->size; i++)
            {
                for (k = 0; k < indent - 1; k++)
                {
                    Log.trace("  ");
                }
                Log.trace("   - ");
                j += dump(js, t + 1 + j, count - j, indent + 1);
                Log.trace("");
            }
            return j + 1;
        }
        return 0;
    }

    static bool doPrint(const char* jsonStr)
    {
        JSMNR_parser parser;
        JSMNR_init(&parser);
        int tokenCountRslt = JSMNR_parse(&parser, jsonStr, strlen(jsonStr),
                    NULL, 1000);
        if (tokenCountRslt < 0)
        {
            Log.error("Failed to parse JSON: %d", tokenCountRslt);
            return false;
        }
        jsmntok_t* pTokens = new jsmntok_t[tokenCountRslt];
        JSMNR_init(&parser);
        tokenCountRslt = JSMNR_parse(&parser, jsonStr, strlen(jsonStr),
                    pTokens, tokenCountRslt);
        if (tokenCountRslt < 0)
        {
            Log.error("Failed to parse JSON: %d", tokenCountRslt);
            delete pTokens;
            return false;
        }
        // Top level item must be an object
        if (tokenCountRslt < 1 || pTokens[0].type != JSMNR_OBJECT)
        {
            Log.error("JSON must have top level object");
            delete pTokens;
            return false;
        }
        Log.trace("Dumping toknext = %d", parser.toknext);
        // Reserve the same amount of space as the original string
        String outStr;
        outStr.reserve(strlen(jsonStr));
        dump(jsonStr, pTokens, parser.toknext, 0);
        delete pTokens;
        Log.trace("---------------------------------");
        Log.trace("RECREATED");
        Log.trace("%s", outStr.c_str());
        return true;
    }

    static bool displayFreeMem()
    {
        uint32_t freemem = System.freeMemory();
        Log.notice("Free memory: %d", freemem);
    }

    static bool testGets()
    {
        bool isValid = false;
    	jsmntype_t objType = JSMNR_UNDEFINED;
    	int objSize = 0;
        bool allValid = true;
        for (int i = 0; i < 1000000; i++)
        {
            String cStr = ConfigManager::getString("things", "NOTFOUND",
        		isValid, objType, objSize, TEST_JSON_STRING_001);
            if (!isValid)
                allValid = false;
            if (i % 100000 == 0)
            {
                Log.trace("CStr %s, allvalid %d", cStr.c_str(), allValid);
                displayFreeMem();
            }
        }
    }

    static void dumpTokens(const char* origStr, jsmntok_t* pTokens, int numTokens)
    {
    	Log.trace("%5s %15s %5s %5s %5s %s", "idx", "type", "strt", "end", "size", "content");
    	for (int i = 0; i < numTokens; i++)
    	{
    		jsmntok_t* pTok = pTokens + i;
    		int toShow = pTok->end - pTok->start;
    		char* elipStr = "";
    		if (toShow > 40)
    		{
    			toShow = 40;
    			elipStr = "...";
    		}
    		Log.trace("%05d %15s %5d %5d %5d %.*s %s", i,
    						ConfigManager::getObjType(pTok->type),
    						pTok->start, pTok->end, pTok->size, toShow, origStr+pTok->start,
    						elipStr)
    	}
    }

    static void runTests()
    {
        int numTokens = 0;
        int strippedLen = ConfigManager::safeStringLen(TEST_JSON_STRING_002, true);
    	char pStrippedStr[strippedLen+1];
    	ConfigManager::safeStringCopy(pStrippedStr, TEST_JSON_STRING_002, 10000, true);
    	jsmntok_t* pTokens = ConfigManager::parseJson(pStrippedStr, numTokens);
    	dumpTokens(pStrippedStr, pTokens, numTokens);

    	bool isValid = false;
    	jsmntype_t objType = JSMNR_UNDEFINED;
    	int objSize = 0;
    	bool testSuccess = true;
    	String myStr, myVal;
    	int testIdx = 0;
    	bool thisTestResult = true;

        Log.trace("---------------------------------");
        displayFreeMem();

    	Log.trace("---------------------------------");
    	thisTestResult = true;
    	myStr = configManager.getString("groups/my-str", "NOTFOUND",
    		isValid, objType, objSize, TEST_JSON_STRING_001);
    	Log.trace("Result str valid %d, type %s, size %d, contents %s",
    		isValid, ConfigManager::getObjType(objType), objSize, myStr.c_str());
    	if (!myStr.equals("TESTSTR"))
    		thisTestResult = false;
    	Log.trace("TEST %d: %s", ++testIdx, thisTestResult ? "PASSED" : "FAILED");
    	testSuccess &&= thisTestResult;

    	Log.trace("---------------------------------");
    	thisTestResult = true;
    	myStr = configManager.getString("groups", "NOTFOUND", isValid,
    		objType, objSize, TEST_JSON_STRING_001);
    	Log.trace("Result str valid %d, type %s, size %d, contents %s",
    		isValid, ConfigManager::getObjType(objType), objSize, myStr.c_str());
    	if (objSize != 2)
    		thisTestResult = false;
    	if (objType != JSMNR_OBJECT)
    		thisTestResult = false;
    	Log.trace("TEST %d: %s", ++testIdx, thisTestResult ? "PASSED" : "FAILED");
    	testSuccess &&= thisTestResult;

    	Log.trace("---------------------------------");
    	thisTestResult = true;
    	myStr = configManager.getString("info", "NOTFOUND", isValid,
    		objType, objSize, TEST_JSON_STRING_002);
    	Log.trace("Middle object valid %d, type %s, size %d, contents %s",
    		isValid, ConfigManager::getObjType(objType), objSize, myStr.c_str());
    	if (objSize != 1)
    		thisTestResult = false;
    	if (objType != JSMNR_OBJECT)
    		thisTestResult = false;
    	myVal = configManager.getString("pet/type", "NOTFOUND2", isValid,
    		objType, objSize, myStr.c_str());
    	Log.trace("Result str valid %d, type %s, size %d, contents %s",
    		isValid, ConfigManager::getObjType(objType), objSize, myVal.c_str());
    	if (!myVal.equals("wolf"))
    		thisTestResult = false;
    	if (objType != JSMNR_STRING)
    		thisTestResult = false;
    	Log.trace("TEST %d: %s", ++testIdx, thisTestResult ? "PASSED" : "FAILED");
    	testSuccess &&= thisTestResult;

    	Log.trace("---------------------------------");
    	thisTestResult = true;
    	myStr = configManager.getString("tags", "NOTFOUND", isValid,
    		objType, objSize, TEST_JSON_STRING_002);
    	Log.trace("Middle object valid %d, type %s, size %d, contents %s",
    		isValid, ConfigManager::getObjType(objType), objSize, myStr.c_str());
    	if (objSize != 7)
    		thisTestResult = false;
    	if (objType != JSMNR_ARRAY)
    		thisTestResult = false;
    	 myVal = configManager.getString("[1]", "NOTFOUND2", isValid,
    	                     objType, objSize, myStr.c_str());
    	 Log.trace("Result str valid %d, type %s, size %d, contents %s",
    	                     isValid, ConfigManager::getObjType(objType), objSize, myVal.c_str());
    	 if (!myVal.equals("veniam"))
    	     thisTestResult = false;
    	 Log.trace("TEST %d: %s", ++testIdx, thisTestResult ? "PASSED" : "FAILED");
    	 testSuccess &&= thisTestResult;

    	Log.trace("---------------------------------");
    	thisTestResult = true;
    	myStr = configManager.getString("info", "NOTFOUND",
    		isValid, objType, objSize, pStrippedStr);
    	Log.trace("Result str valid %d, type %s, size %d, len %d, contents %s",
    		isValid, ConfigManager::getObjType(objType), objSize, myStr.length(), myStr.c_str());
    	if (myStr.length() != 53)
    		thisTestResult = false;
    	Log.trace("TEST %d: %s", ++testIdx, thisTestResult ? "PASSED" : "FAILED");
    	testSuccess &&= thisTestResult;

    	Log.trace("---------------------------------");
    	thisTestResult = true;
    	myStr = configManager.getString("friends[1]/name", "NOTFOUND",
    		isValid, objType, objSize, pStrippedStr);
    	Log.trace("Result str valid %d, type %s, size %d, contents %s",
    		isValid, ConfigManager::getObjType(objType), objSize, myStr.c_str());
    	if (!myStr.equals("GilmoreBray"))
    		thisTestResult = false;
    	Log.trace("TEST %d: %s", ++testIdx, thisTestResult ? "PASSED" : "FAILED");
    	testSuccess &&= thisTestResult;

    	Log.trace("---------------------------------");
    	Log.trace("");
    	Log.trace("TEST RESULTS: %s", testSuccess ? "PASSED" : "FAILED");

        Log.trace("---------------------------------");
        testGets();

        Log.trace("---------------------------------");
        displayFreeMem();
    }
};
