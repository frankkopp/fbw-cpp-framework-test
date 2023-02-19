#include <iostream>

#include <windows.h>
#include <strsafe.h>
#include <string>
#include <random>
#include <array>
#include <cassert>
#include <iomanip>

#include "SimConnect.h"

#include "SimconnectExceptionStrings.h"
#include "logging.h"
#include "longtext.h"

const int updateDelay = 2500;

int quit = 0;
bool initilized = false;

typedef double FLOAT64;
typedef float FLOAT32;
typedef unsigned char BYTE;

HANDLE hSimConnect = nullptr;

enum EVENT_IDS {
  EVENT_SIM_START,
};

enum CLIENT_DATA_IDS {
  EXAMPLE_CLIENT_DATA_ID,
  EXAMPLE2_CLIENT_DATA_ID,
  BIG_CLIENT_DATA_ID,
  HUGE_CLIENT_META_DATA_ID,
  HUGE_CLIENT_DATA_ID,
};

enum DATA_DEFINE_IDS {
  DEFINITION_TITLE,
  EXAMPLE_CLIENT_DATA_DEFINITION_ID,
  EXAMPLE2_CLIENT_DATA_DEFINITION_ID,
  BIG_CLIENT_DATA_DEFINITION_ID,
  HUGE_CLIENT_META_DATA_DEFINITION_ID,
  HUGE_CLIENT_DATA_DEFINITION_ID,
};

enum DATA_REQUEST_IDS {
  REQUEST_TITLE,
  EXAMPLE_CLIENT_DATA_REQUEST_ID,
  EXAMPLE2_CLIENT_DATA_REQUEST_ID,
  BIG_CLIENT_DATA_REQUEST_ID,
  HUGE_CLIENT_META_DATA_REQUEST_ID,
  HUGE_CLIENT_DATA_REQUEST_ID,
};

// Title string sim variable
struct Title {
  char title[256] = "";
} title{};

// ClientDataArea variables
const std::string EXAMPLE_CLIENT_DATA_NAME = "EXAMPLE CLIENT DATA";
struct ExampleClientData {
  FLOAT64 aFloat64;
  FLOAT32 aFloat32;
  INT64 anInt64;
  INT32 anInt32;
  INT16 anInt16;
  INT8 anInt8;
} __attribute__((packed)) exampleClientData{};
const size_t exampleClientDataSize = sizeof(ExampleClientData);

// ClientDataArea variables
const std::string EXAMPLE2_CLIENT_DATA_NAME = "EXAMPLE 2 CLIENT DATA";
struct Example2ClientData {
  INT8 anInt8;
  INT16 anInt16;
  INT32 anInt32;
  INT64 anInt64;
  FLOAT32 aFloat32;
  FLOAT64 aFloat64;
} __attribute__((packed)) example2ClientData{};
const size_t example2ClientDataSize = sizeof(Example2ClientData);

// Big ClientDataArea variable
const std::string BIG_CLIENT_DATA_NAME = "BIG CLIENT DATA";
struct BigClientData {
  std::array<BYTE, SIMCONNECT_CLIENTDATA_MAX_SIZE> dataChunk;
} __attribute__((packed)) bigClientData{};

// Huge client data meta data
const std::string HUGE_CLIENT_META_DATA_NAME = "HUGE CLIENT DATA META DATA";
struct HugeClientMetaData {
  size_t size;
  size_t hash;
} __attribute__((packed)) hugeClientMetaData{};
const size_t hugeClientMetaDataSize = sizeof(HugeClientMetaData);

// Huge client data area
const std::string HUGE_CLIENT_DATA_NAME = "HUGE CLIENT DATA";
constexpr DWORD ChunkSize = SIMCONNECT_CLIENTDATA_MAX_SIZE;
const size_t hugeClientDataSize = longText.size();
const size_t hugeClientDataSizeInBytes = hugeClientDataSize * sizeof(BYTE);
std::size_t hugeClientDataHash;
std::vector<BYTE> hugeClientData{};


