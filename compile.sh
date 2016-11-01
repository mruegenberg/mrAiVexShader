#! /bin/bash
if [ -z "$HFS" ]; then
    export HFS=/opt/hfs15.5.607
    export PATH=$HFS\bin:$PATH
    export CUR=`pwd`
    cd $HFS
    source houdini_setup
    cd $CUR
fi

# note: for Houdini 15.5, you need GCC 4.8!

ARNOLD_DIR=deps/Arnold-4.2.15.1-linux
HOUDINI_DIR=/opt/hfs15.5.607

hcustom -e -i ./build -g src/vexrgb.cpp -L ${ARNOLD_DIR}/bin -lai -I ${ARNOLD_DIR}/include -L ${HOUDINI_DIR}/dsolib -lHoudiniUI -lHoudiniOPZ -lHoudiniOP3 -lHoudiniOP2 -lHoudiniOP1 -lHoudiniSIM -lHoudiniGEO -lHoudiniPRM -lHoudiniUT -lboost_system
