#include <iostream>

#include <strsafe.h>
#include <windows.h>
#include <array>
#include <cassert>
#include <iomanip>
#include <random>
#include <string>
#include <vector>

#include "SimConnect.h"

#include "SimconnectExceptionStrings.h"
#include "fingerprint.h"
#include "logging.h"
#include "longtext.h"

static const int loopThrottleValue = 100000000;

int quit = 0;
bool initilized = false;
uint64_t loopCounter = 0;

typedef double FLOAT64;
typedef float FLOAT32;

HANDLE hSimConnect = nullptr;

enum EVENT_IDS {
  EVENT_SIM_START,
};

enum CLIENT_DATA_IDS {
  EXAMPLE_CLIENT_DATA_ID,
  EXAMPLE2_CLIENT_DATA_ID,
  BIG_CLIENT_DATA_ID,
  STREAM_RECEIVER_META_DATA_ID,  // sim is receiving
  STREAM_RECEIVER_DATA_ID,       // sim is receiving
  STREAM_SENDER_META_DATA_ID,    // sim is sending
  STREAM_SENDER_DATA_ID,         // sim is sending
};

enum DATA_DEFINE_IDS {
  TITLE_DEFINITION_ID,
  EXAMPLE_CLIENT_DATA_DEFINITION_ID,
  EXAMPLE2_CLIENT_DATA_DEFINITION_ID,
  BIG_CLIENT_DATA_DEFINITION_ID,
  STREAM_RECEIVER_META_DATA_DEFINITION_ID,
  STREAM_RECEIVER_DATA_DEFINITION_ID,
  STREAM_SENDER_META_DATA_DEFINITION_ID,
  STREAM_SENDER_DATA_DEFINITION_ID,
};

enum DATA_REQUEST_IDS {
  TITLE_REQUEST_ID,
  EXAMPLE_CLIENT_DATA_REQUEST_ID,
  EXAMPLE2_CLIENT_DATA_REQUEST_ID,
  BIG_CLIENT_DATA_REQUEST_ID,
  STREAM_RECEIVER_META_DATA_REQUEST_ID,
  STREAM_RECEIVER_DATA_REQUEST_ID,
  STREAM_SENDER_META_DATA_REQUEST_ID,
  STREAM_SENDER_DATA_REQUEST_ID,
};

// Title string sim variable
struct Title {
  [[maybe_unused]] char title[256] = "";
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
  std::array<char, SIMCONNECT_CLIENTDATA_MAX_SIZE> dataChunk;
} __attribute__((packed)) bigClientData{};

// ==============================
// STREAM RECEIVER DATA meta data
// sending to sim - sim is receiving

struct StreamMetaData {
  size_t size;
  size_t hash;
} __attribute__((packed));

StreamMetaData streamReceiverMetaData{};
const std::string STREAM_RECEIVER_META_DATA_NAME = "STREAM RECEIVER META DATA";
const size_t streamReceiverMetaDataSize = sizeof(StreamMetaData);

// STREAM RECEIVER DATA area
const std::string STREAM_RECEIVER_DATA_NAME = "STREAM RECEIVER DATA";
constexpr DWORD ChunkSize = SIMCONNECT_CLIENTDATA_MAX_SIZE;
const size_t streamReceiverDataSize = longText.size();
const size_t streamReceiverDataSizeInBytes = streamReceiverDataSize * sizeof(char);
std::size_t streamReceiverDataHash;
std::vector<char> streamReceiverData{};

// ============================
// STREAM SENDER META DATA
// receiving from sim - sim is sending
StreamMetaData streamSenderMetaData{};
const std::string STREAM_SENDER_META_DATA_NAME = "STREAM SENDER META DATA";
const size_t streamSenderMetaDataSize = sizeof(StreamMetaData);

// STREAM RECEIVER DATA 2 area
const std::string STREAM_SENDER_DATA_NAME = "STREAM SENDER DATA";
std::vector<char> streamSenderData{};
std::size_t receivedBytes = 0;
std::size_t expectedByteCount = 0;
int receivedChunks = 0;

