#######################################
#    THIS MODULE WAS WRITTEN          #
#    BY COCO DE VIENNE                #
#    IT IS LICENSED UNDER THE SAME   #
#    CONDITIONS AS PYTHON ITSSELF!    #
#######################################

import os

def find_phrase(filename, phrase):
    if os.path.isfile(filename) == False:
        return 1
    elif os.path.isfile(filename) == True:
        file_obj = open(filename, "r")
        contents = file_obj.readlines()
        filelength = len(contents)
        data =  []
        data.append(phrase)
        for i in range(0, filelength):
            if phrase in i:
                line_num = i + 1
                data.append(line_num)
            else:
                continue
        return data           
    else:
        return 1
