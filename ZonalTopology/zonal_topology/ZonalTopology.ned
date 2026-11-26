package zonal_topology;

// ============================================================================
// ZonalTopology — zonal in-vehicle network with TSN backbone
// ============================================================================
// 4 Zones, 3 ECUs each:
//   Zone 1: CAN Standard  — 3 nodes on shared CAN bus → Gateway_1
//   Zone 2: CAN-FD        — 3 nodes on shared CAN-FD bus → Gateway_2
//   Zone 3: AE with TSN   — 3 nodes, each via TSN switch → Gateway_3
//   Zone 4: LIN           — 3 slaves on shared LIN bus → Gateway_4
//   All gateways connect to Central TSN Switch backbone.
//
// TSN features (via CoRE4INET):
//   • IEEE 802.1Qbv Time-Aware Shaper (TAS) — gate control lists on edge switch uplinks
//   • IEEE 802.1Qav Credit-Based Shaper (CBS) — SR Class A (PCP 7) and B (PCP 6)
//   • IEEE 802.1Q VLAN tagging — ETH nodes send at VID 1 / PCP 4
//   • Stream Reservation Protocol (SRP) — SRPRelay inside TSNEtherSwitch
//
// Traffic class mapping:
//   PCP 7 = SR Class A — safety-critical (CAN Std derived, CBS + TAS protected)
//   PCP 6 = SR Class B — control streams (CAN-FD derived, CBS)
//   PCP 4 = Best-effort priority — standard Ethernet node traffic (VID 1)
//   PCP 0 = Background — untagged gateway-forwarded traffic
// ============================================================================

import fico4omnet.nodes.can.CanNode;
import fico4omnet.bus.can.CanBus;
import multiprotocol_gateway.MultiGateway;
import lin.LinBus;
import lin.LinNode;
import core4inet.nodes.ethernet.tsn.TSNEtherSwitch;
import core4inet.nodes.ethernet.tsn.TSNEtherHost;
import ned.DatarateChannel;

