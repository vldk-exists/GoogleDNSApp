#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

vector<string> split(string a, char b) {
        string c; vector<string> d; for (char i: a) if (i == b) {d.push_back(c); c = "";} else { c += i; }; d.push_back(c); 
        return d;
}



vector<unsigned char> buildPacket(string domain) {
	vector<unsigned char> packet;

	// Transaction ID
	packet.push_back(0xaa);
	packet.push_back(0xaa);
	
	// Flags
	packet.push_back(0x01);
	packet.push_back(0x00);
	
	// Questions
	packet.push_back(0x00);
	packet.push_back(0x01);

	// Answer RRS, Authority RRs, Additional RRs
	packet.push_back(0x00);
	packet.push_back(0x00);

	packet.push_back(0x00);
	packet.push_back(0x00);

	packet.push_back(0x00);
	packet.push_back(0x00);
	
	// Domain encoding
	vector<string> parts = split(domain, '.');
	for (string part: parts) {
		packet.push_back((unsigned char)part.length());
		for (unsigned char i: part) {
			packet.push_back(i);
		}
	}
	// Null terminator
	packet.push_back(0x00);

	// Query type
	packet.push_back(0x00);
	packet.push_back(0x01);

	// Query class
	packet.push_back(0x00);
	packet.push_back(0x01);

	return packet;
} 

string decodeResponse(vector<unsigned char> response) {
    size_t pos = 12;
    while (pos < response.size() && response[pos] != 0) {
        ++pos;
    }
    pos += 5;

    size_t ip_start = pos + 10;
    if (ip_start + 4 > response.size()) {
        return "Invalid response";
    }

    ostringstream ip;
    ip << static_cast<int>(response[ip_start]) << '.'
       << static_cast<int>(response[ip_start + 1]) << '.'
       << static_cast<int>(response[ip_start + 2]) << '.'
       << static_cast<int>(response[ip_start + 3]);

    return ip.str();
}

class DNS {
	public:	
		static string getAddress(string domain) {
			// init
			int dns_socket;
			struct sockaddr_in dns_address;

			dns_socket = socket(AF_INET, SOCK_DGRAM, 0);
			if (dns_socket < 0) {
				cerr << "connect init failed" << endl;
				exit(EXIT_FAILURE);
			}

			dns_address.sin_family = AF_INET;
			dns_address.sin_port = htons(53);
			inet_pton(AF_INET, "8.8.8.8", &dns_address.sin_addr);
			
			// connect
			if (connect(dns_socket, (struct sockaddr *)&dns_address, sizeof(dns_address))) {
				cerr << "connect failed" << endl;
				exit(EXIT_FAILURE);
			}

			// packet building
			vector<unsigned char> packet = buildPacket(domain);
			
			// sending
			send(dns_socket, packet.data(), packet.size(), 0);
			
			// receiving
			unsigned char buffer[512];

			unsigned int received = recv(dns_socket, buffer, sizeof(buffer), 0);
			
			if (received < 1) {
				cerr << "receving failed!" << endl;
				close(dns_socket);
				exit(EXIT_FAILURE);
			}

			// decoding received data
			vector<unsigned char> response(buffer, buffer+received);

			string address = decodeResponse(response);

			close(dns_socket);	

			return address;
		}	
};



int main() {
	string domain = "github.com";
	
	cout << DNS::getAddress(domain) << endl;

	return 0;
}
