/* ESP32 Twitch IRC Chatbot Framework.ino is a program to connect
    an ESP32 to Twitch API and Twitch IRC.
    
    Copyright (C) 2025  Jalopy Jones

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
  */


// THIS PROGRAM REQUIRES THE USE OF THE CREDENTIALS FILE

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <IRCClient.h>
#include <Preferences.h>
#include "credentials.h"  // Include Creds

const char* server = "id.twitch.tv";
const char* port = "443";

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
String refreshToken = "";  // To store the refresh token
String accessToken = "";   // To store the access token

WiFiClientSecure client;

// WiFi client and IRC client
WiFiClient wifiClient;
IRCClient ircClient(twitchServer, twitchPort, wifiClient);

void setupNVS() {
  preferences.begin("twitch", false);
  accessToken = preferences.getString("access_token", "");
  refreshToken = preferences.getString("refresh_token", "");
  Serial.println("Stored Access Token: " + accessToken); // this is a dead token being displayed
  Serial.println("Stored Refresh Token: " + refreshToken); // this is a dead token being displayed
}

void saveTokens(String newAccessToken, String newRefreshToken) {
  preferences.putString("access_token", newAccessToken);
  preferences.putString("refresh_token", newRefreshToken);
  Serial.println("Tokens saved to NVS.");
}

void refreshTokenFunction() {
  client.setCACert(rootCACertificate);

  if (refreshToken.isEmpty()) {
    Serial.println("No refresh token available. Initiating device code flow...");
    Serial.println("Please be patient this may take a minute...");
    obtainDeviceCodeFlowTokens();
    return;
  }

  if (client.connect(server, 443)) {
    // Prepare the request body for refresh token
    String requestBody = "client_id=" + String(client_id) + "&grant_type=refresh_token" + "&refresh_token=" + refreshToken;

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
    //Serial.println("Response: " + response);

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
      Serial.println("Please be patient this may take a minute...");
      obtainDeviceCodeFlowTokens();
    }
  } else {
    Serial.println("Failed to connect for token refresh.");
  }
}

void pollForDeviceVerification(String deviceCode) {
  client.setCACert(rootCACertificate);

  const char* tokenEndpoint = "/oauth2/token";
  const int pollingInterval = 5000;   // 5 seconds
  const int maxPollingTime = 900000;  // 15 minutes
  int elapsedTime = 0;

  Serial.println("Polling for device verification...");
  Serial.println("Please be patient this may take a minute...");

  while (elapsedTime < maxPollingTime) {
    if (client.connect(server, 443)) {
      // Prepare request body
      String requestBody = "client_id=" + String(client_id) + "&device_code=" + deviceCode + "&grant_type=urn:ietf:params:oauth:grant-type:device_code";

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
      //Serial.println("Response: " + response);

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

        //Serial.println("Access Token Obtained: " + accessToken);
        //Serial.println("Refresh Token Obtained: " + refreshToken);
        Serial.println("Access and Refresh Tokens Obtained!");

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
    //Serial.println("Response: " + response);

    // manually parse out the required info
    // Locate "verification_uri"
    int startIndex = response.lastIndexOf("verification_uri") + strlen("verification_uri") + 3;  // Adding dynamic offset
    // Locate the start of the URL by finding the next quote
    int endIndex = response.indexOf("\"", startIndex);
    // Extracting the URL
    String verificationUri = response.substring(startIndex, endIndex);
    Serial.println("");
    // Trim any unnecessary whitespace or characters
    verificationUri.trim();
    Serial.println("Open This Link In A Web Browser: " + verificationUri);
    Serial.println("");
    
    // Extract the device_code
    int deviceCodeStart = response.indexOf("device_code\":\"") + strlen("device_code\":\"");
    int deviceCodeEnd = response.indexOf("\"", deviceCodeStart);
    String deviceCode = response.substring(deviceCodeStart, deviceCodeEnd);
    deviceCode.trim();
    //Serial.println("Device Code: " + deviceCode);

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

  // Set the IRC channel
  ircChannel = "#" + twitchChannelName;

  setupNVS();

  if (accessToken.isEmpty() || refreshToken.isEmpty()) {
    Serial.println("No stored tokens found. Initiating device code flow...");
    Serial.println("Please be patient this may take a minute...");
    obtainDeviceCodeFlowTokens();
  } else {
    Serial.println("Stored tokens found. Checking validity...");
    Serial.println("Please be patient this may take a minute...");
    refreshTokenFunction();  // Attempt to refresh on startup
  }


  // Set the IRC callback
  ircClient.setCallback(callback);
}

// Function to send a message to Twitch chat
void sendTwitchMessage(String message) {
  ircClient.sendMessage(ircChannel, message);
  //Serial.println("Sent message: " + message);
}

// Callback function to handle IRC messages
void callback(IRCMessage ircMessage) {
  if (ircMessage.command == "PING") {
    // Respond to PING with PONG
    ircClient.sendRaw("PONG :" + ircMessage.text);
    Serial.println("Responded to PING with PONG");
    return;
  }

  if (ircMessage.command == "PRIVMSG" && ircMessage.text[0] != '\001') {
    Serial.println("In CallBack");
    String message = "<" + ircMessage.nick + "> " + ircMessage.text;
    Serial.println(message);

    // Example: Respond to specific messages
    if (ircMessage.text.indexOf("hello") > -1) {
      sendTwitchMessage("Hi " + ircMessage.nick + "! How are you?");
    }

    // Add More functionality here for interactions with chat
    // Example: Respond to specific messages
    if (ircMessage.text.indexOf("lezgo") > -1) {
      sendTwitchMessage("LEZGO " + ircMessage.nick + "!");
    }
  }
    
}

// Function to handle connecting to Twitch
void connectToTwitch() {
  Serial.println("Attempting to connect to " + ircChannel);
  String accessToken = preferences.getString("access_token", "");
  accessToken = "oauth:" + accessToken;
  accessToken.trim();

  if (!accessToken.isEmpty()) {
    delay(30);
    //Serial.println("Access token: " + accessToken);

    if (ircClient.connect(botUsername, "", accessToken)) {
      // Join the channel
      ircClient.sendRaw("JOIN " + ircChannel);
      Serial.println("Connected to Twitch chat and joined " + ircChannel);
      sendTwitchMessage("Hello chat! Bot is ready!");
    } else {
      Serial.println("Failed to connect. Retrying in 5 seconds...");
      delay(5000);
    }    
  }
}

void loop() {
  if (!ircClient.connected()) {
    connectToTwitch();
  } else {
    ircClient.loop();  // Continue handling IRC messages

    // add more functionality here implementing millis timers
  }
}
