#ifndef CRE_STATE_H
#define CRE_STATE_H

#include <netdb.h>
#include <string>

using namespace std;

class CreState {
    public:
    bool header_complete = false;
    string username = "";
    string password = "";
    string date = "";
    string hour = "";
    string filename = "";
    size_t filesize = 0;
    string name = "";
    string eid = "";
    int attendance_size=0;
    size_t bytes_written = 0;
    ofstream description;
};

#endif