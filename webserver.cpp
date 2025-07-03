// ./steps/step005.c
/* {{{1 includes */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <cctype>
#include <sys/socket.h>
#include <unistd.h>
/*}}}1*/

// #include "simple_dictionary.h"
using namespace std;

/*{{{1 MACROS */
int32_t PORT = 8080;
const int32_t BUFFER_SIZE = 1024;
string domain = "./";
/*}}}1*/

/*{{{1 struct(s) */
	struct request_info{
		char method[BUFFER_SIZE];
		char uri[BUFFER_SIZE];
		char version[BUFFER_SIZE];
	};
/*}}}1 */

/* {{{1 VARS */
struct request_info err; //this is an error val, so leave blank
struct sockaddr_in host_addr;
struct sockaddr_in client_addr;
int client_addrlen = sizeof(client_addr);
/* }}}1 */


/*{{{1 functions */
/*{{{2 string PromptForHostDir()*/
bool PromptYN(string prompt = ""){
	cout << prompt << endl;

	string input;
	cin >> input;
	
	for(int i = 0; i < input.size(); ++i){
		input[i] = tolower(input[i]);
	}

	if (input == "y" || input == "yes"){
		return(true);
	}
	else if (input == "n" || input == "no"){
		return(false);
	}
	else{
		cout << "input not valid!\n\n";
		return PromptYN();
	}
}
/*}}}2 string PromptForHostDir()*/

/*{{{2 string PromptForHostDir()*/
void PromptForHostDir(){
	bool wants_custom_dir = PromptYN(
		"would you like to use the current directory to serve files? \nif not will use current dir \n\n[y/n]"
	);

	if (wants_custom_dir == 0) {return;}
	cout << "what is the dir?" << endl;
	cin >> domain;
	if (domain[domain.size()-1] != '/'){
		cout << "ADDING / TO DOMAIN";
		domain += '/';
	}
	return;
}
/*}}}2*/

/*{{{2 int CreateSocket()*/
int CreateSocket(){
	printf("creating socket\n");
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd == -1) {
		perror("webserver (socket)");
		return 0;
	}
	printf("socket created successfully\n");

	return sockfd;
}
/*}}}2*/

/*{{{2 CreateAddress*/
struct sockaddr_in CreateAddress(int host_addrlen){
	printf("creating address\n");
	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(PORT);
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	printf("address created successfully\n");
	return(host_addr);
}
/*}}}2*/

/*{{{2*/
int BindSocketToAddress(
    int sockfd,
    const struct sockaddr_in host_addr,
    int host_addrlen
) {
    // Use ::bind to explicitly call the global namespace's bind function (the system call)
    if (::bind(sockfd, reinterpret_cast<const sockaddr*>(&host_addr), host_addrlen) != 0) {
        std::error_code ec(errno, std::system_category());
        std::cerr << "webserver (bind): " << ec.message() << std::endl;
        return 0;
    }
    std::cout << "socket successfully bound to address" << std::endl;
    return 1;
}
/*}}}2*/

/*{{{2 ReadRequest() */
struct request_info ReadRequest(char buffer[BUFFER_SIZE]){
	char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];

	sscanf(buffer, "%s %s %s", method, uri, version);
	struct request_info req; /*= {	
		{*method},
		{*uri},
		{*version}
	};*/

	strcpy(req.method, method);
	strcpy(req.uri, uri);
	strcpy(req.version, version);

	printf("📉 defined uri: %s\n", uri);
	//printf("\tca.sin_addr:ca.sin_port[%s:", inet_ntoa(client_addr.sin_addr));
	printf("\t client_addr: %s \n", inet_ntoa(client_addr.sin_addr));
	printf("\t client_port: %hu \n", ntohs(client_addr.sin_port));
	printf("\t method: %s \n", method);
	printf("\t uri: %s\n", uri);
	printf("\t version: %s \n", version);

	return(req);
};
/*}}}2*/
/*{{{2 Read()*/
struct request_info ReadFromSocket(
	int newsockfd,
	char buffer[BUFFER_SIZE]
){
	int valread = read(
		newsockfd, 
		buffer, 
		BUFFER_SIZE
	);

	if (valread < 0) {
		perror("webserver (read)");
		return(err);
	}

	struct request_info read = ReadRequest(buffer);
	printf("readfromsocket read.uri: %s \n", read.uri);
	if(read.version[0] == '\0'){
		perror("failed to read request");
	}
	return(read);
}
/*}}}2*/

