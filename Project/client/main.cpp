#include "../common/tcp.hpp"
#include "udp.hpp"
#include <filesystem> 
#include <fstream>
#include <iostream>
#include <ctime>
#define GN 74
#define DEFAULT_LOCALHOST "127.0.0.1"
namespace fs = std::filesystem;


int login(ClientState& st, string uid, string password){
    string login_msg = "LIN " + uid + " " + password + "\n";
    char response[10];

    if (send_command_udp(st, login_msg, response)) {
        return -1;
    }
    if (strncmp(response, "RLI OK", 6) == 0){
        st.logged_in = true;
        st.password = password;
        st.username = uid;
        return 0;
    }else if (strncmp(response, "RLI NOK", 7) == 0){
        return 2;
    } else if (strncmp(response, "RLI REG", 7) == 0){
        st.logged_in = true;
        st.password = password;
        st.username = uid;
        return 1;
    }
    else{
        return -2;
    }
    return 0; 
}

// returns negative if failure, returns 0 on success, returns 1 on first login, 2 on LOGIN not ok
int logout(ClientState& st){
    string logout_msg = "LOU " + st.username + " " + st.password + "\n";
    char response[7];

    if (send_command_udp(st, logout_msg, response)) {
        return -1;
    }
    if (strncmp(response, "RLO OK", 6) ==0){
        st.logged_in = false;
        return 0;
    } else if (strncmp(response, "RLO UNR", 7) ==0){
        return 1;
    }else if (strncmp(response, "RLO NOK", 7) == 0){
        return 2;
    } else if (strncmp(response, "RLO WRP", 7) == 0){
        return 3;
    }
    else{
        return -2;
    }
    return 0;
}

//-1 if error receiving. 0 if it worked, 1 if not registerd, 2 if NOK, 3 if wrong password
int unregister(ClientState& st){
    string unregister_msg = "UNR " + st.username + " " + st.password + "\n";
    char response[10];
    
    if (send_command_udp(st, unregister_msg, response)) {
        cout << "Couldn't send message\n";
        return -1;
    }
    if (strncmp(response, "RUR OK", 6) == 0){
        return 0;
    } else if (strncmp(response, "RUR UNR", 7) == 0){
        return 1;
    } else if (strncmp(response, "RUR NOK", 7) == 0){
        return 2;
    } else if (strncmp(response, "RUR WRP", 7) == 0){
        return 3;
    } else{
        return -2;
    }
    return 0;
}

int changePass(ClientState& st, string old_pass, string new_pass) {
    string changePass_msg = "CPS " + st.username + " " + old_pass + " " + new_pass + "\n";
    string response;

    if (send_command_tcp(st, changePass_msg, response)) {
        cout << "Couldn't send message\n";
        return -1;
    }

    if (response.starts_with("RCP OK\n")){
        st.password = new_pass;
        return 0;
    } else if (response.starts_with("RCP NLG\n")){
        return 1;
    } else if (response.starts_with("RCP NOK\n")){
        return 2;
    } else if (response.starts_with("RCP NID\n")){
        return 3;
    } else{
        return -2;
    }
    return 0;
}

int close(ClientState& st, string event) {
    string changePass_msg = "CLS " + st.username + " " + st.password + " " + event  + "\n";
    string response;
    if (send_command_tcp(st, changePass_msg, response)) {
        cout << "Couldn't send message\n";
        return -1;
    }

    if (response.starts_with("RCL OK\n")){
        return 0;
    } else if (response.starts_with("RCL NLG\n")){
        return 1;
    } else if (response.starts_with("RCL NOK\n")){
        return 2;
    } else if (response.starts_with("RCL CLO\n")){
        return 3;
    } else if (response.starts_with("RCL NOE\n")){
        return 4;
    } else if (response.starts_with("RCL EOW\n")){
        return 5;
    } else if (response.starts_with("RCL SLD\n")){
        return 6;
    } else if (response.starts_with("RCL PST\n")){
        return 7;
    }
     else{
        return -2;
    }
    return 0;
}

