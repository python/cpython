"Read and write ZIP files."
# Written by James C. Ahlstrom jim@interet.com
# All rights transferred to CNRI pursuant to the Python contribution agreement

import struct, os, time
import binascii

try:
    import zlib	# We may need its compression method
except:
    zlib = None

class BadZipfile(Exception):
    pass
error = BadZipfile	# The exception raised by this module

# constants for Zip file compression methods
ZIP_STORED = 0
ZIP_DEFLATED = 8
# Other ZIP compression methods not supported

# Here are some struct module formats for reading headers
structEndArchive = "<4s4H2lH"     # 9 items, end of archive, 22 bytes
stringEndArchive = "PK\005\006"   # magic number for end of archive record
structCentralDir = "<4s4B4H3l5H2l"# 19 items, central directory, 46 bytes
stringCentralDir = "PK\001\002"   # magic number for central directory
structFileHeader = "<4s2B4H3l2H"  # 12 items, file header record, 30 bytes
stringFileHeader = "PK\003\004"   # magic number for file header


def is_zipfile(filename):
    """Quickly see if file is a ZIP file by checking the magic number.

    Will not accept a ZIP archive with an ending comment.
    """
    try:
        fpin = open(filename, "rb")
        fpin.seek(-22, 2)		# Seek to end-of-file record
        endrec = fpin.read()
        fpin.close()
        if endrec[0:4] == "PK\005\006" and endrec[-2:] == "\000\000":
            return 1	# file has correct magic number
    except:
        pass


class ZipInfo:
    """Class with attributes describing each file in the ZIP archive."""

    def __init__(self, filename="NoName", date_time=(1980,1,1,0,0,0)):
        self.filename = filename	# Name of the file in the archive
        self.date_time = date_time	# year, month, day, hour, min, sec
        # Standard values:
        self.compress_type = ZIP_STORED	# Type of compression for the file
        self.comment = ""		# Comment for each file
        self.extra = ""			# ZIP extra data
        self.create_system = 0		# System which created ZIP archive
        self.create_version = 20	# Version which created ZIP archive
        self.extract_version = 20	# Version needed to extract archive
        self.reserved = 0		# Must be zero
        self.flag_bits = 0		# ZIP flag bits
        self.volume = 0			# Volume number of file header
        self.internal_attr = 0		# Internal attributes
        self.external_attr = 0		# External file attributes
        # Other attributes are set by class ZipFile:
        # header_offset		Byte offset to the file header
        # file_offset		Byte offset to the start of the file data
        # CRC			CRC-32 of the uncompressed file
        # compress_size		Size of the compressed file
        # file_size		Size of the uncompressed file

    def FileHeader(self):
        """Return the per-file header as a string."""
        dt = self.date_time
        dosdate = (dt[0] - 1980) << 9 | dt[1] << 5 | dt[2]
        dostime = dt[3] << 11 | dt[4] << 5 | dt[5] / 2
        if self.flag_bits & 0x08:
          # Set these to zero because we write them after the file data
          CRC = compress_size = file_size = 0
        else:
          CRC = self.CRC
          compress_size = self.compress_size
          file_size = self.file_size
        header = struct.pack(structFileHeader, stringFileHeader,
                 self.extract_version, self.reserved, self.flag_bits,
                 self.compress_type, dostime, dosdate, CRC,
                 compress_size, file_size,
                 len(self.filename), len(self.extra))
        return header + self.filename + self.extra


