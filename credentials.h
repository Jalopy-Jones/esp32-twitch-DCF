/*  credentials.h is used in conjunction with 
    "ESP32 Twitch IRC Chatbot Framework.ino"
    
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


#ifndef CREDENTIALS_H
#define CREDENTIALS_H

//------- Replace the following! -------//
char ssid[] = "";           // your network SSID (name)
char password[] = "";  // your network key
//The name of the channel that you want the bot to join
String twitchChannelName = "";  //this is case sensitive, must be all lower case!

String ircChannel = ""; // leave this blank
#define secret_ssid ""  // leave this blank

const char* twitchServer = "irc.chat.twitch.tv";
const int twitchPort = 6667;

//The name that you want the bot to have
#define botUsername ""  //make this your channel name, mixed case as you see fit
#define client_id "" // your bot id
#define scopes "chat%3Aread+chat%3Aedit" // your bot permissions (can only be read and write)

#endif
