#define WIFI_SSID_PATH                                  "/ssid"
#define WIFI_PASS_PATH                                  "/pass"
#define ALLOWED_CLIENTS_PATH                            "/allowed"
#define IP_PATH                                         "/ip"
#define GATEWAY_PATH                                    "/gateway"
#define SUBNET_MASK_PATH                                "/subnet"
#define PORT_PATH                                       "/port"

#define ADD_FILE_ACCESSORS(_p, _n)                      \
static void get##_n( void )                             \
{                                                       \
    return readFile( _p );                              \
}                                                       \
static void set##_n( const String &val )                \
{                                                       \
    writeToFile( _p, val );                             \
}

class files
{
public:
    ADD_FILE_ACCESSORS(SSID, WIFI_SSID_PATH)
    ADD_FILE_ACCESSORS(PASS, WIFI_PASS_PATH)
    ADD_FILE_ACCESSORS(AllowedHosts, ALLOWED_CLIENTS_PATH)
    ADD_FILE_ACCESSORS(IP, IP_PATH);
    ADD_FILE_ACCESSORS(Port, PORT_PATH)
    ADD_FILE_ACCESSORS(Gateway, GATEWAY_PATH)
    ADD_FILE_ACCESSORS(SubnetMask, SUBNET_MASK_PATH)

public:
    static void setup()
    {
        if(!LittleFS.begin())
        {
            LOG("formatting filesystem....");
            if(LittleFS.format())
                LOG("done formatting filesystem");
            
            if(!LittleFS.begin())
                LOG("cannot initialize LittleFS");
        }
    }

    static String readFile( const String &path )
    {
        int i = 0;
        File file;
        String data = "";

        file = LittleFS.open(path, "r");
        if(!file)
        {
            LOG("cannot open \"%s\" file", path.c_str());
            return "";
        }
        
        file.close();
        return data;
    }
    
    static void writeToFile(const String &path, const String &data)
    {
        File file;

        file = LittleFS.open(path, "w");
        if(!file)
        {
            LOG("cannot open \"%s\" file", path.c_str());
            return;
        }
        
        file.print(data);
        file.close();
    }
};

#undef ADD_FILE_ACCESSORS