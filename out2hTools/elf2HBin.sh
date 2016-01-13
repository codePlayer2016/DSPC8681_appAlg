export CGT_INSTALL_DIR=/opt/ti/ccsv5/tools/compiler/c6000_7.4.16
export TARGET=6678
export ENDIAN=little

# project name.
DSPPRJ=DSPC8681_DPMV0.93

# input file name.
SRCOUTFILE=${DSPPRJ}.out

# output file name.
OUTPUTFILE=DPUCore_6678.h

# array name
ARRAYNAME=_DPUCore

echo CGT_INSTALL_DIR set as: ${CGT_INSTALL_DIR}
echo TARGET set as: ${TARGET}
echo Converting .out to HEX ...
if [ ${ENDIAN} == little ]
then
${CGT_INSTALL_DIR}/bin/hex6x -order L DPUcoreCtl.rmd ${SRCOUTFILE}
else
${CGT_INSTALL_DIR}/bin/hex6x -order M DPUcoreCtl.rmd ${SRCOUTFILE}
fi

./Bttbl2Hfile DPUcore_temp.btbl DPUcore_temp.h DPUcore_temp.bin

./hfile2array DPUcore_temp.h DPUcore.h _DPUCore

if [ ${ENDIAN} == little ]
then
mv DPUcore.h ${OUTPUTFILE}
else
mv DPUcore.h ${OUTPUTFILE}
fi
chmod -R 777 ${OUTPUTFILE}

