/*
- This code gets a Verification URI
- The user goes to an external device (ie. the pc the esp32 is connected to)
and enter the login and give permissions for the bot to connect to twitch.
- The Access and Refresh Tokens obtained are stored in preferences
- The program polls to see if the stored tokens are valid, if not, refreshes them.

TODO:
add in preset reset (erase preferences: "do you want to use existing network config? Y/n")
add in credentials prompt
add in bot functionality
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

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

Preferences preferences;
String refreshToken = ""; // To store the refresh token
String accessToken = "";  // To store the access token

void setupNVS() {
  preferences.begin("twitch", false);
  accessToken = preferences.getString("access_token", "");
  refreshToken = preferences.getString("refresh_token", "");
  Serial.println("Stored Access Token: " + accessToken);
  Serial.println("Stored Refresh Token: " + refreshToken);
}

void saveTokens(String newAccessToken, String newRefreshToken) {
  preferences.putString("access_token", newAccessToken);
  preferences.putString("refresh_token", newRefreshToken);
  Serial.println("Tokens saved to NVS.");
}


void refreshTokenFunction() {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);

  if (refreshToken.isEmpty()) {
    Serial.println("No refresh token available. Initiating device code flow...");
    obtainDeviceCodeFlowTokens();
    return;
  }

  if (client.connect(server, 443)) {
    // Prepare the request body for refresh token
    String requestBody = "client_id=" + String(client_id) +
                         "&grant_type=refresh_token" +
                         "&refresh_token=" + refreshToken;

    // Send POST request
    client.println("POST /oauth2/token HTTP/1.1");
    client.println("Host: id.twitch.tv");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(requestBody.length()));
    client.println();
    client.print(requestBody);

    // Read the response
    String response = "";
    while (client.connected() || client.available()) {
      if (client.available()) {
        response += client.readString();
      }
    }

    // Debugging the full response
    Serial.println("Response: " + response);

    // Parse response for access_token and refresh_token
    if (response.indexOf("\"access_token\":") != -1) {
      int accessTokenStart = response.indexOf("\"access_token\":\"") + strlen("\"access_token\":\"");
      int accessTokenEnd = response.indexOf("\"", accessTokenStart);
      accessToken = response.substring(accessTokenStart, accessTokenEnd);

      int refreshTokenStart = response.indexOf("\"refresh_token\":\"") + strlen("\"refresh_token\":\"");
      int refreshTokenEnd = response.indexOf("\"", refreshTokenStart);
      refreshToken = response.substring(refreshTokenStart, refreshTokenEnd);

      saveTokens(accessToken, refreshToken);

      Serial.println("Tokens refreshed successfully.");
    } else {
      Serial.println("Failed to refresh tokens. Clearing stored tokens.");
      preferences.remove("access_token");
      preferences.remove("refresh_token");

      Serial.println("Initiating device code flow...");
      obtainDeviceCodeFlowTokens();
    }
  } else {
    Serial.println("Failed to connect for token refresh.");
  }
}



void pollForDeviceVerification(String deviceCode) {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);

  const char* tokenEndpoint = "/oauth2/token";
  const int pollingInterval = 5000; // 5 seconds
  const int maxPollingTime = 900000; // 15 minutes
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

      String response = "";
      while (client.connected() || client.available()) {
        if (client.available()) {
          response += client.readString();
        }
      }

      // Debug response
      Serial.println("Response: " + response);

      if (response.indexOf("\"access_token\":") != -1) {
        // Extract and store tokens
        int startIndex = response.indexOf("\"access_token\":\"") + strlen("\"access_token\":\"");
        int endIndex = response.indexOf("\"", startIndex);
        String newAccessToken = response.substring(startIndex, endIndex);

        int startIndexR = response.indexOf("\"refresh_token\":\"") + strlen("\"refresh_token\":\"");
        int endIndexR = response.indexOf("\"", startIndexR);
        String newRefreshToken = response.substring(startIndexR, endIndexR);

        accessToken = newAccessToken;
        refreshToken = newRefreshToken;

        saveTokens(accessToken, refreshToken);

        Serial.println("Access Token Obtained: " + accessToken);
        Serial.println("Refresh Token Obtained: " + refreshToken);

        break;
      } else if (response.indexOf("authorization_pending") != -1) {
        Serial.println("Authorization pending. Retrying...");
      } else {
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

    // begin polling for validation??
    // use this for info to poll
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

  setupNVS();

  if (accessToken.isEmpty() || refreshToken.isEmpty()) {
    Serial.println("No stored tokens found. Initiating device code flow...");
    obtainDeviceCodeFlowTokens();
  } else {
    Serial.println("Stored tokens found. Checking validity...");
    refreshTokenFunction(); // Attempt to refresh on startup
  }
}

void loop() {
  // You can implement any loop functionality here
}