void initialize() {
  if (initilized)
    return;

  LOG_INFO("Initializing SimConnect connection");

  if (!SUCCEEDED(SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart"))) {
    LOG_ERROR("Failed to subscribe to SimStart event");
  }

  if (!SUCCEEDED(SimConnect_AddToDataDefinition(hSimConnect, TITLE_DEFINITION_ID, "TITLE", nullptr, SIMCONNECT_DATATYPE_STRING256))) {
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
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(hSimConnect, EXAMPLE_CLIENT_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO,
                                                      exampleClientDataSize))) {
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
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(hSimConnect, EXAMPLE2_CLIENT_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO,
                                                      example2ClientDataSize))) {
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
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(hSimConnect, BIG_CLIENT_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO,
                                                      sizeof(bigClientData)))) {
    LOG_ERROR("Adding to client data definition failed: " + BIG_CLIENT_DATA_NAME);
  }

  // Create/allocate the client data area
  if (!SUCCEEDED(SimConnect_CreateClientData(hSimConnect, BIG_CLIENT_DATA_ID, sizeof(bigClientData),
                                             SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT))) {
    LOG_ERROR("Creating client data failed: " + BIG_CLIENT_DATA_NAME);
  }

  // =========================
  // STREAM RECEIVER META DATA

  hresult = SimConnect_MapClientDataNameToID(hSimConnect, STREAM_RECEIVER_META_DATA_NAME.c_str(), STREAM_RECEIVER_META_DATA_ID);
  if (hresult != S_OK) {
    switch (hresult) {
      case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        LOG_ERROR("Client data area STREAM_RECEIVER_META_DATA_NAME already in use: " + STREAM_RECEIVER_META_DATA_NAME);
        break;
      case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        LOG_ERROR("Client data area ID already in use: " + std::to_string(STREAM_RECEIVER_META_DATA_ID));
        break;
      default:
        LOG_ERROR("Mapping client data area STREAM_RECEIVER_META_DATA_NAME to ID failed: " + STREAM_RECEIVER_META_DATA_NAME);
    }
  }

  // Add the data definition to the client data area
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(hSimConnect, STREAM_RECEIVER_META_DATA_DEFINITION_ID,
                                                      SIMCONNECT_CLIENTDATAOFFSET_AUTO, streamReceiverMetaDataSize))) {
    LOG_ERROR("Adding to client data definition failed: " + STREAM_RECEIVER_META_DATA_NAME);
  }

  // Create/allocate the client data area
  if (!SUCCEEDED(SimConnect_CreateClientData(hSimConnect, STREAM_RECEIVER_META_DATA_ID, streamReceiverMetaDataSize,
                                             SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT))) {
    LOG_ERROR("Creating client data failed: " + STREAM_RECEIVER_META_DATA_NAME);
  }

  // =========================
  // STREAM RECEIVER DATA

  hresult = SimConnect_MapClientDataNameToID(hSimConnect, STREAM_RECEIVER_DATA_NAME.c_str(), STREAM_RECEIVER_DATA_ID);
  if (hresult != S_OK) {
    switch (hresult) {
      case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        LOG_ERROR("Client data area STREAM_RECEIVER_DATA_NAME already in use: " + STREAM_RECEIVER_DATA_NAME);
        break;
      case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        LOG_ERROR("Client data area ID already in use: " + std::to_string(STREAM_RECEIVER_DATA_ID));
        break;
      default:
        LOG_ERROR("Mapping client data area STREAM_RECEIVER_DATA_NAME to ID failed: " + STREAM_RECEIVER_DATA_NAME);
    }
  }

  // Add the data definition to the client data area
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(hSimConnect, STREAM_RECEIVER_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO,
                                                      ChunkSize))) {
    LOG_ERROR("Adding to client data definition failed: " + STREAM_RECEIVER_DATA_NAME);
  }

  // Create/allocate the client data area
  if (!SUCCEEDED(
          SimConnect_CreateClientData(hSimConnect, STREAM_RECEIVER_DATA_ID, ChunkSize, SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT))) {
    LOG_ERROR("Creating client data failed: " + STREAM_RECEIVER_DATA_NAME);
  }

  // =========================
  // STREAM SENDER META DATA

  // Map the client data area EXAMPLE2_CLIENT_DATA_NAME to the client data area ID
  hresult = SimConnect_MapClientDataNameToID(hSimConnect, STREAM_SENDER_META_DATA_NAME.c_str(), STREAM_SENDER_META_DATA_ID);
  if (hresult != S_OK) {
    switch (hresult) {
      case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        LOG_ERROR("Client data area STREAM_SENDER_META_DATA_NAME already in use: " + STREAM_SENDER_META_DATA_NAME);
        break;
      case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        LOG_ERROR("Client data area ID already in use: " + std::to_string(STREAM_SENDER_META_DATA_ID));
        break;
      default:
        LOG_ERROR("Mapping client data area STREAM_SENDER_META_DATA_NAME to ID failed: " + STREAM_SENDER_META_DATA_NAME);
    }
  }
  // Add the data definition to the client data area
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(hSimConnect, STREAM_SENDER_META_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO,
                                                      streamSenderMetaDataSize))) {
    LOG_ERROR("Adding to client data definition failed: " + STREAM_SENDER_META_DATA_NAME);
  }
  // Request the client data area when changed
  if (!SUCCEEDED(SimConnect_RequestClientData(hSimConnect, STREAM_SENDER_META_DATA_ID, STREAM_SENDER_META_DATA_REQUEST_ID,
                                              STREAM_SENDER_META_DATA_DEFINITION_ID, SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET))) {
    LOG_ERROR("ClientDataAreaVariable: Requesting client data failed: " + STREAM_SENDER_META_DATA_NAME);
  }

  // =========================
  // STREAM SENDER DATA

  // Map the client data area EXAMPLE2_CLIENT_DATA_NAME to the client data area ID
  hresult = SimConnect_MapClientDataNameToID(hSimConnect, STREAM_SENDER_DATA_NAME.c_str(), STREAM_SENDER_DATA_ID);
  if (hresult != S_OK) {
    switch (hresult) {
      case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        LOG_ERROR("Client data area already in use: " + STREAM_SENDER_DATA_NAME);
        break;
      case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        LOG_ERROR("Client data area ID already in use: " + std::to_string(STREAM_SENDER_DATA_ID));
        break;
      default:
        LOG_ERROR("Mapping client data area " + STREAM_SENDER_DATA_NAME + " to ID + " + std::to_string(STREAM_SENDER_DATA_ID) + " failed");
    }
  }
  // Add the data definition to the client data area
  if (!SUCCEEDED(SimConnect_AddToClientDataDefinition(hSimConnect, STREAM_SENDER_DATA_DEFINITION_ID, SIMCONNECT_CLIENTDATAOFFSET_AUTO,
                                                      ChunkSize))) {
    LOG_ERROR("Adding to client data definition failed: " + STREAM_SENDER_DATA_NAME);
  }
  if (!SUCCEEDED(SimConnect_RequestClientData(hSimConnect, STREAM_SENDER_DATA_ID, STREAM_SENDER_DATA_REQUEST_ID,
                                              STREAM_SENDER_DATA_DEFINITION_ID, SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET))) {
    LOG_ERROR("ClientDataAreaVariable: Requesting client data failed: " + STREAM_SENDER_DATA_NAME);
  }

  initilized = true;
  LOG_INFO("SimConnect connection initialized");
}

