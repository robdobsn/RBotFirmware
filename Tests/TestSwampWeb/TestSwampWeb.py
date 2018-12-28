

import requests
import time

for i in range(100):
    r = requests.get("http://192.168.86.25/sandUI.html")
    time.sleep(0.1)