int list(ClientState& st, string& response){
    string server_response;
    int retval = send_read_lst(st, server_response);
    if(retval != 0){
        return -1;
    }
    response = server_response;
    if(response.starts_with("RLS OK")){
        response = server_response.substr(6);
        return 0;
    }else if (response.starts_with("RLS NOK")){
        response = server_response.substr(7);
        return 1;
    } else {
        return -2;
    }
    return 0;
}

int create_event(ClientState& st, string name, string fname, string date, string hour, string num_people, string &send_eid) {
    fs::path file_path(fname);
    if (!fs::exists(file_path)) {
        cout << "Ficheiro não existe\n";
        return -1;
    }
    size_t file_size = fs::file_size(file_path);

    string header = "CRE " + st.username + ' ' + st.password + ' ' + name + ' ' + date + ' ' 
                    + hour + ' ' + num_people + ' ' + fname + ' ' + to_string(file_size) + ' ';

    string response;
    if(start_tcp(st)){
        return -1;
    }
    if (write_tcp(st, header)) {
        close_tcp(st);
        return -1;
    }

    if (send_file(st.tcp_fd, file_path)) {
        close_tcp(st);
        return -1;
    }
    
    if (read_tcp(st, response)) {
        close_tcp(st);
        return -1;
    }

    close_tcp(st);

    stringstream ss(response);
    string rce, status, eid;
    if (!(ss >> rce >> status >> eid) || rce != "RCE" || parse_eid(eid)) {
        return -2;
    }
    send_eid = eid;
    if (status == "OK"){
        return 0;
    } else if (status == "NLG"){
        return 1;
    } else if (status == "NOK"){
        return 2;
    } else if (status == "WRP"){
        return 3;
    } else{
        return -2;
    }
}

int myevents(ClientState&st, string& response){
    char raw_response[RESPONSE_SIZE_MYEVENTS];
    memset(raw_response,0,sizeof(raw_response));
    string myevents_message = "LME " + st.username + " " + st.password + "\n";
    send_command_udp_myevents(st, myevents_message, raw_response);
    response = raw_response;
    if (strncmp(raw_response, "RME OK", 6) == 0){
        return 0;
    } else if (strncmp(raw_response, "RME NLG", 7) == 0){
        return 1;
    } else if (strncmp(raw_response, "RME NOK", 7) == 0){
        return 2;
    } else if (strncmp(raw_response, "RME WRP", 7) == 0){
        return 3;
    } else{
        return -2;
    }
    return 0; 
}

int myreservations(ClientState& st, string& response){
    char raw_response[RESPONSE_SIZE_MYRESERVATIONS];
    memset(raw_response,0,RESPONSE_SIZE_MYRESERVATIONS);
    string myevents_message = "LMR " + st.username + " " + st.password + "\n";
    send_command_udp_myreservations(st,myevents_message,raw_response);
    response = raw_response;
    if (strncmp(raw_response, "RMR OK", 6) == 0){
        return 0;
    } else if (strncmp(raw_response, "RMR NLG", 7) == 0){
        return 1;
    } else if (strncmp(raw_response, "RMR NOK", 7) == 0){
        return 2;
    } else if (strncmp(raw_response, "RMR WRP", 7) == 0){
        return 3;
    } else{
        return -2;
    }
    return 0;
}

int reserve(ClientState& st, string eid, string value, string& n_seats) {
    string reserve_msg = "RID " + st.username + " " + st.password + " " + eid + " " + value + "\n";
    string response;
    
    if (send_command_tcp(st, reserve_msg, response)) {
        cout << "Couldn't send message\n";
        return -1;
    }

    if (response.starts_with("RRI ACC\n")){
        return 0;
    } else if (response.starts_with("RRI NLG\n")){
        return 1;
    } else if (response.starts_with("RRI NOK\n")){
        return 2;
    } else if (response.starts_with("RRI CLS\n")){
        return 3;
    } else if (response.starts_with("RRI SLD\n")){
        return 4;
    } else if (response.starts_with("RRI REJ")){
        string seats = response.substr(response.find("RRI REJ ") + 8);
        seats.pop_back();
        if (parse_value_reserved(seats)) {
            cout << "Invalid response from server\n";
        }
        n_seats = seats;
        return 5;
    } else if (response.starts_with("RRI PST")){
        return 6;
    } else if (response.starts_with("RRI WRP")){
        return 7;
    } else{
        return -2;
    }
    return 0;
}