void processStreamData(const SIMCONNECT_RECV_CLIENT_DATA* pClientData) {
  std::size_t remainingBytes = expectedByteCount - receivedBytes;
  if (remainingBytes > ChunkSize) {
    remainingBytes = ChunkSize;
  }

  streamSenderData.insert(streamSenderData.end(), (char*)&pClientData->dwData, (char*)&pClientData->dwData + remainingBytes);

  receivedChunks++;
  receivedBytes += remainingBytes;
  //  std::cout << "Received data chunk " << receivedChunks << " of " << remainingBytes << " Byte received: " << STREAM_SENDER_DATA_NAME <<
  //  " ("
  //            << receivedBytes << "/" << expectedByteCount << ") " << std::endl;

  const bool receivedAllData = receivedBytes >= expectedByteCount;
  if (receivedAllData) {
    std::cout << "Received all stream data: " << STREAM_SENDER_DATA_NAME << std::endl;
    const uint64_t fingerPrintFvn = fingerPrintFVN(streamSenderData);
    std::cout << "STREAM SENDER DATA: "
              << " size = " << streamSenderData.size() << " bytes = " << receivedBytes << " chunks = " << receivedChunks
              << " fingerprint = " << std::setw(21) << fingerPrintFvn << " (match = " << std::boolalpha
              << (fingerPrintFvn == streamSenderMetaData.hash) << ")" << std::endl;
    if (!streamSenderData.empty()) {
      std::cout << "Content: "
                << "[" << std::string(streamSenderData.begin(), streamSenderData.begin() + 100) << " ... ]" << std::endl;
    }
    return;
  }
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
    case STREAM_SENDER_META_DATA_REQUEST_ID:
      LOG_INFO("Received client data: " + STREAM_SENDER_META_DATA_NAME);
      std::memcpy(&streamSenderMetaData, &pClientData->dwData, sizeof(streamSenderMetaData));
      streamSenderData.clear();
      receivedBytes = 0;
      receivedChunks = 0;
      expectedByteCount = streamSenderMetaData.size;
      streamSenderData.reserve(expectedByteCount);
      std::cout << "STREAM SENDER DATA ---- ( received from sim ) -----------------------------" << std::endl;
      std::cout << "Stream Sender size     : " << streamSenderMetaData.size << std::endl;
      std::cout << "Stream Sender Data hash: " << streamSenderMetaData.hash << std::endl;
      break;
    case STREAM_SENDER_DATA_REQUEST_ID:
      processStreamData(pClientData);
      break;
    default:
      LOG_WARN("Received unknown client data request ID: " + std::to_string(pClientData->dwRequestID));
      break;
  }
}

