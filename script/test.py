
import fnmatch
import os
import subprocess
import sys


#set pathes
DEV_PATH = os.path.dirname(os.path.realpath(__file__)) + '/../'
BUILD_PATH = DEV_PATH + "/build/"
INSTALL_PATH = DEV_PATH + "/install/"
THIRDPARTY_PATH = DEV_PATH + "/thirdparty/"
CTEST_EXE = DEV_PATH + "/thirdparty/cmake/ctest.exe"

def main():
    #set the PATH to find Thirdparty
    os.environ['PATH'] = THIRDPARTY_PATH + '/boost/bin/'
    os.environ['PATH'] = os.environ['PATH'] + ';' + THIRDPARTY_PATH + '/cmake/'
    
    #find our own components, set the PATH for them
    matches = []
    for root, dirnames, filenames in os.walk( INSTALL_PATH ):
        for filename in fnmatch.filter(dirnames, 'bin'):
            matches.append(os.path.join(root, filename))
    for path in matches:
        os.environ['PATH'] = os.environ['PATH'] + ';' + path

    #search for projects which need to be tested
    for root, dirnames, filenames in os.walk( BUILD_PATH ):
        for filename in fnmatch.filter(filenames, 'CTestTestfile.cmake'):            
            os.chdir( root )
            
            #run the tests
            testCmd = []
            testCmd.append(CTEST_EXE)
            testCmd.append("--no-compress-output")
            testCmd.append("-T")
            testCmd.append("Test")
            testCmd.append(".")
            process = subprocess.Popen(testCmd)
            process.wait()
            print "Testing failed with errorcode: " + str(process.returncode)

    print ""
    print "================================="
    print "====== TESTS SUCCESSFUL ========="
    print "================================="
    
if __name__=="__main__":
    main()