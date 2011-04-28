class VT100Client;

class VT100 {
public:
    VT100(VT100Client*);
    const char* parseBuffer(const char* start, const char* end);

private:
    const char* executeStateMachine(const char* start, const char* end);

    VT100Client* m_client;
};
