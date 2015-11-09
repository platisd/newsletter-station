/* based on Instructables tutorial: http://plat.is/opuw2 
* For ESP8266 AT commands, please refer to the datasheet: http://wiki.iteadstudio.com/ESP8266_Serial_WIFI_Module#AT_Commands */
#define SSID "YOUR_WIFI_NETWORK" //your SSID
#define PASS "YOUR_WIFI_PASSWORD" //your wifi password
#define IP "184.106.153.149" // thingspeak.com IP address
String GET = "GET /update?key=YOUR_CHANNEL_ID&field1="; //GET request url
String API_KEY = "YOUR_API_KEY"; //your thingtweet API key

/* sends the raw AT command to the WiFi module */
void sendCommand(String cmd) {
  if (DEBUG) {
    Serial.print("SEND: ");
    Serial.println(cmd);
  }
  ESP8266.println(cmd);
}

/* commands the wifi module to connect to the specified wifi network */
boolean connectWiFi() {
  ESP8266.println("AT+CWMODE=1");
  loadAnimation(2000);
  String cmd = "AT+CWJAP=\"";
  cmd += SSID;
  cmd += "\",\"";
  cmd += PASS;
  cmd += "\"";
  sendCommand(cmd);
  loadAnimation(5000);
  if (ESP8266.find("OK")) {
    if (DEBUG) Serial.println("RECEIVED: OK");
    return true;
  } else {
    if (DEBUG) Serial.println("RECEIVED: Error");
    return false;
  }
}

/* post the specified string, with a GET request */
boolean postString(const String userInput) {
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd += "\",80";
  sendCommand(cmd);
  loadAnimation(2000);
  if (ESP8266.find("Error")) {
    if (DEBUG) Serial.print("RECEIVED: Error");
    lcd.setCursor(0, 1);
    lcd.print("Module error");
    delay(MSG_WAIT_TIME);
    clearLine(1);
    return false;
  }
  cmd = GET;
  cmd += userInput;
  cmd += "\r\n";
  ESP8266.print("AT+CIPSEND=");
  ESP8266.println(cmd.length());
  if (ESP8266.find(">")) {
    if (DEBUG) Serial.print(">");
    if (DEBUG) Serial.print(cmd);
    ESP8266.print(cmd);
  } else {
    sendCommand("AT+CIPCLOSE");
  }
  if (ESP8266.find("OK")) {
    if (DEBUG) Serial.println("RECEIVED: OK");
    lcd.setCursor(0, 1);
    lcd.print("Email submitted!");
    delay(MSG_WAIT_TIME);
    return true;
  } else {
    if (DEBUG) Serial.println("RECEIVED: Error");
    lcd.setCursor(0, 1);
    lcd.print("Server error");
    delay(MSG_WAIT_TIME);
    return false;
  }
}

/* posts the specified tweet, with a post request, using Thingspeak.com's Thingtweet app */
boolean postTweet(String tweet) {
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd += "\",80";
  sendCommand(cmd);
  loadAnimation(2000);
  if (ESP8266.find("Error")) {
    if (DEBUG) Serial.print("RECEIVED: Error");
    lcd.setCursor(0, 1);
    lcd.print("Module error");
    delay(MSG_WAIT_TIME);
    clearLine(1);
    return false;
  }
  String tweetData = "api_key=";
  tweetData += API_KEY;
  tweetData += "&status=";
  tweetData += tweet;
  cmd = "POST /apps/thingtweet/1/statuses/update HTTP/1.1\n";
  cmd += "Host: api.thingspeak.com\n";
  cmd += "Connection: close\n";
  cmd += "Content-Type: application/x-www-form-urlencoded\n";
  cmd += "Content-Length: ";
  cmd += tweetData.length();
  cmd += "\n\n";
  cmd += tweetData;
  cmd += "\r\n";
  ESP8266.print("AT+CIPSEND=");
  ESP8266.println(cmd.length());
  if (ESP8266.find(">")) {
    if (DEBUG) Serial.print(">");
    if (DEBUG) Serial.print(cmd);
    ESP8266.print(cmd);
  } else {
    sendCommand("AT+CIPCLOSE");
  }
  if (ESP8266.find("OK")) {
    if (DEBUG) Serial.println("RECEIVED: OK");
    lcd.setCursor(0, 1);
    lcd.print("Tweet posted!"); //atm we do not verify if the tweet was actually posted
    delay(MSG_WAIT_TIME);
    return true;
  } else {
    if (DEBUG) Serial.println("RECEIVED: Error");
    lcd.setCursor(0, 1);
    lcd.print("Server error");
    delay(MSG_WAIT_TIME);
    return false;
  }
}

/* evaluates whether we are really connected to the WiFi network, by checking to see if we have got an IP */
boolean validIP() {
  lcd.setCursor(0, 1);
  loadAnimation(5000); //wait otherwise will be getting busy errors
  sendCommand("AT+CIPSTA?"); //AT command for getting the wifi module's IP address
  if (ESP8266.find("+CIPSTA:\"")) { //if we've got a valid answer
    String ip = ESP8266.readStringUntil('"'); //parse the IP
    if (ip.equals("0.0.0.0")) {
      lcd.print("Failed-Retrying");
      delay(MSG_WAIT_TIME);
      return false; //IP 0.0.0.0 means that there is no connection made
    }
    lcd.print(ip); //display the ip
    if (DEBUG) Serial.print("IP: ");
    if (DEBUG) Serial.println(IP);
    delay(MSG_WAIT_TIME);
    return true;
  } else {
    if (DEBUG) Serial.println("ERROR no CIPSTA response");
  }
  if (ESP8266.find("OK")) { //flush the input buffer by going the last word it should contain
    if (DEBUG) Serial.println("OK found");
  } else {
    if (DEBUG) Serial.println("OK not found"); //something isn't working as it should
  }
  lcd.print("Failed-Retrying");
  delay(MSG_WAIT_TIME);
  return false;
}
