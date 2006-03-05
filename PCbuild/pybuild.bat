call "%VS71COMNTOOLS%vsvars32.bat"
devenv.exe /build Debug /out build.txt PCbuild\pcbuild.sln
type build.txt
del build.txt

