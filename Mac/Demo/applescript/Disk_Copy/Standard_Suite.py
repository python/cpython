"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from Macintosh HD:Hulpprogramma's:Disk Copy
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'Core'

class Standard_Suite_Events:

    _argmap_save = {
            '_in' : 'kfil',
            'using_format' : 'SvAs',
            'checksum_verification' : 'VChk',
            'signature_verification' : 'VSig',
            'image_signing' : 'SImg',
            'leave_image_mounted' : 'Moun',
            'percent_free_space' : 'Slop',
            'logical_blocks' : 'Blks',
            'zeroing' : 'Zero',
    }

    def save(self, _object, _attributes={}, **_arguments):
        """save: Save an object
        Required argument: the source object
        Keyword argument _in: the target object
        Keyword argument using_format: the format for the target
        Keyword argument checksum_verification: Should the checksum be verified before saving?
        Keyword argument signature_verification: Should the DigiSigné signature be verified before saving?
        Keyword argument image_signing: Should the image be signed?
        Keyword argument leave_image_mounted: Should the image be mounted after saving?
        Keyword argument percent_free_space: percent free space to reserve (for image folder operation, 0-255%)
        Keyword argument logical_blocks: number of logical blocks in the image (for image folder operation)
        Keyword argument zeroing: Should all the blocks in the image be set to zeros? (for image folder operation)
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the result of the save operation
        """
        _code = 'core'
        _subcode = 'save'

        aetools.keysubst(_arguments, self._argmap_save)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'kfil', _Enum_obj_)
        aetools.enumsubst(_arguments, 'SvAs', _Enum_SvAs)
        aetools.enumsubst(_arguments, 'VChk', _Enum_bool)
        aetools.enumsubst(_arguments, 'VSig', _Enum_bool)
        aetools.enumsubst(_arguments, 'SImg', _Enum_bool)
        aetools.enumsubst(_arguments, 'Moun', _Enum_bool)
        aetools.enumsubst(_arguments, 'Slop', _Enum_long)
        aetools.enumsubst(_arguments, 'Blks', _Enum_long)
        aetools.enumsubst(_arguments, 'Zero', _Enum_bool)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def do_script(self, _object, _attributes={}, **_arguments):
        """do script: Execute an attached script located in the folder "Scripts"
        Required argument: the script to be executed
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'dosc'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class application(aetools.ComponentItem):
    """application - The Disk Copy application """
    want = 'capp'
class version(aetools.NProperty):
    """version - the version of this application """
    which = 'vers'
    want = 'vers'
class name(aetools.NProperty):
    """name - the name of this application """
    which = 'pnam'
    want = 'TEXT'
class comment(aetools.NProperty):
    """comment - the comment associated with the application """
    which = 'comt'
    want = 'TEXT'
class driver_version(aetools.NProperty):
    """driver version - the version of the disk image driver """
    which = 'dVer'
    want = 'vers'
class nonejectable_mode(aetools.NProperty):
    """nonejectable mode - Should mounted images be non-ejectable? """
    which = 'otto'
    want = 'bool'
class save_log_file(aetools.NProperty):
    """save log file - Should the log file be saved on disk? """
    which = 'PSaL'
    want = 'bool'
class use_speech(aetools.NProperty):
    """use speech - Should Disk Copy use spoken feedback? """
    which = 'PTlk'
    want = 'bool'
class smart_Save_As(aetools.NProperty):
    """smart Save As - Should the Save As... dialog box automatically go to the right folder? """
    which = 'PSSP'
    want = 'bool'
class checksum_verification(aetools.NProperty):
    """checksum verification - Should image checksums be verified? """
    which = 'PVeC'
    want = 'bool'
class signature_verification(aetools.NProperty):
    """signature verification - Should digital signatures be verified? """
    which = 'PVeS'
    want = 'bool'
class exclude_DiskScripts(aetools.NProperty):
    """exclude DiskScripts - Should images referenced in DiskScripts/DiskSets be excluded from verification? """
    which = 'PExD'
    want = 'bool'
class exclude_remote_images(aetools.NProperty):
    """exclude remote images - Should images that are located on network volumes be excluded from verification? """
    which = 'PExR'
    want = 'bool'
class image_signing(aetools.NProperty):
    """image signing - Should images be signed with a digital signature? """
    which = 'PSiI'
    want = 'bool'
class leave_image_mounted(aetools.NProperty):
    """leave image mounted - Should images be mounted after they are created? """
    which = 'PMoA'
    want = 'bool'
class erase_confirmation(aetools.NProperty):
    """erase confirmation - Should the user be required to confirm commands that erase disks? """
    which = 'PCoE'
    want = 'bool'
class zeroing(aetools.NProperty):
    """zeroing - Should all blocks of a new image be set to zero? """
    which = 'PZeB'
    want = 'bool'
class default_create_size(aetools.NProperty):
    """default create size - the default size for a new image, in blocks (512 bytes per block) """
    which = 'PDeS'
    want = 'long'
class default_create_name(aetools.NProperty):
    """default create name - the default volume name for a new image """
    which = 'PDeN'
    want = 'TEXT'
class make_multiple_floppies(aetools.NProperty):
    """make multiple floppies - Should the user be prompted to make multiple floppy disk images at a time? """
    which = 'PBuM'
    want = 'bool'
class auto_image_upon_insert(aetools.NProperty):
    """auto image upon insert - Should a newly-inserted disk automatically be processed into an image? """
    which = 'Paim'
    want = 'bool'
class eject_after_auto_image(aetools.NProperty):
    """eject after auto image - Should auto-imaged disks be ejected afterwards? """
    which = 'Pejc'
    want = 'bool'
class auto_copy_upon_floppy_insert(aetools.NProperty):
    """auto copy upon floppy insert - Instead of auto-imaging, should newly-inserted floppy disks be copied? """
    which = 'Pcpf'
    want = 'bool'
class volume_suffix(aetools.NProperty):
    """volume suffix - the default volume name suffix """
    which = 'PDiE'
    want = 'TEXT'
class image_suffix(aetools.NProperty):
    """image suffix - the default image name suffix """
    which = 'PImE'
    want = 'TEXT'
class default_file_system(aetools.NProperty):
    """default file system - the default file system type for new blank images """
    which = 'Pfsy'
    want = 'Fsys'
class default_image_format(aetools.NProperty):
    """default image format - the default image file format """
    which = 'Pdfm'
    want = 'SvAs'

class disk(aetools.ComponentItem):
    """disk - A mounted volume """
    want = 'Disk'

name = name

comment = comment
class locked(aetools.NProperty):
    """locked - Is the disk locked? """
    which = 'islk'
    want = 'bool'
class creation_date(aetools.NProperty):
    """creation date - the creation date of disk """
    which = 'ascd'
    want = 'ldt '
class modification_date(aetools.NProperty):
    """modification date - the modification date of disk """
    which = 'asmo'
    want = 'ldt '
class crc32_checksum(aetools.NProperty):
    """crc32 checksum - the crc-32 checksum of the disk """
    which = 'Xcrc'
    want = 'TEXT'
class disk_copy_4_2e_2_checksum(aetools.NProperty):
    """disk copy 4.2 checksum - the Disk Copy 4.2 checksum of the disk """
    which = 'Xc42'
    want = 'TEXT'
class block_count(aetools.NProperty):
    """block count - the number of blocks on disk """
    which = 'Xblk'
    want = 'long'
class file_system(aetools.NProperty):
    """file system - the file system used on disk """
    which = 'Xfsi'
    want = 'TEXT'

class folder(aetools.ComponentItem):
    """folder - A folder or directory on a disk """
    want = 'Fold'

name = name

comment = comment

creation_date = creation_date

modification_date = modification_date

class disk_image(aetools.ComponentItem):
    """disk image - A disk image file """
    want = 'DImg'

name = name

comment = comment

locked = locked

creation_date = creation_date

modification_date = modification_date
class file_format(aetools.NProperty):
    """file format - the format of the disk image file """
    which = 'Ifmt'
    want = 'TEXT'
class signed(aetools.NProperty):
    """signed - Does the disk image have a DigiSigné signature? """
    which = 'Isin'
    want = 'bool'
class compressed(aetools.NProperty):
    """compressed - Is the disk image compressed? """
    which = 'Icom'
    want = 'bool'
class segmented(aetools.NProperty):
    """segmented - Is the disk image segmented? """
    which = 'Iseg'
    want = 'bool'
class segments(aetools.NProperty):
    """segments - a list of references to other segments that make up a complete image """
    which = 'Isg#'
    want = 'fss '
class disk_name(aetools.NProperty):
    """disk name - the name of the disk this image represents """
    which = 'Idnm'
    want = 'TEXT'

crc32_checksum = crc32_checksum

disk_copy_4_2e_2_checksum = disk_copy_4_2e_2_checksum

block_count = block_count

file_system = file_system
class data_fork_size(aetools.NProperty):
    """data fork size - the size (in bytes) of the data fork of the disk image """
    which = 'Idfk'
    want = 'long'
class resource_fork_size(aetools.NProperty):
    """resource fork size - the size (in bytes) of the resource fork of the disk image """
    which = 'Irfk'
    want = 'long'

class Save_reply_record(aetools.ComponentItem):
    """Save reply record - Result from the save operation """
    want = 'cpyR'
class resulting_target_object(aetools.NProperty):
    """resulting target object - a reference to the target object after it has been saved """
    which = 'rcpO'
    want = 'obj '
class copy_type(aetools.NProperty):
    """copy type - the way in which the target object was saved """
    which = 'rcpT'
    want = 'rcpT'
application._propdict = {
        'version' : version,
        'name' : name,
        'comment' : comment,
        'driver_version' : driver_version,
        'nonejectable_mode' : nonejectable_mode,
        'save_log_file' : save_log_file,
        'use_speech' : use_speech,
        'smart_Save_As' : smart_Save_As,
        'checksum_verification' : checksum_verification,
        'signature_verification' : signature_verification,
        'exclude_DiskScripts' : exclude_DiskScripts,
        'exclude_remote_images' : exclude_remote_images,
        'image_signing' : image_signing,
        'leave_image_mounted' : leave_image_mounted,
        'erase_confirmation' : erase_confirmation,
        'zeroing' : zeroing,
        'default_create_size' : default_create_size,
        'default_create_name' : default_create_name,
        'make_multiple_floppies' : make_multiple_floppies,
        'auto_image_upon_insert' : auto_image_upon_insert,
        'eject_after_auto_image' : eject_after_auto_image,
        'auto_copy_upon_floppy_insert' : auto_copy_upon_floppy_insert,
        'volume_suffix' : volume_suffix,
        'image_suffix' : image_suffix,
        'default_file_system' : default_file_system,
        'default_image_format' : default_image_format,
}
application._elemdict = {
}
disk._propdict = {
        'name' : name,
        'comment' : comment,
        'locked' : locked,
        'creation_date' : creation_date,
        'modification_date' : modification_date,
        'crc32_checksum' : crc32_checksum,
        'disk_copy_4_2e_2_checksum' : disk_copy_4_2e_2_checksum,
        'block_count' : block_count,
        'file_system' : file_system,
}
disk._elemdict = {
}
folder._propdict = {
        'name' : name,
        'comment' : comment,
        'creation_date' : creation_date,
        'modification_date' : modification_date,
}
folder._elemdict = {
}
disk_image._propdict = {
        'name' : name,
        'comment' : comment,
        'locked' : locked,
        'creation_date' : creation_date,
        'modification_date' : modification_date,
        'file_format' : file_format,
        'signed' : signed,
        'compressed' : compressed,
        'segmented' : segmented,
        'segments' : segments,
        'disk_name' : disk_name,
        'crc32_checksum' : crc32_checksum,
        'disk_copy_4_2e_2_checksum' : disk_copy_4_2e_2_checksum,
        'block_count' : block_count,
        'file_system' : file_system,
        'data_fork_size' : data_fork_size,
        'resource_fork_size' : resource_fork_size,
}
disk_image._elemdict = {
}
Save_reply_record._propdict = {
        'resulting_target_object' : resulting_target_object,
        'copy_type' : copy_type,
}
Save_reply_record._elemdict = {
}
_Enum_UIAc = {
        'never_interact' : 'eNvr',      # Don’t allow any interaction at all
        'interact_with_self' : 'eInS',  # Only allow interaction from internal events
        'interact_with_local' : 'eInL', # Allow interaction from any event originating on this machine
        'interact_with_all' : 'eInA',   # Allow interaction from network events
}

_Enum_SvAs = {
        'NDIF_RW' : 'RdWr',     # read/write NDIF disk image
        'NDIF_RO' : 'Rdxx',     # read-only NDIF disk image
        'NDIF_Compressed' : 'ROCo',     # compressed NDIF disk image
        'Disk_Copy_4_2e_2' : 'DC42',    # Disk Copy 4.2 disk image
}

_Enum_rcpT = {
        'block_disk_copy' : 'cpBl',     # block-by-block disk-level copy
        'files_and_file_ID_copy' : 'cpID',      # all files including desktop databases and file ID’s
        'files_and_desktop_info' : 'cpDT',      # all files and most desktop information
        'files_only' : 'cpFI',  # all files but no desktop information
        'disk_image_conversion' : 'cpCV',       # disk image format conversion
        'disk_image_creation' : 'cpCR', # disk image creation
}

_Enum_long = None # XXXX enum long not found!!
_Enum_bool = None # XXXX enum bool not found!!
_Enum_obj_ = None # XXXX enum obj  not found!!

#
# Indices of types declared in this module
#
_classdeclarations = {
        'DImg' : disk_image,
        'capp' : application,
        'Disk' : disk,
        'Fold' : folder,
        'cpyR' : Save_reply_record,
}

_propdeclarations = {
        'Xcrc' : crc32_checksum,
        'PDeS' : default_create_size,
        'Idnm' : disk_name,
        'PSSP' : smart_Save_As,
        'Pcpf' : auto_copy_upon_floppy_insert,
        'pnam' : name,
        'Isin' : signed,
        'otto' : nonejectable_mode,
        'PExD' : exclude_DiskScripts,
        'Iseg' : segmented,
        'islk' : locked,
        'asmo' : modification_date,
        'PTlk' : use_speech,
        'Pfsy' : default_file_system,
        'PVeC' : checksum_verification,
        'Xc42' : disk_copy_4_2e_2_checksum,
        'rcpO' : resulting_target_object,
        'Paim' : auto_image_upon_insert,
        'comt' : comment,
        'PCoE' : erase_confirmation,
        'dVer' : driver_version,
        'PDeN' : default_create_name,
        'PBuM' : make_multiple_floppies,
        'rcpT' : copy_type,
        'PDiE' : volume_suffix,
        'Ifmt' : file_format,
        'Pdfm' : default_image_format,
        'ascd' : creation_date,
        'Pejc' : eject_after_auto_image,
        'PZeB' : zeroing,
        'PExR' : exclude_remote_images,
        'PImE' : image_suffix,
        'PVeS' : signature_verification,
        'PSaL' : save_log_file,
        'Xblk' : block_count,
        'PMoA' : leave_image_mounted,
        'Isg#' : segments,
        'Irfk' : resource_fork_size,
        'Icom' : compressed,
        'Xfsi' : file_system,
        'Idfk' : data_fork_size,
        'vers' : version,
        'PSiI' : image_signing,
}

_compdeclarations = {
}

_enumdeclarations = {
        'SvAs' : _Enum_SvAs,
        'UIAc' : _Enum_UIAc,
        'rcpT' : _Enum_rcpT,
}