int show(ClientState& st, string eid, vector<string>& header_sent) {
    string show_msg = "SED " + eid + "\n";
    string response;

    if (start_tcp(st)) {
        return -1;
    }
    if (write_tcp(st, show_msg)) {
        close_tcp(st);
        return -1;
    }
    if (read_tcp(st, response)) {
        close_tcp(st);
        return -1;
    }
    if (response.starts_with("RSE NOK")) {
        close_tcp(st);
        return 1;
    }

    if (!response.starts_with("RSE OK ")) {
        close_tcp(st);
        cout << "Invalid response from server\n";
        return -1;
    }

    string rse, status;
    string uid, name, date, hour;
    string attendance_size, seats_reserved, fname, fsize_str;

    stringstream ss(response);
    if (!(ss >> rse >> status >> uid >> name >> date >> hour >> attendance_size >> seats_reserved >> fname >> fsize_str)) {
        close_tcp(st);
        cout << "Invalid response format from server\n";
        return -1;
    }
    if (parse_uid(uid) || parse_name(name) || parse_date(date) || parse_hour(hour) || parse_attendance(attendance_size) 
        || parse_seats_reserved(seats_reserved) || parse_filename(fname) || parse_filesize(fsize_str)) {
        close_tcp(st);
        cout << "Invalid response from server\n";
        return -1;
    }
    
    string header = "RSE " + status + " " + uid + " " + name + " " + date + " " + hour
    + " " + attendance_size + " " + seats_reserved + " " + fname + " " + fsize_str + " ";
    header_sent.push_back(uid);
    header_sent.push_back(name);
    header_sent.push_back(date);
    header_sent.push_back(hour);
    header_sent.push_back(attendance_size);
    header_sent.push_back(seats_reserved);
    header_sent.push_back(fname);
    header_sent.push_back(fsize_str);

    size_t fsize = stoi(fsize_str);
    string rest = response.substr(header.size()); // Pedaço que sobrou no primeiro read feito e pertence a Fdata
    ofstream ofs(fname, ios::binary);
    if (!ofs.is_open()) {
        cout << "Não foi possível abrir o ficheiro\n";
        close_tcp(st);
        return -1;
    }

    size_t already_have = rest.size();
    if (already_have > fsize) {
        already_have = fsize; // Ou então return -1?
    }
    if (already_have > 0) {
        ofs.write(rest.data(), already_have);
    }

    int remaining = fsize - already_have;
    while (remaining > 0) {
        string buffer;
        if (read_tcp(st, buffer)) {
            cout << "Erro a ler resposta do servidor\n";
            close_tcp(st);
            return -1;
        }
        int n = buffer.size();
        remaining -= n;
        if (remaining < 0) {
            buffer.pop_back();
            n--;
        }
        ofs.write(buffer.c_str(), n);
    }

    ofs.close();
    close_tcp(st);
    return 0;
}

void handle_login(ClientState& st, vector<string>& args) {
    if (args.size() != 3) {
        cout << "Invalid input. Try again\n";
        return;
    }
    string uid = args[1];
    string password = args[2];

    if (st.logged_in){
        cout << "Already logged in as " << st.username << ". Please logout first" << endl;
        return;
    }

    if (parse_login_logout(uid, password)) {
        cout << "Invalid input. Try again\n";
        return;
    }
    int retval = login(st, uid, password);
    if (retval < 0) { 
        cout << "Login error (RLI ERR)\n";
    } else if (retval == 0){
        cout << "Login successful (RLI OK)\n";
    } else if (retval == 1){
        cout << "New user registered - Login successful (RLI REG)\n";
    } else if (retval == 2){
        cout << "Login error - not logged in or wrong password (RLI NOK) \n";
    }
}

