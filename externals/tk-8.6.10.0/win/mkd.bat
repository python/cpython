@echo off

if exist %1\nul goto end

md %1
if errorlevel 1 goto end

echo Created directory %1

:end


