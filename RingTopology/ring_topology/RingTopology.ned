package ring_topology;

// ============================================================================
// RingTopology — multi-hop backbone with a shared link
// ============================================================================
// Purpose: Generate data that contain cross-zone contention on a shared backbone link.
//
// Design: Replace ZonalTopology's central switch with a multi-hop Ethernet backbone.
//         Add a second independent CAN-FD zone (B).
//
// Zones at the four corners of the Ethernet backbone:
//   Zone A (CAN-FD):  3 ECUs → CAN_FD_Bus_A → GW_A → SW_A              [2 hops]
//   Zone B (CAN-FD):  3 ECUs → CAN_FD_Bus_B → GW_B → SW_B → SW_A      [3 hops]
//   Zone C (CAN Std): 3 ECUs → CAN_Std_Bus  → GW_C → SW_C → SW_B → SW_A [4 hops]
//   Zone D (LIN):     3 ECUs → LIN_Bus       → GW_D → SW_D → SW_A      [3 hops]
//
// Ethernet backbone (1 Gbps): SW_A — SW_B — SW_C (chain)
//                                 \— SW_D (branch)
// NOTE: SW_C and SW_D are NOT directly connected to avoid Ethernet broadcast
//       storms — TSNEtherSwitch uses SRPRelay (no STP) so a full ring would loop.
//       The SW_B→SW_A shared link still provides the cross-zone contention effect.
//
// KEY property: Link SW_B→SW_A is SHARED by Zone B (CAN-FD) and Zone C (CAN Std).
//   → Cross-zone contention: CAN_Std delay depends on CAN_FD_B load.
//   → This effect is not visible from the features of a single flow.
// ============================================================================

import fico4omnet.nodes.can.CanNode;
import fico4omnet.bus.can.CanBus;
import multiprotocol_gateway.MultiGateway;
import lin.LinBus;
import lin.LinNode;
import core4inet.nodes.ethernet.tsn.TSNEtherSwitch;
import ned.DatarateChannel;