void handle_logout(ClientState& st, vector<string>& args) {
    if (args.size() != 1) {
        cout << "Invalid input. Try again\n";
        return;
    }
    int retval=logout(st);
    if (retval < 0) {
        cout << "Logout error (RLO ERR)\n";
    } else if (retval == 0){
        cout << "Logout user " << st.username << " successful (RLO OK)\n";
        st.logged_in=false;
        st.username = "";
        st.password = "";
    }else if (retval == 1){
        cout << "Logout error - User not registered (RLO UNR)\n";
    }else if (retval == 2){
        cout << "Logout error - User not logged in (RLO NOK)\n";
    }
    else if (retval == 3){
        cout << "Logout error - Wrong password (RLO WRP)\n";
    }
}

void handle_unregister(ClientState& st, vector<string>& args) {
    if (args.size() != 1) {
        cout << "Invalid input. Try again\n";
        return;
    }

    int retval = unregister(st);
    if(retval < 0){
        cout << "Unregister error (RUR ERR)\n";
    } else if (retval == 0){
        cout<< "Unregister (and logout) user " << st.username << " successful (RUR OK)\n";
        st.logged_in = false;
        st.username = "";
        st.password = "";
    } else if (retval == 1){
        cout<< "Unregister error - User not registered (RUR UNR)\n";
    } else if (retval == 2){
        cout<< "Unresgister error - User not logged in (RUR NOK)\n";
    } else if (retval == 3){
        cout<< "Unregister error - Wrong password (RUR WRP)\n";
    }
}

void handle_changePass(ClientState& st, vector<string>& args) {
    if (args.size() != 3) {
        cout << "Invalid input. Try again\n";
        return;
    }
    string old_pass = args[1];
    string new_pass = args[2];

    if (parse_password(old_pass) || parse_password(new_pass)) {
        cout << "Invalid input. Try again\n";
        return;
    }

    int retval = changePass(st, old_pass, new_pass);

    if(retval<0){
        cout << "ChangePass error (RCP ERR)\n";
    } else if (retval == 0){
        cout<< "ChangePass user " << st.username << " successful (RCP OK)\n";
    } else if (retval == 1){
        cout<< "ChangePass error - User not logged in (RCP NLG)\n";
    } else if (retval == 2){
        cout<< "ChangePass error - Wrong password (RCP NOK)\n";
    } else if (retval == 3){
        cout<< "ChangePass error - User not registered (RCP NID)\n";
    }
}

void handle_list(ClientState& st, vector<string>&args){
    if (args.size() != 1) {
        cout << "Invalid input. Try again\n";
        return;
    }
    string response;
    int retval = list(st,response);
    
    if (retval == 0){
        const string spaces_3 = "   ";
        const string spaces_6 = "      ";
        const string spaces_8 = "        ";
        const string spaces_9 = "         ";
        cout << spaces_6 << "\nEvent list follows: (RLS OK)\n\n";
        cout << spaces_3 << "EID" << spaces_6 << "name" << spaces_9
        << "event date" << spaces_8 << "state\n\n";
        stringstream ss(response);
        string eid, name, state, date, hour;

        while((ss >> eid >> name >> state >> date >> hour)){
            if (parse_eid(eid) || parse_name(name) || parse_date(date) || parse_hour(hour)) {
                cout << "Bad server response\n";
                return;
            }
            
            string state_str;
            if (state == "0") {
                state_str = "0: Past event (closed)";
            } else if (state == "1") {
                state_str = "1: Accepting reservations";
            } else if (state == "2") {
                state_str = "2: To happen - sold out";
            } else if (state == "3") {
                state_str = "3: Closed by host";
            }

            cout << left << spaces_3 << setw(6) << eid << setw(13) << name << setw(20)
            << (date + " " + hour) << state_str << "\n";
        }
        cout << "\n";

        if (!ss.eof()) {
            cout << "Bad server response\n";
            return;
        }

    } else if(retval == 1){
        cout << "List error - No events (RLS NOK)\n";

    } else{
        cout << "List error (RLS ERR)\n";
    }
}