void initialize() {
  if (initilized) return;

  LOG_INFO("Initializing SimConnect connection");

  if (!SUCCEEDED(SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart"))) {
    LOG_ERROR("Failed to subscribe to SimStart event");
  }

  if (!SUCCEEDED(SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_TITLE, "TITLE", nullptr,
                                                SIMCONNECT_DATATYPE_STRING256))) {
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
  if (!SUCCEEDED(SimConnect_CreateClientData(hSimConnect, EXAMPLE2_CLIENT_DATA_ID, example2ClientDataSize,
                                             SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT))) {
    LOG_ERROR("Creating client data failed: " + EXAMPLE2_CLIENT_DATA_NAME);
  }

  // =========================
  // BIG CLIENT DATA

  hresult = SimConnect_MapClientDataNameToID(hSimConnect, BIG_CLIENT_DATA_NAME.c_str(), BIG_CLIENT_DATA_ID);
  if (hresult != S_OK) {
    switch (hresult) {
      case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        LOG_ERROR("Client data area BIG_CLIENT_DATA_NAME already in use: " + BIG_CLIENT_DATA_NAME);
        break;
      case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        LOG_ERROR("Client data area ID already in use: " + std::to_string(BIG_CLIENT_DATA_ID));
        break;
      default:
        LOG_ERROR("Mapping client data area BIG_CLIENT_DATA_NAME to ID failed: " + BIG_CLIENT_DATA_NAME);
    }
  }

  // Add the data definition to the client data area
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(
    hSimConnect, BIG_CLIENT_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO, sizeof(bigClientData)))) {
    LOG_ERROR("Adding to client data definition failed: " + BIG_CLIENT_DATA_NAME);
  }

  // Create/allocate the client data area
  if (!SUCCEEDED(SimConnect_CreateClientData(hSimConnect, BIG_CLIENT_DATA_ID, sizeof(bigClientData),
                                             SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT))) {
    LOG_ERROR("Creating client data failed: " + BIG_CLIENT_DATA_NAME);
  }

  // =========================
  // HUGE CLIENT META DATA

  hresult = SimConnect_MapClientDataNameToID(hSimConnect, HUGE_CLIENT_META_DATA_NAME.c_str(), HUGE_CLIENT_META_DATA_ID);
  if (hresult != S_OK) {
    switch (hresult) {
      case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        LOG_ERROR("Client data area HUGE_CLIENT_META_DATA_NAME already in use: " + HUGE_CLIENT_META_DATA_NAME);
        break;
      case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        LOG_ERROR("Client data area ID already in use: " + std::to_string(HUGE_CLIENT_META_DATA_ID));
        break;
      default:
        LOG_ERROR("Mapping client data area HUGE_CLIENT_META_DATA_NAME to ID failed: " + HUGE_CLIENT_META_DATA_NAME);
    }
  }

  // Add the data definition to the client data area
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(
    hSimConnect, HUGE_CLIENT_META_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO, hugeClientMetaDataSize))) {
    LOG_ERROR("Adding to client data definition failed: " + HUGE_CLIENT_META_DATA_NAME);
  }

  // Create/allocate the client data area
  if (!SUCCEEDED(SimConnect_CreateClientData(hSimConnect, HUGE_CLIENT_META_DATA_ID, hugeClientMetaDataSize,
                                             SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT))) {
    LOG_ERROR("Creating client data failed: " + HUGE_CLIENT_META_DATA_NAME);
  }

  // =========================
  // HUGE CLIENT DATA

  hresult = SimConnect_MapClientDataNameToID(hSimConnect, HUGE_CLIENT_DATA_NAME.c_str(), HUGE_CLIENT_DATA_ID);
  if (hresult != S_OK) {
    switch (hresult) {
      case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        LOG_ERROR("Client data area HUGE_CLIENT_DATA_NAME already in use: " + HUGE_CLIENT_DATA_NAME);
        break;
      case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        LOG_ERROR("Client data area ID already in use: " + std::to_string(HUGE_CLIENT_DATA_ID));
        break;
      default:
        LOG_ERROR("Mapping client data area HUGE_CLIENT_DATA_NAME to ID failed: " + HUGE_CLIENT_DATA_NAME);
    }
  }

  // Add the data definition to the client data area
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(
    hSimConnect, HUGE_CLIENT_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO, ChunkSize))) {
    LOG_ERROR("Adding to client data definition failed: " + HUGE_CLIENT_DATA_NAME);
  }

  // Create/allocate the client data area
  if (!SUCCEEDED(SimConnect_CreateClientData(hSimConnect, HUGE_CLIENT_DATA_ID, ChunkSize,
                                             SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT))) {
    LOG_ERROR("Creating client data failed: " + HUGE_CLIENT_DATA_NAME);
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
      title = *((Title*) &pData->dwData);
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

void fillWitRandomCharData(std::vector<BYTE> &vec, const size_t size) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, 61);
  const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  for (int i = 0; i < size; ++i) {
    vec.emplace_back(charset[dis(gen)]);
  }
}

// Fowler-Noll-Vo hash function
uint64_t fingerPrintFVN(std::vector<BYTE> &data) {
  const uint64_t FNV_offset_basis = 14695981039346656037ULL;
  const uint64_t FNV_prime = 1099511628211ULL;
  uint64_t hash = FNV_offset_basis;
  for (BYTE c: data) {
    hash ^= static_cast<uint64_t>(c);
    hash *= FNV_prime;
  }
  return hash;
}

