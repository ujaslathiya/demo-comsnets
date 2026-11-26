namespace FiCo4OMNeT;

cplusplus {{
#if defined(__clang__)
#  pragma clang diagnostic ignored "-Winconsistent-missing-override"
#elif defined(__GNUC__) && __GNUC__ >= 5
#  pragma GCC diagnostic ignored "-Wsuggest-override"
#endif
}}

packet CanDataFrame
{
    unsigned int canID;
    bool rtr;
    double period;

    bool isCanFd = false;
    bool brs = false;

    int dlc = 8;
    uint8 payload[];

    unsigned int crc = 0;
    bool esi = false;
    bool extendedID = false;
}

