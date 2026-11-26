package fico4omnet.nodes.can;

import fico4omnet.linklayer.can.CanNodePort;
import fico4omnet.buffer.can.CanInputBuffer;
import fico4omnet.buffer.can.CanOutputBuffer;
import fico4omnet.applications.ISourceApplication;
import fico4omnet.applications.ISinkApplication;
import fico4omnet.scheduler.can.CanClock;

// A node with two independent CAN interfaces (like a gateway or dual-homed controller)
module CanNodeDual
{
    parameters:
        @display("i=device/modem");
        int numSourceApps = default(1); 
        int numSinkApps = default(1);
        int numInputBuffer = default(1);
        int errorperc = default(0);

    gates:
        inout gate1; // Interface 1
        inout gate2; // Interface 2

    submodules:
        canClock: CanClock {
            @display("p=175,125");
        }

        // ===================================
        // INTERFACE 1 (Left Side)
        // ===================================
        canNodePort1: CanNodePort {
            @display("p=75,200");
        }
        bufferIn1[numInputBuffer]: CanInputBuffer {
            @display("p=125,150");
            nodePortName = "canNodePort1";
            gates: in[1];
        }
        bufferOut1: CanOutputBuffer {
            @display("p=25,100");
            nodePortName = "canNodePort1";
            gates: in[parent.numSourceApps];
        }
        sourceApp1[numSourceApps]: <default("CanTrafficSourceAppBase")> like ISourceApplication {
            @display("p=25,50");
        }
        sinkApp1[numSinkApps]: <default("CanTrafficSinkAppBase")> like ISinkApplication {
            @display("p=125,50");
        }

        // ===================================
        // INTERFACE 2 (Right Side)
        // ===================================
        canNodePort2: CanNodePort {
            @display("p=275,200");
        }
        bufferIn2[numInputBuffer]: CanInputBuffer {
            @display("p=225,150");
            nodePortName = "canNodePort2";
            gates: in[1];
        }
        bufferOut2: CanOutputBuffer {
            @display("p=325,100");
            nodePortName = "canNodePort2";
            gates: in[parent.numSourceApps];
        }
        sourceApp2[numSourceApps]: <default("CanTrafficSourceAppBase")> like ISourceApplication {
            @display("p=325,50");
        }
        sinkApp2[numSinkApps]: <default("CanTrafficSinkAppBase")> like ISinkApplication {
            @display("p=225,50");
        }

    connections:
        // Interface 1 Wiring
        canNodePort1.phygate <--> gate1;
        canNodePort1.upperLayerIn <-- bufferOut1.out;
        for i=0..numSourceApps-1 {
            sourceApp1[i].out --> bufferOut1.in[i];
        }

        // Interface 2 Wiring
        canNodePort2.phygate <--> gate2;
        canNodePort2.upperLayerIn <-- bufferOut2.out;
        for i=0..numSourceApps-1 {
            sourceApp2[i].out --> bufferOut2.in[i];
        }
}
