package fico4omnet.applications.can.gateway;

import fico4omnet.applications.ISinkApplication;

simple CanGatewayApp like ISinkApplication
{
    parameters:
        @display("i=block/app");
        //Signal for fragmented frames count
        @signal[gatewayFragmented](type=long);
        //Signal for forwarded frames count
        @signal[gatewayForwarded](type=long);
    gates:
        input inStd;
        input inFd;
        output outStd;
        output outFd;
}
