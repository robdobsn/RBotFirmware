#include "ConfigManager.h"

static const char *TEST_JSON_STRING_001 =
    "{\"user\": \"johndoe\", \"admin\": false, \"uid\": 1000,\n  "
    "\"things\": [\"users\", \"wheel\", \"audio\", \"video\"], "
    "\"groups\": {\"my-array\" : [1,2,3,4], \"my-str\":\"TESTSTR\"}}";

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

    static const char *TEST_JSON_STRING_003 =
    "{\n"
    "\"testdata\":\n"
    "[\n"
    "  {\n"
    "    \"_id\": \"5866af200999160ee078d8de\",\n"
    "    \"index\": 0,\n"
    "    \"guid\": \"20e5721c-4353-4b04-81e7-804fa072d9bd\",\n"
    "    \"isActive\": true,\n"
    "    \"balance\": \"$3,091.09\",\n"
    "    \"picture\": \"http://placehold.it/32x32\",\n"
    "    \"age\": 28,\n"
    "    \"eyeColor\": \"blue\",\n"
    "    \"name\": \"Doreen Todd\",\n"
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
    "  },\n"
    "  {\n"
    "    \"_id\": \"5866af20e8078c5385587e27\",\n"
    "    \"index\": 1,\n"
    "    \"guid\": \"30503624-d3b2-4930-a4a7-bb0f34b18074\",\n"
    "    \"isActive\": false,\n"
    "    \"balance\": \"$3,647.98\",\n"
    "    \"picture\": \"http://placehold.it/32x32\",\n"
    "    \"age\": 22,\n"
    "    \"eyeColor\": \"blue\",\n"
    "    \"name\": \"Leta Lindsay\",\n"
    "    \"gender\": \"female\",\n"
    "    \"company\": \"PORTALIS\",\n"
    "    \"email\": \"letalindsay@portalis.com\",\n"
    "    \"phone\": \"+1 (873) 588-2131\",\n"
    "    \"address\": \"766 Harwood Place, Freetown, New Jersey, 6565\",\n"
    "    \"about\": \"Nulla exercitation veniam magna sint anim qui ex aliqua excepteur duis aliqua commodo. Non Lorem mollit tempor laborum magna. Adipisicing dolor reprehenderit voluptate eiusmod consectetur adipisicing ea id incididunt do labore. Dolor adipisicing cupidatat est ullamco irure veniam aliquip commodo reprehenderit in. Laborum laboris culpa dolor est. Dolor aute consequat consectetur dolore.\\r\\n\",\n"
    "    \"registered\": \"2014-03-28T10:56:16 -00:00\",\n"
    "    \"latitude\": -57.772093,\n"
    "    \"longitude\": -74.130886,\n"
    "    \"tags\": [\n"
    "      \"occaecat\",\n"
    "      \"sit\",\n"
    "      \"incididunt\",\n"
    "      \"officia\",\n"
    "      \"duis\",\n"
    "      \"reprehenderit\",\n"
    "      \"sunt\"\n"
    "    ],\n"
    "    \"friends\": [\n"
    "      {\n"
    "        \"id\": 0,\n"
    "        \"name\": \"Samantha Payne\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 1,\n"
    "        \"name\": \"Curry Puckett\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 2,\n"
    "        \"name\": \"Curtis Tanner\"\n"
    "      }\n"
    "    ],\n"
    "    \"greeting\": \"Hello, Leta Lindsay! You have 4 unread messages.\",\n"
    "    \"favoriteFruit\": \"banana\"\n"
    "  },\n"
    "  {\n"
    "    \"_id\": \"5866af2070294366e8258804\",\n"
    "    \"index\": 2,\n"
    "    \"guid\": \"0eae1d5a-0b0c-43be-80be-561ecd2f82c5\",\n"
    "    \"isActive\": true,\n"
    "    \"balance\": \"$1,913.87\",\n"
    "    \"picture\": \"http://placehold.it/32x32\",\n"
    "    \"age\": 40,\n"
    "    \"eyeColor\": \"blue\",\n"
    "    \"name\": \"Charlene Richardson\",\n"
    "    \"gender\": \"female\",\n"
    "    \"company\": \"EYERIS\",\n"
    "    \"email\": \"charlenerichardson@eyeris.com\",\n"
    "    \"phone\": \"+1 (848) 541-2694\",\n"
    "    \"address\": \"945 Williams Avenue, Fairfield, Hawaii, 2046\",\n"
    "    \"about\": \"Aliquip proident ea aute nisi tempor consectetur minim pariatur cupidatat. Ex dolore sint elit nulla. Ipsum est incididunt ex velit occaecat culpa quis culpa sint elit Lorem pariatur. Excepteur eu sint ipsum amet id eu amet.\\r\\n\",\n"
    "    \"registered\": \"2014-04-25T05:59:14 -01:00\",\n"
    "    \"latitude\": -84.923481,\n"
    "    \"longitude\": 77.946799,\n"
    "    \"tags\": [\n"
    "      \"consequat\",\n"
    "      \"eu\",\n"
    "      \"consectetur\",\n"
    "      \"cupidatat\",\n"
    "      \"nisi\",\n"
    "      \"ullamco\",\n"
    "      \"velit\"\n"
    "    ],\n"
    "    \"friends\": [\n"
    "      {\n"
    "        \"id\": 0,\n"
    "        \"name\": \"Jordan Atkins\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 1,\n"
    "        \"name\": \"Roxanne Dalton\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 2,\n"
    "        \"name\": \"Bray Mooney\"\n"
    "      }\n"
    "    ],\n"
    "    \"greeting\": \"Hello, Charlene Richardson! You have 8 unread messages.\",\n"
    "    \"favoriteFruit\": \"strawberry\"\n"
    "  },\n"
    "  {\n"
    "    \"_id\": \"5866af20daf501e4ed34e44e\",\n"
    "    \"index\": 3,\n"
    "    \"guid\": \"76989c15-c0a4-4b81-a6b3-f494856710f4\",\n"
    "    \"isActive\": true,\n"
    "    \"balance\": \"$1,814.48\",\n"
    "    \"picture\": \"http://placehold.it/32x32\",\n"
    "    \"age\": 25,\n"
    "    \"eyeColor\": \"brown\",\n"
    "    \"name\": \"Pratt Slater\",\n"
    "    \"gender\": \"male\",\n"
    "    \"company\": \"SATIANCE\",\n"
    "    \"email\": \"prattslater@satiance.com\",\n"
    "    \"phone\": \"+1 (921) 504-3794\",\n"
    "    \"address\": \"539 Greenwood Avenue, Crayne, South Dakota, 7491\",\n"
    "    \"about\": \"Amet elit et adipisicing ad fugiat Lorem reprehenderit sunt voluptate sit consectetur. Exercitation ut voluptate reprehenderit consectetur reprehenderit voluptate aliquip sint occaecat reprehenderit. Veniam irure id aliquip aute ad est et elit dolore duis anim. Irure irure ex adipisicing sint nostrud eiusmod velit nostrud sint adipisicing magna id est dolore. Do mollit nostrud reprehenderit consectetur laborum tempor anim irure nisi consectetur ex et. Velit excepteur ut cillum ea adipisicing. Incididunt culpa duis laborum qui deserunt do pariatur.\\r\\n\",\n"
    "    \"registered\": \"2015-02-20T02:55:00 -00:00\",\n"
    "    \"latitude\": -86.039488,\n"
    "    \"longitude\": 33.450148,\n"
    "    \"tags\": [\n"
    "      \"consequat\",\n"
    "      \"elit\",\n"
    "      \"exercitation\",\n"
    "      \"id\",\n"
    "      \"veniam\",\n"
    "      \"ipsum\",\n"
    "      \"ut\"\n"
    "    ],\n"
    "    \"friends\": [\n"
    "      {\n"
    "        \"id\": 0,\n"
    "        \"name\": \"Juliet Miles\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 1,\n"
    "        \"name\": \"Alma Delaney\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 2,\n"
    "        \"name\": \"Steele Osborn\"\n"
    "      }\n"
    "    ],\n"
    "    \"greeting\": \"Hello, Pratt Slater! You have 2 unread messages.\",\n"
    "    \"favoriteFruit\": \"strawberry\"\n"
    "  },\n"
    "  {\n"
    "    \"_id\": \"5866af20f18099dfcf70375a\",\n"
    "    \"index\": 4,\n"
    "    \"guid\": \"9e621b10-fe6e-4f8d-92e6-8bdf734dad01\",\n"
    "    \"isActive\": true,\n"
    "    \"balance\": \"$3,535.61\",\n"
    "    \"picture\": \"http://placehold.it/32x32\",\n"
    "    \"age\": 28,\n"
    "    \"eyeColor\": \"blue\",\n"
    "    \"name\": \"Melissa Saunders\",\n"
    "    \"gender\": \"female\",\n"
    "    \"company\": \"FILODYNE\",\n"
    "    \"email\": \"melissasaunders@filodyne.com\",\n"
    "    \"phone\": \"+1 (990) 469-2402\",\n"
    "    \"address\": \"102 Dekoven Court, Golconda, Rhode Island, 7271\",\n"
    "    \"about\": \"Enim elit do sint incididunt deserunt aliquip sunt tempor occaecat laborum laboris esse. Aute deserunt excepteur ea irure dolore officia id. Officia officia consequat sunt ipsum sit veniam consectetur in exercitation elit labore dolore irure commodo. Exercitation consequat magna nulla labore nostrud consequat qui duis eiusmod.\\r\\n\",\n"
    "    \"registered\": \"2015-05-23T02:44:55 -01:00\",\n"
    "    \"latitude\": -35.15275,\n"
    "    \"longitude\": -10.42528,\n"
    "    \"tags\": [\n"
    "      \"ad\",\n"
    "      \"tempor\",\n"
    "      \"nisi\",\n"
    "      \"consequat\",\n"
    "      \"eu\",\n"
    "      \"labore\",\n"
    "      \"adipisicing\"\n"
    "    ],\n"
    "    \"friends\": [\n"
    "      {\n"
    "        \"id\": 0,\n"
    "        \"name\": \"Amy Newman\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 1,\n"
    "        \"name\": \"Cash Steele\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 2,\n"
    "        \"name\": \"Todd Green\"\n"
    "      }\n"
    "    ],\n"
    "    \"greeting\": \"Hello, Melissa Saunders! You have 9 unread messages.\",\n"
    "    \"favoriteFruit\": \"banana\"\n"
    "  },\n"
    "  {\n"
    "    \"_id\": \"5866af2076d93e1241df83f0\",\n"
    "    \"index\": 5,\n"
    "    \"guid\": \"10cdaf3b-241e-4f1f-a69a-e3aaa2861af1\",\n"
    "    \"isActive\": true,\n"
    "    \"balance\": \"$3,822.25\",\n"
    "    \"picture\": \"http://placehold.it/32x32\",\n"
    "    \"age\": 30,\n"
    "    \"eyeColor\": \"blue\",\n"
    "    \"name\": \"Natalia Holland\",\n"
    "    \"gender\": \"female\",\n"
    "    \"company\": \"GROK\",\n"
    "    \"email\": \"nataliaholland@grok.com\",\n"
    "    \"phone\": \"+1 (960) 487-3691\",\n"
    "    \"address\": \"172 Eastern Parkway, Cumminsville, Florida, 1541\",\n"
    "    \"about\": \"Et et dolore est irure. Ex officia adipisicing quis officia reprehenderit nostrud occaecat ipsum ex amet voluptate. Ut deserunt adipisicing non esse voluptate velit culpa esse dolor. Ut laboris sit pariatur nostrud veniam incididunt deserunt. Officia nulla elit amet minim ad. Lorem quis adipisicing adipisicing tempor nulla esse irure.\\r\\n\",\n"
    "    \"registered\": \"2015-02-04T03:36:50 -00:00\",\n"
    "    \"latitude\": 3.399893,\n"
    "    \"longitude\": -52.91519,\n"
    "    \"tags\": [\n"
    "      \"qui\",\n"
    "      \"qui\",\n"
    "      \"laboris\",\n"
    "      \"eu\",\n"
    "      \"Lorem\",\n"
    "      \"elit\",\n"
    "      \"in\"\n"
    "    ],\n"
    "    \"friends\": [\n"
    "      {\n"
    "        \"id\": 0,\n"
    "        \"name\": \"Sherrie Medina\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 1,\n"
    "        \"name\": \"Beverley Trujillo\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 2,\n"
    "        \"name\": \"Tamera Torres\"\n"
    "      }\n"
    "    ],\n"
    "    \"greeting\": \"Hello, Natalia Holland! You have 9 unread messages.\",\n"
    "    \"favoriteFruit\": \"banana\"\n"
    "  },\n"
    "  {\n"
    "    \"_id\": \"5866af20db85681e566de9d5\",\n"
    "    \"index\": 6,\n"
    "    \"guid\": \"aa214dd8-e746-42c9-bde5-7bc9a5626f9d\",\n"
    "    \"isActive\": false,\n"
    "    \"balance\": \"$3,236.28\",\n"
    "    \"picture\": \"http://placehold.it/32x32\",\n"
    "    \"age\": 33,\n"
    "    \"eyeColor\": \"blue\",\n"
    "    \"name\": \"Jordan Dale\",\n"
    "    \"gender\": \"male\",\n"
    "    \"company\": \"BLUEGRAIN\",\n"
    "    \"email\": \"jordandale@bluegrain.com\",\n"
    "    \"phone\": \"+1 (885) 504-3887\",\n"
    "    \"address\": \"616 Louise Terrace, Kersey, Indiana, 6306\",\n"
    "    \"about\": \"Esse ipsum commodo exercitation occaecat fugiat voluptate in. Reprehenderit voluptate esse Lorem irure incididunt sunt minim ad incididunt dolor. Dolor adipisicing et nulla eiusmod ullamco veniam. Tempor ex anim dolor aute aute laboris ad sint aliquip ex qui. Proident aliqua reprehenderit ad reprehenderit non.\\r\\n\",\n"
    "    \"registered\": \"2014-07-19T10:38:48 -01:00\",\n"
    "    \"latitude\": 86.676303,\n"
    "    \"longitude\": 86.381161,\n"
    "    \"tags\": [\n"
    "      \"et\",\n"
    "      \"ipsum\",\n"
    "      \"nostrud\",\n"
    "      \"exercitation\",\n"
    "      \"nisi\",\n"
    "      \"nisi\",\n"
    "      \"esse\"\n"
    "    ],\n"
    "    \"friends\": [\n"
    "      {\n"
    "        \"id\": 0,\n"
    "        \"name\": \"Jensen Love\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 1,\n"
    "        \"name\": \"Shari Barron\"\n"
    "      },\n"
    "      {\n"
    "        \"id\": 2,\n"
    "        \"name\": \"Elsa Chaney\"\n"
    "      }\n"
    "    ],\n"
    "    \"greeting\": \"Hello, Jordan Dale! You have 7 unread messages.\",\n"
    "    \"favoriteFruit\": \"strawberry\"\n"
    "  }\n"
    "]\n"
    "}";

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
        if (t->type == JSMN_PRIMITIVE)
        {
            Serial.printf("\n\r#Found primitive size %d, start %d, end %d\n\r",
                    t->size, t->start, t->end);
            Serial.printf("%.*s", t->end - t->start, js + t->start);
            return 1;
        }
        else if (t->type == JSMN_STRING)
        {
            Serial.printf("\n\r#Found string size %d, start %d, end %d\n\r",
                    t->size, t->start, t->end);
            Serial.printf("'%.*s'", t->end - t->start, js + t->start);
            return 1;
        }
        else if (t->type == JSMN_OBJECT)
        {
            Serial.printf("\n\r#Found object size %d, start %d, end %d\n\r",
                    t->size, t->start, t->end);
            j = 0;
            for (i = 0; i < t->size; i++)
            {
                for (k = 0; k < indent; k++)
                {
                    Serial.printf("  ");
                }
                j += dump(js, t + 1 + j, count - j, indent + 1);
                Serial.printf(": ");
                j += dump(js, t + 1 + j, count - j, indent + 1);
                Serial.printf("\n\r");
            }
            return j + 1;
        }
        else if (t->type == JSMN_ARRAY)
        {
            Serial.printf("\n\r#Found array size %d, start %d, end %d\n\r",
                    t->size, t->start, t->end);
            j = 0;
            Serial.printf("\n\r");
            for (i = 0; i < t->size; i++)
            {
                for (k = 0; k < indent - 1; k++)
                {
                    Serial.printf("  ");
                }
                Serial.printf("   - ");
                j += dump(js, t + 1 + j, count - j, indent + 1);
                Serial.printf("\n\r");
            }
            return j + 1;
        }
        return 0;
    }



    static bool doPrint(const char* jsonStr)
    {
        jsmn_parser parser;
        jsmn_init(&parser);
        int tokenCountRslt = jsmn_parse(&parser, jsonStr, strlen(jsonStr),
                    NULL, 1000);
        if (tokenCountRslt < 0)
        {
            RD_ERR("Failed to parse JSON: %d", tokenCountRslt);
            return false;
        }
        jsmntok_t* pTokens = new jsmntok_t[tokenCountRslt];
        jsmn_init(&parser);
        tokenCountRslt = jsmn_parse(&parser, jsonStr, strlen(jsonStr),
                    pTokens, tokenCountRslt);
        if (tokenCountRslt < 0)
        {
            RD_ERR("Failed to parse JSON: %d", tokenCountRslt);
            delete pTokens;
            return false;
        }
        // Top level item must be an object
        if (tokenCountRslt < 1 || pTokens[0].type != JSMN_OBJECT)
        {
            RD_ERR("JSON must have top level object");
            delete pTokens;
            return false;
        }
        RD_DBG("Dumping");
        // Reserve the same amount of space as the original string
        String outStr;
        outStr.reserve(strlen(jsonStr));
        ConfigManager::recreateJson(jsonStr, pTokens, parser.toknext, 0, outStr);
        delete pTokens;
        RD_DBG("---------------------------------");
        RD_DBG("RECREATED");
        RD_DBG("%s", outStr.c_str());
        return true;
    }

    static bool testGets()
    {
        /*for (int i = 0; i < 1000000; i++)
        {
            bool isValid = false;
            String cStr = configManager.getString("source", "", isValid);
            if (isValid == false || i % 100000 == 0)
            {
                Serial.printlnf("CStr %s, valid %d", cStr.c_str(), isValid);
                uint32_t freemem = System.freeMemory();
                Serial.print("free memory: ");
                Serial.println(freemem);
            }
        }*/


    }

    static void runTests(ConfigManager& configManager)
    {
        TestConfigManager testConfigManager;
        doPrint(TEST_JSON_STRING_001);


        bool isValid = false;
        jsmntype_t objType = JSMN_UNDEFINED;
        // String myStr = configManager.getString("groups/my-str", "NOTFOUND",
        //                     isValid, objType, TEST_JSON_STRING_001);
        // Serial.printlnf("Result str %d = %s", isValid, myStr.c_str());

        // String myStr = configManager.getString("groups", "NOTFOUND", isValid,
        //                     objType, TEST_JSON_STRING_001);
        // Serial.printlnf("Result str %d = %s", isValid, myStr.c_str());

        String myStr = configManager.getString("info", "NOTFOUND", isValid,
                            objType, TEST_JSON_STRING_002);
        Serial.printlnf("Result str %d = %s", isValid, myStr.c_str());

    }
};
