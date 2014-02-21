#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>


char* strip_trailing_whitespace(char* to_strip) {
	while(isspace(*to_strip)) ++to_strip;
	return strdup(to_strip);
}

char* strip_leading_whitespace(char* to_strip) {
	char* temp = strdup(to_strip);
	if (temp != NULL) {
		char *inner_temp = temp + strlen(to_strip) -1;
		while ((isspace(*inner_temp) || !isprint(*inner_temp) || *inner_temp == 0) 
			&& inner_temp > temp) --inner_temp;
		*++inner_temp = 0;
	}
	return temp;
}

char* trim(char* to_strip) {
	char* temp1 = strip_trailing_whitespace(to_strip);
	char* temp2 = strip_leading_whitespace(temp1);
	free(temp1);
	return temp2;
}

int main(void) {

	int listenfd = 0;
	int connfd = 0;
	int rval;
	char* in_data = "message recieved and understood\n";
	int in_data_len = strlen(in_data);
	struct sockaddr_in serv_addr;
	int port = 8081;
	
	
	char* file_not_found_response = "HTTP/1.1 404 Not Found\nContent-Type: text/html\nConnection: close\n\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 //EN\"\n\"http://www.w3.org/TR/html4/strict.dtd\">\n<html><head><title> 404 Not Found </title></head><body><p> The requested file cannot be found. </p></body></html>";
	char* bad_request_repsonse = "HTTP/1.1 400 Bad Request\nContent-Type: text/html\nConnection: close\n\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 //EN\"\n\"http://www.w3.org/TR/html4/strict.dtd\">\n<html><head><title> 400 Bad Request </title></head><body><p> 400 Bad Request. </p></body></html>";
	char* internal_server_error_response = "HTTP/1.1 500 Interal Server Error\nContent-Type: text/html\nConnection: close\n\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 //EN\"\n\"http://www.w3.org/TR/html4/strict.dtd\">\n<html><head><title> 500 Internal Server Error </title></head><body>	<p> 500 Internal Server Error. </p></body></html>";
	char* content_text_html = "Content-Type: text/html";
	char* content_text_plain = "Content-Type: text/plain";
	char* content_img_jpg = "Content-Type: image/jpeg";
	char* content_img_gif = "Content-Type: image/gif";
	char* content_other = "Content-Type: application/octet-stream";

	//create socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	//memset(&serv_addr, '0', sizeof(serv_addr));
	//memset(readBuffer, '0', sizeof(readBuffer));

	//bind socket
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	
	listen(listenfd, 10);
	
	while(1) {
		
		connfd = accept(listenfd, (struct sockaddr*) 0, 0);

		char read_buffer[1024]; //Buffer to store request from client
		char req_buffer[1024]; //Buffer used for strtok manipulation -> stores copy of read_buffer so manipulation does not affect read_buffer
		char filename_buffer[1024]; //Buffer to store requested filename -> Stores location of file eg /about/index.html
		char hostname_buffer[64]; //Buffer to store server hostname
		char send_buffer[65536]; //Buffer to be sent
		char success_buffer[65535]; //Buffer files to be read in to
		char content_type[64]; //buffer to read content type in to for later use

		memset(read_buffer, 0, sizeof(read_buffer));
		memset(req_buffer, 0, sizeof(req_buffer));
		memset(filename_buffer, 0, sizeof(filename_buffer));
		memset(hostname_buffer, 0, sizeof(hostname_buffer));
		memset(send_buffer, 0, sizeof(send_buffer));
		memset(success_buffer, 0, sizeof(success_buffer));
		if ((rval = recv(connfd, read_buffer, sizeof(read_buffer), 0)) < 0) {
			perror("failed to read stream message\n");
		} else if (rval == 0) {
			printf("ending connection\n");
		} else {
			printf("Start of process...\n");
			//printing full request
			printf("Full request:\n%s\n", read_buffer);
			//check if GET request
			//create duplicated string buffer
			strcpy(req_buffer, read_buffer);
			char* delim = "/";
			char* temp_tok;
			temp_tok = strtok(req_buffer, delim);
			//trim whitespace
			char* trimmed_char;
			trimmed_char = trim(temp_tok);
			//printf("%s\n", trimmed_char);
			char* comp_str = "GET"; 
			if (strcmp(trimmed_char, comp_str) != 0) {
				printf("%d\n", strcmp(trimmed_char, comp_str));
				printf("No GET request\n");
				close(connfd);
				close(listenfd);
				exit(0);
			}
			//check if get features HTTP after a filename -- store everything before HTTP in string
			strcpy(req_buffer, read_buffer);
			comp_str = "HTTP";
			temp_tok = strtok(req_buffer, delim);
			trimmed_char = trim(temp_tok);
			//printf("Trimmed char for HTTP %s\n", trimmed_char);
			memset(filename_buffer, 0, 1024);
			filename_buffer[0] = '/';
			while (temp_tok != NULL) {
				temp_tok = strtok(NULL, delim);
				trimmed_char = trim(temp_tok);
				//internal strok on space to find the full filename
				char internal_comp[sizeof(trimmed_char)+1]; 
				strcpy(internal_comp, trimmed_char);
				char* internal_tok = strtok(internal_comp, " ");
				//add trimmed_char to string to store filename for webpage delivery
				if (strcmp(internal_tok, comp_str) != 0) {
					strcat(filename_buffer, internal_tok);
				}
				if (strcmp(internal_tok, comp_str) == 0) {
					break;
				}
			}			
			printf("Filename: %s\n", filename_buffer);
			//search for Host: and check against the gethostname() function
			strcpy(req_buffer, read_buffer);
			memset(hostname_buffer, 0, 64);
			//printf("Before hostname search\n");
			comp_str = "Host:";
			char* host_start = strstr(req_buffer, comp_str);
			strncpy(hostname_buffer, host_start, 63);
			//hostname_buffer[64] = '\0';
			int i;
			char* temp = strtok(hostname_buffer, " ");
			for (i=0; i < 2; i++) {
				if (i == 1) {
					strcpy(hostname_buffer, temp);
				}
				temp = strtok(NULL, " ");
			}
			//tokenize for \n
			char temp2[64];
			strcpy(temp2, hostname_buffer);
			temp = strtok(temp2, "\n");
			strcpy(hostname_buffer, temp);
			char this_host[64];
			char this_host_extended[64];
			//printf("Fetching local hostname...\n");
			gethostname(this_host, 63);
			strcpy(this_host_extended, this_host);
			strcat(this_host_extended, ".dcs.gla.ac.uk");
			strcat(this_host_extended, ":");
			char port_buffer[33];
			sprintf(port_buffer, "%d", port);
			strcat(this_host_extended, port_buffer);
			char* temp_trim = trim(this_host_extended);
			strcpy(this_host_extended, temp_trim);
			temp_trim = trim(hostname_buffer);
			strcpy(hostname_buffer, temp_trim);
			//check hostname_buffer against this_host and extended
			if ((strcmp(this_host, hostname_buffer) != 0) && strcmp(this_host_extended, hostname_buffer) != 0) {
				printf("This computer hostname is: %s\n", this_host);
				printf("This computer extended hostname is: %s\n", this_host_extended);
				printf("Request hostname is: %s\n", hostname_buffer);
				printf("Hostnames don't match. Exiting.\n");
				strcpy(send_buffer, bad_request_repsonse);
				write(connfd, send_buffer, sizeof(send_buffer));
				close(connfd);
			}
			//Parse for content type
			strcpy(req_buffer, read_buffer);
			delim = ":";
			temp_tok = strtok(req_buffer, delim);
			temp_trim = trim(temp_tok);
			printf("trimming for content %s\n", temp_trim);
			while (temp_tok != NULL) {
				temp_tok = strtok(NULL, delim);
				if (temp_tok != NULL) {
					printf("%s\n", temp_tok);
					temp_trim = trim(temp_tok);
				}
				if (strcmp(temp_trim, "Content-Type") == 0) {
					temp_tok = strtok(NULL, delim);
					printf("temp tok HERPEEEE1 %s\n", temp_tok);
				}
				if (strcmp(temp_trim, "Accept") == 0) {
					temp_tok = strtok(NULL, delim);
					printf("temp token HERPEEEE2%s\n", temp_tok);
				}
			}
			//open file using filename buffer and read all lines of code therein
			FILE* fp;
			char search_location[512];
			getcwd(search_location, 512);
			strcat(search_location, filename_buffer);
			//trim search
			temp_trim = trim(search_location);
			strcpy(search_location, temp_trim);
			fp = fopen(search_location, "r");
			if (fp == NULL) {
				printf("Failed to open file...\n");
				strcpy(send_buffer, file_not_found_response);
				write(connfd, send_buffer, sizeof(send_buffer));
				close(connfd);
			}
			fread(success_buffer, sizeof(success_buffer), 1, fp);
			fclose(fp);
			/*
			size_t newLen = fread(fp, sizeof(char), MAXBUFLEN, fp);
			if (newLen == 0) {
			    fputs("Error reading file", stderr);
			} else {
			    source[++newLen] = '\0';
			}
			fclose(fp);
			}
			*/

			//put HTTP info at start of send buffer then strcat the successbuffer on to it
			strcat(send_buffer, "HTTP/1.1 200 OK\nContent-Type: text/html\nConnection: close\n\n");
			strcat(send_buffer, success_buffer);
			//ensure we send success_buffer or copy to send_buffer
			//strcpy(send_buffer, success_buffer);
			//printf("Send buffer: %s\n", send_buffer);
			write(connfd, send_buffer, sizeof(send_buffer));
			//write(connfd, in_data, in_data_len);
			printf("After write...\n");
			close(connfd);
			printf("Closed...\n");
		}
	}
	
	close(listenfd);
	
	return 0;
}