class ZipFile:
    """Class with methods to open, read, write, close, list zip files."""

    def __init__(self, filename, mode="r", compression=ZIP_STORED):
        """Open the ZIP file with mode read "r", write "w" or append "a"."""
        if compression == ZIP_STORED:
            pass
        elif compression == ZIP_DEFLATED:
            if not zlib:
                raise RuntimeError,\
                      "Compression requires the (missing) zlib module"
        else:
            raise RuntimeError, "That compression method is not supported"
        self.debug = 0	# Level of printing: 0 through 3
        self.NameToInfo = {}	# Find file info given name
        self.filelist = []	# List of ZipInfo instances for archive
        self.compression = compression	# Method of compression
        self.filename = filename
        self.mode = key = mode[0]
        if key == 'r':
            self.fp = open(filename, "rb")
            self._GetContents()
        elif key == 'w':
            self.fp = open(filename, "wb")
        elif key == 'a':
            fp = self.fp = open(filename, "r+b")
            fp.seek(-22, 2)		# Seek to end-of-file record
            endrec = fp.read()
            if endrec[0:4] == stringEndArchive and \
                       endrec[-2:] == "\000\000":
                self._GetContents()	# file is a zip file
                # seek to start of directory and overwrite
                fp.seek(self.start_dir, 0)
            else:		# file is not a zip file, just append
                fp.seek(0, 2)
        else:
            raise RuntimeError, 'Mode must be "r", "w" or "a"'

    def _GetContents(self):
        """Read in the table of contents for the ZIP file."""
        fp = self.fp
        fp.seek(-22, 2)		# Start of end-of-archive record
        filesize = fp.tell() + 22	# Get file size
        endrec = fp.read(22)	# Archive must not end with a comment!
        if endrec[0:4] != stringEndArchive or endrec[-2:] != "\000\000":
            raise BadZipfile, "File is not a zip file, or ends with a comment"
        endrec = struct.unpack(structEndArchive, endrec)
        if self.debug > 1:
            print endrec
        size_cd = endrec[5]		# bytes in central directory
        offset_cd = endrec[6]	# offset of central directory
        x = filesize - 22 - size_cd
        # "concat" is zero, unless zip was concatenated to another file
        concat = x - offset_cd
        if self.debug > 2:
            print "given, inferred, offset", offset_cd, x, concat
        # self.start_dir:  Position of start of central directory
        self.start_dir = offset_cd + concat
        fp.seek(self.start_dir, 0)
        total = 0
        while total < size_cd:
            centdir = fp.read(46)
            total = total + 46
            if centdir[0:4] != stringCentralDir:
                raise BadZipfile, "Bad magic number for central directory"
            centdir = struct.unpack(structCentralDir, centdir)
            if self.debug > 2:
                print centdir
            filename = fp.read(centdir[12])
            # Create ZipInfo instance to store file information
            x = ZipInfo(filename)
            x.extra = fp.read(centdir[13])
            x.comment = fp.read(centdir[14])
            total = total + centdir[12] + centdir[13] + centdir[14]
            x.header_offset = centdir[18] + concat
            x.file_offset = x.header_offset + 30 + centdir[12] + centdir[13]
            (x.create_version, x.create_system, x.extract_version, x.reserved,
                x.flag_bits, x.compress_type, t, d,
                x.CRC, x.compress_size, x.file_size) = centdir[1:12]
            x.volume, x.internal_attr, x.external_attr = centdir[15:18]
            # Convert date/time code to (year, month, day, hour, min, sec)
            x.date_time = ( (d>>9)+1980, (d>>5)&0xF, d&0x1F,
                                     t>>11, (t>>5)&0x3F, (t&0x1F) * 2 )
            self.filelist.append(x)
            self.NameToInfo[x.filename] = x
            if self.debug > 2:
                print "total", total
        for data in self.filelist:
            fp.seek(data.header_offset, 0)
            fheader = fp.read(30)
            if fheader[0:4] != stringFileHeader:
                raise BadZipfile, "Bad magic number for file header"
            fheader = struct.unpack(structFileHeader, fheader)
            fname = fp.read(fheader[10])
            if fname != data.filename:
                raise RuntimeError, \
                      'File name in directory "%s" and header "%s" differ.' % (
                          data.filename, fname)

    def namelist(self):
        """Return a list of file names in the archive."""
        l = []
        for data in self.filelist:
            l.append(data.filename)
        return l

    def infolist(self):
        """Return a list of class ZipInfo instances for files in the
        archive."""
        return self.filelist

    def printdir(self):
        """Print a table of contents for the zip file."""
        print "%-46s %19s %12s" % ("File Name", "Modified    ", "Size")
        for zinfo in self.filelist:
            date = "%d-%02d-%02d %02d:%02d:%02d" % zinfo.date_time
            print "%-46s %s %12d" % (zinfo.filename, date, zinfo.file_size)

    def testzip(self):
        """Read all the files and check the CRC."""
        for zinfo in self.filelist:
            try:
                self.read(zinfo.filename)	# Check CRC-32
            except:
                return zinfo.filename

    def getinfo(self, name):
        """Return the instance of ZipInfo given 'name'."""
        return self.NameToInfo[name]

    def read(self, name):
        """Return file bytes (as a string) for name."""
        if self.mode not in ("r", "a"):
            raise RuntimeError, 'read() requires mode "r" or "a"'
        if not self.fp:
            raise RuntimeError, \
                  "Attempt to read ZIP archive that was already closed"
        zinfo = self.getinfo(name)
        filepos = self.fp.tell()
        self.fp.seek(zinfo.file_offset, 0)
        bytes = self.fp.read(zinfo.compress_size)
        self.fp.seek(filepos, 0)
        if zinfo.compress_type == ZIP_STORED:
            pass
        elif zinfo.compress_type == ZIP_DEFLATED:
            if not zlib:
                raise RuntimeError, \
                      "De-compression requires the (missing) zlib module"
            # zlib compress/decompress code by Jeremy Hylton of CNRI
            dc = zlib.decompressobj(-15)
            bytes = dc.decompress(bytes)
            # need to feed in unused pad byte so that zlib won't choke
            ex = dc.decompress('Z') + dc.flush()
            if ex:
                bytes = bytes + ex
        else:
            raise BadZipfile, \
                  "Unsupported compression method %d for file %s" % \
            (zinfo.compress_type, name)
        crc = binascii.crc32(bytes)
        if crc != zinfo.CRC:
            raise BadZipfile, "Bad CRC-32 for file %s" % name
        return bytes

    def _writecheck(self, zinfo):
        """Check for errors before writing a file to the archive."""
        if self.NameToInfo.has_key(zinfo.filename):
            if self.debug:	# Warning for duplicate names
                print "Duplicate name:", zinfo.filename
        if self.mode not in ("w", "a"):
            raise RuntimeError, 'write() requires mode "w" or "a"'
        if not self.fp:
            raise RuntimeError, \
                  "Attempt to write ZIP archive that was already closed"
        if zinfo.compress_type == ZIP_DEFLATED and not zlib:
            raise RuntimeError, \
                  "Compression requires the (missing) zlib module"
        if zinfo.compress_type not in (ZIP_STORED, ZIP_DEFLATED):
            raise RuntimeError, \
                  "That compression method is not supported"

    def write(self, filename, arcname=None, compress_type=None):
        """Put the bytes from filename into the archive under the name
        arcname."""
        st = os.stat(filename)
        mtime = time.localtime(st[8])
        date_time = mtime[0:6]
        # Create ZipInfo instance to store file information
        if arcname is None:
          zinfo = ZipInfo(filename, date_time)
        else:
          zinfo = ZipInfo(arcname, date_time)
        zinfo.external_attr = st[0] << 16	# Unix attributes
        if compress_type is None:
          zinfo.compress_type = self.compression
        else:
          zinfo.compress_type = compress_type
        self._writecheck(zinfo)
        fp = open(filename, "rb")
        zinfo.flag_bits = 0x08
        zinfo.header_offset = self.fp.tell()	# Start of header bytes
        self.fp.write(zinfo.FileHeader())
        zinfo.file_offset = self.fp.tell()	# Start of file bytes
        CRC = 0
        compress_size = 0
        file_size = 0
        if zinfo.compress_type == ZIP_DEFLATED:
            cmpr = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION,
                 zlib.DEFLATED, -15)
        else:
            cmpr = None
        while 1:
            buf = fp.read(1024 * 8)
            if not buf:
                break
            file_size = file_size + len(buf)
            CRC = binascii.crc32(buf, CRC)
            if cmpr:
                buf = cmpr.compress(buf)
                compress_size = compress_size + len(buf)
            self.fp.write(buf)
        fp.close()
        if cmpr:
            buf = cmpr.flush()
            compress_size = compress_size + len(buf)
            self.fp.write(buf)
            zinfo.compress_size = compress_size
        else:
            zinfo.compress_size = file_size
        zinfo.CRC = CRC
        zinfo.file_size = file_size
        # Write CRC and file sizes after the file data
        self.fp.write(struct.pack("<lll", zinfo.CRC, zinfo.compress_size,
              zinfo.file_size))
        self.filelist.append(zinfo)
        self.NameToInfo[zinfo.filename] = zinfo

    def writestr(self, zinfo, bytes):
        """Write a file into the archive.  The contents is the string
        'bytes'."""
        self._writecheck(zinfo)
        zinfo.file_size = len(bytes)		# Uncompressed size
        zinfo.CRC = binascii.crc32(bytes)	# CRC-32 checksum
        if zinfo.compress_type == ZIP_DEFLATED:
            co = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION,
                 zlib.DEFLATED, -15)
            bytes = co.compress(bytes) + co.flush()
            zinfo.compress_size = len(bytes)	# Compressed size
        else:
            zinfo.compress_size = zinfo.file_size
        zinfo.header_offset = self.fp.tell()	# Start of header bytes
        self.fp.write(zinfo.FileHeader())
        zinfo.file_offset = self.fp.tell()	# Start of file bytes
        self.fp.write(bytes)
        if zinfo.flag_bits & 0x08:
          # Write CRC and file sizes after the file data
          self.fp.write(struct.pack("<lll", zinfo.CRC, zinfo.compress_size,
                zinfo.file_size))
        self.filelist.append(zinfo)
        self.NameToInfo[zinfo.filename] = zinfo

    def __del__(self):
        """Call the "close()" method in case the user forgot."""
        if self.fp:
            self.fp.close()
            self.fp = None

    def close(self):
        """Close the file, and for mode "w" and "a" write the ending
        records."""
        if self.mode in ("w", "a"):		# write ending records
            count = 0
            pos1 = self.fp.tell()
            for zinfo in self.filelist:		# write central directory
                count = count + 1
                dt = zinfo.date_time
                dosdate = (dt[0] - 1980) << 9 | dt[1] << 5 | dt[2]
                dostime = dt[3] << 11 | dt[4] << 5 | dt[5] / 2
                centdir = struct.pack(structCentralDir,
                  stringCentralDir, zinfo.create_version,
                  zinfo.create_system, zinfo.extract_version, zinfo.reserved,
                  zinfo.flag_bits, zinfo.compress_type, dostime, dosdate,
                  zinfo.CRC, zinfo.compress_size, zinfo.file_size,
                  len(zinfo.filename), len(zinfo.extra), len(zinfo.comment),
                  0, zinfo.internal_attr, zinfo.external_attr,
                  zinfo.header_offset)
                self.fp.write(centdir)
                self.fp.write(zinfo.filename)
                self.fp.write(zinfo.extra)
                self.fp.write(zinfo.comment)
            pos2 = self.fp.tell()
            # Write end-of-zip-archive record
            endrec = struct.pack(structEndArchive, stringEndArchive,
                     0, 0, count, count, pos2 - pos1, pos1, 0)
            self.fp.write(endrec)
        self.fp.close()
        self.fp = None


