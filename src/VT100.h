#include <vector>

class VT100Client;

class VT100 {
public:
    VT100(VT100Client*);
    void parseBuffer(const char* start, const char* end);

private:
    VT100Client* m_client;
    int cs;
    int unsignedValue;
    std::vector<int> numberStack;
};
