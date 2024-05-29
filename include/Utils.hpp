#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/process.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <string>
#include <vector>

namespace std {
ostream& operator<<(ostream& os, vector<string> vecs) {
    for (const auto& s : vecs)
        os << s << " ";
    return os;
}
} // namespace std

template <typename T>
std::vector<T> to_array(const std::string& s) {
    std::vector<T> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ','))
        result.push_back(boost::lexical_cast<T>(item));
    return result;
}

int get_requests(const std::string& host_, const std::string& port_, const std::string& url_path,
                 std::ostringstream& out_, std::vector<std::string>& headers,
                 unsigned int timeout) {
    try {
        using namespace boost::asio::ip;
        tcp::iostream request_stream;
        request_stream.connect(host_, port_);
        if (!request_stream) {
            return -1;
        }
        request_stream << "GET " << url_path << " HTTP/1.0\r\n";
        request_stream << "Host: " << host_ << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Cache-Control: no-cache\r\n";
        request_stream << "Connection: close\r\n\r\n";
        request_stream.flush();
        std::string line1;
        std::getline(request_stream, line1);
        if (!request_stream) {
            return -2;
        }
        std::stringstream response_stream(line1);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
            return -1;
        }
        if (status_code != 200) {
            return (int)status_code;
        }
        std::string header;
        while (std::getline(request_stream, header) && header != "\r")
            headers.push_back(header);
        out_ << request_stream.rdbuf();
        return status_code;
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return -3;
    }
}

bool check_date_licence(const std::string& expired_time) {

    std::ostringstream oss;
    std::vector<std::string> headers;
    if (get_requests("worldtimeapi.org", "80", "/api/timezone/Asia/Ho_Chi_Minh", oss, headers, 0) !=
        200) {
        std::cout << "Failed to check time !" << std::endl;
        return false;
    }

    std::vector<std::string> strs;
    boost::split(strs, oss.str(), boost::is_any_of("\""));

    const std::locale loc = std::locale(
        std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%S"));
    std::istringstream iss_current_time(strs[11]);
    std::istringstream iss_expired_time(expired_time);

    boost::posix_time::ptime p_current_time;
    boost::posix_time::ptime p_expired_time;

    iss_current_time.imbue(loc);
    iss_expired_time.imbue(loc);

    iss_current_time >> p_current_time;
    iss_expired_time >> p_expired_time;

    if (p_expired_time.is_not_a_date_time())
        return false;

    return p_current_time < p_expired_time;
}

size_t count_line(const boost::iostreams::mapped_file& mmap, size_t len) {
    auto left = mmap.const_data();
    auto right = left + mmap.size() - len;
    return (right - left) / len;
}