/*{{{2 Write()*/
int Write(
	int newsockfd,
	struct request_info req
){ 
	char fs [BUFFER_SIZE] = domain;

	printf("req.uri = '%s' (length: %lu)\n", req.uri, strlen(req.uri));
	printf("fs = '%s' (length: %lu)\n", fs, strlen(fs));
	printf("strcmp result: %d\n", strcmp(req.uri, fs));

	if(
		strcmp(req.uri, fs) == 0
	){
		printf("🥶 link invalid, redirecting to home\n");
		strcpy(req.uri, "index.html");
	}

	char file_path[BUFFER_SIZE];
	snprintf(
		file_path, 
		BUFFER_SIZE, 
		"%s", 
		/*DOMAIN, */
		req.uri
	);

	printf("attempting to serve file: %s \n", file_path);

	FILE *file = fopen(
		file_path,
		"r"
	);

	char *content_type = "text/plain";
	int last_char = sizeof(last_char);
	printf("last_char: %i", last_char);

	if(strstr(req.uri, ".html") != NULL) {content_type = "text/html";}
	else if(strstr(req.uri, ".css") != NULL) {content_type = "text/css";}
	else if(strstr(req.uri, ".js") != NULL) {content_type = "application/javascript";}
	else if(strstr(req.uri, ".ico") != NULL) {content_type = "image";}
	
	if(file == NULL){
		perror("websever (file open)");
		char resp[BUFFER_SIZE];
		cout << "requesting from: " << domain << endl;
		snprintf(resp, BUFFER_SIZE,
			"HTTP/1.0 404 Not Found\r\n"
			"Server: webserver-c"
			"Content-type: text/plain \r\n"
			"Content-Length: 15\r\n"
			"\r\n"
			"404 Not Found\n"
		);
		printf("sending 404");
		write(newsockfd, resp, strlen(resp));
		printf("sent 404");
		return 0;
	}
	
	char resp[BUFFER_SIZE];

	snprintf(
		resp, BUFFER_SIZE,
		"HTTP/1.0 200 OK\r\n"
		"Server: webserver-c\r\n"
		"Content-type: %s\r\n\r\n",
		/*do not delete below >:c*/
		/* "<html><h1>hellope</h1>\n<p>first message from the server :3</p></html>\r\n"; */
		content_type
	);

	// send header
	int valwrite = write(newsockfd, resp, strlen(resp));
	if (valwrite < 0){
		perror("webserver (write headers)");
		fclose(file);
		return(0);
	}

	//send file contents
	char file_buffer[BUFFER_SIZE];
	size_t bytes_read;
	while((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file))>0){
		valwrite = write(newsockfd, file_buffer, bytes_read);
		if(valwrite < 0){
			perror("webserver (write file)");
			fclose(file);
			return 0;
		}
	}
	
	fclose(file);

	return(1);
}
/*}}}2*/

/*{{{2*/
int ListenForIncomingConnections(
	char buffer[BUFFER_SIZE],
	int sockfd,
	struct sockaddr_in host_addr, 
	int host_addrlen
){
    if (listen(sockfd, SOMAXCONN) != 0) {
        perror("webserver (listen)");
        return 0;
    }
    printf("server listPending for connections\n");

    for (;;) {
        // Accept incoming connections
        int newsockfd = accept(
			sockfd, 
			(struct sockaddr *)&host_addr,
            (socklen_t *)&host_addrlen
		);

        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");

		/* Get client address */
		int sockn = getsockname(
			newsockfd, 
			(struct sockaddr *)&client_addr,
			(socklen_t*)&client_addrlen
		);
		if(sockn < 0){
			perror("webserver (getsockname) \n");
			continue;
		}
		else{
			/*printf("sockn client addr: %n", &socke);*/
		}

        // Read from the socket
		struct request_info read = ReadFromSocket(newsockfd, buffer);
		printf("between fn read.uri: %s\n", read.uri);
		if(read.version[0] == '\0'){
			//printf("nothing to read");
			continue;
		}

		int write = Write(newsockfd, read);
		if(write <= 0){
			continue;	
		}

		close(newsockfd);
    }

	return (1);
}
/*}}}2*/
/*}}}1*/


/*{{{1 int main()*/
int main() {
	PromptForHostDir();

    char buffer[BUFFER_SIZE];

    // Create a socket
	int sockfd = CreateSocket();
    // Create the address to bind the socket to
	int host_addrlen = sizeof(host_addr);
	struct sockaddr_in host_addr = CreateAddress(host_addrlen);

    // Bind the socket to the address
	int bind_pass = BindSocketToAddress( sockfd, host_addr, host_addrlen);
	if(bind_pass == 0){
		printf("failed to bind socket\n");
		return 0;
	}

    // Listen for incoming connections
	int listened = ListenForIncomingConnections(buffer, sockfd, host_addr, host_addrlen);
	if(listened == 0){
		printf("failed to listen to connections\r");
		printf("failed to bind socket\n");
		return 0;
	}

    return 1;
}
/*}}}1*/
