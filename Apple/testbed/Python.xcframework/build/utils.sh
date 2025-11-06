# Utility methods for use in an Xcode project.
#
# An iOS XCframework cannot include any content other than the library binary
# and relevant metadata. However, Python requires a standard library at runtime.
# Therefore, it is necessary to add a build step to an Xcode app target that
# processes the standard library and puts the content into the final app.
#
# In general, these tools will be invoked after bundle resources have been
# copied into the app, but before framework embedding (and signing).
#
# The following is an example script, assuming that:
# * Python.xcframework is in the root of the project
# * There is an `app` folder that contains the app code
# * There is an `app_packages` folder that contains installed Python packages.
# -----
#   set -e
#   source $PROJECT_DIR/Python.xcframework/build/build_utils.sh
#   install_python Python.xcframework app app_packages
# -----

# Copy the standard library from the XCframework into the app bundle.
#
# Accepts one argument:
# 1. The path, relative to the root of the Xcode project, where the Python
#    XCframework can be found.
install_stdlib() {
    PYTHON_XCFRAMEWORK_PATH=$1

    mkdir -p "$CODESIGNING_FOLDER_PATH/python/lib"
    if [ "$EFFECTIVE_PLATFORM_NAME" = "-iphonesimulator" ]; then
        echo "Installing Python modules for iOS Simulator"
        if [ -d "$PROJECT_DIR/$PYTHON_XCFRAMEWORK_PATH/ios-arm64-simulator" ]; then
            SLICE_FOLDER="ios-arm64-simulator"
        else
            SLICE_FOLDER="ios-arm64_x86_64-simulator"
        fi
    else
        echo "Installing Python modules for iOS Device"
        SLICE_FOLDER="ios-arm64"
    fi

    # If the XCframework has a shared lib folder, then it's a full framework.
    # Copy both the common and slice-specific part of the lib directory.
    # Otherwise, it's a single-arch framework; use the "full" lib folder.
    if [ -d "$PROJECT_DIR/$PYTHON_XCFRAMEWORK_PATH/lib" ]; then
        rsync -au --delete "$PROJECT_DIR/$PYTHON_XCFRAMEWORK_PATH/lib/" "$CODESIGNING_FOLDER_PATH/python/lib/"
        rsync -au "$PROJECT_DIR/$PYTHON_XCFRAMEWORK_PATH/$SLICE_FOLDER/lib-$ARCHS/" "$CODESIGNING_FOLDER_PATH/python/lib/"
    else
        rsync -au --delete "$PROJECT_DIR/$PYTHON_XCFRAMEWORK_PATH/$SLICE_FOLDER/lib/" "$CODESIGNING_FOLDER_PATH/python/lib/"
    fi
}

