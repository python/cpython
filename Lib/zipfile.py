"Read and write ZIP files."

import struct, os, time
import binascii

try:
    import zlib # We may need its compression method
except ImportError:
    zlib = None

__all__ = ["BadZipfile", "error", "ZIP_STORED", "ZIP_DEFLATED", "is_zipfile",
           "ZipInfo", "ZipFile", "PyZipFile"]

class BadZipfile(Exception):
    pass
error = BadZipfile      # The exception raised by this module

# constants for Zip file compression methods
ZIP_STORED = 0
ZIP_DEFLATED = 8
# Other ZIP compression methods not supported

# Here are some struct module formats for reading headers
structEndArchive = "<4s4H2lH"     # 9 items, end of archive, 22 bytes
stringEndArchive = "PK\005\006"   # magic number for end of archive record
structCentralDir = "<4s4B4HlLL5HLl"# 19 items, central directory, 46 bytes
stringCentralDir = "PK\001\002"   # magic number for central directory
structFileHeader = "<4s2B4HlLL2H"  # 12 items, file header record, 30 bytes
stringFileHeader = "PK\003\004"   # magic number for file header

# indexes of entries in the central directory structure
_CD_SIGNATURE = 0
_CD_CREATE_VERSION = 1
_CD_CREATE_SYSTEM = 2
_CD_EXTRACT_VERSION = 3
_CD_EXTRACT_SYSTEM = 4                  # is this meaningful?
_CD_FLAG_BITS = 5
_CD_COMPRESS_TYPE = 6
_CD_TIME = 7
_CD_DATE = 8
_CD_CRC = 9
_CD_COMPRESSED_SIZE = 10
_CD_UNCOMPRESSED_SIZE = 11
_CD_FILENAME_LENGTH = 12
_CD_EXTRA_FIELD_LENGTH = 13
_CD_COMMENT_LENGTH = 14
_CD_DISK_NUMBER_START = 15
_CD_INTERNAL_FILE_ATTRIBUTES = 16
_CD_EXTERNAL_FILE_ATTRIBUTES = 17
_CD_LOCAL_HEADER_OFFSET = 18

# indexes of entries in the local file header structure
_FH_SIGNATURE = 0
_FH_EXTRACT_VERSION = 1
_FH_EXTRACT_SYSTEM = 2                  # is this meaningful?
_FH_GENERAL_PURPOSE_FLAG_BITS = 3
_FH_COMPRESSION_METHOD = 4
_FH_LAST_MOD_TIME = 5
_FH_LAST_MOD_DATE = 6
_FH_CRC = 7
_FH_COMPRESSED_SIZE = 8
_FH_UNCOMPRESSED_SIZE = 9
_FH_FILENAME_LENGTH = 10
_FH_EXTRA_FIELD_LENGTH = 11

def is_zipfile(filename):
    """Quickly see if file is a ZIP file by checking the magic number."""
    try:
        fpin = open(filename, "rb")
        endrec = _EndRecData(fpin)
        fpin.close()
        if endrec:
            return True                 # file has correct magic number
    except IOError:
        pass
    return False

def _EndRecData(fpin):
    """Return data from the "End of Central Directory" record, or None.

    The data is a list of the nine items in the ZIP "End of central dir"
    record followed by a tenth item, the file seek offset of this record."""
    fpin.seek(-22, 2)               # Assume no archive comment.
    filesize = fpin.tell() + 22     # Get file size
    data = fpin.read()
    if data[0:4] == stringEndArchive and data[-2:] == "\000\000":
        endrec = struct.unpack(structEndArchive, data)
        endrec = list(endrec)
        endrec.append("")               # Append the archive comment
        endrec.append(filesize - 22)    # Append the record start offset
        return endrec
    # Search the last END_BLOCK bytes of the file for the record signature.
    # The comment is appended to the ZIP file and has a 16 bit length.
    # So the comment may be up to 64K long.  We limit the search for the
    # signature to a few Kbytes at the end of the file for efficiency.
    # also, the signature must not appear in the comment.
    END_BLOCK = min(filesize, 1024 * 4)
    fpin.seek(filesize - END_BLOCK, 0)
    data = fpin.read()
    start = data.rfind(stringEndArchive)
    if start >= 0:     # Correct signature string was found
        endrec = struct.unpack(structEndArchive, data[start:start+22])
        endrec = list(endrec)
        comment = data[start+22:]
        if endrec[7] == len(comment):     # Comment length checks out
            # Append the archive comment and start offset
            endrec.append(comment)
            endrec.append(filesize - END_BLOCK + start)
            return endrec
    return      # Error, return None


class ZipInfo:
    """Class with attributes describing each file in the ZIP archive."""

    def __init__(self, filename="NoName", date_time=(1980,1,1,0,0,0)):
        self.orig_filename = filename   # Original file name in archive
# Terminate the file name at the first null byte.  Null bytes in file
# names are used as tricks by viruses in archives.
        null_byte = filename.find(chr(0))
        if null_byte >= 0:
            filename = filename[0:null_byte]
