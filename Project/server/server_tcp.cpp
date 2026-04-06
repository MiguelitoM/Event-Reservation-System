#include "server_tcp.hpp"

int parse_client_msg_tcp(int fd, string msg, int verbose, const sockaddr_in& addr){
    int ret_val = -1;
    string arg;
    vector<string> args;
    stringstream ss(msg);
    while (ss >> arg) {
        args.push_back(arg);
    }

    if (args.size() <= 0) {
        answer_ERR(fd, verbose, addr);
        return -1;
    }

    if (args[0] == "CLS"){
        if (args.size() != 4) {
            answer_close(CloseResult::RCL_ERR, fd, verbose, addr);
            return -1;
        }
        string uid = args[1];
        string password = args[2];
        string eid = args[3];
        if (parse_uid(uid) || parse_password(password) || parse_eid(eid)) {
            answer_close(CloseResult::RCL_ERR, fd, verbose, addr);
            return -1;
        }
        ret_val = close_event(uid, password, eid);
        answer_close(ret_val, fd, verbose, addr, uid);

    } else if (args[0] == "RID") {
        int remaining = 0;
        if (args.size() != 5) {
            answer_reserve(ReserveResult::RRI_ERR, fd, remaining, verbose, addr);
            return -1;
        }
        string uid = args[1];
        string password = args[2];
        string eid = args[3];
        string number_of_seats_str = args[4];
        if (parse_uid(uid) || parse_password(password) || parse_eid(eid) || parse_reserve_number(number_of_seats_str)) {
            answer_reserve(ReserveResult::RRI_ERR, fd, remaining, verbose, addr);
            return -1;
        }
        int number_of_seats = stoi(number_of_seats_str);
        ret_val = reserve(uid, password, eid, number_of_seats, remaining);
        answer_reserve(ret_val, fd, remaining, verbose, addr, uid);

    } else if (args[0] == "CPS") {
        if (args.size() != 4) {
            answer_changePass(ChangePassResult::RCP_ERR, fd, verbose, addr);
            return -1;
        }
        string uid = args[1];
        string old_password = args[2];
        string new_password = args[3];
        if (parse_uid(uid) || parse_password(old_password) || parse_password(new_password)) {
            answer_changePass(ChangePassResult::RCP_ERR, fd, verbose, addr);
            return -1;
        }
        ret_val = changePass(uid, old_password, new_password);
        answer_changePass(ret_val, fd, verbose, addr, uid);
    } else if (args[0] == "LST") {
        if (args.size() != 1){
            string response = "RLS ERR\n";
            if (verbose) {
                log_answer("RLS ERR", addr);
            }
            safe_write(fd, response);
            return -1;
        }
        list(fd, verbose, addr); // already answers client

    } else if (args[0] == "SED") {
        string err = "RSE ERR\n";
        if (args.size() != 2) {
            if (verbose) {
                log_answer("RSE ERR", addr);
            }
            safe_write(fd, err);
            return -1;
        }
        string eid = args[1];
        if (parse_eid(eid)) {
            if (verbose) {
                log_answer("RSE ERR", addr);
            }
            safe_write(fd, err);
            return -1;
        }

        show(fd, eid, verbose, addr); // already answers client
    }
    else if (args[0] == "CRE"){
        CreState empty_state;
        answer_create(CreateResult::CRE_ERR, fd, empty_state, verbose, addr);
    }
    else{
        answer_ERR(fd, verbose, addr);
        return -1;
    }
    return ret_val;
}

