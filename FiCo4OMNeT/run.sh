#!/usr/bin/env bash

ROOT=fico4omnet/bus/can
CC=$ROOT/CanBusLogic.cc
HH=$ROOT/CanBusLogic.h

echo "[1/5] Backing up files"
cp -n $CC ${CC}.bak 2>/dev/null
cp -n $HH ${HH}.bak 2>/dev/null

echo "[2/5] Patching header"
grep -q "colorCanFrame" $HH || sed -i '/colorError()/a\
    void colorCanFrame(CanDataFrame *df);\
' $HH

echo "[3/5] Patching implementation"

grep -q "setGateColor" $CC || sed -i '/Define_Module/a\
static void setGateColor(omnetpp::cGate *g, const char *color) {\
    if (!g) return;\
    g->getDisplayString().setTagArg("ls", 0, color);\
}\
' $CC

grep -q "colorCanFrame(CanDataFrame" $CC || sed -i '/colorError()/a\
void CanBusLogic::colorCanFrame(CanDataFrame *df)\
{\
    if (!df) return;\
    if (df->isCanFd())\
        df->getDisplayString().setTagArg("i", 1, "green");\
    else\
        df->getDisplayString().setTagArg("i", 1, "blue");\
}\
' $CC

grep -q "colorCanFrame(df)" $CC || sed -i '/handleDataFrame(/,/send(/ s/send(/colorCanFrame(df);\n    send(/' $CC

echo "[4/5] Rebuilding"
make clean
make MODE=release -j

BUILD_RC=$?

echo "[5/5] Done"
if [ $BUILD_RC -ne 0 ]; then
    echo "❌ Build failed — see errors above"
else
    echo "✅ Build OK"
    echo "Run:"
    echo "  cd examples/mixedNodeLayout"
    echo "  ./run omnetpp.ini"
fi

echo
read -p "Press ENTER to close..."

