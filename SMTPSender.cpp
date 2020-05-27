#include "SMTPSender.h"

// BASE64 Coder
//#include <Base64Coder.h>

// use esp8266 core base64 code
#include <base64.h>

// helper macro
#define CLIENT_SEND(fmt, ...)                        \
  do                                                 \
  {                                                  \
    DEBUG_MSG(fmt, ##__VA_ARGS__);                   \
    client->printf_P(PSTR(fmt "\n"), ##__VA_ARGS__); \
  } while (0)

void SMTPSender::begin(const ServerInfo &theServer)
{
  _server = &theServer;
}

bool SMTPSender::queue(const Message &theEmail)
{
  if (smtpState >= cIdle)
  {
    _serverStatus.isSent = false;
    _serverStatus.isError = false;
    _email = &theEmail;
    smtpState = cConnect;
    return true;
  }
  return false;
}

const SMTPSender::Status &SMTPSender::check()
{
  return _serverStatus;
}

const SMTPSender::Status &SMTPSender::send(const Message &theEmail)
{
  if (queue(theEmail))
  {
    while (smtpState <= cQuit)
    {
      handleSMTP();
      delay(25);
    }
  }
  return _serverStatus;
}

void SMTPSender::handleSMTP()
{
  if (_server == nullptr || _email == nullptr)
  {
    _serverStatus.code = 65535;
    _serverStatus.desc = F("begin() not called");
  }
  else if (cConnect == smtpState)
  {
    _serverStatus.code = 65534;
    _serverStatus.desc = F("No connection to SMTP server");
    if (connect())
    {
      smtpState = cGreet;
    }
  }
  else if (cGreet == smtpState)
  {
    if (waitFor(220 /* 220 smtpserver.example.org ESMTP Postfix (Debian/GNU) */, F("No server greeting")))
    {
      CLIENT_SEND("HELO you");
      smtpState = cHelo;
    }
  }
  else if (cHelo == smtpState)
  {
    if (waitFor(250 /* 250 smtpserver.example.org */, F("No HELO reply")))
    {
      CLIENT_SEND("AUTH LOGIN");
      smtpState = cAuth;
    }
  }
  else if (cAuth == smtpState)
  {
    if (waitFor(334 /* 334 VXNlcm5hbWU6 */))
    {
      printClientBase64(_server->login);
      smtpState = cUser;
    }
  }
  else if (cUser == smtpState)
  {
    if (waitFor(334 /* 334 UGFzc3dvcmQ6 */))
    {
      printClientBase64(_server->password);
      smtpState = cPassword;
    }
  }
  else if (cPassword == smtpState)
  {
    if (waitFor(235 /* 235 2.7.0 Authentication successful*/))
    {
      CLIENT_SEND("MAIL FROM: %s", _email->from.c_str());
      smtpState = cFrom;
    }
  }
  else if (cFrom == smtpState)
  {
    if (waitFor(250 /* OK */))
    {
      CLIENT_SEND("RCPT TO: %s", _email->to.c_str());
      smtpState = cTo;
    }
  }
  else if (cTo == smtpState)
  {
    if (waitFor(250 /* OK */))
    {
      CLIENT_SEND("DATA");
      smtpState = cData;
    }
  }
  else if (cData == smtpState)
  {
    if (waitFor(354))
    {
      CLIENT_SEND("From: %s", _email->from.c_str());
      CLIENT_SEND("To: %s", _email->to.c_str());
      CLIENT_SEND("Subject: %s\n", _email->subject.c_str());
      CLIENT_SEND("Mime-Version: 1.0\n"
                  "Content-Type: text/html; charset=utf-8\n"
                  "Content-Transfer-Encoding: 7bit\n"
                  "<!DOCTYPE html><html lang=\"en\">\n%s\n</html>\n\n.",
                  _email->message.c_str());
      smtpState = cSendMessage;
    }
  }
  else if (cSendMessage == smtpState)
  {
    if (waitFor(250))
    {
      CLIENT_SEND("QUIT");
      smtpState = cSendOK;
    }
  }
  else if (cSendOK == smtpState)
  {
    smtpState = cQuit;
    // dont wait for 221 Goofbye... speed things up and just close the connection!
    // if (waitFor(221))
    // {
    //   smtpState = cQuit;
    // }
  }
  else if (cQuit == smtpState)
  {
    client->stop();
    delete client;
    client = NULL;
    _serverStatus.isSent = true;
    smtpState = cIdle;
  }
  else if (smtpState >= cTimeout)
  {
    _serverStatus.isError = true;
  }
}

bool SMTPSender::connect()
{
  if (_server->validateCA)
  {
    DEBUG_MSG("Ignoring CA verification - SMTP only");
  }
  client = new WiFiClient();
  if (NULL == client)
    return false;
  client->connect(_server->servername, _server->port);
  DEBUG_MSG("Connection to %s:%d ... %S", _server->servername.c_str(), _server->port, client->connected() ? PSTR("OK") : PSTR("failed"));
  return client->connected();
}

bool SMTPSSender::connect()
{
  client = new WiFiClientSecure();
  if (NULL == client)
    return false;

  DEBUG_MSG("%SCA validation!", _server->validateCA ? PSTR("") : PSTR("NO "));

  if (_server->validateCA == false)
  {
    // disable CA checks
    reinterpret_cast<WiFiClientSecure *>(client)->setInsecure();
  }

  // Determine if MFLN is supported by a server
  // if it returns true, use the ::setBufferSizes(rx, tx) to shrink
  // the needed BearSSL memory while staying within protocol limits.
  bool mfln = reinterpret_cast<WiFiClientSecure *>(client)->probeMaxFragmentLength(_server->servername, _server->port, 512);

  DEBUG_MSG("MFLN %Ssupported", mfln ? PSTR("") : PSTR("un"));

  if (mfln)
  {
    reinterpret_cast<WiFiClientSecure *>(client)->setBufferSizes(512, 512);
  }

  reinterpret_cast<WiFiClientSecure *>(client)->connect(_server->servername, _server->port);
  return reinterpret_cast<WiFiClientSecure *>(client)->connected();
}

void SMTPSender::printClientBase64(const String &msg)
{
  String tmp = base64::encode(msg, false);
  DEBUG_MSG(">> %s [=> %s]", msg.c_str(), tmp.c_str());
  client->println(tmp);

/*  Base64Coder coder;
  String tmp;

#if defined DEBUG_MSG

#endif

  const char *s = msg.c_str();
  coder.init();

  while (*s)
  {
    coder.encode(*s++);
    for (char c; (c = coder.eget());)
#if defined DEBUG_MSG
      tmp += c;
#else
      client->print(c);
#endif
  }
  for (char c; (c = coder.finalize());)
#if defined DEBUG_MSG
    tmp += c;
#else
    client->print(c);
#endif

#if defined DEBUG_MSG
  DEBUG_MSG(">> %s [=> %s]", msg.c_str(), tmp.c_str());
  client->print(tmp);
#endif
  client->print('\n');
  */
}

bool SMTPSender::waitFor(const uint16_t respCode, const __FlashStringHelper *errorString, uint16_t timeOut)
{
  // initalize waiting
  if (0 == waitUntil)
  {
    waitUntil = millis();
    waitUntil += timeOut;
    _serverStatus.desc.remove(0);
  }
  else
  {
    // timeout
    if ((int32_t)(millis() - waitUntil) >= 0)
    {
      DEBUG_MSG("Waiting for code %u - timeout!", respCode);
      _serverStatus.code = 65535;
      if (errorString)
      {
        _serverStatus.desc = errorString;
      }
      else
      {
        _serverStatus.desc = F("timeout");
      }
      smtpState = cTimeout;
      waitUntil = 0;
      return false;
    }

    // check for bytes from the client
    while (client->available())
    {
      char c = client->read();
      //DEBUG_MSG("readChar() line='%s' <= %c", _serverStatus.desc.c_str(), c);
      if (c == '\n' || c == '\r')
      {
        // filter out empty lines
        _serverStatus.desc.trim();
        if (0 == _serverStatus.desc.length())
          continue;

        // line complete, evaluate code
        _serverStatus.code = strtol(_serverStatus.desc.c_str(), NULL, 0);
        if (respCode != _serverStatus.code)
        {
          smtpState = cError;
          DEBUG_MSG("Waiting for code %u but SMTP server replies: %s", respCode, _serverStatus.desc.c_str());
        }
        else
        {
          DEBUG_MSG("Waiting for code %u success, SMTP server replies: %s", respCode, _serverStatus.desc.c_str());
        }

        waitUntil = 0;
        return (respCode == _serverStatus.code);
      }
      else
      {
        // just add the char
        _serverStatus.desc += c;
      }
    }
  }
  return false;
}