void handle_close(ClientState& st, vector<string>&args){
    if (args.size() != 2) {
        cout << "Invalid input. Try again\n";
        return;
    }
    string event = args[1];
    if (parse_eid(event)){
        cout << "Invalid input. Try again\n";
    }
    int retval = close(st,event);
    
    if (retval < 0) {
        cout << "Close error (RCL ERR)\n";
    }else if (retval==0){
        cout<< "Close successful (RCL OK)\n" ;
    } else if(retval == 1){
        cout << "Close error - User not logged in (RCL NLG)\n";
    }else if(retval == 2){
        cout << "Close error - User is not registered (RCL NOK)\n";
    }else if(retval == 3){
        cout << "Close error - Already closed (RCL CLO)\n";
    }else if(retval == 4){
        cout << "Close error - Unknown event (RCL NOE)\n";
    }else if(retval == 5){
        cout << "Close error - Event not created by user " << st.username << " (RCL EOW)\n";
    }else if(retval == 6){
        cout << "Close error - Event sold out (RCL SLD)\n";
    }else if(retval == 7){
        cout << "Close error - Event already happened (RCL PST)\n";
    }
}

void handle_myevents(ClientState& st, vector<string>&args){
    if (args.size() != 1) {
        cout << "Invalid input. Try again\n";
        return;
    }
    string response;
    int retval = myevents(st,response);
    if (retval == 0){
        cout << "\nList of events create by user " << st.username << " (RME OK):\n\n";
        response.erase(0,response.find("RME OK ")+7);
        string bit;
        vector<string> events;
        stringstream ss(response);
        const string spaces_7 = "       ";
        while (ss >> bit) {
            events.push_back(bit);
        }
        if (events.size() % 2 != 0) {
            cout << "Invalid response from server\n";
            return;
        }
        for(size_t i=0; i < events.size(); i+=2){
            if(events[i].length()!=3){
                cout << "Invalid response from server\n";
                return;
            }
            
            if(events[i+1].length()!=1){
                cout << "Invalid response from server\n";
                return;
            }

            cout << spaces_7 << events[i] << " - ";
            if(events[i+1] == "0"){
                cout << "Past event (closed)\n";
            }
            else if(events[i+1] == "1"){
                cout << "Accepting reservations\n";
            }
            else if(events[i+1] == "2"){
                cout << "To happen - Sold out\n";
            }
            else if(events[i+1] == "3"){
                cout << "Closed by host\n";
            }

            else {
                cout << "Invalid response from server\n";
                return;
            }
        }
        cout << "\n";
        return;
    } else if(retval == 1){
        cout << "MyEvents error - User not logged in (RME NLG)\n";
    }else if(retval == 2){
        cout << "MyEvents error - No events from user " << st.username << " (RME NOK)\n";
    }else if(retval == 3){
        cout << "MyEvents error - Wrong password (RME WRP)\n";
    }else{
        cout << "MyEvents error (RME ERR)\n";
    }
}

void handle_create(ClientState& st, vector<string>&args){
    int retval;
    if (args.size() != 6) {
        cout << "Invalid input. Try again\n";
        return;
    }
    string eid;
    string name = args[1];
    string fname = args[2];
    string date = args[3];
    string hour = args[4];
    string attendees = args[5];

    if (parse_name(name) || parse_filename(fname) || parse_date(date) ||
    parse_hour(hour) || parse_attendance(attendees)) {
        cout << "Invalid input. Try again\n";
        return;  
    }

    retval = create_event(st, name, fname, date, hour, attendees, eid);

    if (retval < 0) {
        cout << "Create error (RCE ERR)\n";
    } else if (retval == 0){
        cout<< "Create successful - EID: " << eid << " (RCE OK)\n";
    } else if (retval == 1){
        cout<< "Create error - User not logged in (RCE NLG)\n";
    } else if (retval == 2){
        cout<< "Create error - Event list full (RCE NOK)\n";
    } else if (retval==3){
        cout<< "Create error - Wrong password (RCE WRP)\n";
    }
}

