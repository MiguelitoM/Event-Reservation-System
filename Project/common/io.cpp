#include "io.hpp"
using namespace std;

int parse_login_logout(string uid, string password) {
    return (parse_uid(uid) || parse_password(password));
}

int parse_uid(string uid) {
    if (uid.size() != 6) {
        return 1;
    }
    for (char c : uid) {
        if (!isdigit(c)) {
            return 1;
        }
    }
    return 0;
}

int parse_password(string password) {
    if (password.size() != 8) {
        return 1;
    }
    for (char c : password) {
        if (!isalnum(c)) {
            return 1;
        }
    }

    return 0;
}

int parse_date(string date){
    tm tm1{};

    char* end = strptime(date.c_str(), "%d-%m-%Y", &tm1);
    if (!end || *end != '\0') {
        return 1;
    }
    int d = tm1.tm_mday;
    int m = tm1.tm_mon;
    int y = tm1.tm_year;
    tm1.tm_hour = 12;
    tm1.tm_min = 0;
    tm1.tm_sec = 0;
    tm1.tm_isdst = -1;
    time_t t = mktime(&tm1);

    tm tm2{};
    localtime_r(&t, &tm2);

    if (!(tm2.tm_mday == d && tm2.tm_mon == m && tm2.tm_year == y)) {
        return 1;
    }
    return 0;
}

int parse_hour(string hour){
    tm tm1{};
    
    char* end = strptime(hour.c_str(), "%H:%M", &tm1);
    if (!end || *end != '\0') {
        return 1;
    }
    int hh = tm1.tm_hour;
    int mm = tm1.tm_min;
    tm1.tm_year = 120;
    tm1.tm_mon  = 0;
    tm1.tm_mday = 15;
    tm1.tm_sec  = 0;
    tm1.tm_isdst = -1;

    time_t t = mktime(&tm1);
    tm tm2{};
    localtime_r(&t, &tm2);

    if (!(tm2.tm_hour == hh && tm2.tm_min == mm)) {
        return 1;
    }
    return 0;
}

int parse_hour_with_seconds(string hour){
    tm tm1{};
    
    char* end = strptime(hour.c_str(), "%H:%M:%S", &tm1);
    if (!end || *end != '\0') {
        return 1;
    }
    int hh = tm1.tm_hour;
    int mm = tm1.tm_min;
    int ss = tm1.tm_sec;
    tm1.tm_year = 120;
    tm1.tm_mon  = 0;
    tm1.tm_mday = 15;
    tm1.tm_isdst = -1;

    time_t t = mktime(&tm1);
    tm tm2{};
    localtime_r(&t, &tm2);

    if (!(tm2.tm_hour == hh && tm2.tm_min == mm && tm2.tm_sec == ss)) {
        return 1;
    }
    return 0;
}

int parse_name(string name) {
    if (name.size() > 10) {
        return 1;
    }
    
    for (char c : name) {
        if (!isalnum(c)) {
            return 1;
        }
    }
    return 0;
}

int parse_filename(string filename){
    if (filename.size() > 24) {
        return 1;
    }
    for (char c : filename) {
        if (!(isalnum(c)|| c=='-'||c=='_'||c=='.')) {
            return 1;
        }
    }
    return 0;
}

int parse_attendance(string attendance) {
    if (!(attendance.size() == 2 || attendance.size() == 3)) {
        return 1;
    }
    for (char c : attendance) {
        if (!isdigit(c)) {
            return 1;
        }
    }
    int attendees = stoi(attendance);
    if (attendees < 10) {
        return 1;
    }
    if (attendees > 999) {
        return 1;
    }
    return 0;
}
int parse_reserve_number(string amount){
    if (amount.size() > 3 || amount.size() < 1) {
        return 1;
    }
    for (char c : amount) {
        if (!isdigit(c)) {
            return 1;
        }
    }
    return 0;
}
int parse_eid(string eid) {
    if (eid.size() != 3) {
        return 1;
    }
    for (char c : eid) {
        if (!isdigit(c)) {
            return 1;
        }
    }
    return 0;
}

int parse_value_reserved(string value) {
    if (!(value.size() == 1 || value.size() == 2 || value.size() == 3)) {
        return 1;
    }
    for (char c : value) {
        if (!isdigit(c)) {
            return 1;
        }
    }
    int attendees = stoi(value);
    if (attendees < 1) {
        return 1;
    }
    if (attendees > 999) {
        return 1;
    }
    return 0;
}

int parse_seats_reserved(string value) {
    if (!(value.size() == 1 || value.size() == 2 || value.size() == 3)) {
        return 1;
    }
    for (char c : value) {
        if (!isdigit(c)) {
            return 1;
        }
    }
    int attendees = stoi(value);
    if (attendees < 0) {
        return 1;
    }
    if (attendees > 999) {
        return 1;
    }
    return 0;
}

int parse_filesize(string filesize) {
    if (filesize.size() > 8) {
        return 1;
    }

    for (char c : filesize) {
        if (!isdigit(c)) {
            return 1;
        }
    }
    int filesize_int = stoi(filesize);
    if (filesize_int < 1) {
        return 1;
    }
    if (filesize_int > MAX_FILESIZE) {
        return 1;
    }
    return 0;
}

bool is_event_past(string& date, string& hour) {
    string datetime = date + " " + hour;
    struct tm event_tm{};
    event_tm.tm_isdst = -1; // Decide sozinho o fuso horário

    strptime(datetime.c_str(), "%d-%m-%Y %H:%M", &event_tm);
    time_t event_time = mktime(&event_tm);
    time_t now = time(nullptr);

    return event_time <= now;
}

int send_file(int fd, fs::path file_path) {
    if (!fs::exists(file_path)) {
        return -1;
    }

    ifstream file(file_path, ios::binary);
    if (!file.is_open()) {
        return -1;
    }

    size_t file_size = fs::file_size(file_path);
    
    if (file_size > MAX_FILESIZE) {
        return -1;
    }
    char buffer[BUFSIZ];
    size_t remaining = file_size;
    while(remaining > 0) {
        size_t to_read;
        if (remaining < BUFSIZ) {
            to_read = remaining;
        } else {
            to_read = BUFSIZ;
        }

        file.read(buffer, to_read);
        if (!file) {
            return -1;
        }

        string chunk(buffer, to_read);
        if (safe_write(fd, chunk)) {
            return -1;
        }

        remaining -= to_read; 
    }

    string new_line = "\n";
    if (safe_write(fd, new_line)) {
        return -1;
    }
    return 0;
}

int safe_read(int fd, string& out) {
    out.clear();
    size_t total_read = 0;
    char buffer[256];

    while(total_read < sizeof(buffer)) {
        int n = read(fd, buffer, sizeof(buffer) - total_read);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ECONNRESET){
                return 0;
            }
            return -1;
        }
        else if (n == 0) {
            break;
        }

        out.append(buffer, n);
        total_read += n;
    }
    return 0;
}

int safe_write(int fd, string& msg) {
    size_t total_written = 0;
    size_t msg_size = msg.size();
    const char* data = msg.data();

    while (total_written < msg_size) {
        size_t to_write = msg_size - total_written;
        if (to_write > BUFFSIZE) {
            to_write = BUFFSIZE;
        }

        int n = write(fd, data + total_written, to_write);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ECONNRESET) {
                return 0;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            return -1;
        }
        else if (n == 0) {
            break;
        }

        total_written += n;
    }
    return 0;
}