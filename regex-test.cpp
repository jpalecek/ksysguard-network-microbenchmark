#include <fstream>
#include "benchmark.h"
#include <regex>
#include <string>
#include <cstdint>
#include <arpa/inet.h>
using std::ifstream;
using namespace std;
// Convert /proc/net/tcp's mangled big-endian notation to a host-endian int32'

uint32_t tcpToInt(const std::string &part)
{
    uint32_t result = 0;
    result |= std::stoi(part.substr(0, 2), 0, 16) << 24;
    result |= std::stoi(part.substr(2, 2), 0, 16) << 16;
    result |= std::stoi(part.substr(4, 2), 0, 16) << 8;
    result |= std::stoi(part.substr(6, 2), 0, 16) << 0;
    return result;
}

uint32_t parse_1()
{
  static std::regex m_socketFileMatch = std::regex("\\s*\\d+: (?:(\\w{8})|(\\w{32})):([A-F0-9]{4}) (.{94}|.{70}) (\\d+) .*", std::regex::ECMAScript | std::regex::optimize);
  int ret = 0;
  ifstream file("regex-test.data");

  std::string data;
  while (std::getline(file, data)) {
    std::smatch match;
    if (!std::regex_match(data, match, m_socketFileMatch)) {
      continue;
    }

    uint32_t address[4] { 0 };
    if (!match.str(1).empty()) {
      address[3] = tcpToInt(match.str(1));
    } else {
      auto ipv6 = match.str(2);
      if (ipv6.compare(0, 24, "0000000000000000FFFF0000") == 0) {
        // Some applications (like Steam) use ipv6 sockets with ipv4.
        // This results in ipv4 addresses that end up in the tcp6 file.
        // They seem to start with 0000000000000000FFFF0000, so if we
        // detect that, assume it is ipv4-over-ipv6.
        address[3] = tcpToInt(ipv6.substr(24,8));
      } else {
        address[0] = tcpToInt(ipv6.substr(0, 8));
        address[1] = tcpToInt(ipv6.substr(8, 8));
        address[2] = tcpToInt(ipv6.substr(16, 8));
        address[3] = tcpToInt(ipv6.substr(24, 8));
      }
        }
        uint16_t port = std::stoi(match.str(3), 0, 16);
        auto inode = std::stoi(match.str(5));
        ret += address[0]+address[1]+address[2]+address[3]+port +inode;

    }
  return ret;
}

uint32_t parse_2()
{
  ifstream file("regex-test.data");
  uint32_t ret = 0;
  
      std::string data;
    while (std::getline(file, data)) {
        // Find the first ':'.
        // Should be within the first 16 characters.
        size_t data_start = data.find(':');
        if (data_start >= 16) {
            // Out of range.
            continue;
        }

        uint32_t address[4] { 0 };
        uint16_t port;
        int inode = 0;	// TODO: 64-bit?

        // Check for IPv4.
        if (data.size() < data_start + 87 + 1) {
            continue;
        }
        if (data[data_start + 10] == ':') {
            // IPv4 entry.
            // Value is one 32-bit hexadecimal chunk.
            // NOTE: The chunks is technically byteswapped, but it needs
            // to be kept in the byteswapped format here.
            char *const ipv4 = &data[data_start + 2];
            ipv4[8] = '\0';
            address[3] = (uint32_t)strtoul(ipv4, nullptr, 16);
            port = (uint16_t)strtoul(&ipv4[9], nullptr, 16);
            
            // Port number (16-bit hexadecimal)
            ipv4[13] = '\0';
          inode = (int)strtol(&ipv4[85], nullptr, 10);
            // inode number (decimal)
        } else {
            // Check for IPv6.
            if (data.size() < data_start + 135 + 1) {
                continue;
            }
            if (data[data_start + 34] == ':') {
                // IPv4 entry.
                // Value is in four 32-bit byteswapped hexadecimal chunks.
                // NOTE: The chunks are technically byteswapped, but they need
                // to be kept in the byteswapped format here.
                char *const ipv6 = &data[data_start + 2];

                if (!memcmp(ipv6, "0000000000000000FFFF0000", 24)) {
                    // Some applications (like Steam) use ipv6 sockets with ipv4.
                    // This results in ipv4 addresses that end up in the tcp6 file.
                    // They seem to start with 0000000000000000FFFF0000, so if we
                    // detect that, assume it is ipv4-over-ipv6.
                    ipv6[32] = '\0';
                    address[3] = (uint32_t)strtoul(&ipv6[24], nullptr, 16);
                } else {
                    ipv6[32] = '\0';
                    address[3] = (uint32_t)strtoul(&ipv6[24], nullptr, 16);
                    ipv6[24] = '\0';
                    address[2] = (uint32_t)strtoul(&ipv6[16], nullptr, 16);
                    ipv6[16] = '\0';
                    address[1] = (uint32_t)strtoul(&ipv6[8], nullptr, 16);
                    ipv6[8] = '\0';
                    address[0] = (uint32_t)strtoul(ipv6, nullptr, 16);
                }

                // Port number (16-bit hexadecimal)
                ipv6[38] = '\0';
                port = (uint16_t)strtoul(&ipv6[33], nullptr, 16);
                inode = (int)strtol(&ipv6[133], nullptr, 10);
            }
        }
            ret += address[0]+address[1]+address[2]+address[3]+port+inode;
    }
    return ret;

}

