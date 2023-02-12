#include <iostream>

#include <windows.h>
#include <strsafe.h>
#include <cstdio>

#include "SimConnect.h"

#include "SimconnectExceptionStrings.h"
#include "logging.h"

const int updateDelay = 5000;

int quit = 0;
bool initilized = false;

typedef double FLOAT64;
typedef float FLOAT32;

HANDLE hSimConnect = nullptr;

enum EVENT_IDS {
  EVENT_SIM_START,
};

enum DATA_DEFINE_IDS {
  EXAMPLE_CLIENT_DATA_DEFINITION_ID,
  EXAMPLE2_CLIENT_DATA_DEFINITION_ID,
  DEFINITION_TITLE,
};

enum DATA_REQUEST_IDS {
  EXAMPLE_CLIENT_DATA_REQUEST_ID,
  EXAMPLE2_CLIENT_DATA_REQUEST_ID,
  REQUEST_TITLE,
};

struct Title {
  char title[256] = "";
} title{};

// ClientDataArea variables
const int EXAMPLE_CLIENT_DATA_ID = 0;
const std::string EXAMPLE_CLIENT_DATA_NAME = "EXAMPLE CLIENT DATA";
struct ExampleClientData {
  FLOAT64 aFloat64;
  FLOAT32 aFloat32;
  INT64 anInt64;
  INT32 anInt32;
  INT16 anInt16;
  INT8 anInt8;
} __attribute__((packed));
const size_t exampleClientDataSize = sizeof(ExampleClientData);
ExampleClientData exampleClientData{};

// ClientDataArea variables
const int EXAMPLE2_CLIENT_DATA_ID = 1;
const std::string EXAMPLE2_CLIENT_DATA_NAME = "EXAMPLE 2 CLIENT DATA";
struct Example2ClientData {
  INT8 anInt8;
  INT16 anInt16;
  INT32 anInt32;
  INT64 anInt64;
  FLOAT32 aFloat32;
  FLOAT64 aFloat64;
} __attribute__((packed));
const size_t example2ClientDataSize = sizeof(Example2ClientData);
Example2ClientData example2ClientData{};

void initialize() {
  if (initilized) return;

  LOG_INFO("Initializing SimConnect connection");

  if (!SUCCEEDED(SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart"))) {
    LOG_ERROR("Failed to subscribe to SimStart event");
  }

  if(!SUCCEEDED(SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_TITLE, "TITLE", nullptr, SIMCONNECT_DATATYPE_STRING32))) {
    LOG_ERROR("Failed to add definition for Title");
  }

  // =========================
  // EXAMPLE CLIENT DATA

  // Map the client data area EXAMPLE_CLIENT_DATA_NAME to the client data area ID
  HRESULT hresult = SimConnect_MapClientDataNameToID(hSimConnect, EXAMPLE_CLIENT_DATA_NAME.c_str(), EXAMPLE_CLIENT_DATA_ID);
  if (hresult != S_OK) {
    switch (hresult) {
      case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        LOG_ERROR("Client data area EXAMPLE_CLIENT_DATA_NAME already in use: " + EXAMPLE_CLIENT_DATA_NAME);
        break;
      case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        LOG_ERROR("Client data area ID already in use: " + std::to_string(EXAMPLE_CLIENT_DATA_ID));
        break;
      default:
        LOG_ERROR("Mapping client data area EXAMPLE_CLIENT_DATA_NAME to ID failed: " + EXAMPLE_CLIENT_DATA_NAME);
    }
  }

  // Add the data definition to the client data area
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(
    hSimConnect, EXAMPLE_CLIENT_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO, exampleClientDataSize))) {
    LOG_ERROR("ClientDataAreaVariable: Adding to client data definition failed: " + EXAMPLE_CLIENT_DATA_NAME);
  }

  // =========================
  // EXAMPLE 2 CLIENT DATA

  // Map the client data area EXAMPLE2_CLIENT_DATA_NAME to the client data area ID
  hresult = SimConnect_MapClientDataNameToID(hSimConnect, EXAMPLE2_CLIENT_DATA_NAME.c_str(), EXAMPLE2_CLIENT_DATA_ID);
  if (hresult != S_OK) {
    switch (hresult) {
      case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        LOG_ERROR("Client data area EXAMPLE2_CLIENT_DATA_NAME already in use: " + EXAMPLE2_CLIENT_DATA_NAME);
        break;
      case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        LOG_ERROR("Client data area ID already in use: " + std::to_string(EXAMPLE2_CLIENT_DATA_ID));
        break;
      default:
        LOG_ERROR("Mapping client data area EXAMPLE2_CLIENT_DATA_NAME to ID failed: " + EXAMPLE2_CLIENT_DATA_NAME);
    }
  }

  // Add the data definition to the client data area
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(
    hSimConnect, EXAMPLE2_CLIENT_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO, example2ClientDataSize))) {
    LOG_ERROR("Adding to client data definition failed: " + EXAMPLE2_CLIENT_DATA_NAME);
  }

  // Create/allocate the client data area
  const DWORD readOnlyFlag = SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT;
  if (!SUCCEEDED(SimConnect_CreateClientData(hSimConnect, EXAMPLE2_CLIENT_DATA_ID, example2ClientDataSize, readOnlyFlag))) {
    LOG_ERROR("Creating client data failed: " + EXAMPLE2_CLIENT_DATA_NAME);
  }

  initilized = true;
  LOG_INFO("SimConnect connection initialized");
}

