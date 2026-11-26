package fico4omnet.nodes.can;

// A simple wrapper that contains two standard CanNodes to act as a bridge
module CanNodeBridge
{
    parameters:
        @display("i=device/server;bgb=250,150,white");

    gates:
        inout gate[2];

    submodules:
        // Interface 1 Node
        node1: CanNode {
            @display("p=75,75");
        }
        
        // Interface 2 Node
        node2: CanNode {
            @display("p=175,75");
        }

    connections:
        gate[0] <--> node1.gate;
        gate[1] <--> node2.gate;
}
