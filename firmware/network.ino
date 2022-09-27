#define WIFI_MAX_CONCECTION_TRIES               10
#define WIFI_TRY_DELAY                          500

#define RECV_BUFFER_SIZE                        3*MAX_LED_ON_STRIP

class network
{
public:
    struct AP
    {
        IPAddress ip, gateway, submask;
        String ssid, pass;
    };

public:
    static void connectToWifi( const AP &ap )
    {
        int ret = 0, i = 0;

        disconnectFromWifi();

        if(!WiFi.config(ap.ip, ap.gateway, ap.submask))
        {
            LOG("failed to apply IPv4 settings");
        }

        printf("attempting to connect to \"%s\"", ap.ssid.c_str());
        WiFi.begin(ap.ssid, ap.pass);

        for(; i < WIFI_MAX_CONCECTION_TRIES; i++)
        {
            printf(".");    

            if(WiFi.status() == WL_CONNECTED)
                break;
            
            delay(WIFI_TRY_DELAY);    
        }

        if(WiFi.status() == WL_CONNECTED)
        {
            printf("OK!\n");

            wifiServer.stop();
            wifiServer = WiFiServer(settings.port);
            wifiServer.begin();

            LOG("connected using IP \"%s\"", getLocalIP());
            LOG("server now listening on port %d", settings.port);
        }
        else
        {
            printf("BAD!\n");
            LOG("failed to connect to \"%s\"", settings.ssid.c_str());
        }
    }


    static void disconnectFromWifi( void )
    {
        int ret = 0;

        WiFi.disconnect();
        ret = -1 ? WiFi.status() == WL_CONNECTED : 0;

        if(!ret)
            LOG("disconnected from \"%s\"", settings.ssid.c_str());
    }

    inline const IPAddress &getLocalIP()
    {
        return WiFi.localIP();
    }

    const char * readIncomming( size_t &len )
    {
        client = wifiServer.available();

        if(!client)
            return NULL;
        
        buffer.len = 0;
        while(client.available && len < sizeof(buffer.data)/sizeof(buffer.data[0]))
            buffer.data[buffer.len ++] = client.read();
        
        len = buffer.len;
        return buffer.data;
    }

private:
    static WiFiServer wifiServer;
    static struct
    {   
        char data[RECV_BUFFER_SIZE];
        size_t len;
    } buffer = {0};
};