network ZonalTopology
{
    parameters:
        @display("bgb=1500,700,white");

    types:
        channel CAN_Link extends ned.DatarateChannel
        {
            // High enough to never bottleneck; CanBusLogic.nextidle governs timing
            datarate = 10Mbps;
            delay = 100ns;
        }
        channel CANFD_Link extends ned.DatarateChannel
        {
            // High enough to never bottleneck; CanBusLogic.nextidle governs timing
            datarate = 10Mbps;
            delay = 100ns;
        }
        channel GW_Uplink extends ned.DatarateChannel
        {
            datarate = 100Mbps;
            delay = 100ns;
        }
        channel ETH_Access extends ned.DatarateChannel
        {
            datarate = 100Mbps;
            delay = 100ns;
        }
        channel ETH_Agg extends ned.DatarateChannel
        {
            datarate = 1Gbps;
            delay = 100ns;
        }

    submodules:
        // ==========================================
        // ZONE 1: CAN Standard (3 ECUs)
        // ==========================================
        CAN_Std_Bus: CanBus {
            numGates = 4;
            @display("i=block/network;p=308.32,172.96");
        }
        CAN_Std_Node_A: CanNode {
            @display("p=150.4,63.920002;i=device/device,green");
        }
        CAN_Std_Node_B: CanNode {
            @display("p=110.92,172.96;i=device/device,green");
        }
        CAN_Std_Node_C: CanNode {
            @display("p=122.200005,289.52;i=device/device,green");
        }

        // ==========================================
        // ZONE 2: CAN-FD (3 ECUs)
        // ==========================================
        CAN_FD_Bus: CanBus {
            numGates = 4;
            @display("i=block/network;p=430.52,535.8");
        }
        CAN_FD_Node_A: CanNode {
            @display("p=133.48,441.80002;i=device/device,gold");
        }
        CAN_FD_Node_B: CanNode {
            @display("p=148.52,535.8;i=device/device,gold");
        }
        CAN_FD_Node_C: CanNode {
            @display("p=195.52,611;i=device/device,gold");
        }

        // ==========================================
        // ZONE 3: Automotive Ethernet with TSN (3 ECUs)
        // TSNEtherSwitch: IEEE 802.1Qbv TAS + IEEE 802.1Qav CBS + SRP
        // TSNEtherHost: IEEE 802.1Q application traffic (PCP 4 / VID 1)
        // ==========================================
        Ethernet_Switch_A: TSNEtherSwitch {
            @display("i=block/network;p=1088.52,302.68");
        }
        Ethernet_Switch_B: TSNEtherSwitch {
            @display("i=block/network;p=1126.12,462.48");
        }
        Ethernet_Switch_C: TSNEtherSwitch {
            @display("i=block/network;p=1088.52,597.84");
        }
        Ethernet_Node_A: TSNEtherHost {
            @display("p=1325.4,246.28;i=device/pc,blue");
        }
        Ethernet_Node_B: TSNEtherHost {
            @display("p=1349.84,424.88;i=device/pc,blue");
        }
        Ethernet_Node_C: TSNEtherHost {
            @display("p=1349.84,597.84;i=device/pc,blue");
        }

        // ==========================================
        // ZONE 4: LIN (3 ECUs)
        // ==========================================
        LIN_Bus: LinBus {
            numGates = 4;
            @display("p=765.16003,99.64;i=block/network,purple");
        }
        LIN_Slave_A: LinNode {
            @display("p=1003.92,18.8;i=device/device,purple");
        }
        LIN_Slave_B: LinNode {
            @display("p=1052.8,99.64;i=device/device,purple");
        }
        LIN_Slave_C: LinNode {
            @display("p=979.48,197.4;i=device/device,purple");
        }

        // ==========================================
        // GATEWAYS & CENTRAL BACKBONE
        // ==========================================
        Gateway_1: MultiGateway {
            parameters:
                numCanPorts = 1;
                numEthPorts = 1;
                @display("p=511.36002,313.96;i=device/router,green");
        }
        Gateway_2: MultiGateway {
            parameters:
                numCanFdPorts = 1;
                numEthPorts = 1;
                @display("p=663.64,548.96;i=device/router,gold");
        }
        Gateway_3: MultiGateway {
            parameters:
                numEthPorts = 4;
                @display("p=934.36,462.48;i=device/router,blue");
        }
        Gateway_4: MultiGateway {
            parameters:
                numLinPorts = 1;
                numEthPorts = 1;
                @display("p=622.28,146.64;i=device/router,purple");
        }
        // CBS-enabled central aggregation switch
        Central_Gateway: TSNEtherSwitch {
            @display("p=691.84,351.56");
        }

    connections allowunconnected:
        // Zone 1: CAN Standard
        CAN_Std_Node_A.gate <--> CAN_Link <--> CAN_Std_Bus.gate[0];
        CAN_Std_Node_B.gate <--> CAN_Link <--> CAN_Std_Bus.gate[1];
        CAN_Std_Node_C.gate <--> CAN_Link <--> CAN_Std_Bus.gate[2];
        CAN_Std_Bus.gate[3] <--> Gateway_1.canPort[0];
        Gateway_1.ethg[0] <--> GW_Uplink <--> Central_Gateway.ethg++;

        // Zone 2: CAN-FD
        CAN_FD_Node_A.gate <--> CANFD_Link <--> CAN_FD_Bus.gate[0];
        CAN_FD_Node_B.gate <--> CANFD_Link <--> CAN_FD_Bus.gate[1];
        CAN_FD_Node_C.gate <--> CANFD_Link <--> CAN_FD_Bus.gate[2];
        CAN_FD_Bus.gate[3] <--> Gateway_2.canFdPort[0];
        Gateway_2.ethg[0] <--> GW_Uplink <--> Central_Gateway.ethg++;

        // Zone 3: Automotive Ethernet with TSN
        Ethernet_Node_A.ethg <--> ETH_Access <--> Ethernet_Switch_A.ethg++;
        Ethernet_Node_B.ethg <--> ETH_Access <--> Ethernet_Switch_B.ethg++;
        Ethernet_Node_C.ethg <--> ETH_Access <--> Ethernet_Switch_C.ethg++;
        Gateway_3.ethg[0] <--> ETH_Agg <--> Ethernet_Switch_A.ethg++;
        Gateway_3.ethg[1] <--> ETH_Agg <--> Ethernet_Switch_B.ethg++;
        Gateway_3.ethg[2] <--> ETH_Agg <--> Ethernet_Switch_C.ethg++;
        Gateway_3.ethg[3] <--> GW_Uplink <--> Central_Gateway.ethg++;

        // Zone 4: LIN
        LIN_Bus.gate[0] <--> LIN_Slave_A.gate;
        LIN_Bus.gate[1] <--> LIN_Slave_B.gate;
        LIN_Bus.gate[2] <--> LIN_Slave_C.gate;
        LIN_Bus.gate[3] <--> Gateway_4.linPort[0];
        Gateway_4.ethg[0] <--> GW_Uplink <--> Central_Gateway.ethg++;
}