void handle_myreservations(ClientState& st, vector<string>&args) {
    int retval;
    if (args.size() != 1) {
        cout << "Invalid input. Try again\n";
        return;
    }
    string response;
    retval = myreservations(st,response);
    if (retval == 0){
        cout << "\nList of reservations by user " << st.username << " (RMR OK)\n\n";
        response.erase(0,response.find("RMR OK ")+7);
        string bit;
        vector<string> reservations;
        stringstream ss(response);
        const string spaces_3 = "   ";
        const string spaces_6 = "      ";
        const string spaces_9 = "         ";
        cout << spaces_6 << "Event" << spaces_3 << "My seats" << spaces_9 << "Reservation date\n\n";
        while (ss >> bit) {
            reservations.push_back(bit);
        }
        if (reservations.size() % 4 != 0 || reservations.size()/4 > 50) {
            cout << "Invalid response from server\n";
            return;
        }
        for(size_t i=0; i < reservations.size(); i+=4){
            if(parse_eid(reservations[i]) || parse_date(reservations[i+1]) || parse_hour_with_seconds(reservations[i+2])
            || parse_value_reserved(reservations[i+3])){
                cout << "Invalid response from server\n";
                return;
            }
            int reservation_n = i/4 + 1;
            cout << setw(2) << reservation_n << ":    " << left << setw(8) << reservations[i] << right<< setw(4) <<
            reservations[i+3] << spaces_9 << reservations[i+1]<<" "<< reservations[i+2] << "\n";
        }
        cout << "\n";
        return;
    }
    else if(retval == 1){
        cout << "MyReservations error - User not logged in (RMR NLG)\n";
    }else if(retval == 2){
        cout << "MyReservations error - No reservations from user " << st.username << " (RMR NOK)\n";
    }else if(retval == 3){
        cout << "MyReservations error - Wrong password (RMR WRP)\n";
    }else{
        cout << "MyReservations error (RMR ERR)\n";
    }
}

void handle_reserve(ClientState& st, vector<string>&args) {
    int retval;
    if (args.size() != 3) {
        cout << "Invalid input. Try again\n";
        return;
    }

    string eid = args[1];
    string value = args[2];

    if (parse_name(eid)) {
        cout << "Invalid eid. Try again\n";
        return;
    }
    if (parse_filename(value)) {
        cout << "Invalid value. Try again\n";
        return;
    }

    string n_seats;
    retval = reserve(st, eid, value, n_seats);

    if (retval < 0) {
        cout << "Reserve error (RRI ERR)\n";
    } else if (retval == 0){
        cout << "Reserve successful (RRI OK)\n";
    } else if (retval == 1){
        cout << "Reserve error - User not logged in (RRI NLG)\n";
    } else if (retval == 2){
        cout << "Reserve error - Unknown event (RRI NOK)\n";
    } else if (retval == 3){
        cout << "Reserve error - Event closed (RRI CLS)\n";
    } else if (retval == 4){
        cout << "Reserve error - Event sold out (RRI SLD)\n";
    } else if (retval == 5){
        cout << "Reserve error - Too many people requested. Available seats: " << n_seats << " (RRI REJ)\n";
    } else if (retval == 6){
        cout << "Reserve error - Event already happened (RRI PST)\n";
    } else if (retval == 7){
        cout << "Reserve error - Wrong password (RRI WRP)\n";
    }
}