network RingTopology
{
    parameters:
        @display("bgb=1600,750,white");

    types:
        channel CAN_Link extends ned.DatarateChannel
        {
            datarate = 10Mbps;
            delay    = 100ns;
        }
        channel CANFD_Link extends ned.DatarateChannel
        {
            datarate = 10Mbps;
            delay    = 100ns;
        }
        channel LIN_Link extends ned.DatarateChannel
        {
            datarate = 1Mbps;   // physical layer
            delay    = 200ns;
        }
        channel GW_Uplink extends ned.DatarateChannel
        {
            datarate = 100Mbps;
            delay    = 100ns;
        }
        // 1 Gbps Ethernet ring backbone between switches
        channel ETH_Ring extends ned.DatarateChannel
        {
            datarate = 1Gbps;
            delay    = 200ns;   // slightly higher: longer cable between corners
        }

    submodules:

        // ══════════════════════════════════════════════════════════
        // ETHERNET RING BACKBONE — 4 TSN switches, one per zone
        // SW_A = main computing zone (destination for all flows)
        // Ring order: SW_A — SW_B — SW_C — SW_D — SW_A
        // ══════════════════════════════════════════════════════════
        SW_A: TSNEtherSwitch {
            @display("p=530,310;i=block/switch,cyan;is=l");
        }
        SW_B: TSNEtherSwitch {
            @display("p=1000,310;i=block/switch,cyan;is=l");
        }
        SW_C: TSNEtherSwitch {
            @display("p=1000,530;i=block/switch,cyan;is=l");
        }
        SW_D: TSNEtherSwitch {
            @display("p=530,530;i=block/switch,cyan;is=l");
        }

        // ══════════════════════════════════════════════════════════
        // ZONE A — CAN-FD  (left of SW_A, 2-hop path)
        // Path: CAN_FD_Bus_A → GW_A → SW_A
        // ══════════════════════════════════════════════════════════
        CAN_FD_Bus_A: CanBus {
            numGates = 4;
            @display("p=240,250;i=block/network,gold;is=l");
        }
        CAN_FD_Node_A0: CanNode {
            @display("p=80,140;i=device/device,gold");
        }
        CAN_FD_Node_A1: CanNode {
            @display("p=80,250;i=device/device,gold");
        }
        CAN_FD_Node_A2: CanNode {
            @display("p=80,370;i=device/device,gold");
        }
        GW_A: MultiGateway {
            parameters:
                numCanFdPorts = 1;
                numEthPorts   = 1;
            @display("p=390,310;i=device/router,gold");
        }

        // ══════════════════════════════════════════════════════════
        // ZONE B — CAN-FD  (right of SW_B, 3-hop path via ring8)
        // Path: CAN_FD_Bus_B → GW_B → SW_B ──[ring8]──► SW_A
        // ══════════════════════════════════════════════════════════
        CAN_FD_Bus_B: CanBus {
            numGates = 4;
            @display("p=1300,180;i=block/network,darkorange;is=l");
        }
        CAN_FD_Node_B0: CanNode {
            @display("p=1480,90;i=device/device,darkorange");
        }
        CAN_FD_Node_B1: CanNode {
            @display("p=1480,200;i=device/device,darkorange");
        }
        CAN_FD_Node_B2: CanNode {
            @display("p=1480,310;i=device/device,darkorange");
        }
        GW_B: MultiGateway {
            parameters:
                numCanFdPorts = 1;
                numEthPorts   = 1;
            @display("p=1140,310;i=device/router,darkorange");
        }

        // ══════════════════════════════════════════════════════════
        // ZONE C — CAN Standard  (right/below SW_C, 4-hop path)
        // Path: CAN_Std_Bus → GW_C → SW_C ──[ring9]──► SW_B ──[ring8]──► SW_A
        // ★ Shares ring8 with Zone B → cross-zone contention
        // ══════════════════════════════════════════════════════════
        CAN_Std_Bus: CanBus {
            numGates = 4;
            @display("p=1300,590;i=block/network,green;is=l");
        }
        CAN_Std_Node_0: CanNode {
            @display("p=1480,490;i=device/device,green");
        }
        CAN_Std_Node_1: CanNode {
            @display("p=1480,600;i=device/device,green");
        }
        CAN_Std_Node_2: CanNode {
            @display("p=1480,700;i=device/device,green");
        }
        GW_C: MultiGateway {
            parameters:
                numCanPorts = 1;
                numEthPorts = 1;
            @display("p=1140,530;i=device/router,green");
        }

        // ══════════════════════════════════════════════════════════
        // ZONE D — LIN  (left/below SW_D, 3-hop path, independent)
        // Path: LIN_Bus → GW_D → SW_D ──[ring10]──► SW_A
        // ══════════════════════════════════════════════════════════
        LIN_Bus: LinBus {
            numGates = 4;
            @display("p=240,590;i=block/network,purple;is=l");
        }
        LIN_Slave_0: LinNode {
            @display("p=80,490;i=device/device,purple");
        }
        LIN_Slave_1: LinNode {
            @display("p=80,600;i=device/device,purple");
        }
        LIN_Slave_2: LinNode {
            @display("p=80,700;i=device/device,purple");
        }
        GW_D: MultiGateway {
            parameters:
                numLinPorts = 1;
                numEthPorts = 1;
            @display("p=390,530;i=device/router,purple");
        }

    connections allowunconnected:

        // ── Ethernet backbone ─────────────────────────────────────────────
        // ring8:  SW_B ↔ SW_A  (★ SHARED by CAN_FD_B + CAN_Std — KEY shared link)
        SW_A.ethg++ <--> ETH_Ring <--> SW_B.ethg++;
        // ring9:  SW_C ↔ SW_B  (used by CAN_Std only)
        SW_B.ethg++ <--> ETH_Ring <--> SW_C.ethg++;
        // ring10: SW_D ↔ SW_A  (used by LIN only — independent branch)
        SW_D.ethg++ <--> ETH_Ring <--> SW_A.ethg++;
        // NOTE: SW_C ↔ SW_D NOT connected — TSNEtherSwitch has no STP, a full ring
        //       causes an Ethernet broadcast storm (frames loop forever). The 3-link
        //       tree topology preserves all hop counts and the shared ring8 link.

        // ── Zone A: CAN-FD_A ──────────────────────────────────────────────
        CAN_FD_Node_A0.gate <--> CANFD_Link <--> CAN_FD_Bus_A.gate[0];
        CAN_FD_Node_A1.gate <--> CANFD_Link <--> CAN_FD_Bus_A.gate[1];
        CAN_FD_Node_A2.gate <--> CANFD_Link <--> CAN_FD_Bus_A.gate[2];
        CAN_FD_Bus_A.gate[3] <--> GW_A.canFdPort[0];
        GW_A.ethg[0] <--> GW_Uplink <--> SW_A.ethg++;

        // ── Zone B: CAN-FD_B (3 hops via ring8) ───────────────────────────
        CAN_FD_Node_B0.gate <--> CANFD_Link <--> CAN_FD_Bus_B.gate[0];
        CAN_FD_Node_B1.gate <--> CANFD_Link <--> CAN_FD_Bus_B.gate[1];
        CAN_FD_Node_B2.gate <--> CANFD_Link <--> CAN_FD_Bus_B.gate[2];
        CAN_FD_Bus_B.gate[3] <--> GW_B.canFdPort[0];
        GW_B.ethg[0] <--> GW_Uplink <--> SW_B.ethg++;

        // ── Zone C: CAN Std (4 hops, crosses ring9 then ring8) ────────────
        CAN_Std_Node_0.gate <--> CAN_Link <--> CAN_Std_Bus.gate[0];
        CAN_Std_Node_1.gate <--> CAN_Link <--> CAN_Std_Bus.gate[1];
        CAN_Std_Node_2.gate <--> CAN_Link <--> CAN_Std_Bus.gate[2];
        CAN_Std_Bus.gate[3] <--> GW_C.canPort[0];
        GW_C.ethg[0] <--> GW_Uplink <--> SW_C.ethg++;

        // ── Zone D: LIN (3 hops via ring10, fully independent) ────────────
        LIN_Bus.gate[0] <--> LIN_Slave_0.gate;
        LIN_Bus.gate[1] <--> LIN_Slave_1.gate;
        LIN_Bus.gate[2] <--> LIN_Slave_2.gate;
        LIN_Bus.gate[3] <--> GW_D.linPort[0];
        GW_D.ethg[0] <--> GW_Uplink <--> SW_D.ethg++;
}
