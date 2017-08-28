call SetUpBuildEnvironment

MD %ZERO_OUTPUT%\Doxygen
SET DoxyFile="%ZERO_SOURCE%\DevTools\Doxygen\DoxyFile"

IF NOT EXIST %DoxyFile% ( echo %DoxyFile% does not exist & GOTO :END )

REM Run each library through doxygen separately. This is significantly faster!
SET DoxyOut="%ZERO_OUTPUT%\Doxygen\ZeroLibraries"
SET Inputs="%ZERO_SOURCE%\ZeroLibraries"
echo Generating doxygen
( type %DoxyFile% & echo OUTPUT_DIRECTORY=%DoxyOut% & echo INPUT=%Inputs% & echo GENERATE_XML=YES) | doxygen.exe -

SET DoxyOut="%ZERO_OUTPUT%\Doxygen\Systems"
SET Inputs="%ZERO_SOURCE%\Systems"
echo Generating doxygen
( type %DoxyFile% & echo OUTPUT_DIRECTORY=%DoxyOut% & echo INPUT=%Inputs% & echo GENERATE_XML=YES) | doxygen.exe -

SET DoxyOut="%ZERO_OUTPUT%\Doxygen\Extensions"
SET Inputs="%ZERO_SOURCE%\Extensions"
echo Generating doxygen
 ( type %DoxyFile% & echo OUTPUT_DIRECTORY=%DoxyOut% & echo INPUT=%Inputs% & echo GENERATE_XML=YES) | doxygen.exe -
 
 SET DoxyOut="%ZERO_OUTPUT%\Doxygen\Projects"
SET Inputs="%ZERO_SOURCE%\Projects"
echo Generating doxygen
 ( type %DoxyFile% & echo OUTPUT_DIRECTORY=%DoxyOut% & echo INPUT=%Inputs% & echo GENERATE_XML=YES) | doxygen.exe -
 