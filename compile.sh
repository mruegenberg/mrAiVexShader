#! /bin/bash
if [ -z "$HFS" ]; then
    echo "hello"
    export HFS=/opt/hfs16.0.600
    export PATH=$HFS\bin:$PATH
    export CUR=`pwd`
    cd $HFS
    echo `pwd`
    source houdini_setup
    cd $CUR
fi

# note: for Houdini 15.5, you need GCC 4.8!

ARNOLD_DIR=deps/Arnold-5.0.0.2-linux
HOUDINI_DIR=/opt/hfs16.0.600

hcustom -e -i ./build -g src/vexrgb.cpp -L ${ARNOLD_DIR}/bin -lai -I ${ARNOLD_DIR}/include -L ${HOUDINI_DIR}/dsolib -lHoudiniUI -lHoudiniOPZ -lHoudiniOP3 -lHoudiniOP2 -lHoudiniOP1 -lHoudiniSIM -lHoudiniGEO -lHoudiniPRM -lHoudiniUT -lboost_system
# hcustom -e -i ./build -g src/vexvolume.cpp -L ${ARNOLD_DIR}/bin -lai -I ${ARNOLD_DIR}/include -L ${HOUDINI_DIR}/dsolib -lHoudiniUI -lHoudiniOPZ -lHoudiniOP3 -lHoudiniOP2 -lHoudiniOP1 -lHoudiniSIM -lHoudiniGEO -lHoudiniPRM -lHoudiniUT -lboost_system