// 1 caso não exista um header válido de create
// 0 caso exista um header válido
// -1 caso exista um erro -> é para fechar a sessão tcp
int try_to_parse_create(int fd, string& msg, CreState &state, int verbose, const sockaddr_in& addr){
    string cre;
    string uid, password, name, date, hour;
    string attendance_size, seats_reserved, fname, fsize_str;
    stringstream ss(msg);
    if (!(ss >> cre >> uid >> password>>name >> date >> hour >> attendance_size >> fname >> fsize_str)) {
        return 1;
    } else if (cre != "CRE" ) {
        return 1;
    } else if (parse_uid(uid) || parse_password(password) || parse_name(name) || parse_date(date) || parse_hour(hour) ||
        parse_attendance(attendance_size) || parse_filename(fname) || parse_filesize(fsize_str)){
        answer_create(CreateResult::CRE_ERR, fd, state,  verbose, addr);
        return -1;
    }
    
    state.header_complete = true;
    state.username = uid;
    state.password = password;
    state.name = name;
    state.date = date;
    state.hour = hour;
    state.attendance_size = stoi(attendance_size);
    state.filename = fname;
    state.filesize = stoi(fsize_str);

    string header = "CRE " + uid + ' ' + password + ' ' + name + ' ' + date + ' ' 
            + hour + ' ' + attendance_size + ' ' + fname + ' ' + fsize_str + ' ';

    string rest = msg.substr(header.size()); // Pedaço que sobrou no primeiro read feito e pertence a Fdata

    int retval = start_create_event(state);
    if(retval!=CreateResult::CRE_OK){
        answer_create(retval, fd, state, verbose, addr);
        return -1;
    }
    retval = write_event(state, rest.c_str(), rest.size());
    if (retval == -1) {
        answer_create(CreateResult::CRE_NOK, fd, state, verbose, addr);
        return -1;
    }
    if (retval == 1) {
        answer_create(CreateResult::CRE_OK, fd, state, verbose, addr);
        return -1;
    }

    return 0;
}

int start_create_event(CreState& state) {
    if(!does_user_exist(state.username)){
        return CreateResult::CRE_NOK;
    }
    if(!is_logged_in(state.username)){
        return CreateResult::CRE_NLG;
    }
    if(!check_password(state.username,state.password)){
        return CreateResult::CRE_WRP;
    }
    state.eid = create_event_dirs();
    if (state.eid == ""){
        return CreateResult::CRE_NOK;
    }
    ofstream event_details(event_details_path(state.eid));
    event_details << state.username + " " + state.password + " " + state.name + " " + state.date + " " + state.hour + " " + to_string(state.attendance_size) + " " + state.filename + " " + to_string(state.filesize) + " ";
    event_details.close();
    ofstream user_file (user_events_dir(state.username)/state.eid);
    user_file.close();
    state.description.open(event_description_dir(state.eid) / state.filename, ios::binary);
    if (!state.description.is_open()) {
        return CreateResult::CRE_ERR;
    }

    return CreateResult::CRE_OK;
}

int write_event(CreState &state, const char* msg, int amount_to_write){
    int end_command = 0;

    if (!state.description.is_open()) {
        return -1;
    }

    size_t already_written = state.bytes_written;
    if (already_written + amount_to_write > state.filesize + 1) {
        destroy_event(state.eid);
        state.description.close();
        return -1;
    }

    if (already_written + amount_to_write == state.filesize + 1) {
        if (msg[amount_to_write-1] != '\n') {
            destroy_event(state.eid);
            state.description.close();
            return -1;
        }
        amount_to_write--;
        end_command = 1;
    }

    state.description.write(msg, amount_to_write);
    state.bytes_written += amount_to_write;
    if (end_command) {
        state.description.close();
    }
    return end_command;
}

int close_event(string uid, string password, string eid){
    if(!does_user_exist(uid)||!check_password(uid,password)){
        return CloseResult::RCL_NOK;
    }
    if(!is_logged_in(uid)){
        return CloseResult::RCL_NLG;
    }
    if(!does_event_exist(eid)){
        return CloseResult::RCL_NOE;
    }
    if(get_event_creator(eid) != uid){
        return CloseResult::RCL_EOW;        
    }
    string date = get_event_date(eid);
    string hour = get_event_hour(eid);
    if(is_event_past(date, hour)) {
        return CloseResult::RCL_PST;
    }
    if(is_closed(eid)){
        return CloseResult::RCL_CLO;
    }
    if(get_event_attendance(eid) - get_event_sold(eid) == 0){
        return CloseResult::RCL_SLD;
    }
    ofstream close_event_file;
    close_event_file.open(event_dir(eid)/"end");
    close_event_file.close();
    return CloseResult::RCL_OK;
}

