call "%VS71COMNTOOLS%vsvars32.bat"
devenv.exe /clean Debug /out clean.txt PCbuild\pcbuild.sln
type clean.txt
del clean.txt

