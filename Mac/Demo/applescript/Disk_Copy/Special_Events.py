"""Suite Special Events: Commands for mounting Disk Copy images
Level 1, version 1

Generated from Macintosh HD:Hulpprogramma's:Disk Copy
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'ddsk'

class Special_Events_Events:

    _argmap_mount = {
            'access_mode' : 'Acss',
            'checksum_verification' : 'VChk',
            'signature_verification' : 'VSig',
            'RAM_caching' : 'Cach',
    }

    def mount(self, _object, _attributes={}, **_arguments):
        """mount: Mounts an Disk Copy image as a disk volume
        Required argument: a reference to the disk image to be mounted
        Keyword argument access_mode: the access mode for mounted volume (default is "any", i.e. best possible)
        Keyword argument checksum_verification: Verify the checksum before mounting?
        Keyword argument signature_verification: Verify the DigiSigné signature before mounting?
        Keyword argument RAM_caching: Cache the disk image in RAM? (if omitted, don't cache)
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a reference to mounted disk
        """
        _code = 'ddsk'
        _subcode = 'Moun'

        aetools.keysubst(_arguments, self._argmap_mount)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'Acss', _Enum_Acss)
        aetools.enumsubst(_arguments, 'VChk', _Enum_bool)
        aetools.enumsubst(_arguments, 'VSig', _Enum_bool)
        aetools.enumsubst(_arguments, 'Cach', _Enum_bool)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_execute_DiskScript = {
            'checksum_verification' : 'VChk',
            'signature_verification' : 'VSig',
    }

    def execute_DiskScript(self, _object, _attributes={}, **_arguments):
        """execute DiskScript: Executes a Disk Copy-specific DiskScript
        Required argument: a reference to the DiskScript to execute
        Keyword argument checksum_verification: Should checksums be verified when mounting images referenced in the DiskScript?
        Keyword argument signature_verification: Should the DigiSigné signature of the DiskScript and the images it references be verified?
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'ddsk'
        _subcode = 'XEQd'

        aetools.keysubst(_arguments, self._argmap_execute_DiskScript)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'VChk', _Enum_bool)
        aetools.enumsubst(_arguments, 'VSig', _Enum_bool)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def unmount(self, _object, _attributes={}, **_arguments):
        """unmount: Unmount and eject (if necessary) a volume
        Required argument: a reference to disk to be unmounted (and ejected)
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'ddsk'
        _subcode = 'Umnt'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_create = {
            'saving_as' : 'SvAs',
            'logical_blocks' : 'Blks',
            'zeroing' : 'Zero',
            'leave_image_mounted' : 'Moun',
            'filesystem' : 'Fsys',
    }

    def create(self, _object, _attributes={}, **_arguments):
        """create: Create a new Disk Copy document
        Required argument: the name of the volume to create
        Keyword argument saving_as: the disk image to be created
        Keyword argument logical_blocks: the number of logical blocks
        Keyword argument zeroing: Should all blocks on the disk be set to zero?
        Keyword argument leave_image_mounted: Should the image be mounted after it is created?
        Keyword argument filesystem: file system to use (Mac OS Standard/compatible, Mac OS Enhanced)
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a reference to newly created disk image (or newly mounted disk)
        """
        _code = 'ddsk'
        _subcode = 'Crea'

        aetools.keysubst(_arguments, self._argmap_create)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'SvAs', _Enum_fss_)
        aetools.enumsubst(_arguments, 'Blks', _Enum_long)
        aetools.enumsubst(_arguments, 'Zero', _Enum_bool)
        aetools.enumsubst(_arguments, 'Moun', _Enum_bool)
        aetools.enumsubst(_arguments, 'Fsys', _Enum_Fsys)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def verify_checksum(self, _object, _attributes={}, **_arguments):
        """verify checksum: Verify the checksum of a Disk Copy 4.2 or a Disk Copy 6.0 read-only document
        Required argument: the disk image to be verified
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the result of the checksum verification
        """
        _code = 'ddsk'
        _subcode = 'Vcrc'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def verify_signature(self, _object, _attributes={}, **_arguments):
        """verify signature: Verify the DigiSigné signature for a Disk Copy document
        Required argument: the disk image to be verified
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Is the DigiSigné signature valid?
        """
        _code = 'ddsk'
        _subcode = 'Vsig'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_sign_image = {
            'using_signer' : 'Sinr',
    }

    def sign_image(self, _object, _attributes={}, **_arguments):
        """sign image: Add a DigiSigné signature to a Disk Copy document
        Required argument: the disk image to be signed
        Keyword argument using_signer: a reference to signer file to use
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'ddsk'
        _subcode = 'Asig'

        aetools.keysubst(_arguments, self._argmap_sign_image)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'Sinr', _Enum_alis)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_create_a_floppy_from = {
            'signature_verification' : 'VSig',
            'erase_confirmation' : 'Cfrm',
            'make_multiple_floppies' : 'Mult',
    }

    def create_a_floppy_from(self, _object, _attributes={}, **_arguments):
        """create a floppy from: create a floppy disk from a Disk Copy document
        Required argument: the disk image to make a floppy from
        Keyword argument signature_verification: Should the DigiSigné signature be verified before creating a floppy disk?
        Keyword argument erase_confirmation: Should the user be asked to confirm the erasure of the previous contents of floppy disks?
        Keyword argument make_multiple_floppies: Should the user be prompted to create multiple floppy disks?
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'ddsk'
        _subcode = 'Bfpy'

        aetools.keysubst(_arguments, self._argmap_create_a_floppy_from)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'VSig', _Enum_bool)
        aetools.enumsubst(_arguments, 'Cfrm', _Enum_bool)
        aetools.enumsubst(_arguments, 'Mult', _Enum_bool)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_check_image = {
            'details' : 'ChDe',
    }

    def check_image(self, _object, _attributes={}, **_arguments):
        """check image: Check the disk image’s internal data structures for any inconsistencies.  Works on NDIF, Disk Copy 4.2, DARTé, or DiskSet images.
        Required argument: the disk image to be verified
        Keyword argument details: Should the disk image details be displayed?
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a record containing a boolean (true/false) value if the image passes consistency tests, and the numbers of warnings and errors
        """
        _code = 'ddsk'
        _subcode = 'Chek'

        aetools.keysubst(_arguments, self._argmap_check_image)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'ChDe', _Enum_bool)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_segment_image = {
            'segment_count' : 'SGCT',
            'segment_size' : 'SGSZ',
            'segment_name' : 'SGNM',
            'image_ID' : 'SGID',
    }

    def segment_image(self, _object, _attributes={}, **_arguments):
        """segment image: Segment a NDIF R/W or R/O image into smaller pieces
        Required argument: the disk image to be segmented
        Keyword argument segment_count: the number of image segments to create
        Keyword argument segment_size: the size of image segments (in blocks) to create
        Keyword argument segment_name: the root name for each image segment file
        Keyword argument image_ID: string used to generate a unique image ID to group the segments
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a list of references to the image segments created
        """
        _code = 'ddsk'
        _subcode = 'SGMT'

        aetools.keysubst(_arguments, self._argmap_segment_image)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_create_SMI = {
            'source_images' : 'SMI1',
            'launching_application' : 'SMI2',
            'launching_document' : 'SMI3',
            'version_string' : 'SMI4',
            'checksum_verification' : 'VChk',
            'signature_verification' : 'VSig',
            'image_signing' : 'SImg',
    }

    def create_SMI(self, _object, _attributes={}, **_arguments):
        """create SMI: Creates a self-mounting image (SMI) from a list of NDIF disk images
        Required argument: the self-mounting image to create
        Keyword argument source_images: a list of references to sources images
        Keyword argument launching_application: the path to an application to launch
        Keyword argument launching_document: the path to a document to open
        Keyword argument version_string: sets the 'vers' 1 resource of the self-mounting image
        Keyword argument checksum_verification: Should the checksum of the source images be verified before creating the SMI?
        Keyword argument signature_verification: Should the DigiSigné signature of the source images be verified before creating the SMI?
        Keyword argument image_signing: Should the SMI be given a digital signature when it is created?
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a reference to the self-mounting image created
        """
        _code = 'ddsk'
        _subcode = 'MSMI'

        aetools.keysubst(_arguments, self._argmap_create_SMI)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'VChk', _Enum_bool)
        aetools.enumsubst(_arguments, 'VSig', _Enum_bool)
        aetools.enumsubst(_arguments, 'SImg', _Enum_bool)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class Verify_Checksum_reply_record(aetools.ComponentItem):
    """Verify Checksum reply record -  """
    want = 'Rcrc'
class validity(aetools.NProperty):
    """validity - true if checksum is valid """
    which = 'Vlid'
    want = 'bool'
class expected_checksum(aetools.NProperty):
    """expected checksum - checksum value stored in the image header (in hexadecimal) """
    which = 'crcE'
    want = 'TEXT'
class calculated_checksum(aetools.NProperty):
    """calculated checksum - checksum value actually calculated (in hexadecimal) """
    which = 'crcA'
    want = 'TEXT'

class Check_Image_reply_record(aetools.ComponentItem):
    """Check Image reply record -  """
    want = 'Rchk'
class consistency(aetools.NProperty):
    """consistency - Does the image pass consistency checks? """
    which = 'Rch1'
    want = 'bool'
class error_count(aetools.NProperty):
    """error count - the number of errors recorded """
    which = 'Rch2'
    want = 'long'
class warning_count(aetools.NProperty):
    """warning count - the number of warnings recorded """
    which = 'Rch3'
    want = 'long'
Verify_Checksum_reply_record._propdict = {
        'validity' : validity,
        'expected_checksum' : expected_checksum,
        'calculated_checksum' : calculated_checksum,
}
Verify_Checksum_reply_record._elemdict = {
}
Check_Image_reply_record._propdict = {
        'consistency' : consistency,
        'error_count' : error_count,
        'warning_count' : warning_count,
}
Check_Image_reply_record._elemdict = {
}
_Enum_Acss = {
        'read_and_write' : 'RdWr',      # read/write access
        'read_only' : 'Rdxx',   # read-only access
        'any' : 'Anyx', # best possible access
}

_Enum_Fsys = {
        'Mac_OS_Standard' : 'Fhfs',     # classic HFS file system
        'compatible_Mac_OS_Extended' : 'Fhf+',  # new HFS+ file system
}

_Enum_alis = None # XXXX enum alis not found!!
_Enum_fss_ = None # XXXX enum fss  not found!!
_Enum_long = None # XXXX enum long not found!!
_Enum_bool = None # XXXX enum bool not found!!

#
# Indices of types declared in this module
#
_classdeclarations = {
        'Rchk' : Check_Image_reply_record,
        'Rcrc' : Verify_Checksum_reply_record,
}

_propdeclarations = {
        'crcE' : expected_checksum,
        'Rch2' : error_count,
        'crcA' : calculated_checksum,
        'Rch3' : warning_count,
        'Vlid' : validity,
        'Rch1' : consistency,
}

_compdeclarations = {
}

_enumdeclarations = {
        'Acss' : _Enum_Acss,
        'Fsys' : _Enum_Fsys,
}