int list(int fd, int verbose, const sockaddr_in& addr){
    bool has_any_events=false;
    string response;
    set<fs::path> sorted_by_name;
    for (auto &entry : fs::directory_iterator(EVENT_PATH)){
        if(is_event(entry.path())){
            sorted_by_name.insert(entry.path());
            has_any_events=true;
        }
    }
    if(!has_any_events){
        response = "RLS NOK\n";
        if (verbose) {
            log_answer("RLS NOK", addr);
        }
        if (safe_write(fd, response)) {
            return -1;
        }

        return 0;
    }
    //--- print the files sorted by filename
    response = "RLS OK";
    if (safe_write(fd,response)) {
        return -1;
    }
    for (auto &filename : sorted_by_name){
        string eid = filename.string().substr(7);
        string name = get_event_name(eid);
        int state = get_event_state(eid);
        string date = get_event_date(eid);
        string hour = get_event_hour(eid);
        response = " " + eid + " " + name + " " + to_string(state) + " " + date + " " + hour;
        if (safe_write(fd,response)) {
            return -1;
        }
    }
    string the_end = string("\n");
    if (safe_write(fd, the_end)) {
        return -1;
    }

    if (verbose) {
        log_answer("RLS OK", addr);
    }
    return 0;
}

int reserve(string uid, string password, string eid, int number_of_seats, int& remaining){
    if (!is_logged_in(uid)){
        return ReserveResult::RRI_NLG;
    }
    if (!check_password(uid, password)){
        return ReserveResult::RRI_WRP;
    }
    if (!does_event_exist(eid)){
        return ReserveResult::RRI_NOK;
    }
    if(is_closed(eid)){
        return ReserveResult::RRI_CLS;
    }
    int sold = get_event_sold(eid);
    remaining = get_event_attendance(eid) - sold;
    if(remaining == 0){
        return ReserveResult::RRI_SLD;
    }

    string date = get_event_date(eid);
    string hour = get_event_hour(eid);
    if(is_event_past(date, hour)) {
        return ReserveResult::RRI_PST;
    }
    if (number_of_seats > remaining){
        return ReserveResult::RRI_REJ;
    }
    set_event_sold(eid, sold + number_of_seats);

    char now_buffer[32];
    struct tm tm_now{};
    time_t t = time(nullptr);
    localtime_r(&t, &tm_now);
    strftime(now_buffer, sizeof(now_buffer),"%Y-%m-%d-%H-%M-%S", &tm_now);
    string now_str(now_buffer);

    string the_final_path_event = now_str + "-" + uid ;
    string the_final_path_user = now_str + "-" + eid ;
    ofstream reserve_user(user_reservations_dir(uid)/the_final_path_user, ios_base::app);
    ofstream reserve_event(event_reservations_dir(eid)/the_final_path_event, ios_base::app);
    reserve_user << number_of_seats << " ";
    reserve_event << number_of_seats << " ";
    reserve_event.close();
    reserve_user.close();

    return ReserveResult::RRI_ACC;
}

int changePass(string uid, string old_pass, string new_pass) {
    if (!does_user_exist(uid)) {
        return ChangePassResult::RCP_NID;
    }
    if (!is_logged_in(uid)) {
        return ChangePassResult::RCP_NLG;
    }
    if (!check_password(uid, old_pass)) {
        return ChangePassResult::RCP_NOK;
    }

    fs::path pass_path = password_path(uid);
    ofstream passwd(pass_path, ios::trunc);
    if (!passwd.is_open()) {
        return ChangePassResult::RCP_ERR;
    }
    passwd << new_pass;
    passwd.close();
    return ChangePassResult::RCP_OK;
}

