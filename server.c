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
#include <sys/stat.h>


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

int file_size_func(char* file_name) {
	struct stat info_buffer;
	char search_location[512];
	getcwd(search_location, 512);
	strcat(search_location, file_name);
	char* temp_trim = trim(search_location);
	strcpy(search_location, temp_trim);
	stat(search_location, &info_buffer);
	return (int) info_buffer.st_size;
}

char* read_file(char* file_buffer, char* file_name, char* type) {
	FILE* fp;
	struct stat info_buffer;
	char search_location[512];
	getcwd(search_location, 512);
	strcat(search_location, file_name);
	char* temp_trim = trim(search_location);
	strcpy(search_location, temp_trim);
	stat(search_location, &info_buffer);
	if ((file_buffer = realloc(file_buffer, info_buffer.st_size)) == NULL) {
		printf("Malloc error.\n");
		exit(0);
	} 
	if (strcmp("error", type) == 0) {
		return NULL;
	} else if ((strcmp("text/html", type) == 0) || (strcmp("text/plain", type) == 0)) {
		printf("Text request made...\n");
		fp = fopen(search_location, "r");
		if (fp == NULL) {
			return NULL;
		}
		fread(file_buffer, info_buffer.st_size, 1, fp);
		//printf("%s\n", file_buffer);
		fclose(fp);
		return file_buffer;
	} else if ((strcmp("image/jpg", type) == 0) || (strcmp("image/gif", type) == 0) || (strcmp("image/ico", type) == 0)) {
		printf("Image request made...\n");
		printf("Size of image: %d\n", info_buffer.st_size);
		fp = fopen(search_location, "rb");
		if (fp == NULL) {
			return NULL;
		}
		fread(file_buffer, info_buffer.st_size, 1, fp);
		fclose(fp);
		return file_buffer;
	} else {
		return NULL;
	}
}

