#include "utils.hpp"

fs::path user_dir(const std::string& uid) {
    return fs::path(USER_PATH) / uid;
}

fs::path event_dir(const std::string& eid) {
    return fs::path(EVENT_PATH) / eid;
}

fs::path event_description_dir(const std::string& eid) {
    return fs::path(EVENT_PATH) / eid / "description";
}

fs::path event_description_path(const std::string& eid) {
    fs::path desc_dir = event_description_dir(eid);
    for (const auto& entry : fs::directory_iterator(desc_dir)) {
        return entry.path();
    }
    return {};
}

fs::path password_path(const std::string& uid) {
    return user_dir(uid) / "passwd";
}

fs::path logged_in_path(const std::string& uid) {
    return user_dir(uid) / "logged_in";
}

fs::path event_count_path(){
    return fs::path(string(EVENT_PATH)+string("/")+string(EVENT_COUNT));
}




bool is_closed(string eid){
    return fs::exists(event_dir(eid)/"end");
}



fs::path event_details_path(string eid){
    return fs::path(string(EVENT_PATH)+string("/")+eid+string("/details"));
}

fs::path event_reservations_dir(string eid){
    return fs::path(EVENT_PATH) / eid / "reservations"; 
}

fs::path user_reservations_dir(const std::string& uid){
    return fs::path(USER_PATH) / uid / "reservations";
}

fs::path user_events_dir(const std::string& uid){
    return fs::path(USER_PATH) / uid / "events";
}

fs::path event_total_res_path(string eid){
    return fs::path(EVENT_PATH) / eid / "total-res"; 
}

bool is_logged_in(string uid){
   return fs::exists(logged_in_path(uid));
}

bool check_password(string uid, string password) {
    fs::path passwd_path = password_path(uid);
    string true_pwd;

    ifstream passwd(passwd_path);
    passwd >> true_pwd;
    passwd.close();
    return (password == true_pwd);
}

bool does_user_exist(string uid){
   return fs::exists(password_path(uid)); 
}

bool does_event_exist(string eid){
   return fs::exists(event_dir(eid)); 
}

void destroy_all_events(){
    fs::path events(EVENT_PATH);
    fs::remove_all(events);
    if (!fs::exists(fs::path(EVENT_PATH))){
        fs::create_directory(fs::path(EVENT_PATH));
    }
    fs::path event_count = string(EVENT_PATH)+string("/")+string(EVENT_COUNT);
    if (!fs::exists(event_count)){
        ofstream event_count_file(event_count);
        event_count_file << 0;
        event_count_file.close();
    }
    return;
}

string create_event_dirs(){
    //getting the new event uid
    ifstream event_count_file_read(event_count_path());
    string event_count_string ; 
    event_count_file_read>>event_count_string;
    event_count_file_read.close();
    int event_count=stoi(event_count_string);
    if(event_count == 999){
        return "";
    }
    event_count++;
    ofstream event_count_file_write(event_count_path());
    event_count_file_write << event_count;
    event_count_file_write.close();

    string uid;
    if (event_count<10){
        uid=string("00")+to_string(event_count);
    } else if (event_count<100){
        uid= string("0")+ to_string(event_count);

    } else{
        uid = to_string(event_count);
    }

    //creating all the directories
    fs::create_directories(event_description_dir(uid));
    fs::create_directories(event_reservations_dir(uid));
    ofstream total_res (event_total_res_path(uid));
    total_res<<0;
    total_res.close();
    ofstream details (event_details_path(uid));
    details.close();
    return uid;
}

void destroy_event(string eid){
    fs::path events(EVENT_PATH);
    fs::remove_all(events/eid);
}

string get_event_creator(string eid){
    string cre;
    string uid;
    ifstream ss(event_details_path(eid));
    if (!(ss >> uid )) {
        cout << "Could not get creator\n";
        return "";
    }
    ss.close();
    return uid;
}

int get_event_attendance(string eid){
    string uid, password, name, date, hour;
    string attendance_size, seats_reserved, fname, fsize_str;
    ifstream ss(event_details_path(eid));
    if (!(ss  >> uid >> password >> name >> date >> hour >> attendance_size)) {
        cout << "Could not get attendance\n";
        return -1;
    } 
    ss.close();
    return stoi(attendance_size);
}

string get_event_date(string eid){
    string uid, password, name, date, hour;
    string attendance_size, seats_reserved, fname, fsize_str;
    ifstream ss(event_details_path(eid));
    if (!(ss  >> uid >> password >> name >> date >> hour >> attendance_size)) {
        cout << "Could not get date\n";
        return "-1";
    } 
    ss.close();
    return date;
}

string get_event_hour(string eid){
    string uid, password, name, date, hour;
    string attendance_size, seats_reserved, fname, fsize_str;
    ifstream ss(event_details_path(eid));
    if (!(ss  >> uid >> password >> name >> date >> hour >> attendance_size)) {
        cout << "Could not get hour\n";
        return "-1";
    } 
    ss.close();
    return hour;
}
string get_event_name(string eid){
    string uid, password, name, date, hour;
    string attendance_size, seats_reserved, fname, fsize_str;
    ifstream ss(event_details_path(eid));
    if (!(ss  >> uid >> password >> name >> date >> hour >> attendance_size)) {
        cout << "Could not get hour\n";
        return "-1";
    } 
    ss.close();
    return name;
}
int get_event_state(string eid){
    string date, hour;
    date = get_event_date(eid);
    hour = get_event_hour(eid);
    int sold = get_event_sold(eid);
    int attendance = get_event_attendance(eid);
    if (is_event_past(date,hour)){
        return 0;
    }
    if (attendance - sold == 0){
        return 2;
    }
    if (is_closed(eid)){
        return 3;
    }
    return 1;
}

int get_event_sold(string eid){
    ifstream sold;
    sold.open(event_total_res_path(eid));
    string sold_str;
    if (!(sold  >> sold_str)) {
        cout << "Could not get sold\n";
        return -1;
    } 
    sold.close();
    return stoi(sold_str);
}
bool is_event(fs::path event_dir){
    return fs::exists(event_dir/"details");
}

int set_event_sold(string eid, int tickets){
    ofstream sold;
    sold.open(event_total_res_path(eid));
    if (!(sold  << tickets)) {
        cout << "Could not get sold\n";
        return -1;
    } 
    sold.close();
    return 0;
}