void processReceivedSimObjectData(SIMCONNECT_RECV* pRecv) {
  const auto pData = reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA*>(pRecv);
  switch (pData->dwRequestID) {
    case TITLE_REQUEST_ID:
      LOG_INFO("Received sim object data: Title");
      title = *((Title*)&pData->dwData);
      break;
    default:
      LOG_WARN("Received unknown sim object data request ID: " + std::to_string(pData->dwRequestID));
      break;
  }
}

void CALLBACK dispatchCallback(SIMCONNECT_RECV* pRecv, [[maybe_unused]] DWORD cbData, [[maybe_unused]] void* pContext) {
  switch (pRecv->dwID) {
    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
      processReceivedSimObjectData(pRecv);
      break;

    case SIMCONNECT_RECV_ID_CLIENT_DATA:
      processReceivedClientData(pRecv);
      break;

    case SIMCONNECT_RECV_ID_EVENT: {
      auto* evt = (SIMCONNECT_RECV_EVENT*)pRecv;
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
      LOG_ERROR("Exception in SimConnect connection: " +
                SimconnectExceptionStrings::getSimConnectExceptionString(static_cast<SIMCONNECT_EXCEPTION>(pException->dwException)) +
                " send_id:" + std::to_string(pException->dwSendID) + " index:" + std::to_string(pException->dwIndex));
      break;
    }

    case SIMCONNECT_RECV_ID_QUIT:
      quit = 1;
      break;

    case SIMCONNECT_RECV_ID_SYSTEM_STATE: {
      auto* const pState = reinterpret_cast<SIMCONNECT_RECV_SYSTEM_STATE*>(pRecv);
      LOG_INFO("SIMCONNECT_RECV_ID_SYSTEM_STATE: " + std::to_string(pState->dwInteger) + " " + pState->szString + " " +
               std::to_string(pState->dwRequestID) + " " + std::to_string(pState->fFloat));
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

void sendStreamingClientData() {
  // =========================
  // STREAM RECEIVER META DATA

  streamReceiverMetaData.size = streamReceiverDataSizeInBytes;
  streamReceiverMetaData.hash = streamReceiverDataHash;
  if (!SUCCEEDED(SimConnect_SetClientData(hSimConnect, STREAM_RECEIVER_META_DATA_ID, STREAM_RECEIVER_META_DATA_DEFINITION_ID,
                                          SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0, streamReceiverMetaDataSize,
                                          &streamReceiverMetaData))) {
    LOG_ERROR("Setting data to sim for " + STREAM_RECEIVER_META_DATA_NAME +
              " with dataDefId=" + std::to_string(STREAM_RECEIVER_META_DATA_DEFINITION_ID) + " failed!");
    return;
  }
  std::cout << "STREAM RECEIVER DATA  ---- ( sent to sim ) ------------------------------" << std::endl;
  std::cout << "STREAM RECEIVER DATA size: " << streamReceiverMetaData.size << " STREAM RECEIVER DATA hash: " << streamReceiverMetaData.hash
            << std::endl;

  // =========================
  // STREAM RECEIVER DATA
  int chunkCount = 0;
  size_t sentBytes = 0;

  std::cout << "STREAM RECEIVER DATA size: " << streamReceiverDataSize << std::endl;

  assert((streamReceiverDataSizeInBytes == streamReceiverDataSize) &&
         "STREAM RECEIVER DATA size is not equal to STREAM RECEIVER DATA size in bytes");

  while (sentBytes < streamReceiverDataSize) {
    size_t remainingBytes = streamReceiverData.size() - sentBytes;
    chunkCount++;
    if (remainingBytes >= ChunkSize) {
      // std::cout << "Sending chunk: " << std::setw(2) << ++chunkCount << " Sent bytes: " << sentBytes << " Remaining bytes: " <<
      // remainingBytes << std::endl;

      auto pDataSet = &streamReceiverData[sentBytes];
      if (!SUCCEEDED(SimConnect_SetClientData(hSimConnect, STREAM_RECEIVER_DATA_ID, STREAM_RECEIVER_DATA_DEFINITION_ID,
                                              SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0, ChunkSize, pDataSet))) {
        LOG_ERROR("Setting data to sim for " + STREAM_RECEIVER_DATA_NAME +
                  " with dataDefId=" + std::to_string(STREAM_RECEIVER_DATA_DEFINITION_ID) + " failed!");
        break;
      }
      sentBytes += ChunkSize;
    } else {
      std::array<char, ChunkSize> buffer{};
      auto const pDataSet = &streamReceiverData[sentBytes];
      memcpy(buffer.data(), pDataSet, remainingBytes);

      if (!SUCCEEDED(SimConnect_SetClientData(hSimConnect, STREAM_RECEIVER_DATA_ID, STREAM_RECEIVER_DATA_DEFINITION_ID,
                                              SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0, ChunkSize, buffer.data()))) {
        LOG_ERROR("Setting data to sim for " + STREAM_RECEIVER_DATA_NAME +
                  " with dataDefId=" + std::to_string(STREAM_RECEIVER_DATA_DEFINITION_ID) + " failed!");
        break;
      }
      sentBytes += remainingBytes;
    }
  }
  std::cout << "STREAM RECEIVER DATA  ---- ( sent to sim ) -----------------------------------" << std::endl;
  std::cout << "Sent " << chunkCount << " chunks" << " Sent bytes: " << sentBytes << std::endl;
}

void simconnectLoop() {
  while (quit == 0) {
    if (!initilized) {
      getDispatch();
      Sleep(500);
      continue;
    }

    loopCounter++;
    if (loopCounter % loopThrottleValue == 0) {
      std::cout << "loopCounter: " << loopCounter << std::endl;

      // Request title
      if (!SUCCEEDED(SimConnect_RequestDataOnSimObject(hSimConnect, TITLE_REQUEST_ID, TITLE_DEFINITION_ID, SIMCONNECT_OBJECT_ID_USER,
                                                       SIMCONNECT_PERIOD_ONCE, SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT))) {
        LOG_ERROR("Requesting title failed");
        break;
      }

      // =========================
      // EXAMPLE CLIENT DATA
      if (!SUCCEEDED(SimConnect_RequestClientData(hSimConnect, EXAMPLE_CLIENT_DATA_ID, EXAMPLE_CLIENT_DATA_REQUEST_ID,
                                                  EXAMPLE_CLIENT_DATA_DEFINITION_ID, SIMCONNECT_CLIENT_DATA_PERIOD_ONCE))) {
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

      if (!SUCCEEDED(SimConnect_SetClientData(hSimConnect, EXAMPLE2_CLIENT_DATA_ID, EXAMPLE2_CLIENT_DATA_DEFINITION_ID,
                                              SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0, example2ClientDataSize, &example2ClientData))) {
        LOG_ERROR("Setting data to sim for " + EXAMPLE2_CLIENT_DATA_NAME +
                  " with dataDefId=" + std::to_string(EXAMPLE2_CLIENT_DATA_DEFINITION_ID) + " failed!");
        break;
      }

      // =========================
      // BIG CLIENT DATA

      if (!SUCCEEDED(SimConnect_SetClientData(hSimConnect, BIG_CLIENT_DATA_ID, BIG_CLIENT_DATA_DEFINITION_ID,
                                              SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0, sizeof(bigClientData), &bigClientData))) {
        LOG_ERROR("Setting data to sim for " + BIG_CLIENT_DATA_NAME + " with dataDefId=" + std::to_string(BIG_CLIENT_DATA_DEFINITION_ID) +
                  " failed!");
        break;
      }

      sendStreamingClientData();
    }

    // =========================
    // DISPATCH
    getDispatch();

    // =========================
    // OUTPUT
    if (loopCounter % loopThrottleValue == 0) {
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

      std::cout << "BIG META DATA  ---- ( sent to sim ) ------------------------------" << std::endl;
      std::cout << "Big client data size: " << sizeof(bigClientData) << std::endl;
      std::cout << "Fingerprint: " << fingerPrintFVN(std::vector(bigClientData.dataChunk.begin(), bigClientData.dataChunk.end()))
                << std::endl;
    }
  }
}

void prepareTestData() {
  // Prepare test data for big client data
  std::cout << "Preparing test data for big client data..." << std::endl;
  auto l = std::min(longText.size(), static_cast<size_t>(SIMCONNECT_CLIENTDATA_MAX_SIZE));
  copy(longText.begin(), longText.begin() + l, bigClientData.dataChunk.data());
  std::cout << "Big client data: " << std::endl;

  // Prepare test data for STREAM RECEIVER DATA
  std::cout << "Preparing test data for STREAM RECEIVER DATA..." << std::endl;
  std::cout << "STREAM RECEIVER DATA size: " << streamReceiverData.size() << std::endl;
  streamReceiverData.reserve(streamReceiverDataSizeInBytes);
  streamReceiverData = std::vector<char>(longText.begin(), longText.end());
  //  fillWithRandomCharData(streamReceiverData, streamReceiverDataSize);
  streamReceiverDataHash = fingerPrintFVN(streamReceiverData);
  std::cout << "STREAM RECEIVER DATA size: " << streamReceiverData.size() * sizeof(char) << std::endl;
  std::cout << "STREAM RECEIVER DATA hash: " << streamReceiverDataHash << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  using namespace std;

  cout << "FBW CPP Framework Testing" << endl;
  prepareTestData();

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
