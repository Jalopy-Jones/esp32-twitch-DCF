#include <WiFi.h>
#include <WiFiClientSecure.h>

const char* ssid = ""; // your network name
const char* password = ""; // your network password

const char* server = "id.twitch.tv";
const char* port = "443";

const char* client_id = ""; // your bot id
const char* scopes = "chat%3Aread+chat%3Aedit";

const char* rootCACertificate =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
  "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
  "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
  "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
  "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
  "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
  "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
  "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
  "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
  "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
  "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
  "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
  "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
  "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
  "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
  "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
  "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
  "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
  "-----END CERTIFICATE-----\n";



void pollForDeviceVerification(String deviceCode) {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);

  const char* tokenEndpoint = "/oauth2/token";
  const int pollingInterval = 5000; // 5 seconds
  const int maxPollingTime = 900000; // set to 15 minutes (60000; 60 seconds (example limit))
  int elapsedTime = 0;

  Serial.println("Polling for device verification...");

  while (elapsedTime < maxPollingTime) {
    if (client.connect(server, 443)) {
      // Prepare request body
      String requestBody = "client_id=" + String(client_id) + 
                           "&device_code=" + deviceCode + 
                           "&grant_type=urn:ietf:params:oauth:grant-type:device_code";

      // Send POST request
      client.println("POST " + String(tokenEndpoint) + " HTTP/1.1");
      client.println("Host: id.twitch.tv");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Content-Length: " + String(requestBody.length()));
      client.println();
      client.print(requestBody);

      Serial.println(requestBody);
      Serial.println(requestBody.length());

      String response = "";
      while (client.connected() || client.available()) {
        if (client.available()) {
          response += client.readString();
        }
      }

      // Print response for debugging
      Serial.println("Response: " + response);

     // Check if access token is in the response
      if (response.indexOf("\"access_token\":") != -1) {
        // Extract access token
        int startIndex = response.indexOf("\"access_token\":\"") + strlen("\"access_token\":\"");
        int endIndex = response.indexOf("\"", startIndex);
        String accessToken = response.substring(startIndex, endIndex);

        Serial.println("Access Token Obtained: " + accessToken);

        // Stop polling as the user has successfully verified
        break;
      } else if (response.indexOf("authorization_pending") != -1) {
        // Authorization is still pending; wait and continue polling
        Serial.println("Authorization pending. Retrying...");
      } else {
        // Handle unexpected errors or other response statuses
        Serial.println("Error or unexpected response. Stopping polling.");
        break;
      }
    } else {
      Serial.println("Failed to connect to server for polling.");
      break;
    }

    client.stop();
    delay(pollingInterval);
    elapsedTime += pollingInterval;
  }

  if (elapsedTime >= maxPollingTime) {
    Serial.println("Polling timed out. User did not verify in time.");
  }
}


// Function to make the POST request
void obtainDeviceCodeFlowTokens() {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);

  if (client.connect(server, 443)) {
    String requestBody = "client_id=" + String(client_id) + "&scopes=" + String(scopes);
    client.println("POST /oauth2/device HTTP/1.1");
    client.println("Host: id.twitch.tv");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(requestBody.length()));
    client.println();

    client.print(requestBody);

    String response = "";
    while (client.connected() || client.available()) {
      if (client.available()) {
        response += client.readString();
      }
    }
    // print out to make sure theres a response
    Serial.println("Response: " + response);

    // manually parse out the required info
    // Locate "verification_uri"
    int startIndex = response.lastIndexOf("verification_uri") + strlen("verification_uri") + 3;  // Adding dynamic offset
    // Locate the start of the URL by finding the next quote
    int endIndex = response.indexOf("\"", startIndex);
    // Extracting the URL
    String verificationUri = response.substring(startIndex, endIndex);
    // Trim any unnecessary whitespace or characters
    verificationUri.trim();
    Serial.println("Open This Link In A Web Browser: " + verificationUri);

    // Use this for info to poll
    // Extract the device_code
    int deviceCodeStart = response.indexOf("device_code\":\"") + strlen("device_code\":\"");
    int deviceCodeEnd = response.indexOf("\"", deviceCodeStart);
    String deviceCode = response.substring(deviceCodeStart, deviceCodeEnd);
    deviceCode.trim();
    Serial.println("Device Code: " + deviceCode);

    // Start polling for device verification
    pollForDeviceVerification(deviceCode);

  }

  else {
    Serial.println("Connection failed.");
  }
}


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");

  obtainDeviceCodeFlowTokens();
}

void loop() {
  // You can implement any loop functionality here
}
