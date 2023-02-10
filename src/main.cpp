#include <iostream>

#include <windows.h>
#include <strsafe.h>
#include <cstdio>
#include <iostream>
#include <chrono>
#include <cmath>

#include "SimConnect.h"

#include "SimconnectExceptionStrings.h"
#include "logging.h"

int quit = 0;
bool initilized = false;
bool flightStarted = false;
bool inFlight = false;

HANDLE hSimConnect = nullptr;

enum EVENT_IDS {
  EVENT_SIM_START,
};

void initialize() {
  if (!SUCCEEDED(SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart"))) {
    LOG_ERROR("Failed to subscribe to SimStart event");
  };

  initilized = true;
  LOG_INFO("SimConnect connection initialized");
};

void CALLBACK DispatchCallback(SIMCONNECT_RECV* pRecv, DWORD cbData, void* pContext) {

  switch (pRecv->dwID) {

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
      LOG_INFO("SIMCONNECT_RECV_ID_SIMOBJECT_DATA");
      break;

    case SIMCONNECT_RECV_ID_CLIENT_DATA:
      LOG_INFO("SIMCONNECT_RECV_ID_CLIENT_DATA");
      break;

    case SIMCONNECT_RECV_ID_EVENT: {
      LOG_INFO("SIMCONNECT_RECV_ID_EVENT");

      auto* evt = (SIMCONNECT_RECV_EVENT*) pRecv;
      switch (evt->uEventID) {
        case EVENT_SIM_START:
          LOG_INFO("EVENT_SIM_START");
          flightStarted = true;
          break;

        default:
          break;
      }
      break;
    }

    case SIMCONNECT_RECV_ID_EVENT_EX1:
      LOG_INFO("SIMCONNECT_RECV_ID_EVENT_EX1");
      break;

    case SIMCONNECT_RECV_ID_OPEN:
      LOG_INFO("SimConnect connection opened");
      initialize();
      break;

    case SIMCONNECT_RECV_ID_EXCEPTION: {
      auto* const pException = reinterpret_cast<SIMCONNECT_RECV_EXCEPTION*>(pRecv);
      LOG_ERROR("Exception in SimConnect connection: "
                + SimconnectExceptionStrings::getSimConnectExceptionString(
        static_cast<SIMCONNECT_EXCEPTION>(pException->dwException))
                + " send_id:" + std::to_string(pException->dwSendID)
                + " index:" + std::to_string(pException->dwIndex));
      break;
    }

    case SIMCONNECT_RECV_ID_QUIT:
      quit = 1;
      break;

    default:
      LOG_WARN("Unknown/Unimplemented SimConnect message received: " + std::to_string(pRecv->dwID));
      break;
  }
}

int main(int argc, char* argv[]) {

  printf("FBW CPP Framework Testing");

  if (!SUCCEEDED(SimConnect_Open(&hSimConnect, "fbw-cpp-framework-test", nullptr, 0, nullptr, 0))) {
    printf("\nUnable to connect to Flight Simulator!");
    return 1;
  }

  while (quit == 0) {
    SimConnect_CallDispatch(hSimConnect, DispatchCallback, nullptr);
    if (!inFlight) {
      if (flightStarted) {
        LOG_INFO("Flight started");
        flightStarted = false;
        inFlight = true;
      }
      else {
        LOG_INFO_BLOCK(
          std::cout << "\r" << "Waiting for flight to start..." << std::flush;
        );
      }
      Sleep(200);
    }
    Sleep(1);
  }

  if (!SUCCEEDED(SimConnect_Close(hSimConnect))) {
    printf("\nUnable to disconnect from Flight Simulator!");
    return 1;
  }
  printf("\nDisconnected from Flight Simulator!");

  return 0;
}
