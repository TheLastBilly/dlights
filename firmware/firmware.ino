#include <vector>

#include "LittleFS.h"
#include <FastLED.h>

#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>

#define WIFI_SSID_PATH                          "/ssid"
#define WIFI_PASS_PATH                          "/pass"
#define ALLOWED_CLIENTS_PATH                    "/allowed"
#define IP_PATH                                 "/ip"
#define GATEWAY_PATH                            "/gateway"
#define SUBNET_MASK_PATH                        "/subnet"
#define PORT_PATH                               "/port"

#define HOST_SEPARATOR_TAB                      ','
#define WIFI_MAX_CONCECTION_TRIES               50
#define WIFI_MIN_PORT                           1000

#define LED_DEFAULT_DATA_PIN                    D2

#define MAX_LED_ON_STRIP                        120
#define SERIAL_BAUDRATE                         115200

#define MAX_PRINTF_BUFFER                       128
#define MAX_CMD_BUFFER_SIZE                     3*MAX_LED_ON_STRIP
#define FILE_BUF_SIZE                           128
#define COMMAND_PROMPT                          "~ "

#define CREATE_SETTING_ACCESSOR(_name, _path)   \
int                                             \
set##_name(const String &val)                   \
{                                               \
    int ret = 0;                                \
    ret = writeToFile(#_path, val);             \
    if(ret)                                     \
    {                                           \
        LOG("cannot set "#_name": (%d)", ret);  \
    }                                           \
    else                                        \
    {                                           \
        LOG(#_name " has been set to \"%s\"",   \
            val.c_str())                        \
    }                                           \
                                                \
    return ret;                                 \
}                                               \
String                                          \
get##_name()                                    \
{                                               \
    String val = "";                            \
    int ret = 0;                                \
    ret = readFromFile(#_path, val);            \
    if(ret)                                     \
    {                                           \
        LOG("cannot get value for "#_name":"    \
        "(%d)", ret);                           \
        return "";                              \
    }                                           \
    return val;                                 \
}

#define _printf(_fmt, ...)                      \
{                                               \
    sprintf(_PRINTF_BUF,_fmt"%s", __VA_ARGS__,  \
    "\0");                                      \
    Serial.print(_PRINTF_BUF);                  \
}
#define printf(...)                             _printf(__VA_ARGS__,"")
#define _LOG(_fmt, ...)                         _printf(_fmt "\n", __VA_ARGS__)
#define LOG(...)                                _LOG(__VA_ARGS__,"")

struct
{
    int port;

    String ssid, pass;
    std::vector<IPAddress> allowedHosts;

    IPAddress ip, gateway, submask;
} settings;

struct command_t
{
    const char * name, description;
    void (*func)(const String &argument);
};

static CRGB LED_BUFFER[MAX_LED_ON_STRIP]        = {0};
static char CMD_BUFFER[MAX_CMD_BUFFER_SIZE]     = {0};
static char _PRINTF_BUF[MAX_PRINTF_BUFFER]      = {0};

static WiFiServer wifiServer = WiFiServer(WIFI_MIN_PORT);

int
writeToFile(const String &path, const String &data)
{
    File file;

    file = LittleFS.open(path, "w");
    if(!file)
    {
        LOG("cannot open \"%s\" file", path.c_str());
        return -1;
    }
    
    file.print(data);
    file.close();
    return 0;
}

int
readFromFile(const String &path, String &data)
{
    int i = 0;
    File file;

    file = LittleFS.open(path, "r");
    if(!file)
    {
        LOG("cannot open \"%s\" file", path.c_str());
        data = "";
        return -1;
    }
    
    data = file.readString();
    
    file.close();
    return 0;
}

CREATE_SETTING_ACCESSOR(SSID, WIFI_SSID_PATH)
CREATE_SETTING_ACCESSOR(PASS, WIFI_PASS_PATH)
CREATE_SETTING_ACCESSOR(AllowedHosts, ALLOWED_CLIENTS_PATH)
CREATE_SETTING_ACCESSOR(IP, IP_PATH);
CREATE_SETTING_ACCESSOR(Port, PORT_PATH)
CREATE_SETTING_ACCESSOR(Gateway, GATEWAY_PATH)
CREATE_SETTING_ACCESSOR(SubnetMask, SUBNET_MASK_PATH)

void
updateSettings( void )
{
    IPAddress ip = {0};
    String allowed = "", current = "";
    int ret = 0, i = 0, s = 0;
    bool f = false;

    memset(&settings, 0, sizeof(settings));

    settings.ssid = getSSID();
    settings.pass = getPASS();

    LOG("WiFi's SSID set to \"%s\"", settings.ssid.c_str());

    ip.fromString(getIP());
    settings.ip = ip;

    ip.fromString(getGateway());
    settings.gateway = ip;

    ip.fromString(getSubnetMask());
    settings.submask = ip;

    settings.allowedHosts.clear();
    allowed = getAllowedHosts();
    for(; i < allowed.length(); i++)
    {
        if(allowed.charAt(i) == HOST_SEPARATOR_TAB || 
            (f = (i >= allowed.length() -1)))
        {
            if(f)
                current = allowed.substring(s);
            else
                current = allowed.substring(s, i);

            if(ip.fromString(current))
            {
                settings.allowedHosts.push_back(ip);
                LOG("added \"%s\" to allowed hosts", ip.toString());
            }
            else
            { LOG("invalid host \"%s\"", current.c_str()); }

            s = i+1;
        }
    }

    current = getPort();
    if((i = current.toInt()) >= WIFI_MIN_PORT)
    {
        settings.port = i;
        LOG("server port set to %d", settings.port );
    }
    else
    {
        LOG("invalid port %d", settings.port);
    }
}

int
disconnectFromWifi( void )
{
    int ret = 0;

    WiFi.disconnect();
    ret = -1 ? WiFi.status() == WL_CONNECTED : 0;

    if(!ret)
        LOG("disconnected from \"%s\"", settings.ssid.c_str());

    return ret;
}

String
getLocalIP()
{
    return WiFi.localIP().toString();
}

int
connectToWifi( void )
{
    int ret = 0, i = 0;

    disconnectFromWifi();

    if(!WiFi.config(settings.ip, settings.gateway, settings.submask))
    {
        LOG("failed to apply IPv4 settings");
    }

    printf("attempting to connect to \"%s\"", settings.ssid.c_str());
    WiFi.begin(settings.ssid, settings.pass);

    for(; i < WIFI_MAX_CONCECTION_TRIES; i++)
    {
        printf(".");    

        if(WiFi.status() == WL_CONNECTED)
            break;
        
        delay(500);    
    }

    if(WiFi.status() == WL_CONNECTED)
    {
        printf("OK!\n");

        wifiServer.stop();
        wifiServer = WiFiServer(settings.port);
        wifiServer.begin();
        
        LOG("connected using IP \"%s\"", getLocalIP());

        LOG("server now listening on port %d", settings.port);

        return 0;
    }
    else
    {
        printf("BAD!\n");
        LOG("failed to connect to \"%s\"", settings.ssid.c_str());
        return 1;
    }
}

void
printAllowedHosts( void )
{
    LOG("%s", getAllowedHosts().c_str());
}

void
printInfo( void )
{
    String temp = "";

    LOG("SSID: %s", getSSID());

    temp = (WiFi.status() == WL_CONNECTED) ?
        "True" : "False";
    LOG("WiFi connected: %s", temp.c_str());

    LOG("IP: %s", getLocalIP().c_str());

    LOG("Gateway: %s", getGateway().c_str());
    LOG("SubnetMask: %s", getSubnetMask().c_str());

    printf("Allowed Hosts: ");
    printAllowedHosts();

    LOG("Port: %s", getPort());
}

void
stop( void )
{
    while(1) 
        delay(100);
}


const command_t COMMANDS[0] PROGMEM = {
    (command_t){
        .name = "setSSID",
        .description = "Set's the SSID the WiFi module"
            " will connect to",
        .func = setSSID
    },
    (command_t){
        .name = "setPASS",
        .description = "Set's the password the WiFi"
            " module will use to connect to the Access Point",
        .func = setSSID
    },
    (command_t){
        .name = "connectToWifi",
        .description = "Connects to the WiFi network",
    }
};

int
processCommand( const String &line )
{
    int i = 0;
    String command = "", argument = "",  temp = "";

    if(line.length() < 1)
        return -1;
    
    command = line;

    i = line.indexOf(' ');
    if(i < line.length() - 1 && i > 0)
    {
        command = command.substring(0, i);
        argument = line.substring(i+1);
    }

#define IS_COMMAND(_str)                        (command.indexOf(_str) == 0)

    if(IS_COMMAND("setSSID"))
        setSSID(argument);

    else if(IS_COMMAND("setPASS"))
        setPASS(argument);

    else if(IS_COMMAND("connectToWifi"))
        connectToWifi();
        
    else if(IS_COMMAND("clearHosts"))
    {
        setAllowedHosts("");
    }

    else if(IS_COMMAND("addHost"))
    {
        temp = getAllowedHosts();
        if(temp.length() > 0)
            temp += HOST_SEPARATOR_TAB;

        temp += argument;
        setAllowedHosts(temp);
    }

    else if(IS_COMMAND("listHosts"))
    {
        printAllowedHosts();
    }

    else if(IS_COMMAND("info"))
    {
        printInfo();
    }

    else if(IS_COMMAND("setIP"))
    {
        setIP(argument);
    }

    else if(IS_COMMAND("setGateway"))
    {
        setGateway(argument);
    }

    else if(IS_COMMAND("setSubnetMask"))
    {
        setSubnetMask(argument);
    }

    else if(IS_COMMAND("setPort"))
    {
        setPort(argument);
    }

    else if(IS_COMMAND("updateSettings"))
    {
        updateSettings();
    }

    else
    {
        LOG("bad command \"%s\"", command.c_str());
    }

#undef IS_COMMAND
}

void 
setup( void )
{
    int ret = 0;
    String data = "LOL";

    Serial.begin(SERIAL_BAUDRATE);
    printf("\n");
    if(!LittleFS.begin())
    {
        LOG("formatting filesystem....");
        if(LittleFS.format())
            LOG("done formatting filesystem");
        
        if(!LittleFS.begin())
            LOG("cannot initialize LittleFS");
    }

    LOG("applying settings...");
    updateSettings();

    connectToWifi();

    FastLED.addLeds<WS2812, LED_DEFAULT_DATA_PIN, GRB>(LED_BUFFER,
        MAX_LED_ON_STRIP);
    
    printf(COMMAND_PROMPT);
}

void 
loop( void )
{
    WiFiClient client;
    CRGB rgb;
    int i = 0, s = 0, ret = 0;
    char c = 0;

    // Handle serial commands
    if((ret = Serial.available()) > 0 )
    {
        static int index = 0;
        bool valid_character = false;

        c = Serial.read();

#define CLEAR_BUFFER()                                  \
{                                                       \
    memset(CMD_BUFFER, 0, index*sizeof(CMD_BUFFER[0])); \
    index = 0;                                          \
    printf(COMMAND_PROMPT);                             \
}

        if(index > 0 && (c == 13 || c == '\r' || c == '\n'))
        {
            printf("\n");
            processCommand(String(CMD_BUFFER));
            CLEAR_BUFFER();
        }
        else
        {
            if(c != '\n' && c != 13)
            {
                valid_character = true;
                switch(c)
                {
                    case 3:
                        printf("^C\n");
                        CLEAR_BUFFER();
                        valid_character = false;
                        break;
                    case 8:
                        printf("^B");
                        valid_character = false;
                        break;
                    default:
                        printf("%c", c);
                }
            }

            if(index < MAX_CMD_BUFFER_SIZE-1 && valid_character)
                CMD_BUFFER[index++] = c;
            
        }

#undef CLEAR_BUFFER

    }

    // Handle IP commands
    client = wifiServer.available();

    if(client)
    {
        i = 0;
        for(const IPAddress &c : settings.allowedHosts)
        {
            if(client.remoteIP() == c)
            {
                i = 1;
                break;
            }
        }
        
        if(i)
        {
            uint8_t * ptr = NULL;

            LOG("received valid connection from \"%s\"", 
                client.remoteIP().toString().c_str());

            i = 0;
            s = 0;
            ptr = (uint8_t *)LED_BUFFER;
            while(client && client.available() > 0)
            {
                if(i >= sizeof(LED_BUFFER))
                    break;
                
                rgb.raw[i % 3] = client.read();
                if(i%3 == 2)
                {
                    if(s >= sizeof(LED_BUFFER)/sizeof(LED_BUFFER[0]))
                        break;
                        
                    LED_BUFFER[s++] = rgb;
                }

                i++;
            }
        }

        else
        {
            LOG("received invalid request from \"%s\"",
                client.remoteIP().toString().c_str());
        }     
    }

    FastLED.show();
}