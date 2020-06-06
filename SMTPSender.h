/** \mainpage SMTPSender library
 *
 * MIT license
 * written by Renzo Mischianti
 * enhanced by Daniel Plasa:
 * 1. split into a plain SMTP and a SMTPS class to save code space in case only SMTP is needed.
 * 2. drop base64 code, use ESP8266 core's base64 encoder instead
 * 3. Supply now two ways of sending an email:
 *    a)  send()  - blocking, returns if sent or error
 *    b)  queue() - non-blocking, returns immedeate. call check() for status of process
 * 4. use esp8266 polledTimeout timeout library for the timeout handling
 * 
 *    When using non-blocking mode, be sure to call handleSMTP() frequently, e.g. in loop().
 */

#ifndef SMTPSender_h
#define SMTPSender_h

#include <WString.h>
#include <time.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PolledTimeout.h>
using esp8266::polledTimeout::oneShotMs; // import the type to the local namespace

// Use ESP8266 Core Debug functionality
#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(fmt, ...)                                               \
	do                                                                    \
	{                                                                     \
		DEBUG_ESP_PORT.printf_P(PSTR("[SMTP] " fmt "\n"), ##__VA_ARGS__); \
		yield();                                                          \
	} while (0)
#else
#define DEBUG_MSG(...)
#endif

class SMTPSender
{
public:
	static constexpr int16_t errorUninitialized = -1;
	static constexpr int16_t errorAlreadyInProgress = -2;
	static constexpr int16_t errorConnectionFailed = -3;
	static constexpr int16_t errorTimeout = -4;

	struct ServerInfo
	{
		ServerInfo(const String &_l, const String &_pw, const String &_sn, uint16_t _p, bool v = false) : login(_l), password(_pw), servername(_sn), port(_p), validateCA(v) {}
		ServerInfo() = default;
		String login;
		String password;
		String servername;
		uint16_t port;
		bool validateCA = false;
	};

	struct Message
	{
		Message(const String &_f, const String &_t, const String &_s, const String &_m) : from(_f), to(_t), subject(_s), message(_m) {}
		Message() = default;
		time_t date;
		String from;
		String to;
		String subject;
		String message;
	};

	typedef enum
	{
		OK,
		PROGRESS,
		ERROR,
	} TransferResult;
	
	typedef struct
	{
		TransferResult result = OK;
		int16_t code;
		String desc;
	} Status;

	SMTPSender();

	// initialize Sender with your smtp server credentials
	void begin(const ServerInfo &server);

	// queue a message (if possible) send it nonblocking via handleSMTP()
	bool queue(const Message &the_email);

	// check status
	const Status &check();

	// send a message blocking - this will call internally queue & handleSMTP until mail is sent
	const Status &send(const Message &the_email);

	// call freqently (e.g. in loop()), when using non-blocking mode
	void handleSMTP();

protected:
	enum internalState
	{
		cConnect = 0,
		cGreet,
		cHelo,
		cAuth,
		cUser,
		cPassword,
		cFrom,
		cTo,
		cData,
		cSendMessage,
		cSendOK,
		cQuit,
		cIdle,
		cTimeout,
		cError
	};
	internalState smtpState = cIdle;

	WiFiClient *client = nullptr;
	const ServerInfo *_server;
	const Message *_email;

	Status _serverStatus;
	virtual bool connect();
	void printClientBase64(const String &msg);

    oneShotMs aTimeout;  // timeout from esp8266 core library
	bool waitFor(const int16_t respCode, const __FlashStringHelper *errorString = nullptr, uint16_t timeOut = 10000);
};

// basically just the same as SMTP sender but has a different connect() method to account for SSL/TLS
// connection stuff
class SMTPSSender : public SMTPSender
{
public:
	SMTPSSender() = default;

private:
	virtual bool connect() override;
};

#endif