int show(int fd, string eid, int verbose, const sockaddr_in& addr) {
    if (!does_event_exist(eid)) {
        string response = "RSE NOK\n";
        if (verbose) {
            response.pop_back();
            log_answer(response, addr);
        }
        safe_write(fd, response);
        return ShowResult::RSE_NOK;
    }

    string uid, password, name, date, hour;
    string attendance_size, seats_reserved, fname, fsize;
    ifstream ss(event_details_path(eid));
    if (!(ss  >> uid >> password >> name >> date >> hour >> attendance_size >> fname >> fsize)) {
        string response = "RSE NOK\n";
        safe_write(fd, response);
        if (verbose) {
            response.pop_back();
            log_answer(response, addr);
        }
        return ShowResult::RSE_NOK;
    } 
    ss.close();

    string reserved = to_string(get_event_sold(eid));
    string header = "RSE OK " + uid + " " + name + " " + date + " " + hour + " " + attendance_size +
                    " " + reserved + " " + fname + " " + fsize + " ";
    size_t fsize_number = stoi(fsize);
    if( !fs::exists(event_description_dir(eid)/fname) ||fs::file_size(event_description_path(eid))!=fsize_number){
        string response = "RSE NOK\n";
        safe_write(fd, response);
        if (verbose) {
            response.pop_back();
            log_answer(response, addr);
        }
        return ShowResult::RSE_NOK;
    }
    fs::path desc_path = event_description_path(eid);

    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }

    if (pid == 0){
        // Child process sends header+file to client

        if (safe_write(fd, header) || send_file(fd, desc_path)) {
            close(fd);
            if (verbose) {
                string response = "RSE NOK";
                log_answer(response, addr);
            }
            _exit(-1);
        }
        if (verbose) {
            string response = "RSE OK";
            log_answer(response, addr);
        }
        close(fd);
        _exit(0);
    }
    return 0;
}

int answer_changePass(int ret_val, int fd, int verbose, const sockaddr_in& addr, string uid) {
    string response;
    if (ret_val == ChangePassResult::RCP_ERR) {
        response = "RCP ERR\n";
    } else if (ret_val == ChangePassResult::RCP_OK) {
        response = "RCP OK\n";
    } else if (ret_val == ChangePassResult::RCP_NOK) {
        response = "RCP NOK\n";
    } else if (ret_val == ChangePassResult::RCP_NLG) {
        response = "RCP NLG\n";
    } else if (ret_val == ChangePassResult::RCP_NID) {
        response = "RCP NID\n";
    }

    if(safe_write(fd, response)){
        return -1;
    }

    if (verbose) {
        response.pop_back();
        log_answer(response, addr, uid);
    }
    return 0;
}

int answer_create(int ret_val, int fd, CreState& state, int verbose, const sockaddr_in& addr){
    string response;
    if (ret_val == CreateResult::CRE_ERR) {
        response = "RCE ERR\n";
    } else if (ret_val == CreateResult::CRE_OK) {
        response = "RCE OK "+ state.eid + "\n";
    }
    else if (ret_val == CreateResult::CRE_NLG) {
        response = "RCE NLG\n";
    }
    else if (ret_val == CreateResult::CRE_WRP) {
        response = "RCE WRP\n";
    }
    else if (ret_val == CreateResult::CRE_NOK) {
        response = "RCE NOK\n";
    }
    
    if(safe_write(fd, response)){
        return -1;
    }

    if (verbose) {
        response.pop_back();
        log_answer(response, addr, state.username);
    }
    return 0;
}

int answer_close(int ret_val, int fd, int verbose, const sockaddr_in& addr, string uid) {
    string response;
    
    if (ret_val == CloseResult::RCL_ERR) {
        response = "RCL ERR\n";
    } else if (ret_val == CloseResult::RCL_OK) {
        response = "RCL OK\n";
    } else if (ret_val == CloseResult::RCL_NOK) {
        response = "RCL NOK\n";
    } else if (ret_val == CloseResult::RCL_NLG) {
        response = "RCL NLG\n";
    } else if (ret_val == CloseResult::RCL_NOE) {
        response = "RCL NOE\n";
    } else if (ret_val == CloseResult::RCL_EOW) {
        response = "RCL EOW\n";
    } else if (ret_val == CloseResult::RCL_SLD) {
        response = "RCL SLD\n";
    } else if (ret_val == CloseResult::RCL_PST) {
        response = "RCL PST\n";
    } else if (ret_val == CloseResult::RCL_CLO) {
        response = "RCL CLO\n";
    }

    if(safe_write(fd, response)){
        return -1;
    }

    if (verbose) {
        response.pop_back();
        log_answer(response, addr, uid);
    }
    return 0;
}

