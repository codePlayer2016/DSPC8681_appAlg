#! /bin/bash

CRC_EXE=crcAdd.exe
CRC_DIR=/gitToolsForCrc/tools4Crc/out

SRC_FILE=/gitToolsForCrc/tools4Crc/out/dspCodeImg.h
SRC_DIR=/gitToolsForCrc/tools4Crc/out

DEST_DIR=/home/hawke/gitDSPC8681/Linux_8681Driver/inc
OLD_DEST_FILE=/home/hawke/gitDSPC8681/Linux_8681Driver/inc/dspCodeImg.h

# generate the dspCodeImg file
cd ${CRC_DIR}
#pwd
./${CRC_EXE}

# delet the old dspCodeImg file
chmod -R 777 ${OLD_DEST_FILE}
rm -rf ${OLD_DEST_FILE}

# mv the update file to the dest.
chmod -R 777 ${SRC_FILE} 
mv ${SRC_FILE} ${DEST_DIR}
