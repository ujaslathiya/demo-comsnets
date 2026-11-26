#include "RoutingTable.h"

namespace multiprotocol_gateway {

RoutingTable::RoutingTable() {
  // ==========================================
  // ZONE-TO-BACKBONE ROUTES
  // IDs 100/101/102 = CAN STD A/B/C (lower ID = higher CAN arbitration priority)
  // IDs 200/201/202 = CAN FD  A/B/C (lower ID = higher CAN arbitration priority)
  // ==========================================
  canRoutes[100] = {Protocol::ETHERNET, 0, 1};
  canRoutes[101] = {Protocol::ETHERNET, 0, 1};
  canRoutes[102] = {Protocol::ETHERNET, 0, 1};

  canRoutes[200] = {Protocol::ETHERNET, 0, 1};
  canRoutes[201] = {Protocol::ETHERNET, 0, 1};
  canRoutes[202] = {Protocol::ETHERNET, 0, 1};

  // ==========================================
  // ETHERNET-TO-ZONES ROUTES (Segmentation)
  // ==========================================
  // 3. Ethernet -> Standard CAN (ID 120)
  ethRoutes[2576] = {Protocol::CAN, 0, 120}; 

  // 4. Ethernet -> CAN-FD (ID 220)
  ethRoutes[2848] = {Protocol::CAN_FD, 0, 220};

  // Legacy/Default rules
  ethRoutes[0xFFFF] = {Protocol::CAN_FD, 0, 0x2B};
}

bool RoutingTable::lookupCanRoute(unsigned int canId, RouteEntity &outRoute) {
  auto it = canRoutes.find(canId);
  if (it != canRoutes.end()) {
    outRoute = it->second;
    return true;
  }
  return false;
}

bool RoutingTable::lookupEthRoute(int destId, RouteEntity &outRoute) {
  auto it = ethRoutes.find(destId);
  if (it != ethRoutes.end()) {
    outRoute = it->second;
    return true;
  }
  return false;
}

bool RoutingTable::lookupLinRoute(unsigned int linPid, RouteEntity &outRoute) {
  auto it = linRoutes.find(linPid);
  if (it != linRoutes.end()) {
    outRoute = it->second;
    return true;
  }
  return false;
}

} // namespace multiprotocol_gateway
