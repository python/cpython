#!/bin/sh
#
# Christian Reis <kiko@async.com.br>
#
# Moves 
# 
# Example:
#
#   blackjesus:~> ./move-faqwiz.sh 2\.1 3\.2
#   Moving FAQ question 02.001 to 03.002

if [ x$2 == x ]; then
    echo "Need 2 args: original_version final_version."
    exit 2
fi

if [ ! -d data -o ! -d data/RCS ]; then
    echo "Run this inside the faqwiz data/ directory's parent dir."
    exit 2
fi

function cut_n_pad() {
    t=`echo $1 | cut -d. -f $2`
    export $3=`echo $t | awk "{ tmp = \\$0; l = length(tmp); for (i = 0; i < $2-l+1; i++) { tmp = "0".tmp } print tmp  }"`
}

cut_n_pad $1 1 prefix1
cut_n_pad $1 2 suffix1
cut_n_pad $2 1 prefix2
cut_n_pad $2 2 suffix2
tmpfile=tmp$RANDOM.tmp
file1=faq$prefix1.$suffix1.htp
file2=faq$prefix2.$suffix2.htp

echo "Moving FAQ question $prefix1.$suffix1 to $prefix2.$suffix2" 

sed -e "s/$1\./$2\./g" data/$file1 > ${tmpfile}1
sed -e "s/$1\./$2\./g" data/RCS/$file1,v > ${tmpfile}2

if [ -f data/$file2 ]; then
    echo "Target FAQ exists. Won't clobber."
    exit 2
fi

mv ${tmpfile}1 data/$file2
mv ${tmpfile}2 data/RCS/$file2,v
mv data/$file1 data/$file1.orig
mv data/RCS/$file1,v data/RCS/$file1,v.orig

