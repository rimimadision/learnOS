#!/usr/bin/python3

import os
import sys

os.system("git add .");
os.system("git commit -m \"" + sys.argv[1] + "\"");
os.system("git push");
