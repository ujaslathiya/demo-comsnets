package fico4omnet.nodes.can;

import fico4omnet.linklayer.can.CanNodePort;
import fico4omnet.buffer.can.CanInputBuffer;
import fico4omnet.buffer.can.CanOutputBuffer;
import fico4omnet.scheduler.can.CanClock;
import fico4omnet.applications.ISinkApplication;

module CanGateway
{
    parameters:
        @display("i=device/router;bgb=300,250");
        int numInputBuffer = default(2); // One for each port
        int errorperc = default(0);

    gates:
        inout gateStd;
        inout gateFd;

    submodules:
        // Port for Standard CAN Bus
        portStd: CanNodePort {
            @display("p=50,200");
            canPortInput.nodeOutputBufferName = "bufferOutStd";
            gates:
                phygate;
        }

        // Port for CAN-FD Bus
        portFd: CanNodePort {
            @display("p=250,200");
            canPortInput.nodeOutputBufferName = "bufferOutFd";
            gates:
                phygate;
        }
        
        // Incoming Buffers
        bufferInStd: CanInputBuffer {
            @display("p=50,120");
            nodePortName = "portStd";
            gates:
                in[1];
        }
        
        bufferInFd: CanInputBuffer {
            @display("p=250,120");
            nodePortName = "portFd";
            gates:
                in[1];
        }
        
        // Outgoing Buffers
        bufferOutStd: CanOutputBuffer {
            @display("p=100,120");
            nodePortName = "portStd";
            MOB = false; // FIFO Mode: Allow burst of fragments with same ID
            gates:
                in[1];
        }
        
        bufferOutFd: CanOutputBuffer {
            @display("p=200,120");
            nodePortName = "portFd";
            MOB = false; // FIFO Mode
            gates:
                in[1];
        }
        
        // The Brains: Gateway Application
        gatewayApp: <default("CanGatewayApp")> like ISinkApplication {
            @display("p=150,50");
        }
        
        canClock: CanClock {
            @display("p=150,150");
        }
        
    connections:
        // Physical Connections
        portStd.phygate <--> gateStd;
        portFd.phygate <--> gateFd;
        
        // Output Flow: App -> Buffer -> Port
        gatewayApp.outStd --> bufferOutStd.in[0];
        bufferOutStd.out --> portStd.upperLayerIn;
        
        gatewayApp.outFd --> bufferOutFd.in[0];
        bufferOutFd.out --> portFd.upperLayerIn;
        
        // Input Flow: Port -> Buffer -> App
        portStd.upperLayerOut --> bufferInStd.in[0];
        bufferInStd.out --> gatewayApp.inStd;
        
        portFd.upperLayerOut --> bufferInFd.in[0];
        bufferInFd.out --> gatewayApp.inFd;
}
