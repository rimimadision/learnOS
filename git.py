#!/usr/bin/python3

import os
import sys

os.chdir("..")
os.system("ssh-agent -s")
os.system("ssh-add 20020112")
os.chdir("learnOS")
os.system("git add .")
os.system("git commit -m \"" + sys.argv[1] + "\"")
os.system("git push")