void simconnectLoop() {
  while (quit == 0) {

    if (!initilized) {
      getDispatch();
      Sleep(500);
      continue;
    }

    // Request title
    if (!SUCCEEDED(SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_TITLE, DEFINITION_TITLE,
                                                     SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE,
                                                     SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT))) {
      LOG_ERROR("Requesting title failed");
      break;
    }

    // =========================
    // EXAMPLE CLIENT DATA
    if (!SUCCEEDED(SimConnect_RequestClientData(hSimConnect,
                                                EXAMPLE_CLIENT_DATA_ID,
                                                EXAMPLE_CLIENT_DATA_REQUEST_ID,
                                                EXAMPLE_CLIENT_DATA_DEFINITION_ID,
                                                SIMCONNECT_CLIENT_DATA_PERIOD_ONCE))) {
      LOG_ERROR("ClientDataAreaVariable: Requesting client data failed: " + EXAMPLE_CLIENT_DATA_NAME);
      break;
    }

    // =========================
    // EXAMPLE 2 CLIENT DATA

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
      break;
    }

    // =========================
    // BIG CLIENT DATA

    if (!SUCCEEDED(SimConnect_SetClientData(
      hSimConnect, BIG_CLIENT_DATA_ID, BIG_CLIENT_DATA_DEFINITION_ID,
      SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0,
      sizeof(bigClientData), &bigClientData))) {

      LOG_ERROR("Setting data to sim for " + BIG_CLIENT_DATA_NAME + " with dataDefId="
                + std::to_string(BIG_CLIENT_DATA_DEFINITION_ID) + " failed!");
      break;
    }

    // =========================
    // HUGE CLIENT META DATA

    hugeClientMetaData.size = hugeClientDataSizeInBytes;
    hugeClientMetaData.hash = hugeClientDataHash;
    if (!SUCCEEDED(SimConnect_SetClientData(
      hSimConnect, HUGE_CLIENT_META_DATA_ID, HUGE_CLIENT_META_DATA_DEFINITION_ID,
      SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0,
      hugeClientMetaDataSize, &hugeClientMetaData))) {

      LOG_ERROR("Setting data to sim for " + HUGE_CLIENT_META_DATA_NAME + " with dataDefId="
                + std::to_string(HUGE_CLIENT_META_DATA_DEFINITION_ID) + " failed!");
      break;
    }

    // =========================
    // HUGE CLIENT DATA

    int chunkCount = 0;
    std::size_t sentBytes = 0;

    std::cout << "Huge client data size: " << hugeClientDataSize << std::endl;
    std::cout << "Huge client data size in bytes: " << hugeClientDataSizeInBytes << std::endl;

    assert((hugeClientDataSizeInBytes == hugeClientDataSize) && "Huge client data size is not equal to huge client data size in bytes");

    while (sentBytes < hugeClientDataSize) {
      std::size_t remainingBytes = hugeClientData.size() - sentBytes;
      std::cout << "Sent bytes: " << sentBytes << std::endl;
      std::cout << "Remaining bytes: " << remainingBytes << std::endl;

      if (remainingBytes >= ChunkSize) {
        std::cout << "Sending chunk: " << std::setw(2) << ++chunkCount << std::endl;
        //        for (int i = 0; i < ChunkSize; ++i) {
        //          std::cout << hugeClientData[sentBytes + i];
        //        }
        //        std::cout << std::endl;

        BYTE* pDataSet = &hugeClientData[sentBytes];
        if (!SUCCEEDED(SimConnect_SetClientData(
          hSimConnect, HUGE_CLIENT_DATA_ID, HUGE_CLIENT_DATA_DEFINITION_ID,
          SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0,
          ChunkSize, pDataSet))) {

          LOG_ERROR("Setting data to sim for " + HUGE_CLIENT_DATA_NAME + " with dataDefId="
                    + std::to_string(HUGE_CLIENT_DATA_DEFINITION_ID) + " failed!");
          break;
        }
        sentBytes += ChunkSize;
      }
      else {
        std::array<BYTE, ChunkSize> buffer{};
        BYTE* const pDataSet = &hugeClientData[sentBytes];
        std::memcpy(buffer.data(), pDataSet, remainingBytes);

        std::cout << "Sending chunk: " << std::setw(2) << ++chunkCount << " (last) " << std::endl;
        std::cout << "Sent bytes: " << sentBytes << std::endl;
        std::cout << "Remaining bytes: " << remainingBytes << std::endl;
        std::cout << "hugeClientDataSize: " << hugeClientData.size() << std::endl;
        //        for (char i: buffer) {
        //          std::cout << i;
        //          if (i == 0) {
        //            break;
        //          }
        //        }
        //        std::cout << std::endl;

        if (!SUCCEEDED(SimConnect_SetClientData(hSimConnect, HUGE_CLIENT_DATA_ID, HUGE_CLIENT_DATA_DEFINITION_ID,
                                           SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0,
                                           ChunkSize, buffer.data()))) {
          LOG_ERROR("Setting data to sim for " + HUGE_CLIENT_DATA_NAME + " with dataDefId="
                    + std::to_string(HUGE_CLIENT_DATA_DEFINITION_ID) + " failed!");
          break;
        }
        sentBytes += remainingBytes;
      }
    }

    // =========================
    // DISPATCH

    getDispatch();

    //    std::cout << "TITLE      " << title.title << std::endl;
    //
    //    std::cout << "DATA 1 ---- ( requested from sim ) --------------------------------" << std::endl;
    //    std::cout << "FLOAT64    " << exampleClientData.aFloat64 << std::endl;
    //    std::cout << "FLOAT32    " << exampleClientData.aFloat32 << std::endl;
    //    std::cout << "INT64      " << exampleClientData.anInt64 << std::endl;
    //    std::cout << "INT32      " << exampleClientData.anInt32 << std::endl;
    //    std::cout << "INT16      " << exampleClientData.anInt16 << std::endl;
    //    std::cout << "INT8       " << int(exampleClientData.anInt8) << std::endl;
    //
    //    std::cout << "DATA 2 ---- ( sent to sim ) ---------------------------------------" << std::endl;
    //    std::cout << "INT8       " << int(example2ClientData.anInt8) << std::endl;
    //    std::cout << "INT16      " << example2ClientData.anInt16 << std::endl;
    //    std::cout << "INT32      " << example2ClientData.anInt32 << std::endl;
    //    std::cout << "INT64      " << example2ClientData.anInt64 << std::endl;
    //    std::cout << "FLOAT32    " << example2ClientData.aFloat32 << std::endl;
    //    std::cout << "FLOAT64    " << example2ClientData.aFloat64 << std::endl;

    std::cout << "BIG META DATA  ---- ( sent to sim ) ------------------------------" << std::endl;
    std::cout << "Big client data size: " << sizeof(bigClientData) << std::endl;

    std::cout << "HUGE META DATA  ---- ( sent to sim ) ------------------------------" << std::endl;
    std::cout << "Huge client data size: " << hugeClientMetaData.size << std::endl;
    std::cout << "Huge client data hash: " << hugeClientMetaData.hash << std::endl;

    std::cout << "HUGE DATA  ---- ( sent to sim ) -----------------------------------" << std::endl;
    std::cout << "Huge client data size: " << hugeClientData.size() << std::endl;

    Sleep(updateDelay);
  }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {

  using namespace std;

  cout << "FBW CPP Framework Testing" << endl;

  // Prepare test data for big client data
  cout << "Preparing test data for big client data..." << std::endl;
  auto l = std::min(longText.size(), static_cast<std::size_t>( SIMCONNECT_CLIENTDATA_MAX_SIZE));
  copy(longText.begin(), longText.begin() + l, bigClientData.dataChunk.data());
  //  std::cout << "Big client data: " << std::endl;
  //  for (BYTE i: bigClientData.dataChunk) {
  //    std::cout << i;
  //    if (i == 0) {
  //      break;
  //    }
  //  }
  //  std::cout << std::endl;

  // Prepare test data for huge client data
  cout << "Preparing test data for huge client data..." << endl;
  cout << "Huge Client Data size: " << hugeClientData.size() << endl;
  hugeClientData.reserve(hugeClientDataSizeInBytes);
  hugeClientData = std::vector<BYTE>(longText.begin(), longText.end());
  //  fillWitRandomCharData(hugeClientData, hugeClientDataSize);
  hugeClientDataHash = fingerPrintFVN(hugeClientData);
  cout << "Huge client data size: " << hugeClientData.size() * sizeof(char) << endl;
  cout << "Huge client data hash: " << hugeClientDataHash << endl;
  //  std::cout << "Huge client data: " << endl;
  //  for (char i : hugeClientData) {
  //    std::cout << i;
  //    if (i == 0) {
  //      break;
  //    }
  //  }

  if (!SUCCEEDED(SimConnect_Open(&hSimConnect, "fbw-cpp-framework-test", nullptr, 0, nullptr, 0))) {
    cout << "Unable to connect to Flight Simulator!" << endl;
    return 1;
  }
  cout << "Connected to Flight Simulator!" << endl;

  simconnectLoop();

  if (!SUCCEEDED(SimConnect_Close(hSimConnect))) {
    cout << "Unable to disconnect from Flight Simulator!" << endl;
    return 1;
  }
  cout << "Disconnected from Flight Simulator!" << endl;

  return 0;
}