# This is used to ensure paths in generated ZIP files always use
# forward slashes as the directory separator, as required by the
# ZIP format specification.
        if os.sep != "/":
            filename = filename.replace(os.sep, "/")
        self.filename = filename        # Normalized file name
        self.date_time = date_time      # year, month, day, hour, min, sec
        # Standard values:
        self.compress_type = ZIP_STORED # Type of compression for the file
        self.comment = ""               # Comment for each file
        self.extra = ""                 # ZIP extra data
        self.create_system = 0          # System which created ZIP archive
        self.create_version = 20        # Version which created ZIP archive
        self.extract_version = 20       # Version needed to extract archive
        self.reserved = 0               # Must be zero
        self.flag_bits = 0              # ZIP flag bits
        self.volume = 0                 # Volume number of file header
        self.internal_attr = 0          # Internal attributes
        self.external_attr = 0          # External file attributes
        # Other attributes are set by class ZipFile:
        # header_offset         Byte offset to the file header
        # file_offset           Byte offset to the start of the file data
        # CRC                   CRC-32 of the uncompressed file
        # compress_size         Size of the compressed file
        # file_size             Size of the uncompressed file

    def FileHeader(self):
        """Return the per-file header as a string."""
        dt = self.date_time
        dosdate = (dt[0] - 1980) << 9 | dt[1] << 5 | dt[2]
        dostime = dt[3] << 11 | dt[4] << 5 | (dt[5] // 2)
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
    """ Class with methods to open, read, write, close, list zip files.

    z = ZipFile(file, mode="r", compression=ZIP_STORED)

    file: Either the path to the file, or a file-like object.
          If it is a path, the file will be opened and closed by ZipFile.
    mode: The mode can be either read "r", write "w" or append "a".
    compression: ZIP_STORED (no compression) or ZIP_DEFLATED (requires zlib).
    """

    fp = None                   # Set here since __del__ checks it

    def __init__(self, file, mode="r", compression=ZIP_STORED):
        """Open the ZIP file with mode read "r", write "w" or append "a"."""
        if compression == ZIP_STORED:
            pass
        elif compression == ZIP_DEFLATED:
            if not zlib:
                raise RuntimeError,\
                      "Compression requires the (missing) zlib module"
        else:
            raise RuntimeError, "That compression method is not supported"
        self.debug = 0  # Level of printing: 0 through 3
        self.NameToInfo = {}    # Find file info given name
        self.filelist = []      # List of ZipInfo instances for archive
        self.compression = compression  # Method of compression
        self.mode = key = mode.replace('b', '')[0]

        # Check if we were passed a file-like object
        if isinstance(file, basestring):
            self._filePassed = 0
            self.filename = file
            modeDict = {'r' : 'rb', 'w': 'wb', 'a' : 'r+b'}
            self.fp = open(file, modeDict[mode])
        else:
            self._filePassed = 1
            self.fp = file
            self.filename = getattr(file, 'name', None)

        if key == 'r':
            self._GetContents()
        elif key == 'w':
            pass
        elif key == 'a':
            try:                        # See if file is a zip file
                self._RealGetContents()
                # seek to start of directory and overwrite
                self.fp.seek(self.start_dir, 0)
            except BadZipfile:          # file is not a zip file, just append
                self.fp.seek(0, 2)
        else:
            if not self._filePassed:
                self.fp.close()
                self.fp = None
            raise RuntimeError, 'Mode must be "r", "w" or "a"'

    def _GetContents(self):
        """Read the directory, making sure we close the file if the format
        is bad."""
        try:
            self._RealGetContents()
        except BadZipfile:
            if not self._filePassed:
                self.fp.close()
                self.fp = None
            raise

    def _RealGetContents(self):
        """Read in the table of contents for the ZIP file."""
        fp = self.fp
        endrec = _EndRecData(fp)
        if not endrec:
            raise BadZipfile, "File is not a zip file"
        if self.debug > 1:
            print endrec
        size_cd = endrec[5]             # bytes in central directory
        offset_cd = endrec[6]   # offset of central directory
        self.comment = endrec[8]        # archive comment
        # endrec[9] is the offset of the "End of Central Dir" record
        x = endrec[9] - size_cd
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
            filename = fp.read(centdir[_CD_FILENAME_LENGTH])
            # Create ZipInfo instance to store file information
            x = ZipInfo(filename)
            x.extra = fp.read(centdir[_CD_EXTRA_FIELD_LENGTH])
            x.comment = fp.read(centdir[_CD_COMMENT_LENGTH])
            total = (total + centdir[_CD_FILENAME_LENGTH]
                     + centdir[_CD_EXTRA_FIELD_LENGTH]
                     + centdir[_CD_COMMENT_LENGTH])
            x.header_offset = centdir[_CD_LOCAL_HEADER_OFFSET] + concat
            # file_offset must be computed below...
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
            # file_offset is computed here, since the extra field for
            # the central directory and for the local file header
            # refer to different fields, and they can have different
            # lengths
            data.file_offset = (data.header_offset + 30
                                + fheader[_FH_FILENAME_LENGTH]
                                + fheader[_FH_EXTRA_FIELD_LENGTH])
            fname = fp.read(fheader[_FH_FILENAME_LENGTH])
            if fname != data.orig_filename:
                raise RuntimeError, \
                      'File name in directory "%s" and header "%s" differ.' % (
                          data.orig_filename, fname)

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
                self.read(zinfo.filename)       # Check CRC-32
            except BadZipfile:
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
        if zinfo.filename in self.NameToInfo:
            if self.debug:      # Warning for duplicate names
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
        mtime = time.localtime(st.st_mtime)
        date_time = mtime[0:6]
        # Create ZipInfo instance to store file information
        if arcname is None:
            zinfo = ZipInfo(filename, date_time)
        else:
            zinfo = ZipInfo(arcname, date_time)
        zinfo.external_attr = (st[0] & 0xFFFF) << 16L      # Unix attributes
        if compress_type is None:
            zinfo.compress_type = self.compression
        else:
            zinfo.compress_type = compress_type
        self._writecheck(zinfo)
        fp = open(filename, "rb")
        zinfo.flag_bits = 0x00
        zinfo.header_offset = self.fp.tell()    # Start of header bytes
        # Must overwrite CRC and sizes with correct data later
        zinfo.CRC = CRC = 0
        zinfo.compress_size = compress_size = 0
        zinfo.file_size = file_size = 0
        self.fp.write(zinfo.FileHeader())
        zinfo.file_offset = self.fp.tell()      # Start of file bytes
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
        # Seek backwards and write CRC and file sizes
        position = self.fp.tell()       # Preserve current position in file
        self.fp.seek(zinfo.header_offset + 14, 0)
        self.fp.write(struct.pack("<lLL", zinfo.CRC, zinfo.compress_size,
              zinfo.file_size))
        self.fp.seek(position, 0)
        self.filelist.append(zinfo)
        self.NameToInfo[zinfo.filename] = zinfo

    def writestr(self, zinfo_or_arcname, bytes):
        """Write a file into the archive.  The contents is the string
        'bytes'.  'zinfo_or_arcname' is either a ZipInfo instance or
        the name of the file in the archive."""
        if not isinstance(zinfo_or_arcname, ZipInfo):
            zinfo = ZipInfo(filename=zinfo_or_arcname,
                            date_time=time.localtime(time.time()))
            zinfo.compress_type = self.compression
        else:
            zinfo = zinfo_or_arcname
        self._writecheck(zinfo)
        zinfo.file_size = len(bytes)            # Uncompressed size
        zinfo.CRC = binascii.crc32(bytes)       # CRC-32 checksum
        if zinfo.compress_type == ZIP_DEFLATED:
            co = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION,
                 zlib.DEFLATED, -15)
            bytes = co.compress(bytes) + co.flush()
            zinfo.compress_size = len(bytes)    # Compressed size
        else:
            zinfo.compress_size = zinfo.file_size
        zinfo.header_offset = self.fp.tell()    # Start of header bytes
        self.fp.write(zinfo.FileHeader())
        zinfo.file_offset = self.fp.tell()      # Start of file bytes
        self.fp.write(bytes)
        if zinfo.flag_bits & 0x08:
            # Write CRC and file sizes after the file data
            self.fp.write(struct.pack("<lLL", zinfo.CRC, zinfo.compress_size,
                  zinfo.file_size))
        self.filelist.append(zinfo)
        self.NameToInfo[zinfo.filename] = zinfo

    def __del__(self):
        """Call the "close()" method in case the user forgot."""
        self.close()

    def close(self):
        """Close the file, and for mode "w" and "a" write the ending
        records."""
        if self.fp is None:
            return
        if self.mode in ("w", "a"):             # write ending records
            count = 0
            pos1 = self.fp.tell()
            for zinfo in self.filelist:         # write central directory
                count = count + 1
                dt = zinfo.date_time
                dosdate = (dt[0] - 1980) << 9 | dt[1] << 5 | dt[2]
                dostime = dt[3] << 11 | dt[4] << 5 | (dt[5] // 2)
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
            self.fp.flush()
        if not self._filePassed:
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
                            os.stat(file_pyo).st_mtime >= os.stat(file_py).st_mtime:
            fname = file_pyo    # Use .pyo file
        elif not os.path.isfile(file_pyc) or \
             os.stat(file_pyc).st_mtime < os.stat(file_py).st_mtime:
            import py_compile
            if self.debug:
                print "Compiling", file_py
            try:
                py_compile.compile(file_py, file_pyc, None, True)
            except py_compile.PyCompileError,err:
                print err.msg
            fname = file_pyc
        else:
            fname = file_pyc
        archivename = os.path.split(fname)[1]
        if basename:
            archivename = "%s/%s" % (basename, archivename)
        return (fname, archivename)
