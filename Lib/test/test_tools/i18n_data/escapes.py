import gettext as _


# Special characters that are always escaped in the POT file
_('"\t\n\r\\')

# All ascii characters 0-31
_('\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n'
  '\x0b\x0c\r\x0e\x0f\x10\x11\x12\x13\x14\x15'
  '\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f')

# All ascii characters 32-126
_(' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ'
  '[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~')

# ascii char 127
_('\x7f')

# characters 128-255
_('\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90'
  '\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\xa0'
  '¡¢£¤¥¦§¨©ª«¬\xad®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞ'
  'ßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ')

# some characters >= 256
_('ě š č ř α β γ δ ㄱ ㄲ ㄴ ㄷ')