#include <boost/spirit/include/qi_uint.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
#include <boost/spirit/include/qi_no_skip.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

uint32_t parse_3()
{
  using qi::phrase_parse;
  using qi::repeat;
  using qi::no_skip;
  using qi::uint_;
  using qi::_1;
  using qi::eps;
  using phoenix::ref;
  using ascii::space;
  using qi::uint_parser;
  using qi::omit;
  using qi::lexeme;
  using qi::hex;
  using qi::char_;
  qi::rule<std::string::iterator, void(), ascii::space_type> hex_word_p = lexeme[+omit[char_("0-9A-F:")]];

  int ret = 0;
  ifstream file("regex-test.data");

  std::string data;
  while (std::getline(file, data)) {

    phrase_parse(data.begin(), data.end(),
                 omit[uint_] > ':' > lexeme[repeat(1, 4)[uint_parser<uint32_t, 16, 8, 8>()]][(
                     [&](const std::vector<uint32_t>& v) {
                       if(v.size() == 1)
                         ret += v[0];
                       else if(v.size() == 4) {
                         if (v[0] == 0 && v[1] == 0 && v[2] == 0xffff0000)
                           ret += v[3];
                         else
                           ret += v[0] + v[1] + v[2] + v[3];
                       }
                     }
                                                                                              )] >
                 ':' > hex[([&](uint32_t port){ ret+=port; })] >
                 hex_word_p > //remote
                 hex_word_p > //st
                 hex_word_p > //queue
                 hex_word_p > //tr
                 hex_word_p > //retr
                 hex_word_p > //uid
                 hex_word_p > //timeout
                 uint_[([&](uint32_t inode) {ret+=inode;}) ] //inode
                 ,
                 space);
  }
  return ret;
}

uint32_t parse_4()
{
  int ret = 0;
  ifstream file("regex-test.data");

  std::string data;
  while (std::getline(file, data)) {
    char address_data[33];
    int port, inode;
    if (sscanf(data.c_str(), " %*d: %32[0-9A-F]:%x %*32[0-9A-F]:%*x %*x %*x:%*x %*x:%*x %*x %*x %*x %d", address_data, &port, &inode) == 3) {
      if (strnlen(address_data, 32) == 8) {
        uint32_t ipv4addr = strtoul(address_data, nullptr, 16);
        ret += ipv4addr;
      } else {
        uint32_t ipv6addr[4];
        for (int i=3; i>= 0; i--) {
          address_data[i*8 + 8] = 0;
          ipv6addr[i] = strtoul(address_data+i*8, nullptr, 16);
        }
        if (ipv6addr[0] == 0 && ipv6addr[1] == 0 && ipv6addr[2] == 0xffff0000)
          ret += ipv6addr[3];
        else
          ret += ipv6addr[0] + ipv6addr[1] + ipv6addr[2] + ipv6addr[3];
      }
      ret += port + inode;
    }
  }
  return ret;
}

template <class T> struct tuple_seq;
template <class ... Ts> struct tuple_seq<tuple<Ts...> > { using type = typename seq<Ts...>::type; };
template <class F, class Tuple, class ... Is>
void do_something_with_tuple(const Tuple& t, F f, type_list<Is...>* s_)
{
  (f(std::get<Is::value>(t)), ...);
}

int main()
{
  std::cout << parse_1() << " " << parse_2() << " " << parse_3()  << " " << parse_4() << "\n";
  auto results = tournament(gettimeofday_timer(), parse_1, parse_2, parse_3, parse_4);
  do_something_with_tuple(results, [] (float f) {
                                     std::cout << f << " ";
                                   }, (typename tuple_seq<typeof(results)>::type*)0);
  std::cout << std::endl;
}
