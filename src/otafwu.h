#pragma once

#include <Particle.h>
#include <experimental/optional>

#define CLOUD_HOST "raspberrypi"
#define CLOUD_API_PORT 9000
#define FW_CHUNK_SIZE 256

#include <HttpClient.h>
#include <ppc.h>

bool FwGet(http_request_t&, FileTransfer::Descriptor&);

constexpr http_header_t ota_http_headers[] = {
      {"Accept", "*/*"},
      {nullptr,  nullptr}
};


bool otaUpdating;
std::experimental::optional<String> otaToken;

int FirmwareAvailable(String token) {
   otaToken = token;
   return 0;
}

bool EnableOTA(ppc::MQTTCloud& c) {
   return c.function("FwUpdateAvailable", FirmwareAvailable);
}

bool PerformUpdate() {
   if(!otaUpdating && otaToken) {
      otaUpdating = true;

      http_request_t request;
      http_response_t response;

      request.hostname = CLOUD_HOST;
      request.port = CLOUD_API_PORT;
      request.path = "/vX/ota/" + *otaToken;

      {
         auto http = std::make_unique<HttpClient>();
         http->del(request, response);  // HEAD request hacked as a DEL for now because HttpClient doesnt have a HEAD
      }

      if(200 == response.status) {
         std::string lstr(response.body.c_str());
         auto length = std::atoi(response.body.c_str());
         Log.info("Firmware update size: %d", length);
         if(length) {
            FileTransfer::Descriptor fw;
            fw.file_address = 0;
            fw.chunk_size = FW_CHUNK_SIZE;
            fw.size = static_cast<uint16_t>(length);
            fw.store = FileTransfer::Store::FIRMWARE;
            auto ec = Spark_Prepare_For_Firmware_Update(fw, 0, nullptr);
            bool tx = false;
            if(!ec) {
               fw.chunk_address = fw.file_address;
               tx = FwGet(request, fw);
            }
            auto fwec = Spark_Finish_Firmware_Update(fw, tx, nullptr);


            if(tx && fwec) {
               Log.info("Firmware updated");
            }
            else if(ec) {
               Log.error("Spark_Prepare_For_Firmware_Update fail: %d", ec);
            }
            else if(fwec) {
               Log.error("Spark_Finish_Firmware_Update fail: %d", fwec);
            }
         }
      }
      otaUpdating = false;
   }

   delay(1000);
   return false;
}


// adapted from HttpClient
bool FwGet(http_request_t& r, FileTransfer::Descriptor& fw) {
   auto client = std::make_unique<TCPClient>();
   if(!client->connect(r.hostname, r.port)) {
      Log.error("Failed to connect to %s:%d", CLOUD_HOST, CLOUD_API_PORT);
      return false;
   }

   bool connected = false;
   if(r.hostname != nullptr) connected = client->connect(r.hostname, r.port ? r.port : 80);
   else connected = client->connect(r.ip, r.port);

   if(!connected) {
      client->stop();
      return false;
   }

   client->printlnf("GET %s HTTP/1.0", r.path.c_str());
   client->printlnf("Host: %s", r.hostname.c_str());
   client->println("Content-Length: 0");
   client->println();
   client->flush();

   std::array<uint8_t, FW_CHUNK_SIZE> buffer = {};

   system_tick_t lastRead = millis();
   bool done = false;
   bool error = false;
   bool timeout = false;
   bool inHeaders = true;
   uint16_t actualTimeout = r.timeout == 0 ? 500 : r.timeout;
   char lastChar = 0;
   int available = 0;
   int recv = 0;

   do {
      while(inHeaders && client->available()) {
         int c = client->read();
         lastRead = millis();

         if(c == -1) {
            error = true;
            break;
         }

         if((c == '\n') && (lastChar == '\n')) {
            int status = std::atoi(reinterpret_cast<const char*>(&buffer[9]));
            if(200 != status) {
               return false;
            }

            buffer = {};
            inHeaders = false;
            continue;
         }
         else if(c != '\r') {
            lastChar = c;
         }
      }

      while(!inHeaders && (available = client->available())) {
         uint32_t rd = buffer.size() < available ? buffer.size() : available;
         int sz = client->read(buffer.data(), rd);
         recv += sz;

         fw.chunk_size = sz;
         int cec = Spark_Save_Firmware_Chunk(fw, buffer.data(), nullptr);

         if(cec) {
            Log.error("Failed to save chunk, %d (%d)", cec, recv);
            return false;
         }

         fw.chunk_address += fw.chunk_size;
         if(recv >= fw.size) {
            done = true;
            break;
         }
      }

      if(!done) {
         timeout = millis() - lastRead > actualTimeout;
         if(!error && !timeout)
            delay(200);
      }
   } while(client->connected() && !done && !timeout && !error);

   client->stop();  // does destructor do this?
   return !inHeaders && !timeout && !error;
}