void handle_show(ClientState& st, vector<string>&args) {
    int retval;

    if (args.size() != 2) {
        cout << "Invalid input. Try again\n";
        return;
    }

    string eid = args[1];

    if (parse_name(eid)) {
        cout << "Invalid eid. Try again\n";
        return;
    }
    vector<string> response;
    retval = show(st, eid,response);

    if (retval < 0) {
        cout << "Incorrect show attempt\n";
        return;
    } else if (retval == 0){
        bool past = is_event_past(response[2], response[3]);
        int capacity = stoi(response[4]);
        int reserved = stoi(response[5]);
        bool sold_out = (reserved >= capacity);
        cout << "Show successful (RSE OK)\n";
        cout << "Event "<< response[1] << " created by " << response[0] << ".\n";
        if (past){
            cout << "The event has already happened.\n";
        } else {
            cout << "The event is scheduled for: " << response[2] << " " << response[3] << "\n";
        }
        if(sold_out){
            cout << "The event is sold out.\n";
        } else {
            cout << "The event has " << capacity - reserved << " tickets available.\n";
        }
        if (past || sold_out) {
            cout << "The event is currently closed.\n";
        } else {
            cout << "The event is currently open.\n";
        }

        fs::path dir = fs::current_path();
        cout << "Stored in directory: " << dir.string() << "\n";
        cout << "File name: " << response[6] << ", size: " << response[7] << " bytes\n";

    } else {
        cout << "Something went wrong, (RSE NOK)\n";
    }
}

int parser(ClientState& st){
    while(true){
        string command, arg;
        vector<string> args;

        if (st.logged_in) {
            cout << "User [" << st.username << "] > ";
        }
        else {
            cout << "User > ";
        }

        getline(cin, command);
        stringstream ss(command);
        while (ss >> arg) {
            args.push_back(arg);
        }
        
        if (args.size() > 0) {

            if (args[0] == "exit") {
                if (st.logged_in) {
                    cout << "You need to logout first\n";
                    continue;
                }
                cout << "Closing programm" << "\n";
                return 0;
            }

            if (args[0] == "login") {
                handle_login(st, args);
            }
            else if (args[0] == "list"){
                handle_list(st, args);
            }
            else if (args[0] == "show") {
                handle_show(st, args);
            }

            else if (args[0] == "changePass") {
                if (!st.logged_in) {
                    cout << "User is not logged in\n";
                    continue;
                }
                handle_changePass(st, args);
            }
            else if (args[0] == "unregister") {
                if (!st.logged_in) {
                    cout << "User is not logged in\n";
                    continue;
                }
                handle_unregister(st, args);
            }
            else if (args[0] == "logout") {
                if (!st.logged_in) {
                    cout << "User is not logged in\n";
                    continue;
                }
                handle_logout(st, args);
            }
            else if (args[0] == "create"){
                if (!st.logged_in) {
                    cout << "User is not logged in\n";
                    continue;
                }
                handle_create(st, args);
            }
            else if (args[0] == "close") {
                if (!st.logged_in) {
                    cout << "User is not logged in\n";
                    continue;
                }
                handle_close(st, args);
            }
            else if (args[0] == "myevents" || args[0] == "mye"){
                if (!st.logged_in) {
                    cout << "User is not logged in\n";
                    continue;
                }
                handle_myevents(st, args);
            }
            else if (args[0] == "reserve"){
                if (!st.logged_in) {
                    cout << "User is not logged in\n";
                    continue;
                }
                handle_reserve(st, args);
            }
            else if (args[0] == "myreservations" || args[0] == "myr"){
                if (!st.logged_in) {
                    cout << "User is not logged in\n";
                    continue;
                }
                handle_myreservations(st, args);
            }
            else {
                cout << "No command found\n";
            }
        }
        else {
            return 0;
        }
    }
    return 0;
}

int main(int argc, char *argv[]){
    ClientState client_state;
    char domain[100]; // Default localhost
    snprintf(domain, sizeof(domain), "%s",DEFAULT_LOCALHOST);
    int port = 58000 + GN; //port

    if (argc > 5) {
        cout << "Too many arguments\n";
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-n" && i+1 < argc) {
            snprintf(domain, sizeof(domain), "%s",argv[++i]);
        }
        else if (arg == "-p" && i+1 < argc) {
            port = stoi(argv[++i]);
        }
    }
    char s_port[10];
    snprintf(s_port, sizeof(s_port), "%d", port);
    cout << "domain: " << domain << "\n";
    cout << "port: " << port << "\n";

    client_state.domain = domain;
    client_state.port = s_port;

    if (start_udp(client_state, domain, s_port)) {
        return 1;
    }

    parser(client_state);
    freeaddrinfo(client_state.server_addr_udp);
    close(client_state.udp_fd);
}