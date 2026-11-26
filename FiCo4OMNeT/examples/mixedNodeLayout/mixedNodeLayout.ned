//Mixed CAN and CAN-FD Network Demo
package fico4omnet.examples.mixedNodeLayout;

import fico4omnet.bus.can.CanBus;
import fico4omnet.nodes.can.CanNode;

//
// Network demonstrating Classical CAN and CAN-FD coexisting on same bus
// canNode sends Classical CAN (green device)
// canFdNode sends CAN-FD (blue device)
//
network MixedNodeLayout
{
    parameters:
        @display("bgb=550,350,white");
    submodules:
        bus: CanBus {
            parameters:
                @display("p=275,175");
                numGates = 4;
        }
        canNode: CanNode {
            @display("p=100,100;i=device/device_s,green");
        }
        canNode2: CanNode {
            @display("p=100,250;i=device/device_s,green");
        }
        canFdNode: CanNode {
            @display("p=450,100;i=device/device_s,gold");
        }
        canFdNode2: CanNode {
            @display("p=450,250;i=device/device_s,gold");
        }
    connections:
        bus.gate[0] <--> canNode.gate;
        bus.gate[1] <--> canFdNode.gate;
        bus.gate[2] <--> canNode2.gate;
        bus.gate[3] <--> canFdNode2.gate;
}
