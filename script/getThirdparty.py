#!/usr/bin/env python

'''
    This script tries to automatically select the right thirdparty for your
    case and gets it from Github.
    Run with -i to select the right Thirdparty by yourself.
'''

import os
import shutil
import sys
import urllib
import zipfile

ZIP_SUFFIX = "/archive/master.zip"
VS13_URL = "https://github.com/monsdar/CxxProf-Thirdparty-vc120"

KNOWN_URLS = []
KNOWN_URLS.append(VS13_URL)

#set pathes
DEV_PATH = os.path.dirname(os.path.realpath(__file__)) + '/../'

def selectThirdparty():
    print ""
    print "Available Thirdparty packages:"
    print ""

    for index, url in enumerate(KNOWN_URLS):
        print str(index) + "\t- " + url
    input = raw_input("Please select the Thirdparty you want to use: ")
    
    inputNumber = 0;
    try:
        inputNumber = int(input)
    except ValueError:
        print "Please enter a valid number!"
        exit(1)
        
    if( inputNumber >= len(KNOWN_URLS) or inputNumber < 0 ):
        print "Your input was not in list!"
        exit(1)
        
    return KNOWN_URLS[inputNumber]

def findThirdparty():
    #this is currently just needed for travis-ci.org
    #to get it working with other systems we should test for OS etc
    if(os.environ.get("CXX") == "g++"):
        print "No g++ environment available for now"
        exit(1)
    elif(os.environ.get("CXX") == "clang++"):
        print "No clang++ environment available for now"
        exit(1)
    else:
        #return VS13 by default... this should be changed
        return VS13_URL
        
def downloadThirdparty(url):
    print "Downloading " + url.split('/')[-1] + " --- patience please..."
    urllib.urlretrieve(url, "master.zip")
    
def unpackThirdparty():
    fileHandle = open('master.zip', 'rb')
    zipHandle = zipfile.ZipFile(fileHandle)
    print "Extracting master.zip with " + str(len(zipHandle.namelist())) + " files, this could take some time..."
    zipHandle.extractall()
    fileHandle.close()
    
def moveThirdparty(thirdpartyUrl):
    repoName = thirdpartyUrl.split('/')[-1]
    dirName = repoName + "-master"
    try:
        shutil.move(dirName + "/thirdparty", "../")
    except:
        pass

def main():
    thirdpartyUrl = ""
    if( "-i" in sys.argv ):
        thirdpartyUrl = selectThirdparty()
    else:
        thirdpartyUrl = findThirdparty()

    downloadThirdparty(thirdpartyUrl + ZIP_SUFFIX)
    unpackThirdparty()
    moveThirdparty(thirdpartyUrl)

    print ""
    print "================================="
    print "==== THIRDPARTY SUCCESSFUL ======"
    print "================================="
        
if __name__=="__main__":
    main()

