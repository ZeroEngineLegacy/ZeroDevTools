echo off

pushd ..\..\
SET ZERO_SOURCE=%CD%
SET ZERO_OUTPUT=%ZERO_SOURCE%\BuildOutput
popd

call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat"