# Convert a single .so library into a framework that iOS can load.
#
# Accepts three arguments:
# 1. The path, relative to the root of the Xcode project, where the Python
#    XCframework can be found.
# 2. The base path, relative to the installed location in the app bundle, that
#    needs to be processed. Any .so file found in this path (or a subdirectory
#    of it) will be processed.
# 2. The full path to a single .so file to process. This path should include
#    the base path.
install_dylib () {
    PYTHON_XCFRAMEWORK_PATH=$1
    INSTALL_BASE=$2
    FULL_EXT=$3

    # The name of the extension file
    EXT=$(basename "$FULL_EXT")
    # The name and location of the module
    MODULE_PATH=$(dirname "$FULL_EXT")
    MODULE_NAME=$(echo $EXT | cut -d "." -f 1)
    # The location of the extension file, relative to the bundle
    RELATIVE_EXT=${FULL_EXT#$CODESIGNING_FOLDER_PATH/}
    # The path to the extension file, relative to the install base
    PYTHON_EXT=${RELATIVE_EXT/$INSTALL_BASE/}
    # The full dotted name of the extension module, constructed from the file path.
    FULL_MODULE_NAME=$(echo $PYTHON_EXT | cut -d "." -f 1 | tr "/" ".");
    # A bundle identifier; not actually used, but required by Xcode framework packaging
    FRAMEWORK_BUNDLE_ID=$(echo $PRODUCT_BUNDLE_IDENTIFIER.$FULL_MODULE_NAME | tr "_" "-")
    # The name of the framework folder.
    FRAMEWORK_FOLDER="Frameworks/$FULL_MODULE_NAME.framework"

    # If the framework folder doesn't exist, create it.
    if [ ! -d "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER" ]; then
        echo "Creating framework for $RELATIVE_EXT"
        mkdir -p "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER"
        cp "$PROJECT_DIR/$PYTHON_XCFRAMEWORK_PATH/build/$PLATFORM_FAMILY_NAME-dylib-Info-template.plist" "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER/Info.plist"
        plutil -replace CFBundleExecutable -string "$FULL_MODULE_NAME" "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER/Info.plist"
        plutil -replace CFBundleIdentifier -string "$FRAMEWORK_BUNDLE_ID" "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER/Info.plist"
    fi

    echo "Installing binary for $FRAMEWORK_FOLDER/$FULL_MODULE_NAME"
    mv "$FULL_EXT" "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER/$FULL_MODULE_NAME"
    # Create a placeholder .fwork file where the .so was
    echo "$FRAMEWORK_FOLDER/$FULL_MODULE_NAME" > ${FULL_EXT%.so}.fwork
    # Create a back reference to the .so file location in the framework
    echo "${RELATIVE_EXT%.so}.fwork" > "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER/$FULL_MODULE_NAME.origin"

    # If the framework provides an xcprivacy file, install it.
    if [ -e "$MODULE_PATH/$MODULE_NAME.xcprivacy" ]; then
        echo "Installing XCPrivacy file for $FRAMEWORK_FOLDER/$FULL_MODULE_NAME"
        XCPRIVACY_FILE="$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER/PrivacyInfo.xcprivacy"
        if [ -e "$XCPRIVACY_FILE" ]; then
          rm -rf "$XCPRIVACY_FILE"
        fi
        mv "$MODULE_PATH/$MODULE_NAME.xcprivacy" "$XCPRIVACY_FILE"
    fi

    echo "Signing framework as $EXPANDED_CODE_SIGN_IDENTITY_NAME ($EXPANDED_CODE_SIGN_IDENTITY)..."
    /usr/bin/codesign --force --sign "$EXPANDED_CODE_SIGN_IDENTITY" ${OTHER_CODE_SIGN_FLAGS:-} -o runtime --timestamp=none --preserve-metadata=identifier,entitlements,flags --generate-entitlement-der "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER"
}

# Process all the dynamic libraries in a path into Framework format.
#
# Accepts two arguments:
# 1. The path, relative to the root of the Xcode project, where the Python
#    XCframework can be found.
# 2. The base path, relative to the installed location in the app bundle, that
#    needs to be processed. Any .so file found in this path (or a subdirectory
#    of it) will be processed.
process_dylibs () {
    PYTHON_XCFRAMEWORK_PATH=$1
    LIB_PATH=$2
    find "$CODESIGNING_FOLDER_PATH/$LIB_PATH" -name "*.so" | while read FULL_EXT; do
        install_dylib $PYTHON_XCFRAMEWORK_PATH "$LIB_PATH/" "$FULL_EXT"
    done
}

# The entry point for post-processing a Python XCframework.
#
# Accepts 1 or more arguments:
# 1. The path, relative to the root of the Xcode project, where the Python
#    XCframework can be found. If the XCframework is in the root of the project,
# 2+. The path of a package, relative to the root of the packaged app, that contains
#     library content that should be processed for binary libraries.
install_python() {
    PYTHON_XCFRAMEWORK_PATH=$1
    shift

    install_stdlib $PYTHON_XCFRAMEWORK_PATH
    PYTHON_VER=$(ls -1 "$CODESIGNING_FOLDER_PATH/python/lib")
    echo "Install Python $PYTHON_VER standard library extension modules..."
    process_dylibs $PYTHON_XCFRAMEWORK_PATH python/lib/$PYTHON_VER/lib-dynload

    for package_path in $@; do
        echo "Installing $package_path extension modules ..."
        process_dylibs $PYTHON_XCFRAMEWORK_PATH $package_path
    done
}