class PyZipFile(ZipFile):
    """Class to create ZIP archives with Python library files and packages."""

    def writepy(self, pathname, basename = ""):
        """Add all files from "pathname" to the ZIP archive.

        If pathname is a package directory, search the directory and
        all package subdirectories recursively for all *.py and enter
        the modules into the archive.  If pathname is a plain
        directory, listdir *.py and enter all modules.  Else, pathname
        must be a Python *.py file and the module will be put into the
        archive.  Added modules are always module.pyo or module.pyc.
        This method will compile the module.py into module.pyc if
        necessary.
        """
        dir, name = os.path.split(pathname)
        if os.path.isdir(pathname):
            initname = os.path.join(pathname, "__init__.py")
            if os.path.isfile(initname):
                # This is a package directory, add it
                if basename:
                    basename = "%s/%s" % (basename, name)
                else:
                    basename = name
                if self.debug:
                    print "Adding package in", pathname, "as", basename
                fname, arcname = self._get_codename(initname[0:-3], basename)
                if self.debug:
                    print "Adding", arcname
                self.write(fname, arcname)
                dirlist = os.listdir(pathname)
                dirlist.remove("__init__.py")
                # Add all *.py files and package subdirectories
                for filename in dirlist:
                    path = os.path.join(pathname, filename)
                    root, ext = os.path.splitext(filename)
                    if os.path.isdir(path):
                        if os.path.isfile(os.path.join(path, "__init__.py")):
                            # This is a package directory, add it
                            self.writepy(path, basename)  # Recursive call
                    elif ext == ".py":
                        fname, arcname = self._get_codename(path[0:-3],
                                         basename)
                        if self.debug:
                            print "Adding", arcname
                        self.write(fname, arcname)
            else:
                # This is NOT a package directory, add its files at top level
                if self.debug:
                    print "Adding files from directory", pathname
                for filename in os.listdir(pathname):
                    path = os.path.join(pathname, filename)
                    root, ext = os.path.splitext(filename)
                    if ext == ".py":
                        fname, arcname = self._get_codename(path[0:-3],
                                         basename)
                        if self.debug:
                            print "Adding", arcname
                        self.write(fname, arcname)
        else:
            if pathname[-3:] != ".py":
                raise RuntimeError, \
                      'Files added with writepy() must end with ".py"'
            fname, arcname = self._get_codename(pathname[0:-3], basename)
            if self.debug:
                print "Adding file", arcname
            self.write(fname, arcname)

    def _get_codename(self, pathname, basename):
        """Return (filename, archivename) for the path.

        Given a module name path, return the correct file path and
        archive name, compiling if necessary.  For example, given
        /python/lib/string, return (/python/lib/string.pyc, string).
        """
        file_py  = pathname + ".py"
        file_pyc = pathname + ".pyc"
        file_pyo = pathname + ".pyo"
        if os.path.isfile(file_pyo) and \
                            os.stat(file_pyo)[8] >= os.stat(file_py)[8]:
            fname = file_pyo	# Use .pyo file
        elif not os.path.isfile(file_pyc) or \
             os.stat(file_pyc)[8] < os.stat(file_py)[8]:
            import py_compile
            if self.debug:
                print "Compiling", file_py
            py_compile.compile(file_py, file_pyc)
            fname = file_pyc
        else:
            fname = file_pyc
        archivename = os.path.split(fname)[1]
        if basename:
            archivename = "%s/%s" % (basename, archivename)
        return (fname, archivename)
