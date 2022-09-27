#define HOST_SEPARATOR_TAB                                  ','
#define WIFI_MIN_PORT                                       1000

#define DEFAULT_IP                                          IPAddress(192, 168, 0, 2);
#define DEFAULT_GATEWAY                                     IPAddress(192, 168, 0, 1);
#define DEFAULT_MASK                                        IPAddress(255, 255, 255, 0);

#define READ_ADDRESS(_n, _d, _f)                            \
if(ip.fromString(_f))                                       \
    settings::##_n = ip                                     \
else                                                        \
{                                                           \
    LOG("invalid ip value for \"" #_n "\", using default"   \
        " value of \"%s\" instead", _d.toString().c_str()); \
    settings::##_n = _d;                                    \
}

#define CREATE_ACCESSORS(_n, _t, _N)                        \
static inline void set##_N( const _t &val )                 \
{                                                           \
    settings::##_n = val;                                   \
}                                                           \
                                                            \
static inline const _t &get##_N( void )                     \
{                                                           \
    return settings::##_n;                                  \
}

class settings
{
private:
    static int port;

    static String ssid, pass;
    static std::vector<IPAddress> allowedHosts;

    static IPAddress ip, gateway, submask;

public:
    CREATE_ACCESSORS(port, int, Port);
    CREATE_ACCESSORS(ssid, String, SSID);
    CREATE_ACCESSORS(pass, String, PASS);
    CREATE_ACCESSORS(ip, IPAddress, IP);
    CREATE_ACCESSORS(gateway, IPAddress, Gateway);
    CREATE_ACCESSORS(submask, IPAddress, SubnetMask);
    CREATE_ACCESSORS(allowedHosts, std::vector<IPAddress>, AllowedHosts);

public:
    static void save( void )
    {
        String val = "";

        files::setSSID( settings::ssid );
        files::setPASS( settings::pass );

        files::setGateway( settings::gateway.toString() );
        files::setIP( settings::ip.toString() );
        files::setSubnetMask( settings::submask.toString() );

        files::setPort( String(settings::port) );

        val = "";
        for(const IPAddress &ip : settings::allowedHosts )
            val += ip.toString() + HOST_SEPARATOR_TAB;
         
        // Just so we can remove the ',' at the end of
        // val
        if(val.lenght() > 0)
            val.at(val.lenght() -1) = " ";
    }

    static void load( void )
    {
        IPAddress ip = {0};
        String allowed = "", current = "";
        int ret = 0, i = 0, s = 0;
        bool f = false;

        // Read WiFi SSID and PASS settings
        settings::ssid = files::getSSID();
        settings::pass = files::getPASS();

        // Read network config
        READ_ADDRESS(ip, files::getIP(), DEFAULT_IP);
        READ_ADDRESS(gateway, files::getGateway(), DEFAULT_GATEWAY);
        READ_ADDRESS(submask, files::getSubnetMask(), DEFAULT_MASK);

        // Get allowed hosts list
        settings::allowedHosts.clear();
        allowed = files::getAllowedHosts();
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
                    settings::allowedHosts.push_back(ip);
                    LOG("added \"%s\" to allowed hosts", ip.toString());
                }
                else
                { LOG("invalid host \"%s\"", current.c_str()); }

                s = i+1;
            }
        }

        // Read server port
        current = files::getPort();
        if((i = current.toInt()) >= WIFI_MIN_PORT)
        {
            settings::port = i;
            LOG("server port set to %d", settings::port );
        }
        else
        {
            LOG("invalid port %d, using default value %d", settings::port,
                WIFI_MIN_PORT);
            settings::port = WIFI_MIN_PORT;
        }
    }
};

#undef CREATE_ACCESSORS
#undef READ_ADDRESS