int answer_reserve(int ret_val, int fd, int remaining, int verbose, const sockaddr_in& addr, string uid) {
    string response;
    
    if (ret_val == ReserveResult::RRI_ERR) {
        response = "RRI ERR\n";
    } else if (ret_val == ReserveResult::RRI_ACC) {
        response = "RRI ACC\n";
    } else if (ret_val == ReserveResult::RRI_NOK) {
        response = "RRI NOK\n";
    } else if (ret_val == ReserveResult::RRI_NLG) {
        response = "RRI NLG\n";
    } else if (ret_val == ReserveResult::RRI_CLS) {
        response = "RRI CLS\n";
    } else if (ret_val == ReserveResult::RRI_REJ) {
        response = "RRI REJ " + to_string(remaining) + "\n";
    } else if (ret_val == ReserveResult::RRI_SLD) {
        response = "RRI SLD\n";
    } else if (ret_val == ReserveResult::RRI_PST) {
        response = "RRI PST\n";
    } else if (ret_val == ReserveResult::RRI_WRP) {
        response = "RRI WRP\n";
    }

    if(safe_write(fd, response)){
        return -1;
    }

    if (verbose) {
        response.pop_back();
        log_answer(response, addr, uid);
    }
    return 0;
}

int answer_ERR(int fd, int verbose, const sockaddr_in& addr) {
    string message = "ERR\n";
    if (safe_write(fd,message)) {
        return 1;
    }
    if (verbose) {
        log_answer("ERR", addr);
    }
    return 0;
}

void disconnect_tcp(int fd, map<int, CreState> &states, fd_set& inputs, map<int, string>& tcp_buffers,
                    map<int, time_t>& connection_start, map<int, sockaddr_in>& tcp_clients){
    FD_CLR(fd, &inputs);
    close(fd);
    states.erase(fd);
    tcp_buffers.erase(fd);
    connection_start.erase(fd);
    tcp_clients.erase(fd);
}

bool time_out(int fd, map<int, time_t>& connection_start) {
    time_t now = time(nullptr);
    auto it = connection_start.find(fd);
    if (it != connection_start.end()) {
        if (now - it->second > SESSION_TIMEOUT) {
            return true;
        }
    }
    return false;
}

void log_tcp_request(const string& msg, const sockaddr_in& addr) {
    string cmd, pretty;
    string uid = "Unknown";
    stringstream ss(msg);
    ss >> cmd;

    bool has_uid = false;

    if (cmd == "CRE" || cmd == "CLS" || cmd == "RID" || cmd == "CPS"){
        has_uid = true;
    }

    if (cmd == "CRE") {
        pretty = "Create";
    } else if (cmd == "CLS") {
        pretty = "Close";
    } else if (cmd == "LST") {
        pretty = "List";
    } else if (cmd == "SED") {
        pretty = "Show";
    } else if (cmd == "RID") {
        pretty = "Reserve";
    } else if (cmd == "CPS") {
        pretty = "ChangePass";
    } else {
        pretty = "Unknown";
    }

    if (has_uid) {
        string maybe_uid;
        if (ss >> maybe_uid) {
            if (!parse_uid(maybe_uid)) {
                uid = maybe_uid;
            }
        }
    }

    cout << "Request: " << pretty << ", UID: " << uid << ", IP: " 
        << inet_ntoa(addr.sin_addr) << ", port: " 
        << ntohs(addr.sin_port) << "\n";
}

void log_answer(string answer, const sockaddr_in& addr, string uid) {
    if (uid == "") {
        uid = "Unknown";
    }

    cout << "Answer: " << answer << ", UID: " << uid << ", IP: " 
    << inet_ntoa(addr.sin_addr) << ", port: " 
    << ntohs(addr.sin_port) << "\n";
}