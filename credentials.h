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