void processReceivedClientData(SIMCONNECT_RECV* pRecv) {
  const auto pClientData = reinterpret_cast<const SIMCONNECT_RECV_CLIENT_DATA*>(pRecv);

  switch (pClientData->dwRequestID) {
    case EXAMPLE_CLIENT_DATA_REQUEST_ID:
      LOG_INFO("Received client data: " + EXAMPLE_CLIENT_DATA_NAME);
      std::memcpy(&exampleClientData, &pClientData->dwData, exampleClientDataSize);
      break;
    case EXAMPLE2_CLIENT_DATA_REQUEST_ID:
      LOG_INFO("Received client data: " + EXAMPLE2_CLIENT_DATA_NAME);
      std::memcpy(&example2ClientData, &pClientData->dwData, example2ClientDataSize);
      break;
    default:
      LOG_WARN("Received unknown client data request ID: " + std::to_string(pClientData->dwRequestID));
      break;
  }
}

void processReceivedSimObjectData(SIMCONNECT_RECV* pRecv) {
  const auto pData = reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA*>(pRecv);
  switch (pData->dwRequestID) {
    case REQUEST_TITLE:
      LOG_INFO("Received sim object data: Title");
      title = *((Title*)&pData->dwData);
      break;
    default:
      LOG_WARN("Received unknown sim object data request ID: " + std::to_string(pData->dwRequestID));
      break;
  }
}

