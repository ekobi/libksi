@ECHO OFF

CALL "%ProgramW6432%\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

REM CURL_DIR and OPENSSL_DIR must contain include and lib directories. 
REM Runntime MT and MTd supported


SET CURL_DIR=C:\Users\Taavi\Documents\GuardTime\LIBS\curl-7.37.0
SET OPENSSL_DIR=C:\Users\Taavi\Documents\GuardTime\LIBS\openssl-0.9.8g-win64
SET OPENSSL_CA_FILE=C:\Users\Taavi\Documents\GuardTime\ksicapi\test\resource\tlv\mock.crt
SET RUNNTIME=MTd

nmake clean
nmake RTL="%RUNNTIME%" CURL_DIR="%CURL_DIR%" OPENSSL_CA_FILE="%OPENSSL_CA_FILE%" OPENSSL_DIR="%OPENSSL_DIR%" all

pause