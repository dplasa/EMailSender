/*
   This is an example sketch to show the use of the espFTP server.

   Please replace
     YOUR_SSID and YOUR_PASS
   with your WiFi's values and compile.

   If you want to see debugging output of the FTP server, please
   select select an Serial Port in the Arduino IDE menu Tools->Debug Port

   Send L via Serial Monitor, to display the contents of the FS
   Send F via Serial Monitor, to fromat the FS

   This example is provided as Public Domain
   Daniel Plasa <dplasa@gmail.com>

*/
#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined ESP32
#include <WiFi.h>
#endif

#include <SMTPSender.h>

SMTPSender::ServerInfo mailServer("usename", "password", "smtp.example.com", 25);
SMTPSender::Message message("from@example.com", "to@example.com", "Testsubject", "");
SMTPSender mailSender;

const char *ssid PROGMEM = "YOUR_SSID";
const char *password PROGMEM = "YOUR_WIFI_PASSWORD";

uint32_t startTime;

void setup()
{
  Serial.begin(74880);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.printf_P(PSTR("."));
  }
  Serial.printf_P(PSTR("\nConnected to %S, IP address is %s\n"), ssid, WiFi.localIP().toString().c_str());

  // setup the SMTP sender with ServerInfo
  mailSender.begin(mailServer);

  // blocking send a Mail
  startTime = millis();
  const SMTPSender::Status &s = mailSender.send(message);
  Serial.printf_P(PSTR("Sending took %lu ms, mail was %ssent, code %d (%s)\n"), millis() - startTime, s.isSent ? "" : "not ", s.code, s.desc.c_str());

  // non-blocking send a Mail
  message.subject += " -- Teil 2";
  startTime = millis();
  if (mailSender.queue(message))
  {
    Serial.printf_P(PSTR("Message successfully queued, it will arrive soon...\n"), &mailServer);
  }
}

bool isSent = false;
void loop()
{
  // this is all you need for non-blocking mode
  // make sure to call handleSMTP() frequently
  mailSender.handleSMTP();

  // check for send status
  if (!isSent)
  {
    const SMTPSender::Status &s = mailSender.check();
    if (s.isSent)
    {
      isSent = true;
      Serial.printf_P(PSTR("Sending took %lu ms, mail was %ssent, code %d (%s)\n"), millis() - startTime, s.isSent ? "" : "not ", s.code, s.desc.c_str());
    }
    else if (s.isError)
    {
      isSent = true;
      Serial.printf_P(PSTR("Error after %lu ms, mail was not sent, code %d (%s)\n"), millis() - startTime, s.code, s.desc.c_str());
    }
  }
  delay(25);
}
