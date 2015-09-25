/* based on Instructables tutorial: http://plat.is/opuw2 */

void sendCommand(String cmd) {
  if (DEBUG) {
    Serial.print("SEND: ");
    Serial.println(cmd);
  }
  ESP8266.println(cmd);
}

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
