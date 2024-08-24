#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <regex.h>

#include <pw.h>
#include <ui.h>
#include <udp.h>

#define BUFSIZE 2048 * sizeof(int16_t)

char *server_address = NULL;
unsigned port = -1;

int is_valid_ip(const char *ip)
{
	const char *pattern = "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$";
	regex_t regex;

	if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
	{
		fprintf(stderr, "Could not compile regex\n");
		return 0;
	}

	int result = regexec(&regex, ip, 0, NULL, 0);

	regfree(&regex);

	return result == 0;
}

void display_help()
{
	char *help_message =
		"Usage: awim --ip [IP] --port [PORT]\n"
		"If port and(or) IP address were not specified, the program will ask for them.\n\n"
		"Author: rotlir.\n"
		"The repository is available at https://github.com/rotlir/awim-client/\n";
	printf("%s", help_message);
}

int main(int argc, char *argv[])
{	
	int address_from_args = 0;
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--help"))
		{
			display_help();
			return 0;
		}
		if (!strcmp(argv[i], "--ip"))
		{
			i++;
			server_address = argv[i];
			address_from_args = 1;
		}
		if (!strcmp(argv[i], "--port"))
		{
			i++;
			port = atoi(argv[i]);
		}
	}
	if (server_address == NULL)
	{
		printf("Enter IP address displayed in app: ");
		server_address = malloc(16);
		fgets(server_address, 16, stdin);
		server_address[strcspn(server_address, "\n")] = '\0';
	}
	if (!is_valid_ip(server_address))
	{
		printf("\"%s\" is not a valid IPv4 address.\n", server_address);
		return 0;
	}
	if (port == -1)
	{
		printf("Enter port: ");
		char s_port[6];
		fgets(s_port, 6, stdin);
		s_port[strcspn(s_port, "\n")] = '\0';
		port = atoi(s_port);
	}
	if (port < 1 || port > 65535)
	{
		printf("The port you entered is incorrect.\n");
		return 0;
	}
	printf("Connecting to %s:%i\n", server_address, port);
	udp_init(server_address, port);
	int exit_code = start_pipewire(&argc, argv, BUFSIZE);
	if (!address_from_args) free(server_address);
	return exit_code;
}