void CALLBACK dispatchCallback(SIMCONNECT_RECV* pRecv,
                               [[maybe_unused]] DWORD cbData,
                               [[maybe_unused]] void* pContext) {

  switch (pRecv->dwID) {

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
      LOG_INFO("SIMCONNECT_RECV_ID_SIMOBJECT_DATA");
      processReceivedSimObjectData(pRecv);
      break;

    case SIMCONNECT_RECV_ID_CLIENT_DATA:
      processReceivedClientData(pRecv);
      break;

    case SIMCONNECT_RECV_ID_EVENT: {
      LOG_INFO("SIMCONNECT_RECV_ID_EVENT");

      auto* evt = (SIMCONNECT_RECV_EVENT*) pRecv;
      switch (evt->uEventID) {
        case EVENT_SIM_START:
          LOG_INFO("EVENT_SIM_START");
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

    case SIMCONNECT_RECV_ID_SYSTEM_STATE: {
      auto* const pState = reinterpret_cast<SIMCONNECT_RECV_SYSTEM_STATE*>(pRecv);
      LOG_INFO("SIMCONNECT_RECV_ID_SYSTEM_STATE: "
               + std::to_string(pState->dwInteger)
               + " " + pState->szString + " "
               + std::to_string(pState->dwRequestID)
               + " " + std::to_string(pState->fFloat));
      break;
    }

    default:
      LOG_WARN("Unknown/Unimplemented SimConnect message received: " + std::to_string(pRecv->dwID));
      break;
  }
}

void getDispatch() {
  SIMCONNECT_RECV* ptrData;
  DWORD cbData;
  while (SUCCEEDED(SimConnect_GetNextDispatch(hSimConnect, &ptrData, &cbData))) {
    dispatchCallback(ptrData, cbData, nullptr);
  }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {

  printf("FBW CPP Framework Testing");

  if (!SUCCEEDED(SimConnect_Open(&hSimConnect, "fbw-cpp-framework-test", nullptr, 0, nullptr, 0))) {
    printf("\nUnable to connect to Flight Simulator!");
    return 1;
  }

  while (quit == 0) {

    if (!initilized) {
      getDispatch();
      Sleep(500);
      continue;
    }

    // Request title
    if (!SUCCEEDED(SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_TITLE, DEFINITION_TITLE, SIMCONNECT_OBJECT_ID_USER,SIMCONNECT_PERIOD_ONCE, SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT))) {
      LOG_ERROR("Requesting title failed");
      continue;
    }

    // Request example client data
    if (!SUCCEEDED(SimConnect_RequestClientData(hSimConnect,
                                                EXAMPLE_CLIENT_DATA_ID,
                                                EXAMPLE_CLIENT_DATA_REQUEST_ID,
                                                EXAMPLE_CLIENT_DATA_DEFINITION_ID,
                                                SIMCONNECT_CLIENT_DATA_PERIOD_ONCE))) {
      LOG_ERROR("ClientDataAreaVariable: Requesting client data failed: " + EXAMPLE_CLIENT_DATA_NAME);
      continue;
    }

    // Change and write example 2 client data
    example2ClientData.aFloat64 += 0.33;
    example2ClientData.aFloat32 += 0.33;
    example2ClientData.anInt64 += 2;
    example2ClientData.anInt32 += 2;
    example2ClientData.anInt16 += 2;
    example2ClientData.anInt8 += 2;

    if (!SUCCEEDED(SimConnect_SetClientData(
      hSimConnect, EXAMPLE2_CLIENT_DATA_ID, EXAMPLE2_CLIENT_DATA_DEFINITION_ID,
      SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0,
      example2ClientDataSize, &example2ClientData))) {

      LOG_ERROR("Setting data to sim for " + EXAMPLE2_CLIENT_DATA_NAME + " with dataDefId="
                + std::to_string(EXAMPLE2_CLIENT_DATA_DEFINITION_ID) + " failed!");
    }

    getDispatch();

    std::cout << "TITLE      " << title.title << std::endl;

    std::cout << "DATA 1 ---- ( requested from sim ) --------------------------------" << std::endl;
    std::cout << "FLOAT64    " << exampleClientData.aFloat64 << std::endl;
    std::cout << "FLOAT32    " << exampleClientData.aFloat32 << std::endl;
    std::cout << "INT64      " << exampleClientData.anInt64 << std::endl;
    std::cout << "INT32      " << exampleClientData.anInt32 << std::endl;
    std::cout << "INT16      " << exampleClientData.anInt16 << std::endl;
    std::cout << "INT8       " << int(exampleClientData.anInt8) << std::endl;

    std::cout << "DATA 2 ---- ( sent to sim ) ---------------------------------------" << std::endl;
    std::cout << "INT8       " << int(example2ClientData.anInt8) << std::endl;
    std::cout << "INT16      " << example2ClientData.anInt16 << std::endl;
    std::cout << "INT32      " << example2ClientData.anInt32 << std::endl;
    std::cout << "INT64      " << example2ClientData.anInt64 << std::endl;
    std::cout << "FLOAT32    " << example2ClientData.aFloat32 << std::endl;
    std::cout << "FLOAT64    " << example2ClientData.aFloat64 << std::endl;

    Sleep(updateDelay);
  }

  if (!SUCCEEDED(SimConnect_Close(hSimConnect))) {
    printf("\nUnable to disconnect from Flight Simulator!");
    return 1;
  }
  printf("\nDisconnected from Flight Simulator!");

  return 0;
}
