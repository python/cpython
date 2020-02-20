#!/bin/sh

#
# Purpose: test all machine configurations, pydebug, refleaks, release build
#          and release build with valgrind.
#
# Synopsis: ./runall-memorydebugger.sh [--all-configs64 | --all-configs32]
#
# Requirements: valgrind
#

# Set additional CFLAGS and LDFLAGS for ./configure
ADD_CFLAGS=
ADD_LDFLAGS=


CONFIGS_64="x64 uint128 ansi64 universal"
CONFIGS_32="ppro ansi32 ansi-legacy universal"

VALGRIND="valgrind --tool=memcheck --leak-resolution=high \
          --suppressions=Misc/valgrind-python.supp"

# Get args
case $@ in
     *--all-configs64*)
         CONFIGS=$CONFIGS_64
         ;;
     *--all-configs32*)
         CONFIGS=$CONFIGS_32
         ;;
     *)
         CONFIGS="auto"
         ;;
esac

# gmake required
GMAKE=`which gmake`
if [ X"$GMAKE" = X"" ]; then
    GMAKE=make
fi

# Pretty print configurations
print_config ()
{
    len=`echo $@ | wc -c`
    margin="#%"`expr \( 74 - $len \) / 2`"s"

    echo ""
    echo "# ========================================================================"
    printf $margin ""
    echo $@
    echo "# ========================================================================"
    echo ""
}


cd ..

# test_decimal: refleak, regular and Valgrind tests
for config in $CONFIGS; do

        unset PYTHON_DECIMAL_WITH_MACHINE
        libmpdec_config=$config
        if [ X"$config" != X"auto" ]; then
            PYTHON_DECIMAL_WITH_MACHINE=$config
            export PYTHON_DECIMAL_WITH_MACHINE
        else
            libmpdec_config=""
        fi

        ############ refleak tests ###########
        print_config "refleak tests: config=$config"
        printf "\nbuilding python ...\n\n"

        cd ../../
        $GMAKE distclean > /dev/null 2>&1
        ./configure CFLAGS="$ADD_CFLAGS" LDFLAGS="$ADD_LDFLAGS" --with-pydebug > /dev/null 2>&1
        $GMAKE | grep _decimal

        printf "\n\n# ======================== refleak tests ===========================\n\n"
        ./python -m test -uall -R 2:2 test_decimal


        ############ regular tests ###########
        print_config "regular tests: config=$config"
        printf "\nbuilding python ...\n\n"

        $GMAKE distclean > /dev/null 2>&1
        ./configure CFLAGS="$ADD_CFLAGS" LDFLAGS="$ADD_LDFLAGS" > /dev/null 2>&1
        $GMAKE | grep _decimal

        printf "\n\n# ======================== regular tests ===========================\n\n"
        ./python -m test -uall test_decimal


        ########### valgrind tests ###########
        valgrind=$VALGRIND
        case "$config" in
            # Valgrind has no support for 80 bit long double arithmetic.
            ppro) valgrind= ;;
            auto) case `uname -m` in
                      i386|i486|i586|i686) valgrind= ;;
                  esac
        esac

        print_config "valgrind tests: config=$config"
        printf "\nbuilding python ...\n\n"
        $GMAKE distclean > /dev/null 2>&1
        ./configure CFLAGS="$ADD_CFLAGS" LDFLAGS="$ADD_LDFLAGS" --without-pymalloc > /dev/null 2>&1
        $GMAKE | grep _decimal

        printf "\n\n# ======================== valgrind tests ===========================\n\n"
        $valgrind ./python -m test -uall test_decimal

        cd Modules/_decimal
done

# deccheck
cd ../../
for config in $CONFIGS; do

        unset PYTHON_DECIMAL_WITH_MACHINE
        if [ X"$config" != X"auto" ]; then
            PYTHON_DECIMAL_WITH_MACHINE=$config
            export PYTHON_DECIMAL_WITH_MACHINE
        fi

        ############ debug ############
        print_config "deccheck: config=$config --with-pydebug"
        printf "\nbuilding python ...\n\n"

        $GMAKE distclean > /dev/null 2>&1
        ./configure CFLAGS="$ADD_CFLAGS" LDFLAGS="$ADD_LDFLAGS" --with-pydebug > /dev/null 2>&1
        $GMAKE | grep _decimal

        printf "\n\n# ========================== debug ===========================\n\n"
        ./python Modules/_decimal/tests/deccheck.py

        ########### regular ###########
        print_config "deccheck: config=$config "
        printf "\nbuilding python ...\n\n"

        $GMAKE distclean > /dev/null 2>&1
        ./configure CFLAGS="$ADD_CFLAGS" LDFLAGS="$ADD_LDFLAGS" > /dev/null 2>&1
        $GMAKE | grep _decimal

        printf "\n\n# ======================== regular ===========================\n\n"
        ./python Modules/_decimal/tests/deccheck.py

        ########### valgrind ###########
        valgrind=$VALGRIND
        case "$config" in
            # Valgrind has no support for 80 bit long double arithmetic.
            ppro) valgrind= ;;
            auto) case `uname -m` in
                      i386|i486|i586|i686) valgrind= ;;
                  esac
        esac

        print_config "valgrind deccheck: config=$config "
        printf "\nbuilding python ...\n\n"

        $GMAKE distclean > /dev/null 2>&1
        ./configure CFLAGS="$ADD_CFLAGS" LDFLAGS="$ADD_LDFLAGS" --without-pymalloc > /dev/null 2>&1
        $GMAKE | grep _decimal

        printf "\n\n# ======================== valgrind ==========================\n\n"
        $valgrind ./python Modules/_decimal/tests/deccheck.py
done