int main(void) {

	int listenfd = 0;
	int connfd = 0;
	int rval;
	struct sockaddr_in serv_addr;
	int port = 8081;
	
	
	char* file_not_found_response = "HTTP/1.1 404 Not Found\nContent-Type: text/html\nConnection: close\n\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 //EN\"\n\"http://www.w3.org/TR/html4/strict.dtd\">\n<html><head><title> 404 Not Found </title></head><body><p> The requested file cannot be found. </p></body></html>";
	char* bad_request_repsonse = "HTTP/1.1 400 Bad Request\nContent-Type: text/html\nConnection: close\n\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 //EN\"\n\"http://www.w3.org/TR/html4/strict.dtd\">\n<html><head><title> 400 Bad Request </title></head><body><p> 400 Bad Request. </p></body></html>";
	char* internal_server_error_response = "HTTP/1.1 500 Interal Server Error\nContent-Type: text/html\nConnection: close\n\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 //EN\"\n\"http://www.w3.org/TR/html4/strict.dtd\">\n<html><head><title> 500 Internal Server Error </title></head><body>	<p> 500 Internal Server Error. </p></body></html>";
	char* content_text_html = "Content-Type: text/html\n";
	char* content_text_plain = "Content-Type: text/plain\n";
	char* content_img_jpg = "Content-Type: image/jpeg\n";
	char* content_img_gif = "Content-Type: image/gif\n";
	char* content_other = "Content-Type: application/octet-stream\n";
	char* content_length = "Content-Length: ";

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
		char send_buffer[65536]; //Buffer to build header to be sent
		char* final_message; //final resizable message
		char* final_header;
		char* file_buffer = NULL;
		int file_size = 0;
		char accept_type[64];
		char port_buffer[33];
		//char success_buffer[65535]; //Buffer files to be read in to

		memset(read_buffer, 0, sizeof(read_buffer));
		memset(req_buffer, 0, sizeof(req_buffer));
		memset(filename_buffer, 0, sizeof(filename_buffer));
		memset(hostname_buffer, 0, sizeof(hostname_buffer));
		memset(send_buffer, 0, sizeof(send_buffer));
		memset(accept_type, 0, sizeof(accept_type));
		memset(port_buffer, 0, sizeof(port_buffer));
		//memset(success_buffer, 0, sizeof(success_buffer));
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
			//printf("Filename: %s\n", filename_buffer);
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
			gethostname(this_host, 63);
			strcpy(this_host_extended, this_host);
			strcat(this_host_extended, ".dcs.gla.ac.uk");
			strcat(this_host_extended, ":");
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
			strcpy(req_buffer, filename_buffer);
			delim = ".";
			temp_tok = strtok(req_buffer, delim);
			temp_trim = trim(temp_tok);
			//delete this int after testing
			while (temp_tok != NULL) {
				if (temp_tok != NULL) {
					temp_trim = trim(temp_tok);
				}
				if (strcmp(temp_trim, "html") == 0) {
					strcat(accept_type, "text/html");
					break;
				} else if  (strcmp(temp_trim, "htm") == 0) {
					strcat(accept_type, "text/html");
					break;
				} else if (strcmp(temp_trim, "txt") == 0) {
					strcat(accept_type, "text/plain");
					break;
				} else if (strcmp(temp_trim, "jpg") == 0) {
					strcat(accept_type, "image/jpg");
					break;
				} else if (strcmp(temp_trim, "jpeg") == 0) {
					strcat(accept_type, "image/jpg");
					break;
				} else if (strcmp(temp_trim, "gif") == 0) {
					strcat(accept_type, "image/gif");
					break;
				} else if (strcmp(temp_trim, "ico") == 0) {
					strcat(accept_type, "image/ico");
					break;
				}
				temp_tok = strtok(NULL, delim);
			}
			if (accept_type[0] == 0) {
				strcat(accept_type, "application/octet-stream");
			}
			//open file using filename buffer and read all lines of code therein
			char* success_buffer;
			if ((success_buffer = read_file(file_buffer, filename_buffer, accept_type)) == NULL) {
				printf("Failed to open file...\n");
				strcpy(send_buffer, file_not_found_response);
				write(connfd, send_buffer, sizeof(send_buffer));
				close(connfd);
				continue;
			}
			strcat(send_buffer, "HTTP/1.1 200 OK\n");
			char temp_file_size_buffer[64];
			if ((strcmp(accept_type, "text/html") == 0) || (strcmp(accept_type, "text/htm") == 0)) {
				strcat(send_buffer, content_text_html);
				file_size = file_size_func(filename_buffer);
				strcat(send_buffer, content_length);
				sprintf(temp_file_size_buffer, "%d", file_size);
				strcat(send_buffer, temp_file_size_buffer);
				strcat(send_buffer, "\n");
			} else if ((strcmp(accept_type, "image/jpg") == 0) || (strcmp(accept_type, "image/jpeg") == 0)) {
				strcat(send_buffer, content_img_jpg);
				file_size = file_size_func(filename_buffer);
				strcat(send_buffer, content_length);
				sprintf(temp_file_size_buffer, "%d", file_size);
				strcat(send_buffer, temp_file_size_buffer);
				strcat(send_buffer, "\n");
			} else if (strcmp(accept_type, "application/octet-stream") == 0) {
				strcat(send_buffer, content_other);
				file_size = file_size_func(filename_buffer);
				strcat(send_buffer, content_length);
				sprintf(temp_file_size_buffer, "%d", file_size);
				strcat(send_buffer, temp_file_size_buffer);
				strcat(send_buffer, "\n");
			} else if (strcmp(accept_type, "image/gif") == 0) {
				strcat(send_buffer, content_img_gif);
				file_size = file_size_func(filename_buffer);
				strcat(send_buffer, content_length);
				sprintf(temp_file_size_buffer, "%d", file_size);
				strcat(send_buffer, temp_file_size_buffer);
				strcat(send_buffer, "\n");
			} else if (strcmp(accept_type, "text/plain") == 0) {
				strcat(send_buffer, content_text_plain);
				file_size = file_size_func(filename_buffer);
				strcat(send_buffer, content_length);
				sprintf(temp_file_size_buffer, "%d", file_size);
				strcat(send_buffer, temp_file_size_buffer);
				strcat(send_buffer, "\n");
			} 
			strcat(send_buffer, "Connection: keep alive\n\r\n\0");
			//scan through send_buffer til '\0' 
			int size_of_send_buffer = 0;
			while (send_buffer[size_of_send_buffer] != '\0') {
				size_of_send_buffer++;
			}
			if ((final_header = malloc(size_of_send_buffer)) == NULL) {
				printf("Malloc error.\n");
				exit(0);
			} 
			memset(final_header, 0, size_of_send_buffer);
			if ((final_message = malloc(file_size)) == NULL) {
				printf("Malloc error.\n");
				exit(0);
			} 
			strncpy(final_header, send_buffer, (size_t) size_of_send_buffer);
			write(connfd, final_header, strlen(final_header));
			memcpy(final_message, success_buffer, file_size);
			write(connfd, final_message, file_size);
			printf("Full return: %s\n", final_header);
			printf("And finally: %s\n", final_message);
			printf("After write...\n");
			close(connfd);
			printf("Closed...\n");
			free(success_buffer);
			free(final_message);
			free(final_header);
		}
	}
	
	close(listenfd);
	
	return 0;
}