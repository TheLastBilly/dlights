#define CLI_BUFFER_SIZE                                 128
#define COMMAND_PROMPT                                  "~ "

#define CLEAR_BUFFER()                                  \
{                                                       \
    memset(buffer.data, 0, sizeof(buffer.data));        \
    index = 0;                                          \
    printf(COMMAND_PROMPT);                             \
}

#define IS_COMMAND(_str)                                (command.indexOf(_str) == 0)

class shell
{
public:
    static void readIncomming()
    {
        static int index = 0;
        int i = 0, ret = 0;
        char c = 0;
        bool valid_character = false;

        if((ret = Serial.available()) < 1 )
            return;
        
        c = Serial.read();

        if(index > 0 && (c == 13 || c == '\r' || c == '\n'))
        {
            printf("\n");
            processCommand(String(buffer.data));
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

            if(index < CLI_BUFFER_SIZE-1 && valid_character)
                buffer.data[index++] = c;
        }
    }

private:

    static void processCommand( const String &line )
    {
        int i = 0;
        IPAddress ip;
        std::vector<IPAddress> hosts;
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

        if(IS_COMMAND("setSSID"))
            settings::setSSID(argument);

        else if(IS_COMMAND("setPASS"))
            settings::setPASS(argument);

        else if(IS_COMMAND("connectToWifi"))
            network::connectToWifi();
            
        else if(IS_COMMAND("clearHosts"))
        {
            hosts.clear();
            setttings::setAllowedHosts(hosts);
        }

        else if(IS_COMMAND("addHost"))
        {
            if(ip.fromString(argument))
            {
                hosts = settings::getAllowedHosts();
                hosts.push_back(ip);
                settings::setAllowedHosts(hosts);
            }
            else
            {
                LOG("invalid address \"%s\"", argument);
            }
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
            if(ip.fromString(argument))
                settings::setIP(ip);
            else
            {
                LOG("invalid address \"%s\"", argument);
            }
        }

        else if(IS_COMMAND("setGateway"))
        {
            if(ip.fromString(argument))
                settings::setGateway(ip);
            else
            {
                LOG("invalid address \"%s\"", argument);
            }
        }

        else if(IS_COMMAND("setSubnetMask"))
        {
            if(ip.fromString(argument))
                settings::setSubnetMask(ip);
            else
            {
                LOG("invalid address \"%s\"", argument);
            }
        }

        else if(IS_COMMAND("setPort"))
        {
            i = argument.toInt();
            if(i < WIFI_MIN_PORT)
                LOG("invalid port (port < %d)", WIFI_MIN_PORT);
            else
                settings::setPort(argument.toInt());
        }

        else if(IS_COMMAND("save"))
        {
            settings::save();
        }

        else
        {
            LOG("bad command \"%s\"", command.c_str());
        }
    }

private:
    static struct
    {
        char data[CLI_BUFFER_SIZE];
    } buffer;
};

#undef IS_COMMAND
#undef CLEAR_BUFFER